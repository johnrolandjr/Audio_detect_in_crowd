/*
 * bass_fir.h
 *
 *  Created on: Aug 26, 2018
 *      Author: Beau
 */

#ifndef DSP_BASS_FIR_H_
#define DSP_BASS_FIR_H_

void initBassFir(void);

int bassFir_filterBlock( q15_t * pInput, q15_t * pOutput, unsigned int count );

#endif /* DSP_BASS_FIR_H_ */
