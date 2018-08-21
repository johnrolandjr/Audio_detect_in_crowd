/*
 * stage_1_filterbank.c
 *
 *  Created on: Aug 20, 2018
 *      Author: Beau
 */
/******************************* SOURCE LICENSE *********************************
Copyright (c) 2018 MicroModeler.

A non-exclusive, nontransferable, perpetual, royalty-free license is granted to the Licensee to
use the following Information for academic, non-profit, or government-sponsored research purposes.
Use of the following Information under this License is restricted to NON-COMMERCIAL PURPOSES ONLY.
Commercial use of the following Information requires a separately executed written license agreement.

This Information is distributed WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

******************************* END OF LICENSE *********************************/

// A commercial license for MicroModeler DSP can be obtained at http://www.micromodeler.com/launch.jsp

#include "stage_1_filterbank.h"

#include <stdlib.h> // For malloc/free
#include <string.h> // For memset

q15_t filter1_coefficients[12] =
{
// Scaled for a 16x16:32 Direct form 1 IIR filter with:
// Feedback shift = 14
// Output shift = 14
// Input  has a maximum value of 1, scaled by 2^15
// Output has a maximum value of 1.6997154508031926, scaled by 2^14

    43, 0, 87, 43, 29353, -13174,// b0 Q13(0.00531), 0, b1 Q13(0.0106), b2 Q13(0.00531), a1 Q14(1.79), a2 Q14(-0.804)
    32, 0, 64, 32, 31140, -14974// b0 Q14(0.00195), 0, b1 Q14(0.00391), b2 Q14(0.00195), a1 Q14(1.90), a2 Q14(-0.914)

};


filter1Type *filter1_create( void )
{
	filter1Type *result = (filter1Type *)malloc( sizeof( filter1Type ) );	// Allocate memory for the object
	filter1_init( result );											// Initialize it
	return result;																// Return the result
}

void filter1_destroy( filter1Type *pObject )
{
	free( pObject );
}

 void filter1_init( filter1Type * pThis )
{
	arm_biquad_cascade_df1_init_q15(	&pThis->instance, filter1_numStages, filter1_coefficients, pThis->state, filter1_postShift );
	filter1_reset( pThis );

}

 void filter1_reset( filter1Type * pThis )
{
	memset( &pThis->state, 0, sizeof( pThis->state ) ); // Reset state to 0
	pThis->output = 0;									// Reset output

}

 int filter1_filterBlock( filter1Type * pThis, short * pInput, short * pOutput, unsigned int count )
{
	arm_biquad_cascade_df1_fast_q15( &pThis->instance, pInput, pOutput, count );
	return count;

}

