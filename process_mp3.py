"""
@file process_mp3.py
@author James Vollmer (jrvollmer@wisc.edu) - Team 01
@brief Python script to convert MP3 files to an array of samples
"""
import librosa
import matplotlib.pyplot as plt
import numpy as np
from argparse import ArgumentParser


def process_mp3(mp3_file, sampling_rate, out_file, duration):
    # Process the audio file to get an array of sample points
    s, sr = librosa.load(mp3_file, sr=sampling_rate, duration=duration)
    print(len(s))

    # Scale and offset samples to be within [0, 4095]
    rng = max(s) - min(s)
    s = np.astype(np.round(4095.0 * s / rng), np.int32)
    s = s[s != 0] - min(s)
    print(len(s))

    # Output array to file for copying into the C file
    np.savetxt(out_file, [s], fmt='0x%03x', delimiter=",")

    # Plot for manual validation of waveform
    plt.plot(range(len(s)), s)
    plt.show()


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("-f", "--mp3-file", type=str, help="Path to the mp3 file to process")
    parser.add_argument("-o", "--output-file", type=str, default="./samples.csv", help="Path to the output file")
    parser.add_argument("-r", "--sample-rate", choices=range(20001), type=int, default=None, help="Sampling rate when processing the audio file")
    parser.add_argument("-d", "--duration", type=float, default=None, help="How many seconds of the audio file to keep in sample")
    args = parser.parse_args()

    process_mp3(args.mp3_file, args.sample_rate, args.output_file, args.duration)
