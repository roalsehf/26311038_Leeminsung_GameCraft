"""사막 바람 생성."""

from array import array
import json
import math
from pathlib import Path
import random
import sys
import wave


SAMPLE_RATE = 22050
DURATION_SECONDS = 24
SEED = 26311038
OUTPUT = Path(__file__).resolve().parents[1] / "resource" / "sound" / "desert_wind.wav"


def circular_lowpass(samples, cutoff_hz):
    """반복 구간 저역 필터."""
    retention = math.exp(-2.0 * math.pi * cutoff_hz / SAMPLE_RATE)
    contribution = 1.0 - retention
    state = 0.0
    for sample in samples:
        state = retention * state + contribution * sample
    # 반복 경계 초기값
    state /= 1.0 - retention ** len(samples)
    filtered = array("d")
    for sample in samples:
        state = retention * state + contribution * sample
        filtered.append(state)
    return filtered


def rms(samples):
    return math.sqrt(sum(sample * sample for sample in samples) / len(samples))


def generate():
    sample_count = SAMPLE_RATE * DURATION_SECONDS
    rng = random.Random(SEED)
    noise = array("d", (rng.uniform(-1.0, 1.0) for _ in range(sample_count)))
    low = circular_lowpass(noise, 85.0)
    medium = circular_lowpass(noise, 650.0)
    high = circular_lowpass(noise, 2400.0)
    # 초저역 제거
    sub = circular_lowpass(noise, 18.0)
    low_band = array("d", (a - b for a, b in zip(low, sub)))
    mid_band = array("d", (a - b for a, b in zip(medium, low)))
    air_band = array("d", (a - b for a, b in zip(high, medium)))
    low_gain = 0.70 / rms(low_band)
    mid_gain = 0.25 / rms(mid_band)
    air_gain = 0.05 / rms(air_band)
    samples = array("d")
    for index in range(sample_count):
        phase = math.tau * index / sample_count
        # 돌풍 세기 조절
        gust = (0.68 + 0.18 * math.sin(phase + 0.4)
                + 0.10 * math.sin(3.0 * phase + 1.2)
                + 0.04 * math.sin(5.0 * phase + 0.7))
        texture = (low_gain * low_band[index] + mid_gain * mid_band[index]
                   + air_gain * air_band[index])
        samples.append(texture * gust)

    # 배경음 음량 조절
    gain = min(0.09 / rms(samples), 0.45 / max(abs(sample) for sample in samples))
    for index in range(sample_count):
        samples[index] *= gain

    # 반복 시작점 선택
    seam = min(range(sample_count), key=lambda i:
               abs(samples[i]) + 8.0 * abs(samples[i] - samples[i - 1]))
    samples = samples[seam:] + samples[:seam]
    pcm = array("h", (round(sample * 32767.0) for sample in samples))
    if sys.byteorder != "little":
        pcm.byteswap()

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUTPUT), "wb") as audio:
        audio.setnchannels(1)
        audio.setsampwidth(2)
        audio.setframerate(SAMPLE_RATE)
        audio.writeframes(pcm.tobytes())

    # 음량 검사
    half_second = SAMPLE_RATE // 2
    levels = [rms(samples[i:i + half_second])
              for i in range(0, sample_count - half_second + 1, half_second)]
    report = {
        "output": str(OUTPUT),
        "sample_rate": SAMPLE_RATE,
        "channels": 1,
        "bits_per_sample": 16,
        "duration_seconds": DURATION_SECONDS,
        "peak_dbfs": round(20.0 * math.log10(max(abs(x) for x in samples)), 2),
        "rms_dbfs": round(20.0 * math.log10(rms(samples)), 2),
        "loop_boundary_step_pcm": abs(round(samples[0] * 32767) - round(samples[-1] * 32767)),
        "quietest_half_second_rms_dbfs": round(20.0 * math.log10(min(levels)), 2),
        "loudest_half_second_rms_dbfs": round(20.0 * math.log10(max(levels)), 2),
        "clipped_samples": sum(abs(sample) >= 1.0 for sample in samples),
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    generate()
