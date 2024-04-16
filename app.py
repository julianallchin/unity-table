from os import environ

from flask import Flask, jsonify, render_template, request

environ["PYGAME_HIDE_SUPPORT_PROMPT"] = "1"

import json
import os
import threading
import time
from collections import deque

import librosa
import numpy as np
import pygame
import soundfile as sf
import logging

app = Flask(__name__)
log = logging.getLogger("werkzeug")
log.setLevel(logging.ERROR)

volume_changes = deque()


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/update", methods=["POST"])
def update():
    global next_beat_time, bar_duration, volume_changes
    data = request.json
    index = int(data["index"])
    state = data["state"]

    if index not in samples:
        return jsonify(success=False, error="Invalid index")

    sample = samples[index]

    new_volume = 1.0 if state else 0.0
    current_time = time.time()
    elapsed_time = (current_time - next_beat_time) % bar_duration

    if elapsed_time < (bar_duration / 2) and new_volume == 1.0:
        # Remove any existing volume changes for this index
        volume_changes = deque([item for item in volume_changes if item[0] != index])
        # Queue the new volume change
        volume_changes.append((index, new_volume))
    else:
        # Remove any existing volume changes for this index before applying immediately
        volume_changes = deque([item for item in volume_changes if item[0] != index])
        sample["channel"].set_volume(new_volume)

    return jsonify(success=True)


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


def timing_thread():
    global next_beat_time
    start_time = time.time()
    next_beat_time = start_time + beat_duration

    while True:
        time_to_wait = next_beat_time - time.time()
        if time_to_wait > 0:
            time.sleep(time_to_wait)

        while volume_changes:
            index, volume = volume_changes.popleft()
            samples[index]["channel"].set_volume(volume)
            print(f"Set volume for channel {index} to {volume}.")

        next_beat_time += beat_duration


if __name__ == "__main__":
    # Initial setup for pygame mixer
    pygame.mixer.init()
    pygame.mixer.set_num_channels(32)
    current_set = "set2"
    samples_dir = "samples/"
    config_path = os.path.join(samples_dir, current_set, "config.json")

    # Read configuration
    with open(config_path, "r") as f:
        config = json.load(f)

    bpm = config["bpm"]
    beat_duration = 60 / bpm
    bar_duration = beat_duration * 4

    samples = load_samples(current_set)
    play_samples(samples)

    timing = threading.Thread(target=timing_thread)
    timing.daemon = True
    timing.start()
    app.run(debug=False)
