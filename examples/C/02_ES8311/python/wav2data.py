import wave
import numpy as np
import sys

def wav_to_c_array(input_wav, output_header):
    # Open and verify the WAV file format
    try:
        with wave.open(input_wav, 'rb') as wav:
            n_channels = wav.getnchannels()
            sample_width = wav.getsampwidth()
            frame_rate = wav.getframerate()
            n_frames = wav.getnframes()
            audio_data = wav.readframes(n_frames)
    except Exception as e:
        print(f"Unable to read WAV file: {str(e)}")
        sys.exit(1)

    # Format Check
    required_params = {
        "Sampling rate": (frame_rate, 24000),
        "Bit Depth": (sample_width*8, 16),
        "Number of channels": (n_channels, 1)
    }
    
    for param, (actual, required) in required_params.items():
        if actual != required:
            print(f"Parameter error: {param} Should be{required}, The actual detection is{actual}")
            sys.exit(1)

    # Convert to 16-bit signed integer array
    dtype = np.int16 if sample_width == 2 else None
    if dtype is None:
        print("Unsupported bit depth)
        sys.exit(1)

    # Handle endianness and verify data integrity
    audio_array = np.frombuffer(audio_data, dtype=dtype)
    
    # Generate C header file
    with open(output_header, 'w') as f:
        f.write("#ifndef AUDIO_DATA_H\n")
        f.write("#define AUDIO_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"// Sampling rate: {frame_rate}Hz\n")
        f.write(f"// Bit Depth: {sample_width*8}bit\n")
        f.write(f"// Audio duration: {n_frames/frame_rate:.2f}s\n")
        f.write(f"const uint32_t AUDIO_SAMPLES = {len(audio_array)};\n")
        
        # Optimized storage format: 4-byte alignment, 16 samples per line
        f.write("const int16_t audio_data[] __attribute__((aligned(4))) = {\n")
        for i in range(0, len(audio_array), 16):
            line = audio_array[i:i+16]
            hex_values = [f"0x{int(v) & 0xFFFF:04X}" for v in line]
            f.write("    " + ", ".join(hex_values))
            f.write(",\n" if i+16 < len(audio_array) else "\n")
        f.write("};\n\n")
        f.write("#endif // AUDIO_DATA_H\n")

    print(f"Conversion successful! Output file: {output_header}")
    print(f"Total number of samples: {len(audio_array)}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("How to use: python wav_to_c.py <input.wav> <output.h>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    wav_to_c_array(input_file, output_file)