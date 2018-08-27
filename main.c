/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/** @file
 * @defgroup nrf_adc_example main.c
 * @{
 * @ingroup nrf_adc_example
 * @brief ADC Example Application main file.
 *
 * This file contains the source code for a sample application using ADC.
 *
 * @image html example_board_setup_a.jpg "Use board setup A for this example."
 */

#include <bass.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "boards.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_drv_power.h"
#include "nrf_drv_pwm.h"
#include "arm_common_tables.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "hann.h"
#include "envelope_extractor.h"
#include "processing_debug.h"
#include "nrf_mtx.h"
#include "bass_fir.h"

#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define SAMPLES_IN_BUFFER 512
volatile uint8_t state = 1;

static const nrf_drv_timer_t m_timer = NRF_DRV_TIMER_INSTANCE(0);
static nrf_saadc_value_t     m_buffer_pool[2][SAMPLES_IN_BUFFER];
static nrf_ppi_channel_t     m_ppi_channel;
static uint32_t              m_adc_evt_counter;
static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);
static nrf_pwm_values_common_t seq_values[] =
{
    0
};
nrf_pwm_sequence_t const seq =
{
    .values.p_common = seq_values,
    .length          = NRF_PWM_VALUES_LENGTH(seq_values),
    .repeats         = 0,
    .end_delay       = 0
};
arm_rfft_instance_q15  		  RealFFT_Instance;
arm_cfft_radix4_instance_q15  ComplexFFT_Instance;
//q15_t pFft[SAMPLES_IN_BUFFER<<1];
//q15_t pFftMags[SAMPLES_IN_BUFFER];
bassType * pBassLpf;
static nrf_mtx_t mtx;

#define EXPECTED_MAX_SUM 2400
#define EXPECTED_MAX_MAG 512
volatile int bProcessing = 0;
q15_t processInput[SAMPLES_IN_BUFFER];
volatile q15_t pFiltered_0[SAMPLES_IN_BUFFER];
void timer_handler(nrf_timer_event_t event_type, void * p_context)
{

}

#define SAMPLE_PERIOD_US 63 // 63US Period = 15873 Hz

void saadc_sampling_event_init(void)
{
    ret_code_t err_code;

    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
    err_code = nrf_drv_timer_init(&m_timer, &timer_cfg, timer_handler);
    APP_ERROR_CHECK(err_code);

    /* setup m_timer for compare event every 400ms */
    uint32_t ticks = nrf_drv_timer_us_to_ticks(&m_timer, SAMPLE_PERIOD_US);
    nrf_drv_timer_extended_compare(&m_timer,
                                   NRF_TIMER_CC_CHANNEL0,
                                   ticks,
                                   NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                   false);
    nrf_drv_timer_enable(&m_timer);

    uint32_t timer_compare_event_addr = nrf_drv_timer_compare_event_address_get(&m_timer,
                                                                                NRF_TIMER_CC_CHANNEL0);
    uint32_t saadc_sample_task_addr   = nrf_drv_saadc_sample_task_get();

    /* setup ppi channel so that timer compare event is triggering sample task in SAADC */
    err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(m_ppi_channel,
                                          timer_compare_event_addr,
                                          saadc_sample_task_addr);
    APP_ERROR_CHECK(err_code);
}

void saadc_sampling_event_enable(void)
{
    ret_code_t err_code = nrf_drv_ppi_channel_enable(m_ppi_channel);

    APP_ERROR_CHECK(err_code);
}

void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        ret_code_t err_code;

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER);
        APP_ERROR_CHECK(err_code);

        if(nrf_mtx_trylock(&mtx))
        {
			//copy data into processing buffer
			for(int i=0; i < SAMPLES_IN_BUFFER; i++)
			{
				processInput[i] = (q15_t) (p_event->data.done.p_buffer[i]);
			}
			bProcessing = 1;
			nrf_mtx_unlock(&mtx);
        }
    }
}

void processData(void)
{
	/*
	 * Process #2: Filter -> Full-Wave Rectify -> Envelope -> Sum Threshold
	 */

	//STAGE 1: FILTERBANK
	bassFir_filterBlock( processInput, pFiltered_0, SAMPLES_IN_BUFFER );

	//Stage 2: remove DC Offset and Full-Wave Rectify
	int avg = 0;
	for(int i=0; i<SAMPLES_IN_BUFFER; i++)
	{
		avg += (int)pFiltered_0[i];
	}
	avg >>= 9;
	for(int i=0; i<SAMPLES_IN_BUFFER; i++)
	{
		pFiltered_0[i] -= avg;
		if(pFiltered_0[i] < 0)
			pFiltered_0[i] *= -1;
	}
	//chart_data(pFiltered_0, SAMPLES_IN_BUFFER);

	//Stage 3: calculate energy under curve and above threshold
	int value = evelop_extractor(pFiltered_0,SAMPLES_IN_BUFFER);
	seq_values[0] = value;
}

void saadc_init(void)
{
    ret_code_t err_code;
    nrf_saadc_channel_config_t channel_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN0);

    err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1], SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);

    //prepare mutex that it will share with foreground
    nrf_mtx_init(&mtx);

}

void led_pwm_init(void)
{
	nrf_drv_pwm_config_t const config0 =
	{
		.output_pins =
		{
			BSP_LED_0 | NRF_DRV_PWM_PIN_INVERTED,   // channel 0
			NRF_DRV_PWM_PIN_NOT_USED,				// channel 1
			NRF_DRV_PWM_PIN_NOT_USED,				// channel 2
			NRF_DRV_PWM_PIN_NOT_USED 				// channel 3
		},
		.irq_priority = APP_IRQ_PRIORITY_LOWEST,
		.base_clock   = NRF_PWM_CLK_1MHz,
		.count_mode   = NRF_PWM_MODE_UP,
		.top_value    = 10000,
		.load_mode    = NRF_PWM_LOAD_COMMON,
		.step_mode    = NRF_PWM_STEP_AUTO
	};

	APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm0, &config0, NULL));

	nrf_drv_pwm_simple_playback(&m_pwm0, &seq, 3, NRF_DRV_PWM_FLAG_LOOP);
}

void dsp_init(void)
{
	initBassFir();
}

/**
 * @brief Function for main application entry.
 */
int main(void)
{
    uint32_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    //NRF_LOG_INFO("SAADC HAL simple example.\r\n");
    saadc_init();
    saadc_sampling_event_init();
    saadc_sampling_event_enable();

    //Initialize the PWM
    led_pwm_init();

    //Init DSP Units
    dsp_init();

    //init debug light
    /*
    NRF_GPIO->OUTCLR = 1<<LED_2;
    NRF_GPIO->PIN_CNF[LED_2] = ((uint32_t)GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos)
                                   | ((uint32_t)GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
                                   | ((uint32_t)GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
                                   | ((uint32_t)GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                                   | ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
     */

    while (1)
    {
    	if(nrf_mtx_trylock(&mtx))
    	{
			if(bProcessing != 0)
			{
				//NRF_GPIO->OUTSET = 1<<LED_2;
				// PROCESS TIME TAKES 3.7ms, at 512 samples @16kHz, thats a span of 33ms, we are good on processing time :)
				processData();
				//NRF_GPIO->OUTCLR = 1<<LED_2;
				bProcessing = 0;
			}
			nrf_mtx_unlock(&mtx);
    	}
        NRF_LOG_FLUSH();
    }
}


/** @} */
