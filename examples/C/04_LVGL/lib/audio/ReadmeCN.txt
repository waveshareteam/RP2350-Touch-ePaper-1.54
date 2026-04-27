/*****************************************************************************
* | File      	:   Readme_CN.txt
* | Author      :   
* | Function    :   Help with use
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-11-4
* | Info        :   在这里提供一个中文版本的使用文档，以便你的快速使用
******************************************************************************/
这个文件是帮助您使用本例程。
在这里简略的描述本工程的使用：

1.基本信息：
(1)Sine_440hz      // 输出440HZ正弦波
(2)Happy_birthday  // 输出包含不同音调的音乐
(3)Loopback_test   // 麦克风回环测试
(4)Music_out       // 播放歌曲

2.音频相关宏定义：
您可以在\lib\audio\audio_data目录下对audio_data.h的宏定义进行修改
PICO_MCLK_FREQ          // 主时钟频率
PICO_SAMPLE_FREQ        // 采样频率
PICO_AUDIO_COUNT        // 通道数
PICO_AUDIO_RES_IN       // 输入位深度
PICO_AUDIO_RES_OUT      // 输出位深度
PICO_AUDIO_MIC_GAIN     // 输入增益
PICO_AUDIO_VOLUME       // 输出音量
PICO_AUDIO_DOUT         // 数据输出引脚
PICO_AUDIO_DIN          // 数据输入引脚
PICO_AUDIO_MCLK         // 主时钟引脚
PICO_AUDIO_LRCLK        // 左右声道时钟引脚
PICO_AUDIO_BCLK         // 位时钟引脚
PICO_AUDIO_SM_DOUT      // 输出状态机编号
PICO_AUDIO_SM_DIN       // 输入状态机编号
PICO_AUDIO_SM_MCLK      // 时钟状态机编号

3.wav格式音频转C语言数组
示例中提供了一个名为 wav2data.py 的Python脚本，位于 \lib\audio\python 目录下，
该脚本可以将24KHz采样率、16位深度的WAV音频文件转换为C语言数组格式。
通过使用此脚本处理音频文件后，生成的 .h 文件可以直接添加到您的项目中以实现音频播放功能，
运行示例: python wav2data.py music.wav music.h
为了确保音频文件符合所需的格式要求，您可以使用如Audacity等音频编辑工具来进行格式转换。
