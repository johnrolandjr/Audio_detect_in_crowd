/*
 * bass_fir.c
 *
 *  Created on: Aug 26, 2018
 *      Author: Beau
 */

#include "arm_math.h"

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 16000 Hz

fixed point precision: 16 bits

* 0 Hz - 200 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = n/a

* 500 Hz - 8000 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = n/a

*/

#define FILTER_TAP_NUM 74

static q15_t bass_taps[FILTER_TAP_NUM] = {
  -151,
  -9,
  -6,
  0,
  9,
  20,
  35,
  53,
  75,
  100,
  129,
  162,
  199,
  239,
  283,
  331,
  381,
  434,
  490,
  547,
  606,
  666,
  726,
  785,
  844,
  901,
  956,
  1007,
  1056,
  1100,
  1139,
  1173,
  1202,
  1225,
  1241,
  1251,
  1254,
  1251,
  1241,
  1225,
  1202,
  1173,
  1139,
  1100,
  1056,
  1007,
  956,
  901,
  844,
  785,
  726,
  666,
  606,
  547,
  490,
  434,
  381,
  331,
  283,
  239,
  199,
  162,
  129,
  100,
  75,
  53,
  35,
  20,
  9,
  0,
  -6,
  -9,
  -151,
  0
};

/* -------------------------------------------------------------------
 * Declare State buffer of size (numTaps + blockSize - 1)
 * ------------------------------------------------------------------- */

q15_t firStateQ15[512 + FILTER_TAP_NUM];
arm_fir_instance_q15 bass_Fir_Instance;
void initBassFir(void)
{
	arm_status stat = arm_fir_init_q15(&bass_Fir_Instance, FILTER_TAP_NUM, &bass_taps[0], &firStateQ15[0], 512);
    memset( &firStateQ15[0], 0, sizeof( firStateQ15 ) ); // Reset state to 0
}

int bassFir_filterBlock( q15_t * pInput, q15_t * pOutput, unsigned int count )
{

	arm_fir_fast_q15(&bass_Fir_Instance,pInput,pOutput,count);
	return count;

}
