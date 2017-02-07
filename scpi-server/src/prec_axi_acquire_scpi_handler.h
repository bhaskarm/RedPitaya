/**
 * $Id: $
 *
 * @brief Red Pitaya Scpi server acquire SCPI commands interface
 *
 * @Author Red Pitaya
 *
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 */


#ifndef PREC_AXI_ACQUIRE_SCPI_HANDLER_H_
#define PREC_AXI_ACQUIRE_SCPI_HANDLER_H_

#include "scpi/types.h"

scpi_result_t RP_AxiAcqStart(scpi_t * context);
scpi_result_t RP_AxiAcqStartQ(scpi_t * context);
scpi_result_t RP_AxiAcqStop(scpi_t * context);
scpi_result_t RP_AxiAcqStopQ(scpi_t * context);
scpi_result_t RP_AxiAcqReadCalib(scpi_t * context);
scpi_result_t RP_AxiAcqReset(scpi_t * context);
scpi_result_t RP_AxiAcqSampleLength(scpi_t * context);
scpi_result_t RP_AxiAcqSampleLengthQ(scpi_t * context);

// Hack to add uin32 parser command
scpi_bool_t SCPI_ParamBufferUInt32(scpi_t * context, uint32_t *data, uint32_t *size, scpi_bool_t mandatory);
#endif /* PREC_AXI_SCPI_ACQUIRE_H_ */
