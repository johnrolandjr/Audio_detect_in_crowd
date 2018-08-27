/*
 * envelope_extractor.c
 *
 *  Created on: Aug 21, 2018
 *      Author: Beau
 */

#include "envelope_extractor.h"
#include "raisedCos.h"
#include "stdlib.h"
#include "nrf_log.h"
#include "processing_debug.h"

#define DECIMATION_SKIP_SAMPLE_CNT 46
#define WINDOW_LEN 4
#define SAMPLE_THRESH 50
#define MAX_SUM 30000
int evelop_extractor(q15_t * pInput, int numIn)
{
	int loudness = 0;

	//Double Window to extract the envelope
	/* works but, the output isn't exactly what I want it, doesn't fully capture the envelope
	 * so, we will just calculate area under the curve (and above threshold)
	int envSize = numIn - (WINDOW_LEN << 1);
	q15_t env[numIn];
	for(int a=WINDOW_LEN; a< (numIn - WINDOW_LEN); a++)
	{
		int left=0,right=0;
		for(int b=WINDOW_LEN; b> 0 ; b--)
		{
			if(pInput[a - b] > left)
				left = pInput[a - b];
		}
		for(int b=1; b <= WINDOW_LEN; b++)
		{
			if(pInput[a + b] > right)
				right = pInput[a +b];
		}
		if(left > right)
			env[a-WINDOW_LEN] = right;
		else
			env[a-WINDOW_LEN] = left;
	}

	for(int i=numIn - (WINDOW_LEN << 1); i< numIn; i++)
	{
		env[i] = 0;
	}

	chart_data(env, numIn);
	*/
	//calculate loudness
	int sumOverthreshold = 0;
	for(int a=0; a<numIn; a++)
	{
		if(pInput[a] > SAMPLE_THRESH)
			sumOverthreshold += pInput[a] - SAMPLE_THRESH;
	}

	//NRF_LOG_RAW_INFO("%d\r\n", sumOverthreshold);

	if(sumOverthreshold > MAX_SUM)
		loudness = 10000;
	else
		loudness = sumOverthreshold * 10000 / MAX_SUM;
	return loudness;
}
