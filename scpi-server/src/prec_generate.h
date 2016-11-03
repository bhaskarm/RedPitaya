/**
 * $Id: $
 *
 * @brief Red Pitaya Scpi server generate SCPI commands interface
 *
 * @Author Red Pitaya
 *
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 */


#ifndef PREC_SCPI_GENERATE_H_
#define PREC_SCPI_GENERATE_H_

#include "scpi/types.h"

scpi_result_t RP_GenState(scpi_t * context);
scpi_result_t RP_GenStateQ(scpi_t * context);
scpi_result_t RP_GenReset(scpi_t * context);
scpi_result_t RP_GenFrequency(scpi_t * context);
scpi_result_t RP_GenFrequencyQ(scpi_t * context);
scpi_result_t RP_GenWaveForm(scpi_t * context);
scpi_result_t RP_GenWaveFormQ(scpi_t * context);
scpi_result_t RP_GenAmplitude(scpi_t * context);
scpi_result_t RP_GenAmplitudeQ(scpi_t * context);
scpi_result_t RP_GenOffset(scpi_t * context);
scpi_result_t RP_GenOffsetQ(scpi_t * context);
scpi_result_t RP_GenPhase(scpi_t * context);
scpi_result_t RP_GenPhaseQ(scpi_t * context);
scpi_result_t RP_GenDutyCycle(scpi_t * context);
scpi_result_t RP_GenDutyCycleQ(scpi_t * context);
scpi_result_t RP_GenArbitraryWaveFormFixedPoint(scpi_t * context);
scpi_result_t RP_GenArbitraryWaveFormFixedPointQ(scpi_t * context);
scpi_result_t RP_GenArbitraryWaveForm(scpi_t * context);
scpi_result_t RP_GenArbitraryWaveFormQ(scpi_t * context);
scpi_result_t RP_GenGenerateMode(scpi_t * context);
scpi_result_t RP_GenGenerateModeQ(scpi_t * context);
scpi_result_t RP_GenBurstCount(scpi_t * context);
scpi_result_t RP_GenBurstCountQ(scpi_t * context);
scpi_result_t RP_GenPhaseBits(scpi_t * context);
scpi_result_t RP_GenPhaseBitsQ(scpi_t * context);
scpi_result_t RP_GenBurstRepetitions(scpi_t * context);
scpi_result_t RP_GenBurstRepetitionsQ(scpi_t * context);
scpi_result_t RP_GenBurstPeriod(scpi_t * context);
scpi_result_t RP_GenBurstPeriodQ(scpi_t * context);
scpi_result_t RP_GenTriggerSource(scpi_t * context);
scpi_result_t RP_GenTriggerSourceQ(scpi_t * context);
scpi_result_t RP_GenTrigger(scpi_t *context);

// Hack to add uin32 parser command
scpi_bool_t SCPI_ParamBufferUInt32(scpi_t * context, uint32_t *data, uint32_t *size, scpi_bool_t mandatory);
#endif /* PREC_SCPI_GENERATE_H_ */
