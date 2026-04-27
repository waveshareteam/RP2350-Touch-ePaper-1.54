/*****************************************************************************
* | File      	:   audio_pio.c
* | Author      :   Waveshare Team
* | Function    :   ES8311 control related PIO interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-02-26
* | Info        :   
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "audio_pio.h"
#include "audio_data.h"
#include "audio_pio.pio.h"
#include "music.h"

/******************************************************************************
function: Mclk frequency modification
parameter:
    mclk_freq :  mclk freq
******************************************************************************/								
void set_mclk_frequency(uint32_t mclk_freq) 
{
	double system_clock_frequency = clock_get_hz(clk_sys);
    double div = (system_clock_frequency / mclk_freq) / 5; 
    pio_sm_set_clkdiv(pico_audio.pio_1, pico_audio.sm_mclk, div);
}

/******************************************************************************
function: 16 bit unsigned audio data processing
parameter:
    audio :  16-bit audio array
    len   :  The length of the array 
return:  The address of a 32-bit array
******************************************************************************/	
int32_t* data_treating(const int16_t *audio , uint32_t len)
{
	int32_t *samples = (int32_t *)calloc(len, sizeof(int32_t));
	for(uint32_t i = 0; i < len; i++)
	{
		if(pico_audio.channel_count == 1)
		{
			samples[i] = audio[i] * 65536;
		}
		else
		{
			samples[i] = audio[i] * 65536 + audio[i];
		}
	}
	return samples;
}

/******************************************************************************
function: audio out
parameter:
    samples :  32-bit audio array
    len     :  The length of the array
******************************************************************************/	
void audio_out(int32_t *samples, int32_t len) 
{
	for(uint16_t i = 0; i < len; i++)
	   	pio_sm_put_blocking(pico_audio.pio_2, pico_audio.sm_dout, samples[i]);
}

/******************************************************************************
function: PIO output initialization
parameter:
******************************************************************************/	
void dout_pio_init()
{
    pio_sm_claim(pico_audio.pio_2, pico_audio.sm_dout);
    uint offset = pio_add_program(pico_audio.pio_2, &audio_pio_program);
	audio_pio_program_init(pico_audio.pio_2, pico_audio.sm_dout , offset, pico_audio.audio_dout, pico_audio.audio_lrclk);
	pio_sm_set_clkdiv(pico_audio.pio_2, pico_audio.sm_dout, 1.0f);
    pio_sm_set_enabled(pico_audio.pio_2, pico_audio.sm_dout , true);
}

/******************************************************************************
function: PIO input initialization
parameter:
******************************************************************************/	
void din_pio_init()
{
    pio_sm_claim(pico_audio.pio_1, pico_audio.sm_din);
    uint offset = pio_add_program(pico_audio.pio_1, &read_pio_program);
	read_pio_program_init(pico_audio.pio_1, pico_audio.sm_din , offset, pico_audio.audio_din, pico_audio.audio_lrclk);
    pio_sm_set_clkdiv(pico_audio.pio_1, pico_audio.sm_din, 1.0f);
    pio_sm_set_enabled(pico_audio.pio_1, pico_audio.sm_din , true);
}

/******************************************************************************
function: MCLK pin PIO initialization
parameter:
******************************************************************************/	
void mclk_pio_init()
{
    pio_sm_claim(pico_audio.pio_1, pico_audio.sm_mclk);
    uint offset = pio_add_program(pico_audio.pio_1, &mclk_pio_program);
    mclk_pio_program_init(pico_audio.pio_1, pico_audio.sm_mclk, offset, pico_audio.audio_mclk);
    set_mclk_frequency(pico_audio.mclk_freq);
    pio_sm_set_enabled(pico_audio.pio_1, pico_audio.sm_mclk , true);
}

/******************************************************************************
function: Play Happy Birthday
******************************************************************************/	
void Happy_birthday_out()
{
    //MCLK
    mclk_pio_init();
    //WRITE
    dout_pio_init(); 
	int len = 124800;
    while (true) 
    {	
      for(int i = 0; i < len; i++)
        pio_sm_put_blocking(pico_audio.pio_2, pico_audio.sm_dout, Happy_birsday[i] * 65535);
    }
}

/******************************************************************************
function: Output 440HZ sine wave test
******************************************************************************/	
void Sine_440hz_out()
{
    //MCLK
    mclk_pio_init();
    //WRITE
    dout_pio_init(); 
	int len = 24000;
    while (true) 
    {	
      for(int i = 0; i < len; i++)
        pio_sm_put_blocking(pico_audio.pio_2, pico_audio.sm_dout, Sine_440hz[i] * 65535);
    }
}

/******************************************************************************
function: Recording and playback loopback test
******************************************************************************/	
void Loopback_test()
{
    static int32_t *mic_sample_buffers;
    static int mic_num_samples = 120000;

    mic_sample_buffers = malloc(mic_num_samples * sizeof(int32_t));
    memset(mic_sample_buffers, 0, mic_num_samples * sizeof(int32_t)); 

    //MCLK
    mclk_pio_init();
    //READ
    din_pio_init();
    //WRITE
    dout_pio_init();
    pio_sm_set_enabled(pico_audio.pio_2, pico_audio.sm_dout, false);
    
    while (true) 
    {	
        //READ
        pio_sm_set_enabled(pico_audio.pio_1, pico_audio.sm_din, true);
        for (int i = 0; i < mic_num_samples; i ++)
            mic_sample_buffers[i] = pio_sm_get_blocking(pico_audio.pio_1, pico_audio.sm_din);
        pio_sm_set_enabled(pico_audio.pio_1, pico_audio.sm_din, false);

        //WRITE
        pio_sm_set_enabled(pico_audio.pio_2, pico_audio.sm_dout, true);
        for(int i = 0; i < mic_num_samples; i++)
            pio_sm_put_blocking(pico_audio.pio_2, pico_audio.sm_dout, mic_sample_buffers[i]);
        pio_sm_set_enabled(pico_audio.pio_2, pico_audio.sm_dout, false);
    }
}

/******************************************************************************
function: Play music
******************************************************************************/	
void Music_out()
{
    //MCLK
    mclk_pio_init();
    //WRITE
    dout_pio_init(); 
    while (true) 
    {	
      for(int i = 0; i < AUDIO_SAMPLES; i++)
        pio_sm_put_blocking(pico_audio.pio_2, pico_audio.sm_dout, audio_data[i] * 65535);
    }
}
