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

// Begin header file, filter1.h

#ifndef FILTER1_H_ // Include guards
#define FILTER1_H_

#ifndef ARM_MATH_CM4 //ensures that we fully utilize the ARM M4's DSP unit
	#define ARM_MATH_CM4	// Use ARM Cortex M4
#endif
#define __FPU_PRESENT 1		// Does this device have a floating point unit?
#include <arm_math.h>	// Include CMSIS header

// Link with library: libarm_cortexM4_mathL.a (or equivalent)
// Add CMSIS/Lib/GCC to the library search path
// Add CMSIS/Include to the include search path
extern q15_t filter1_coefficients[12];
static const int filter1_numStages = 2;
static const int filter1_postShift = 1;

typedef struct
{
	arm_biquad_casd_df1_inst_q15 instance;
	q15_t state[8];
	q15_t output;
} filter1Type;


filter1Type *filter1_create( void );
void filter1_destroy( filter1Type *pObject );
 void filter1_init( filter1Type * pThis );
 void filter1_reset( filter1Type * pThis );
#define filter1_writeInput( pThis, input )  \
	arm_biquad_cascade_df1_fast_q15( &pThis->instance, &input, &pThis->output, 1 );

#define filter1_readOutput( pThis )  \
	pThis->output


 int filter1_filterBlock( filter1Type * pThis, short * pInput, short * pOutput, unsigned int count );
#define filter1_outputToFloat( output )  \
	(( (1.0f/16384) * (output) ))

#define filter1_inputFromFloat( input )  \
	((short)(32768f * (input)))

#endif // FILTER1_H_
