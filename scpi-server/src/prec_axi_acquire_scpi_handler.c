
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#include "prec_axi_acquire_scpi_handler.h"

#include "common.h"
//#include "scpi/parser.h"
//#include "scpi/units.h"


/* configuration constants */
#define SERVER_IP_ADDR          "192.168.2.101"
#define SERVER_IP_PORT_A        5001
#define SERVER_IP_PORT_B        5002
#define ACQUISITION_LENGTH      200000          /* samples */
#define PRE_TRIGGER_LENGTH      40000           /* samples */
#define DECIMATION              DE_1            /* one of enum decimation */
#define TRIGGER_MODE            TR_MANUAL       /* one of enum trigger */
#define TRIGGER_THRESHOLD       0               /* ADC counts, 2048 ≃ +0.25V */

/* internal constants */
#define READ_BLOCK_SIZE         16384
#define SEND_BLOCK_SIZE         17752
#define RAM_A_ADDRESS           0x08000000UL
#define RAM_A_SIZE              0x0C000000UL
#define RAM_B_ADDRESS           0x14000000UL
#define RAM_B_SIZE              0x0C000000UL
#define UDP_BUFFER_SIZE         SEND_BLOCK_SIZE*4 // UDP buffer used to store data read from RAM before transmissing over UDP. 4x the transmit size to buffer stuck packets


/* data types */
enum equalizer {
	EQ_OFF,
	EQ_LV,
	EQ_HV
};
enum trigger {
	TR_OFF = 0,
	TR_MANUAL,
	TR_CH_A_RISING,
	TR_CH_A_FALLING,
	TR_CH_B_RISING,
	TR_CH_B_FALLING,
	TR_EXT_RISING,
	TR_EXT_FALLING,
	TR_ASG_RISING,
	TR_ASG_FALLING
};
enum decimation {
	DE_OFF          = 0,
	DE_1            = 0x00001,
	DE_8            = 0x00008,
	DE_64           = 0x00040,
	DE_1024         = 0x00400,
	DE_8192         = 0x02000,
	DE_65536        = 0x10000
};

struct queue {
	pthread_mutex_t mutex;
	pthread_t       sender;
	int             started;
	unsigned int    read_end;
	uint8_t         *buf;
	int             sock_fd;
};


/* macros and prototypes */
/* note: the circular buffer macros may evaluate each of their arguments once, more
 *       than once or not at all. don't use expressions with side-effects */
/* add offsets within circular buffer */
#define CIRCULAR_ADD(arg1, arg2, size) \
	(((arg1) + (arg2)) % (size))
/* subtract offsets within circular buffer */
#define CIRCULAR_SUB(arg1, arg2, size) \
	((arg1) >= (arg2) ? (arg1) - (arg2) : (size) + (arg1) - (arg2))
/* calculate distance within circular buffer */
#define CIRCULAR_DIST(argfrom, argto, size) \
	CIRCULAR_SUB((argto), (argfrom), (size))
/* memcpy from circular source to linear target */
#define CIRCULARSRC_MEMCPY(target, src_base, src_offs, src_size, length) \
	do { \
		if ((src_offs) + (length) <= (src_size)) { \
			memcpy((target), (void *)(src_base) + (src_offs), (length)); \
		} else { \
			unsigned int __len1 = (src_size) - (src_offs); \
			memcpy((target), (void *)(src_base) + (src_offs), __len1); \
			memcpy((void *)(target) + __len1, (src_base), (length) - __len1); \
		} \
	} while (0)

static void scope_reset(void);
static void scope_set_filters(enum equalizer eq, int shaping, volatile uint32_t *base);
static void scope_setup_input_parameters(enum decimation dec, enum equalizer ch_a_eq, enum equalizer ch_b_eq, int ch_a_shaping, int ch_b_shaping);
static void scope_setup_trigger_parameters(int thresh_a, int thresh_b, int hyst_a, int hyst_b, int deadtime);
static void scope_setup_axi_recording(void);
static void scope_activate_trigger(enum trigger trigger);
static void read_worker(struct queue *a, struct queue *b);
static void *send_worker(void *data);


/* module global variables */
static volatile void *scope;    /* access to fpga registers must not be optimized */
static void *buf_a = MAP_FAILED;
static void *buf_b = MAP_FAILED;
static struct queue queue_a = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.started = 0,
	.read_end = 0,
	.buf = NULL,
	.sock_fd = -1,
};
static struct queue queue_b = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.started = 0,
	.read_end = 0,
	.buf = NULL,
	.sock_fd = -1,
};


/* functions */
/*
 * main without paramater evaluation - all configuration is done through
 * constants for the purposes of this example.
 */
int start_all_threads(void)
{
	int rc;
	int mem_fd;
	void *smap = MAP_FAILED;
	struct sockaddr_in srv_addr;

	/* acquire pointers to mapped bus regions of fpga and dma ram */
	mem_fd = open("/dev/mem", O_RDWR);
	if (mem_fd < 0) {
		fprintf(stderr, "open /dev/mem failed, %s\n", strerror(errno));
		rc = -1;
		goto main_exit;
	}

	smap = mmap(NULL, 0x00100000UL, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd, 0x40100000UL);
	buf_a = mmap(NULL, RAM_A_SIZE, PROT_READ, MAP_SHARED, mem_fd, RAM_A_ADDRESS);
	buf_b = mmap(NULL, RAM_B_SIZE, PROT_READ, MAP_SHARED, mem_fd, RAM_B_ADDRESS);
	if (smap == MAP_FAILED || buf_a == MAP_FAILED || buf_b == MAP_FAILED) {
		fprintf(stderr, "mmap failed, %s - scope %p buf_a %p buf_b %p\n",
		        strerror(errno), smap, buf_a, buf_b);
		rc = -2;
		goto main_exit;
	}
	scope = smap;

	/* allocate cacheable buffers */
	queue_a.buf = malloc(ACQUISITION_LENGTH * 2);
	queue_b.buf = malloc(ACQUISITION_LENGTH * 2);
	if (queue_a.buf == NULL || queue_b.buf == NULL) {
		fprintf(stderr, "malloc failed, %s - buf a %p buf b %p\n",
		        strerror(errno), queue_a.buf, queue_b.buf);
		rc = -3;
		goto main_exit;
	}

	/* setup udp sockets */
	queue_a.sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
	queue_b.sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (queue_a.sock_fd < 0 || queue_b.sock_fd < 0) {
		fprintf(stderr, "create socket failed, %s - sock_fd a %d sock_fd b %d\n",
		        strerror(errno), queue_a.sock_fd, queue_b.sock_fd);
		rc = -4;
		goto main_exit;
	}

	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);
	srv_addr.sin_port = htons(SERVER_IP_PORT_A);

	if (connect(queue_a.sock_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
		fprintf(stderr, "connect A failed, %s\n", strerror(errno));
		rc = -5;
		goto main_exit;
	}

	srv_addr.sin_port = htons(SERVER_IP_PORT_B);

	if (connect(queue_b.sock_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
		fprintf(stderr, "connect B failed, %s\n", strerror(errno));
		rc = -5;
		goto main_exit;
	}

	/* initialize scope */
	scope_reset();
	scope_setup_input_parameters(DECIMATION, EQ_LV, EQ_LV, 1, 1);
	scope_setup_trigger_parameters(TRIGGER_THRESHOLD, TRIGGER_THRESHOLD, 50, 50, 1250);
	scope_setup_axi_recording();

	/* start socket senders */
	rc = pthread_create(&queue_a.sender, NULL, send_worker, &queue_a);
	if (rc != 0) {
		fprintf(stderr, "start sender A failed, %s\n", strerror(rc));
		rc = -6;
		goto main_exit;
	}
	queue_a.started = 1;

	rc = pthread_create(&queue_b.sender, NULL, send_worker, &queue_b);
	if (rc != 0) {
		fprintf(stderr, "start sender B failed, %s\n", strerror(rc));
		rc = -6;
		goto main_exit;
	}
	queue_b.started = 1;

	/* start reader in main-thread */
	read_worker(&queue_a, &queue_b);

main_exit:
	/* cleanup */
	if (queue_a.started) {
		pthread_cancel(queue_a.sender);
		pthread_join(queue_a.sender, NULL);
	}
	if (queue_b.started) {
		pthread_cancel(queue_b.sender);
		pthread_join(queue_b.sender, NULL);
	}
	if (smap != MAP_FAILED)
		munmap(smap, 0x00100000UL);
	if (buf_a != MAP_FAILED)
		munmap(buf_a, RAM_A_SIZE);
	if (buf_b != MAP_FAILED)
		munmap(buf_b, RAM_B_SIZE);
	if (queue_a.buf)
		free(queue_a.buf);
	if (queue_b.buf)
		free(queue_b.buf);
	if (mem_fd >= 0)
		close(mem_fd);
	if (queue_a.sock_fd >= 0)
		close(queue_a.sock_fd);
	if (queue_b.sock_fd >= 0)
		close(queue_b.sock_fd);

	return rc;
}

static void scope_reset(void)
{
	*(uint32_t *)(scope + 0x00000) = 2;     /* reset scope */
}

static void scope_set_filters(enum equalizer eq,
                              int shaping,
                              volatile uint32_t *base)
{
	/* equalization filter */
	switch (eq) {
	case EQ_HV:
		*(base + 0) = 0x4c5f;   /* filter coeff aa */
		*(base + 1) = 0x2f38b;  /* filter coeff bb */
		break;
	case EQ_LV:
		*(base + 0) = 0x7d93;   /* filter coeff aa */
		*(base + 1) = 0x437c7;  /* filter coeff bb */
		break;
	case EQ_OFF:
		*(base + 0) = 0x0;      /* filter coeff aa */
		*(base + 1) = 0x0;      /* filter coeff bb */
		break;
	}

	/* shaping filter */
	if (shaping) {
		*(base + 2) = 0xd9999a; /* filter coeff kk */
		*(base + 3) = 0x2666;   /* filter coeff pp */
	} else {
		*(base + 2) = 0xffffff; /* filter coeff kk */
		*(base + 3) = 0x0;      /* filter coeff pp */
	}
}

static void scope_setup_input_parameters(enum decimation dec,
                                         enum equalizer ch_a_eq,
                                         enum equalizer ch_b_eq,
                                         int ch_a_shaping,
                                         int ch_b_shaping)
{
	*(uint32_t *)(scope + 0x00014) = dec;                           /* decimation */
	*(uint32_t *)(scope + 0x00028) = (dec != DE_OFF) ? 1 : 0;       /* enable averaging */

	scope_set_filters(ch_a_eq, ch_a_shaping, (uint32_t *)(scope + 0x00030)); /* filter coeff base channel a */
	scope_set_filters(ch_b_eq, ch_b_shaping, (uint32_t *)(scope + 0x00040)); /* filter coeff base channel b */
}

static void scope_setup_trigger_parameters(int thresh_a,
                                           int thresh_b,
                                           int hyst_a,
                                           int hyst_b,
                                           int deadtime)
{
	*(uint32_t *)(scope + 0x00008) = thresh_a;      /* channel a trigger threshold */
	*(uint32_t *)(scope + 0x0000c) = thresh_b;      /* channel b trigger threshold */
	/* the legacy recording logic controls when the trigger mode will be reset. we want
	 * that to happen as soon as possible (because that's the signal that a trigger event
	 * occured, and the pre-trigger samples are already waiting for transmission), so set
	 * some small value > 0 here */
	*(uint32_t *)(scope + 0x00010) = 10;            /* legacy post trigger samples */
	*(uint32_t *)(scope + 0x00020) = hyst_a;        /* channel a trigger hysteresis */
	*(uint32_t *)(scope + 0x00024) = hyst_b;        /* channel b trigger hysteresis */
	*(uint32_t *)(scope + 0x00090) = deadtime;      /* trigger deadtime */
}

static void scope_setup_axi_recording(void)
{
	*(uint32_t *)(scope + 0x00050) = RAM_A_ADDRESS;                 /* buffer a start */
	*(uint32_t *)(scope + 0x00054) = RAM_A_ADDRESS + RAM_A_SIZE;    /* buffer a stop */
	*(uint32_t *)(scope + 0x00058) = ACQUISITION_LENGTH - PRE_TRIGGER_LENGTH + 64; /* channel a post trigger samples */
	*(uint32_t *)(scope + 0x00070) = RAM_B_ADDRESS;                 /* buffer b start */
	*(uint32_t *)(scope + 0x00074) = RAM_B_ADDRESS + RAM_B_SIZE;    /* buffer b stop */
	*(uint32_t *)(scope + 0x00078) = ACQUISITION_LENGTH - PRE_TRIGGER_LENGTH + 64; /* channel b post trigger samples */

	*(uint32_t *)(scope + 0x0005c) = 1;     /* enable channel a axi */
	*(uint32_t *)(scope + 0x0007c) = 1;     /* enable channel b axi */
}

static void scope_activate_trigger(enum trigger trigger)
{
	/* TODO maybe use the 'keep armed' flag without reset, to have better pre-trigger data when a trigger immediately follows the previous recording */
	*(uint32_t *)(scope + 0x00000) = 3;             /* reset and arm scope */
	*(uint32_t *)(scope + 0x00000) = 0;             /* armed for trigger */
	*(uint32_t *)(scope + 0x00004) = trigger;       /* trigger source */
}

/*
 * arms the scope and waits for trigger. once a trigger occurs, it reads samples
 * from dma ram and puts them on the channel queues. advances each queue's
 * queue->read_end for each block that was copied. rinse and repeat. access to
 * read_end is protected by queue->mutex.
 */
static void read_worker(struct queue *a, struct queue *b)
{
	unsigned int start_pos_a, start_pos_b;
	unsigned int curr_pos_a, curr_pos_b;
	unsigned int read_pos_a, read_pos_b;
	size_t length_a, length_b;
	int a_first, a_ready, b_first, b_ready;
	int did_something;

	do {
	        printf("Arming read worker...\n");
		a_first = b_first = 1;
		a_ready = b_ready = 0;

		scope_activate_trigger(TRIGGER_MODE);

		/* wait for trigger */
		while (*(uint32_t *)(scope + 0x00004))
			usleep(5);

		start_pos_a = *(uint32_t *)(scope + 0x00060);   /* channel a trigger pointer */
		start_pos_b = *(uint32_t *)(scope + 0x00080);   /* channel b trigger pointer */

		start_pos_a = CIRCULAR_SUB(start_pos_a - RAM_A_ADDRESS, PRE_TRIGGER_LENGTH * 2, RAM_A_SIZE);
		start_pos_b = CIRCULAR_SUB(start_pos_b - RAM_B_ADDRESS, PRE_TRIGGER_LENGTH * 2, RAM_B_SIZE);

		did_something = 1;

		do {
			if (!did_something)
				usleep(5);
			did_something = 0;

			/* get buffer positions */
			if (pthread_mutex_lock(&a->mutex) != 0)
				goto read_worker_exit;
			read_pos_a = a->read_end;
			if (pthread_mutex_unlock(&a->mutex) != 0)
				goto read_worker_exit;

			if (pthread_mutex_lock(&b->mutex) != 0)
				goto read_worker_exit;
			read_pos_b = b->read_end;
			if (pthread_mutex_unlock(&b->mutex) != 0)
				goto read_worker_exit;

			/* before starting, test if senders are ready */
			if (a_first && read_pos_a == 0) {
				a_first = 0;
				a_ready = 1;
			}
			if (b_first && read_pos_b == 0) {
				b_first = 0;
				b_ready = 1;
			}

			/* get current recording positions */
			curr_pos_a = *(uint32_t *)(scope + 0x00064);    /* channel a current write pointer */
			curr_pos_b = *(uint32_t *)(scope + 0x00084);    /* channel b current write pointer */
			curr_pos_a -= RAM_A_ADDRESS;
			curr_pos_b -= RAM_B_ADDRESS;

			/* calculate block sizes */
			if (read_pos_a + READ_BLOCK_SIZE <= ACQUISITION_LENGTH * 2)
				length_a = READ_BLOCK_SIZE;
			else
				length_a = ACQUISITION_LENGTH * 2 - read_pos_a;
			if (read_pos_b + READ_BLOCK_SIZE <= ACQUISITION_LENGTH * 2)
				length_b = READ_BLOCK_SIZE;
			else
				length_b = ACQUISITION_LENGTH * 2 - read_pos_b;

			/* copy if sender is ready and a full block is available in the dma ram */
			if (a_ready && CIRCULAR_DIST(start_pos_a, curr_pos_a, RAM_A_SIZE) >= length_a) {
				CIRCULARSRC_MEMCPY(a->buf + read_pos_a, buf_a, start_pos_a, RAM_A_SIZE, length_a);
				start_pos_a = CIRCULAR_ADD(start_pos_a, length_a, RAM_A_SIZE);

				if (read_pos_a + length_a >= ACQUISITION_LENGTH * 2) {
	                                printf("CH A all samples read\n");
					a_ready = 0; /* stop if all samples were copied */
                                }
				if (pthread_mutex_lock(&a->mutex) != 0)
					goto read_worker_exit;
				if (a->read_end == read_pos_a)
					a->read_end += length_a;
				else
					a_ready = 0; /* stop if sender resetted read_end */
				if (pthread_mutex_unlock(&a->mutex) != 0)
					goto read_worker_exit;

				did_something = 1;
			}
			if (b_ready && CIRCULAR_DIST(start_pos_b, curr_pos_b, RAM_B_SIZE) > length_b) {
				CIRCULARSRC_MEMCPY(b->buf + read_pos_b, buf_b, start_pos_b, RAM_B_SIZE, length_b);
				start_pos_b = CIRCULAR_ADD(start_pos_b, length_b, RAM_B_SIZE);

				if (read_pos_b + length_b >= ACQUISITION_LENGTH * 2) {
	                                printf("CH B all samples read\n");
					b_ready = 0; /* stop if all samples were copied */
                                }
				if (pthread_mutex_lock(&b->mutex) != 0)
					goto read_worker_exit;
				if (b->read_end == read_pos_b)
					b->read_end += length_b;
				else
					b_ready = 0; /* stop if sender resetted read_end */
				if (pthread_mutex_unlock(&b->mutex) != 0)
					goto read_worker_exit;

				did_something = 1;
			}
		} while (a_first || a_ready || b_first || b_ready);
	} while (1);

read_worker_exit:
	printf("All read worker exiting...\n");
	return;
}

/*
 * sends samples from a struct queue. synchronisation with the queue is done via
 * queue->read_end. send_worker will send data from 0 to read_end and will reset
 * read_end to 0 once ACQUISITION_LENGTH samples have been transmitted. then it
 * will wait until read_end advances from 0 and start all over. access to read_end
 * is protected by queue->mutex.
 */
static void *send_worker(void *data)
{
	struct queue *q = (struct queue *)data;
	unsigned int send_pos = 0;
	ssize_t sent;
	size_t length;

	do {
		if (pthread_mutex_lock(&q->mutex) != 0)
			goto send_worker_exit;
		if (q->read_end >= ACQUISITION_LENGTH * 2 && send_pos >= ACQUISITION_LENGTH * 2) {
			send_pos = 0;
			q->read_end = 0;
		}
		length = q->read_end - send_pos;
		if (pthread_mutex_unlock(&q->mutex) != 0)
			goto send_worker_exit;

		if (length > 0) {
			do {
				if (length > SEND_BLOCK_SIZE)
					sent = send(q->sock_fd, q->buf + send_pos, SEND_BLOCK_SIZE, 0);
				else
					sent = send(q->sock_fd, q->buf + send_pos, length, 0);
				if (sent > 0) {
					send_pos += sent;
					length -= sent;
				}
			} while (sent >= 0 && length > 0);

			if (sent < 0)
				goto send_worker_exit;
		} else {
			usleep(5);
		}
	} while (1);

send_worker_exit:
	printf("Send worker exiting...\n");
	return NULL;
}

scpi_result_t RP_AxiAcqStart(scpi_t *context) {
    int result = RP_OK;
    
    start_all_threads();
    if (RP_OK != result) {
        RP_LOG(LOG_ERR, "*AXIACQ:START Failed to start Red Pitaya acquire: %s\n", rp_GetError(result));
        return SCPI_RES_ERR;
    }

    RP_LOG(LOG_INFO, "*AXIACQ:START Successful started Red Pitaya acquire.\n");
    return SCPI_RES_OK;
}   
    
scpi_result_t RP_AxiAcqStop(scpi_t *context) {
    int result = RP_OK;
    
    if (RP_OK != result) {
        RP_LOG(LOG_ERR, "*AXIACQ:STOP Failed to stop Red Pitaya acquisition: %s\n", rp_GetError(result));
        return SCPI_RES_ERR;
    }

    RP_LOG(LOG_INFO, "*AXIACQ:STOP Successful stopped Red Pitaya acquire.\n");
    return SCPI_RES_OK;
}

scpi_result_t RP_AxiAcqReset(scpi_t *context) {
    int result = RP_OK;

    if (RP_OK != result) {
        RP_LOG(LOG_ERR, "*AXIACQ:RST Failed to reset Red Pitaya acquire: %s\n", rp_GetError(result));
        return SCPI_RES_ERR;
    }

    context->binary_output = false;

    RP_LOG(LOG_INFO, "*AXIACQ:RST Successful reset  Red Pitaya acquire.\n");
    return SCPI_RES_OK;
}

scpi_result_t RP_AxiAcqStartQ(scpi_t * context) {
    return SCPI_RES_OK;
}
scpi_result_t RP_AxiAcqStopQ(scpi_t * context) {
    return SCPI_RES_OK;
}
scpi_result_t RP_AxiAcqThreshold(scpi_t * context) {
    return SCPI_RES_OK;
}
scpi_result_t RP_AxiAcqThresholdQ(scpi_t * context) {
    return SCPI_RES_OK;
}
scpi_result_t RP_AxiAcqBurstRepetitions(scpi_t * context) {
    return SCPI_RES_OK;
}
scpi_result_t RP_AxiAcqBurstRepetitionsQ(scpi_t * context) {
    return SCPI_RES_OK;
}
scpi_result_t RP_AxiAcqTriggerSource(scpi_t * context) {
    return SCPI_RES_OK;
}
scpi_result_t RP_AxiAcqTriggerSourceQ(scpi_t * context) {
    return SCPI_RES_OK;
}
scpi_result_t RP_AxiAcqTrigger(scpi_t *context) {
    return SCPI_RES_OK;
}

