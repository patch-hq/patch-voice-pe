"""Augment generated 'hey patch' clips into microWakeWord spectrogram features.
Usage: python augment.py <clips_dir> <out_dir> [<extra real-recordings dir>]"""
import os, sys
from mmap_ninja.ragged import RaggedMmap
from microwakeword.audio.augmentation import Augmentation
from microwakeword.audio.clips import Clips
from microwakeword.audio.spectrograms import SpectrogramGeneration

src, out = sys.argv[1], sys.argv[2]
clips = Clips(input_directory=src, file_pattern='*.wav', max_clip_duration_s=None,
              remove_silence=False, random_split_seed=10, split_count=0.1)
aug = Augmentation(augmentation_duration_s=3.2,
    augmentation_probabilities={"SevenBandParametricEQ":0.1,"TanhDistortion":0.1,"PitchShift":0.1,
        "BandStopFilter":0.1,"AddColorNoise":0.1,"AddBackgroundNoise":0.75,"Gain":1.0,"RIR":0.5},
    impulse_paths=['mit_rirs'], background_paths=['fma_16k','audioset_16k'],
    background_min_snr_db=-5, background_max_snr_db=10, min_jitter_s=0.195, max_jitter_s=0.205)
for split, name, rep, slide in [("training","train",2,10),("validation","validation",1,10),("testing","test",1,1)]:
    d = os.path.join(out, split); os.makedirs(d, exist_ok=True)
    sg = SpectrogramGeneration(clips=clips, augmenter=aug, slide_frames=slide, step_ms=10)
    RaggedMmap.from_generator(out_dir=os.path.join(d,'wakeword_mmap'),
        sample_generator=sg.spectrogram_generator(split=name, repeat=rep), batch_size=100, verbose=True)
