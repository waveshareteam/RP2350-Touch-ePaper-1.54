/********************************************************************************
* | File : Readme_EN.txt
* | Author :
* | Function : Help with use
* | Info :
*----------------
* | This version: V1.0
* | Date : 2025-11-4
* | Info : Here is a Chinese version of the usage document for your quick use
******************************************************************************/
This file is to help you use this example.
Here is a brief description of the use of this project:

1. Basic information:
(1)Sine_440hz.uf2     // Output 440HZ sine wave
(2)Happy_birthday.uf2 // Output music with different tones
(3)Loopback_test.uf2  // Microphone loopback test
(4)Music_out.uf2      // Play songs

2. Audio related macro definitions:
You can modify the macro definitions of audio_data.h in the \lib\audio\audio_data directory
PICO_MCLK_FREQ          // Master clock frequency
PICO_SAMPLE_FREQ        // Sampling frequency
PICO_AUDIO_COUNT        // Number of channels
PICO_AUDIO_RES_IN       // Input bit depth
PICO_AUDIO_RES_OUT      // Output bit depth
PICO_AUDIO_MIC_GAIN     // Input gain
PICO_AUDIO_VOLUME       // Output volume
PICO_AUDIO_DOUT         // Data output pin
PICO_AUDIO_DIN          // Data input pin
PICO_AUDIO_MCLK         // Master clock pin
PICO_AUDIO_LRCLK        // Left and right channel clock pins
PICO_AUDIO_BCLK         // Bit clock pin
PICO_AUDIO_SM_DOUT      // Output state machine number
PICO_AUDIO_SM_DIN       // Input state machine number
PICO_AUDIO_SM_MCLK      // Clock state machine number

3. Convert wav format audio to C language array
A Python script named wav2data.py is provided in the example, located in the \lib\audio\python directory.
This script can convert a WAV audio file with a sampling rate of 24KHz and a depth of 16 bits into a C language array format.
After processing the audio file using this script, the generated .h file can be directly added to your project to implement the audio playback function.
Run the example: python wav2data.py music.wav music.h
To ensure that the audio file meets the required format requirements, you can use audio editing tools such as Audacity to perform format conversion.
