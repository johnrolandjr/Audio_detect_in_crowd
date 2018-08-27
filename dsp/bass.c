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

#include "bass.h"

#include <stdlib.h> // For malloc/free
#include <string.h> // For memset

q15_t bass_coefficients[12] =
{
// Scaled for a 16x16:32 Direct form 1 IIR filter with:
// Feedback shift = 14
// Output shift = 14
// Input  has a maximum value of 1, scaled by 2^15
// Output has a maximum value of 1.6698209558014798, scaled by 2^14

    7, 0, 14, 7, 31642, -15294,// b0 Q13(0.000883), 0, b1 Q13(0.00177), b2 Q13(0.000883), a1 Q14(1.93), a2 Q14(-0.933)
    8, 0, 16, 8, 32201, -15924// b0 Q13(0.000977), 0, b1 Q13(0.00195), b2 Q13(0.000977), a1 Q14(1.97), a2 Q14(-0.972)

};


bassType *bass_create( void )
{
	bassType *result = (bassType *)malloc( sizeof( bassType ) );	// Allocate memory for the object
	bass_init( result );											// Initialize it
	return result;																// Return the result
}

void bass_destroy( bassType *pObject )
{
	free( pObject );
}

 void bass_init( bassType * pThis )
{
	arm_biquad_cascade_df1_init_q15(	&pThis->instance, bass_numStages, bass_coefficients, pThis->state, bass_postShift );
	bass_reset( pThis );

}

 void bass_reset( bassType * pThis )
{
	memset( &pThis->state, 0, sizeof( pThis->state ) ); // Reset state to 0
	pThis->output = 0;									// Reset output

}

 int bass_filterBlock( bassType * pThis, short * pInput, short * pOutput, unsigned int count )
{
	arm_biquad_cascade_df1_fast_q15( &pThis->instance, pInput, pOutput, count );
	return count;

}


