import json
import os
import time
import librosa
import pygame
import soundfile as sf
import serial
import logging

log = logging.getLogger("werkzeug")
log.setLevel(logging.ERROR)

serial_port = "/dev/cu.usbmodem157963901"
samples_dir = "samples/"

baud_rate = 9600
max_rings = 20


def load_samples(sample_set):
    config_path = os.path.join(samples_dir, sample_set, "config.json")
    with open(config_path, "r") as f:
        sample_config = json.load(f)

    samples = {}
    for i, sample_info in enumerate(sample_config["samples"]):
        file_path = os.path.join(samples_dir, sample_set, sample_info["file"])

        # Load the sample with librosa
        y, sr = librosa.load(
            file_path,
            sr=None,
        )

        # If there is a bpm key on the object, remap the bpm of the sample to the target bpm
        if "bpm" in sample_info:
            # Calculate the speed change
            speed_change = sample_config["bpm"] / sample_info["bpm"]
            print(f"Changing speed of {sample_info['file']} by {speed_change:.2f}")
            # Change the speed of the sound
            y = librosa.effects.time_stretch(y, rate=speed_change)

        # Export the sound to a temporary file and load it into Pygame
        temp_file = "temp.wav"
        sf.write(temp_file, y, sr)
        pygame_sound = pygame.mixer.Sound(temp_file)

        channel = pygame.mixer.Channel(i)
        channel.set_volume(0.0)
        samples[i] = {
            "sound": pygame_sound,
            "channel": channel,
            "play_mode": sample_info["play_mode"],
            "volume": 0.0,  # Start muted
        }

    return samples


def play_samples(samples):
    for sample in samples.values():
        sample["channel"].play(sample["sound"], loops=-1)


def main():
    global samples

    # Initial setup for pygame mixer
    pygame.mixer.init()
    pygame.mixer.set_num_channels(32)
    current_set = "set1"
    config_path = os.path.join(samples_dir, current_set, "config.json")

    # Read configuration
    with open(config_path, "r") as f:
        config = json.load(f)

    bpm = config["bpm"]
    beat_duration = 60 / bpm
    bar_duration = beat_duration * 4

    samples = load_samples(current_set)
    play_samples(samples)

    # Initialize serial communication
    ser = serial.Serial(serial_port, baud_rate)

    while True:
        if ser.in_waiting > 0:
            data = ser.readline().decode("utf-8").strip()
            volumes = data.split(",")
            for i in range(len(volumes)):
                try:
                    volume = float(volumes[i])
                    if 0 <= volume <= 1 and i < len(samples):
                        samples[i]["channel"].set_volume(volume)
                except ValueError:
                    continue

        time.sleep(0.05)  # Add a small delay to avoid high CPU usage


if __name__ == "__main__":
    main()
