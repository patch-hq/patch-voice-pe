# Hey Patch wake word training

Trains the microWakeWord model in `components/patch_wakeword/model/hey_patch.tflite`.

1. Python 3.11 venv with microWakeWord (editable clone), piper-sample-generator, `datasets<4`, `setuptools<81`, `tensorboard`, CPU torch.
2. Positives: `python -m piper_sample_generator 'hey patch' --model en_US-libritts_r-medium.pt --max-samples 8000 --max-speakers 600 --length-scales 0.9 1.0 1.15`.
3. Hard negatives (near-miss phrases, same generator): hey batch, hey match, hey pat, patch, hey perch, hey catch, a patch.
4. `augment.py <clips> <features>` adds RIR, noise and music (MIT RIRs, AudioSet, FMA) and writes spectrogram features. Run on the positives and on the hard negatives.
5. Negative sets: pre-made `speech`, `dinner_party`, `no_speech`, `dinner_party_eval` from `kahrendt/microwakeword` on Hugging Face.
6. `python train_config.py && ./train.sh` (10000 steps, about 45 min on 16 CPU cores).

Real recordings of Tom saying "Hey Patch" are not in the training set yet. Drop them in as extra clips for step 4 and retrain.

## Results (v1, synthetic only)

Quantised streaming model on the held-out sets. Misses are on held-out synthetic positives. False wakes are per hour on noisy dinner-party audio.

| cutoff | misses | false wakes per hour |
|---|---|---|
| 0.98 | 22% | 0 |
| 0.93 | 14% | 0.19 |
| 0.87 | 10% | 0.38 |

Firmware ships with cutoff 0.97. Live false wakes per day and real-voice misses are not measured yet.
