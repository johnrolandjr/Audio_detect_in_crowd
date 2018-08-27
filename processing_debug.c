/*
 * processing_debug.c
 *
 *  Created on: Aug 26, 2018
 *      Author: Beau
 */

#include "processing_debug.h"

#define SLOWER_FACTOR_RATE 0
void chart_data(q15_t * pData, int size)
{
	static int slowCnt = SLOWER_FACTOR_RATE;
	if(slowCnt <= 0)
	{
		slowCnt = SLOWER_FACTOR_RATE;
		NRF_LOG_RAW_INFO("s");
		for(int a=0; a<size;a++)
		{
			if(a< (size-1)){
				NRF_LOG_RAW_INFO("%d,", pData[a]);

			}else{
				NRF_LOG_RAW_INFO("%d\r\n", pData[a]);
			}
			NRF_LOG_FLUSH();
		}

	}else
	{
		slowCnt--;
	}
}
