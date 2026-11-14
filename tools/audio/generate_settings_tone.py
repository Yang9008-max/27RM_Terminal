"""Generate the original, MIT-licensed settings preview tone. No sampled audio."""
from pathlib import Path
import math
import struct
import wave

output = Path(__file__).resolve().parents[2] / "resources/settings_audio/preview.wav"
output.parent.mkdir(parents=True, exist_ok=True)
rate = 24000
duration = 0.8
samples = []
for i in range(int(rate * duration)):
    t = i / rate
    # A quiet two-note chime with smooth attack/release, purely synthesized.
    envelope = min(1.0, t / 0.02, (duration - t) / 0.1)
    frequency = 660 if t < 0.4 else 880
    samples.append(int(0.25 * 32767 * envelope * math.sin(2 * math.pi * frequency * t)))
with wave.open(str(output), "wb") as wav:
    wav.setparams((1, 2, rate, 0, "NONE", "not compressed"))
    wav.writeframes(struct.pack("<" + "h" * len(samples), *samples))
