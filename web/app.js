/**
 * POLIRITMIK METRONOM - SES MOTORU VE GÖRSELLEŞTİRME (PRO METRONOME STANDARD)
 * 
 * Mimari:
 * 1. Web Audio API Lookahead Scheduler (Chris Wilson modeli, zero-drift)
 * 2. C++ shared_core algoritma portu (Bjorklund Euclidean, Poliritmik/Polimetrik)
 * 3. Transient Perküsif Sinüs Sentezi (Pitch & Gain Envelope)
 * 4. HTML5 Canvas PolygonPhaseView (Retina 60/120 FPS N-gen faz dönüşü ve tepe çakışması)
 * 5. Kompakt Jog Wheel, Tap Tempo, Drawer (Ön Ayarlar & Antrenörler) ve Tek Satır Katman Şeritleri
 */

'use strict';

/* ==========================================================================
   1. BJORKLUND EUCLIDEAN ALGORİTMASI (shared_core/src/bjorklund.cpp Portu)
   ========================================================================== */

const BeatAccent = {
  MUTE: 0,
  NORMAL: 1,
  DOWNBEAT: 2
};

const SoundPreset = {
  DIGITAL: 0,
  WOODBLOCK: 1,
  MECHANICAL: 2,
  SNARE_RIM: 3,
  HIHAT: 4
};

const SOUND_PRESET_NAMES = {
  [SoundPreset.DIGITAL]: 'Digital',
  [SoundPreset.WOODBLOCK]: 'Woodblock',
  [SoundPreset.MECHANICAL]: 'Mechanical',
  [SoundPreset.SNARE_RIM]: 'Snare/Rim',
  [SoundPreset.HIHAT]: 'Hi-Hat'
};

const RhythmMode = {
  POLYRHYTHMIC: 0, // Ortak ölçü süresinde X:Y oranları
  POLYMETRIC: 1,   // Ortak alt bölüntü adımı, bağımsız ölçü uzunlukları
  EUCLIDEAN: 2     // Bjorklund E(k, n)
};

class Bjorklund {
  /**
   * k vuruş ve n adım için Bjorklund Euclidean ritim dizisini hesaplar.
   * @param {number} pulses - k
   * @param {number} steps - n
   * @param {boolean} downbeatFirst - İlk vuruş Downbeat olsun mu
   * @returns {number[]} BeatAccent dizisi [0, 1, 2]
   */
  static generate(pulses, steps, downbeatFirst = true) {
    if (!steps || steps <= 0) return [];
    if (pulses <= 0) {
      return new Array(steps).fill(BeatAccent.MUTE);
    }
    if (pulses >= steps) {
      const arr = new Array(steps).fill(BeatAccent.NORMAL);
      if (downbeatFirst) arr[0] = BeatAccent.DOWNBEAT;
      return arr;
    }

    let sequences = [];
    for (let i = 0; i < pulses; i++) sequences.push([true]);
    for (let i = 0; i < steps - pulses; i++) sequences.push([false]);

    let countOnes = pulses;
    let countZeros = steps - pulses;

    while (countZeros > 1) {
      const numPairs = Math.min(countOnes, countZeros);
      for (let i = 0; i < numPairs; i++) {
        const tail = sequences[sequences.length - 1 - i];
        sequences[i] = sequences[i].concat(tail);
      }
      sequences = sequences.slice(0, sequences.length - numPairs);
      countZeros = sequences.length - numPairs;
      countOnes = numPairs;
    }

    const result = [];
    let isFirstHit = true;

    for (const seq of sequences) {
      for (const hit of seq) {
        if (result.length >= steps) break;
        if (hit) {
          if (isFirstHit && downbeatFirst) {
            result.push(BeatAccent.DOWNBEAT);
            isFirstHit = false;
          } else {
            result.push(BeatAccent.NORMAL);
            isFirstHit = false;
          }
        } else {
          result.push(BeatAccent.MUTE);
        }
      }
    }

    return result;
  }
}

/* ==========================================================================
   2. ANTRENÖR MOTORLARI (Tempo Trainer & Mute Trainer)
   ========================================================================== */

class TrainerEngine {
  constructor() {
    this.tempoTrainer = {
      enabled: false,
      startBpm: 100,
      targetBpm: 150,
      stepBpm: 2,
      barsInterval: 4,
      autoReverse: true,
      barCounter: 0,
      increasing: true
    };

    this.muteTrainer = {
      enabled: false,
      playBars: 4,
      silentBars: 2,
      barCounter: 0,
      isMuted: false
    };
  }

  reset() {
    this.tempoTrainer.barCounter = 0;
    this.tempoTrainer.increasing = this.tempoTrainer.targetBpm >= this.tempoTrainer.startBpm;
    this.muteTrainer.barCounter = 0;
    this.muteTrainer.isMuted = false;
  }

  onBarCompleted(currentBpm) {
    let newBpm = currentBpm;

    // 1. Tempo Trainer
    if (this.tempoTrainer.enabled && this.tempoTrainer.barsInterval > 0) {
      this.tempoTrainer.barCounter++;
      if (this.tempoTrainer.barCounter >= this.tempoTrainer.barsInterval) {
        this.tempoTrainer.barCounter = 0;

        if (this.tempoTrainer.increasing) {
          newBpm += this.tempoTrainer.stepBpm;
          if (newBpm >= this.tempoTrainer.targetBpm) {
            newBpm = this.tempoTrainer.targetBpm;
            if (this.tempoTrainer.autoReverse) {
              this.tempoTrainer.increasing = false;
            }
          }
        } else {
          newBpm -= this.tempoTrainer.stepBpm;
          if (newBpm <= this.tempoTrainer.startBpm) {
            newBpm = this.tempoTrainer.startBpm;
            if (this.tempoTrainer.autoReverse) {
              this.tempoTrainer.increasing = true;
            }
          }
        }
      }
    }

    // 2. Mute Trainer
    if (this.muteTrainer.enabled) {
      this.muteTrainer.barCounter++;
      if (!this.muteTrainer.isMuted) {
        if (this.muteTrainer.barCounter >= this.muteTrainer.playBars) {
          this.muteTrainer.isMuted = true;
          this.muteTrainer.barCounter = 0;
        }
      } else {
        if (this.muteTrainer.barCounter >= this.muteTrainer.silentBars) {
          this.muteTrainer.isMuted = false;
          this.muteTrainer.barCounter = 0;
        }
      }
    } else {
      this.muteTrainer.isMuted = false;
    }

    return { newBpm, isAudible: !this.muteTrainer.isMuted };
  }
}

/* ==========================================================================
   3. KATMAN MODELİ (Layer)
   ========================================================================== */

const LAYER_PALETTE = [
  '#00f0ff', // Cyan (Layer 1)
  '#ff9100', // Orange (Layer 2)
  '#ff007f', // Magenta (Layer 3)
  '#00ff88', // Lime (Layer 4)
  '#9d4edd', // Purple (Layer 5)
  '#ffd166'  // Amber (Layer 6)
];

const DEFAULT_FREQS = [1200, 800, 1600, 600, 950, 1400];

class Layer {
  constructor(id, pulses = 4, totalSteps = 4, mode = RhythmMode.POLYRHYTHMIC) {
    this.id = id;
    this.name = `Layer ${id + 1}`;
    this.color = LAYER_PALETTE[id % LAYER_PALETTE.length];
    this.mode = mode;
    this.pulses = pulses;        // Poliritmik pay veya Euclidean vuruş sayısı
    this.totalSteps = totalSteps;// Polimetrik adım veya Euclidean toplam adım
    this.subdivision = 1;        // 1=Çeyrek, 2=Sekizlik, 3=Üçleme, 4=Onaltılık

    // Mikser & Ses Kiti
    this.volume = 0.85;
    this.pan = (id === 0) ? -0.4 : (id === 1 ? 0.4 : 0.0);
    this.pitchShift = 1.0;
    this.soundPreset = (id === 0) ? SoundPreset.HIHAT : (id === 1 ? SoundPreset.SNARE_RIM : SoundPreset.WOODBLOCK);
    this.synthFreq = DEFAULT_FREQS[id % DEFAULT_FREQS.length];
    this.isMuted = false;
    this.isSolo = false;

    // Vurgu dizisi
    this.accents = [];
    this.updateAccents();

    // Zamanlayıcı durumu
    this.currentStep = 0;
    this.nextTriggerTime = 0;
    this.lastBarStartTime = 0;
    this.currentBarDuration = 1.0;
  }

  updateAccents() {
    if (this.mode === RhythmMode.EUCLIDEAN) {
      this.accents = Bjorklund.generate(this.pulses, this.totalSteps, true);
    } else if (this.mode === RhythmMode.POLYRHYTHMIC) {
      const count = Math.max(1, this.pulses);
      this.accents = new Array(count).fill(BeatAccent.NORMAL);
      this.accents[0] = BeatAccent.DOWNBEAT;
    } else if (this.mode === RhythmMode.POLYMETRIC) {
      const count = Math.max(1, this.totalSteps);
      this.accents = new Array(count).fill(BeatAccent.NORMAL);
      this.accents[0] = BeatAccent.DOWNBEAT;
    }
  }

  cycleAccent(stepIndex) {
    if (stepIndex < 0 || stepIndex >= this.accents.length) return;
    const current = this.accents[stepIndex];
    if (current === BeatAccent.DOWNBEAT) {
      this.accents[stepIndex] = BeatAccent.NORMAL;
    } else if (current === BeatAccent.NORMAL) {
      this.accents[stepIndex] = BeatAccent.MUTE;
    } else {
      this.accents[stepIndex] = BeatAccent.DOWNBEAT;
    }
  }
}

/* ==========================================================================
   4. DÜŞÜK GECİKMELİ SES MOTORU (Lookahead Scheduler)
   ========================================================================== */

class AudioEngine {
  constructor() {
    this.audioCtx = null;
    this.masterGainNode = null;

    this.lookaheadMs = 25.0;
    this.scheduleAheadSec = 0.12;
    this.timerId = null;

    this.bpm = 120.0;
    this.timeSigNum = 4;
    this.timeSigDen = 4;
    this.isPlaying = false;

    this.trainers = new TrainerEngine();
    this.layers = [];
    this.initDefaultLayers();

    this.barCount = 0;
    this.masterBarStartTime = 0;
    this.masterBarDuration = 2.0;

    this.presetBuffers = null;

    this.onBeatScheduled = null;
    this.onBarCompleted = null;
    this.onBpmChanged = null;
  }

  initDefaultLayers() {
    const layer1 = new Layer(0, 3, 3, RhythmMode.POLYRHYTHMIC);
    const layer2 = new Layer(1, 4, 4, RhythmMode.POLYRHYTHMIC);
    this.layers = [layer1, layer2];
  }

  ensureAudioContext() {
    if (!this.audioCtx) {
      const AudioContextClass = window.AudioContext || window.webkitAudioContext;
      this.audioCtx = new AudioContextClass({ latencyHint: 'interactive' });
      this.masterGainNode = this.audioCtx.createGain();
      this.masterGainNode.gain.setValueAtTime(0.85, this.audioCtx.currentTime);
      this.masterGainNode.connect(this.audioCtx.destination);
      this.initSoundPresets();
    }
    if (this.audioCtx.state === 'suspended') {
      return this.audioCtx.resume();
    }
    return Promise.resolve();
  }

  setMasterVolume(val) {
    if (this.masterGainNode && this.audioCtx) {
      this.masterGainNode.gain.setTargetAtTime(Math.max(0, Math.min(1, val)), this.audioCtx.currentTime, 0.015);
    }
  }

  setBpm(newBpm) {
    this.bpm = Math.max(30, Math.min(300, newBpm));
    if (this.onBpmChanged) {
      this.onBpmChanged(this.bpm);
    }
  }

  start() {
    if (this.isPlaying) return;
    this.ensureAudioContext().then(() => {
      this.isPlaying = true;
      this.barCount = 0;
      this.trainers.reset();

      const now = this.audioCtx.currentTime + 0.05;
      this.masterBarStartTime = now;
      this.masterBarDuration = (this.timeSigNum * (4.0 / this.timeSigDen)) * (60.0 / this.bpm);

      for (const layer of this.layers) {
        layer.currentStep = 0;
        layer.lastBarStartTime = now;
        layer.currentBarDuration = this.masterBarDuration;
        layer.nextTriggerTime = now;
      }

      this.timerId = setInterval(() => this.scheduler(), this.lookaheadMs);
    });
  }

  stop() {
    if (!this.isPlaying) return;
    this.isPlaying = false;
    if (this.timerId) {
      clearInterval(this.timerId);
      this.timerId = null;
    }
  }

  resetPhases() {
    if (!this.audioCtx) return;
    const now = this.audioCtx.currentTime;
    this.masterBarStartTime = now;
    for (const layer of this.layers) {
      layer.currentStep = 0;
      layer.lastBarStartTime = now;
      layer.nextTriggerTime = now;
    }
  }

  scheduler() {
    if (!this.isPlaying || !this.audioCtx) return;

    const currentTime = this.audioCtx.currentTime;
    const scheduleWindowEnd = currentTime + this.scheduleAheadSec;

    this.masterBarDuration = (this.timeSigNum * (4.0 / this.timeSigDen)) * (60.0 / this.bpm);

    // 1. Ana Ölçü sınır kontrolü
    while (this.masterBarStartTime + this.masterBarDuration <= scheduleWindowEnd) {
      this.masterBarStartTime += this.masterBarDuration;
      this.barCount++;

      const { newBpm } = this.trainers.onBarCompleted(this.bpm);
      if (newBpm !== this.bpm) {
        this.setBpm(newBpm);
      }

      if (this.onBarCompleted) {
        this.onBarCompleted(this.barCount, this.trainers);
      }
    }

    // 2. Katman vuruşlarını planla
    const anySolo = this.layers.some(l => l.isSolo);

    for (let layerIdx = 0; layerIdx < this.layers.length; layerIdx++) {
      const layer = this.layers[layerIdx];
      const isAudible = anySolo ? layer.isSolo : !layer.isMuted;

      if (layer.mode === RhythmMode.POLYRHYTHMIC) {
        this.schedulePolyrhythmic(layer, layerIdx, scheduleWindowEnd, isAudible);
      } else if (layer.mode === RhythmMode.POLYMETRIC) {
        this.schedulePolymetric(layer, layerIdx, scheduleWindowEnd, isAudible);
      } else if (layer.mode === RhythmMode.EUCLIDEAN) {
        this.scheduleEuclidean(layer, layerIdx, scheduleWindowEnd, isAudible);
      }
    }
  }

  schedulePolyrhythmic(layer, layerIdx, scheduleWindowEnd, isAudible) {
    const pulses = Math.max(1, layer.pulses);

    while (layer.nextTriggerTime < scheduleWindowEnd) {
      if (layer.nextTriggerTime >= layer.lastBarStartTime + layer.currentBarDuration) {
        layer.lastBarStartTime += layer.currentBarDuration;
        layer.currentBarDuration = this.masterBarDuration;
        layer.currentStep = 0;
      }

      const accent = layer.accents[layer.currentStep % layer.accents.length] ?? BeatAccent.NORMAL;

      if (isAudible && !this.trainers.muteTrainer.isMuted) {
        this.synthesizeClick(layer, accent, layer.nextTriggerTime);
      }

      if (this.onBeatScheduled) {
        this.onBeatScheduled({
          layerIndex: layerIdx,
          stepIndex: layer.currentStep,
          totalSteps: pulses,
          accent: accent,
          time: layer.nextTriggerTime
        });
      }

      layer.currentStep++;
      if (layer.currentStep < pulses) {
        layer.nextTriggerTime = layer.lastBarStartTime + (layer.currentStep * layer.currentBarDuration) / pulses;
      } else {
        layer.nextTriggerTime = layer.lastBarStartTime + layer.currentBarDuration;
      }
    }
  }

  schedulePolymetric(layer, layerIdx, scheduleWindowEnd, isAudible) {
    const totalSteps = Math.max(1, layer.totalSteps);
    const subdiv = Math.max(1, layer.subdivision);
    const stepDuration = (60.0 / this.bpm) / subdiv;

    while (layer.nextTriggerTime < scheduleWindowEnd) {
      const accent = layer.accents[layer.currentStep % layer.accents.length] ?? BeatAccent.NORMAL;

      if (isAudible && !this.trainers.muteTrainer.isMuted) {
        this.synthesizeClick(layer, accent, layer.nextTriggerTime);
      }

      if (this.onBeatScheduled) {
        this.onBeatScheduled({
          layerIndex: layerIdx,
          stepIndex: layer.currentStep,
          totalSteps: totalSteps,
          accent: accent,
          time: layer.nextTriggerTime
        });
      }

      layer.currentStep = (layer.currentStep + 1) % totalSteps;
      layer.nextTriggerTime += stepDuration;
    }
  }

  scheduleEuclidean(layer, layerIdx, scheduleWindowEnd, isAudible) {
    const totalSteps = Math.max(1, layer.totalSteps);

    while (layer.nextTriggerTime < scheduleWindowEnd) {
      if (layer.nextTriggerTime >= layer.lastBarStartTime + layer.currentBarDuration) {
        layer.lastBarStartTime += layer.currentBarDuration;
        layer.currentBarDuration = this.masterBarDuration;
        layer.currentStep = 0;
      }

      const accent = layer.accents[layer.currentStep % layer.accents.length] ?? BeatAccent.MUTE;

      if (isAudible && !this.trainers.muteTrainer.isMuted) {
        this.synthesizeClick(layer, accent, layer.nextTriggerTime);
      }

      if (this.onBeatScheduled) {
        this.onBeatScheduled({
          layerIndex: layerIdx,
          stepIndex: layer.currentStep,
          totalSteps: totalSteps,
          accent: accent,
          time: layer.nextTriggerTime
        });
      }

      layer.currentStep++;
      if (layer.currentStep < totalSteps) {
        layer.nextTriggerTime = layer.lastBarStartTime + (layer.currentStep * layer.currentBarDuration) / totalSteps;
      } else {
        layer.nextTriggerTime = layer.lastBarStartTime + layer.currentBarDuration;
      }
    }
  }

  initSoundPresets() {
    if (this.presetBuffers) return;
    const sr = this.audioCtx.sampleRate || 48000;
    this.presetBuffers = {};

    const createBuffer = (durationSec, fillFn) => {
      const numFrames = Math.max(1, Math.floor(sr * durationSec));
      const buf = this.audioCtx.createBuffer(1, numFrames, sr);
      const data = buf.getChannelData(0);
      let peak = 0.0001;

      for (let i = 0; i < numFrames; i++) {
        const t = i / sr;
        const val = fillFn(t, i, numFrames, sr);
        data[i] = val;
        const absVal = Math.abs(val);
        if (absVal > peak) peak = absVal;
      }

      // Zero-latency attack: normalize peak to 0.88 (prevent clipping & volume match)
      const normFactor = 0.88 / peak;
      for (let i = 0; i < numFrames; i++) {
        data[i] *= normFactor;
      }
      return buf;
    };

    // 0: DIGITAL (Clean Test Sinüs / Çirp)
    const digDown = createBuffer(0.035, (t) => {
      const freq = (t < 0.006) ? (2400 - (2400 - 1600) * (t / 0.006)) : 1600;
      const attack = Math.min(1.0, t / 0.0003);
      const env = attack * Math.exp(-t * 95.0);
      return Math.sin(2 * Math.PI * freq * t) * env;
    });
    const digNorm = createBuffer(0.025, (t) => {
      const freq = (t < 0.005) ? (1500 - (1500 - 1000) * (t / 0.005)) : 1000;
      const attack = Math.min(1.0, t / 0.0003);
      const env = attack * Math.exp(-t * 130.0);
      return Math.sin(2 * Math.PI * freq * t) * env;
    });

    // 1: WOODBLOCK (Organik Rezonanslı Ahşap Tokmak & Claves)
    const woodDown = createBuffer(0.045, (t) => {
      const attack = Math.min(1.0, t / 0.0002);
      const strike = Math.sin(2 * Math.PI * (1200 + 1800 * Math.exp(-t * 400.0)) * t) * Math.exp(-t * 350.0);
      const res = (
        0.55 * Math.sin(2 * Math.PI * 2150 * t) +
        0.28 * Math.sin(2 * Math.PI * 3250 * t) +
        0.12 * Math.sin(2 * Math.PI * 4300 * t)
      ) * Math.exp(-t * 70.0);
      return attack * (0.4 * strike + 0.6 * res);
    });
    const woodNorm = createBuffer(0.035, (t) => {
      const attack = Math.min(1.0, t / 0.0002);
      const strike = Math.sin(2 * Math.PI * (800 + 1200 * Math.exp(-t * 450.0)) * t) * Math.exp(-t * 400.0);
      const res = (
        0.60 * Math.sin(2 * Math.PI * 1400 * t) +
        0.30 * Math.sin(2 * Math.PI * 2100 * t) +
        0.10 * Math.sin(2 * Math.PI * 2900 * t)
      ) * Math.exp(-t * 90.0);
      return attack * (0.35 * strike + 0.65 * res);
    });

    // 2: MECHANICAL (Geleneksel Piramit Metronom Zili & Çift Maşa Tıkırtısı)
    const mechDown = createBuffer(0.085, (t) => {
      const attack = Math.min(1.0, t / 0.0002);
      const bell = (
        0.65 * Math.sin(2 * Math.PI * 1760 * t) +
        0.35 * Math.sin(2 * Math.PI * 3520 * t)
      ) * Math.exp(-t * 38.0);
      const box = Math.sin(2 * Math.PI * 340 * t) * Math.exp(-t * 120.0);
      return attack * (0.6 * bell + 0.4 * box);
    });
    const mechNorm = createBuffer(0.030, (t) => {
      const attack = Math.min(1.0, t / 0.00015);
      const cavity = (
        0.60 * Math.sin(2 * Math.PI * 1150 * t) +
        0.40 * Math.sin(2 * Math.PI * 750 * t)
      ) * Math.exp(-t * 140.0);
      const click = Math.sin(2 * Math.PI * 2400 * t) * Math.exp(-t * 500.0);
      return attack * (0.75 * cavity + 0.25 * click);
    });

    // 3: SNARE_RIM (Tok Akustik Rimshot & Kuru Cross-stick)
    const noise = (t, i) => {
      const x = Math.sin(i * 12.9898 + t * 78.233) * 43758.5453;
      return (x - Math.floor(x)) * 2 - 1;
    };
    const snareDown = createBuffer(0.065, (t, i) => {
      const attack = Math.min(1.0, t / 0.0002);
      const bodyFreq = 140 + 70 * Math.exp(-t * 120.0);
      const body = Math.sin(2 * Math.PI * bodyFreq * t) * Math.exp(-t * 55.0);
      const rim = (0.7 * Math.sin(2 * Math.PI * 1350 * t) + 0.3 * Math.sin(2 * Math.PI * 2600 * t)) * Math.exp(-t * 80.0);
      const wires = noise(t, i) * Math.exp(-t * 60.0);
      return attack * (0.35 * body + 0.35 * rim + 0.30 * wires);
    });
    const snareNorm = createBuffer(0.028, (t, i) => {
      const attack = Math.min(1.0, t / 0.00015);
      const stick = Math.sin(2 * Math.PI * 1650 * t) * Math.exp(-t * 135.0);
      const shell = Math.sin(2 * Math.PI * 580 * t) * Math.exp(-t * 110.0);
      const microNoise = noise(t, i) * Math.exp(-t * 300.0) * 0.15;
      return attack * (0.55 * stick + 0.35 * shell + microNoise);
    });

    // 4: HIHAT (Vurgulu Yarı Açık & Keskin Kapalı Çıtlaması)
    const hihatDown = createBuffer(0.085, (t, i) => {
      const attack = Math.min(1.0, t / 0.0002);
      const metal = (
        Math.sin(2 * Math.PI * 295 * t) * 0.15 +
        Math.sin(2 * Math.PI * 545 * t) * 0.20 +
        Math.sin(2 * Math.PI * 800 * t) * 0.20 +
        Math.sin(2 * Math.PI * 1200 * t) * 0.15 +
        Math.sin(2 * Math.PI * 3600 * t) * 0.15 +
        Math.sin(2 * Math.PI * 5800 * t) * 0.15
      );
      const sizzle = noise(t, i) * 0.6;
      return attack * (metal * 0.45 + sizzle * 0.55) * Math.exp(-t * 40.0);
    });
    const hihatNorm = createBuffer(0.025, (t, i) => {
      const attack = Math.min(1.0, t / 0.00015);
      const metal = (
        Math.sin(2 * Math.PI * 800 * t) * 0.25 +
        Math.sin(2 * Math.PI * 1200 * t) * 0.25 +
        Math.sin(2 * Math.PI * 3600 * t) * 0.25 +
        Math.sin(2 * Math.PI * 5800 * t) * 0.25
      );
      const sizzle = noise(t, i) * 0.7;
      return attack * (metal * 0.35 + sizzle * 0.65) * Math.exp(-t * 160.0);
    });

    this.presetBuffers = {
      [SoundPreset.DIGITAL]: [digNorm, digDown],
      [SoundPreset.WOODBLOCK]: [woodNorm, woodDown],
      [SoundPreset.MECHANICAL]: [mechNorm, mechDown],
      [SoundPreset.SNARE_RIM]: [snareNorm, snareDown],
      [SoundPreset.HIHAT]: [hihatNorm, hihatDown]
    };
  }

  synthesizeClick(layer, accent, triggerTime) {
    if (accent === BeatAccent.MUTE || !this.audioCtx) return;

    if (!this.presetBuffers) {
      this.initSoundPresets();
    }

    const isDownbeat = (accent === BeatAccent.DOWNBEAT);
    const presetId = layer.soundPreset ?? SoundPreset.DIGITAL;
    const variationIdx = isDownbeat ? 1 : 0;
    const buffer = this.presetBuffers?.[presetId]?.[variationIdx];

    if (!buffer) return;

    const source = this.audioCtx.createBufferSource();
    source.buffer = buffer;

    // Pitch shift desteği (varsa katman frekans oranını da hesaba kat)
    let rate = layer.pitchShift || 1.0;
    if (presetId === SoundPreset.DIGITAL && layer.synthFreq) {
      rate *= (layer.synthFreq / 1000.0);
    }
    source.playbackRate.setValueAtTime(Math.max(0.2, Math.min(4.0, rate)), triggerTime);

    const gainNode = this.audioCtx.createGain();
    const velocity = isDownbeat ? 1.15 : 0.8;
    const targetGain = Math.max(0, Math.min(1.5, layer.volume * velocity));
    gainNode.gain.setValueAtTime(targetGain, triggerTime);

    if (this.audioCtx.createStereoPanner) {
      const panner = this.audioCtx.createStereoPanner();
      panner.pan.setValueAtTime(Math.max(-1, Math.min(1, layer.pan)), triggerTime);
      source.connect(gainNode);
      gainNode.connect(panner);
      panner.connect(this.masterGainNode);
    } else {
      source.connect(gainNode);
      gainNode.connect(this.masterGainNode);
    }

    source.start(triggerTime);
  }
}

/* ==========================================================================
   5. HTML5 CANVAS POLYGON PHASE VIEW (Geometrik N-gen Görselleştirici)
   ========================================================================== */

class CanvasPolygonVisualizer {
  constructor(canvasElement, audioEngine) {
    this.canvas = canvasElement;
    this.ctx = this.canvas.getContext('2d');
    this.engine = audioEngine;

    this.showLabels = false;
    this.hitRipples = [];

    this.initCanvasSize();
    window.addEventListener('resize', () => this.initCanvasSize());

    this.render = this.render.bind(this);
    requestAnimationFrame(this.render);
  }

  initCanvasSize() {
    const rect = this.canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    const size = Math.min(rect.width, rect.height) || 300;

    this.canvas.width = size * dpr;
    this.canvas.height = size * dpr;
    this.ctx.resetTransform();
    this.ctx.scale(dpr, dpr);
    this.width = size;
    this.height = size;
  }

  triggerHitVisual(layerIndex, accent) {
    this.hitRipples.push({
      layerIndex,
      accent,
      progress: 0,
      maxRadius: 28,
      opacity: 1.0
    });
  }

  render() {
    requestAnimationFrame(this.render);

    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;
    if (!w || !h) return;

    ctx.clearRect(0, 0, w, h);

    const centerX = w / 2;
    const centerY = h / 2;
    const maxRadius = Math.min(w, h) / 2 * 0.82;

    // 1. Dış Dairesel Kılavuz
    ctx.save();
    ctx.beginPath();
    ctx.arc(centerX, centerY, maxRadius, 0, Math.PI * 2);
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
    ctx.lineWidth = 1.5;
    ctx.stroke();

    ctx.beginPath();
    ctx.arc(centerX, centerY, maxRadius * 0.55, 0, Math.PI * 2);
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.03)';
    ctx.lineWidth = 1;
    ctx.stroke();
    ctx.restore();

    const layers = this.engine.layers;
    const numLayers = Math.max(1, layers.length);

    // 2. Çoklu N-gen Poligon Çizimleri
    for (let i = 0; i < layers.length; i++) {
      const layer = layers[i];
      const radius = maxRadius * (0.35 + (0.65 * (i + 1)) / (numLayers + 1));

      let sides = 4;
      if (layer.mode === RhythmMode.POLYRHYTHMIC) {
        sides = Math.max(2, layer.pulses);
      } else if (layer.mode === RhythmMode.POLYMETRIC) {
        sides = Math.max(2, layer.totalSteps);
      } else if (layer.mode === RhythmMode.EUCLIDEAN) {
        sides = Math.max(2, layer.totalSteps);
      }

      let phase = 0;
      if (this.engine.isPlaying && this.engine.audioCtx) {
        const audioTime = this.engine.audioCtx.currentTime;
        if (layer.mode === RhythmMode.POLYMETRIC) {
          const stepDur = (60.0 / this.engine.bpm) / Math.max(1, layer.subdivision);
          const totalDur = stepDur * Math.max(1, layer.totalSteps);
          phase = ((audioTime - layer.lastBarStartTime) / totalDur) % 1.0;
        } else {
          phase = ((audioTime - layer.lastBarStartTime) / layer.currentBarDuration) % 1.0;
        }
        if (phase < 0) phase += 1.0;
      }

      const rotation = phase * Math.PI * 2;
      this.drawPolygon(ctx, centerX, centerY, radius, sides, rotation, layer.color, layer.isMuted);

      // Yörünge Göstergesi (Orbital Beacon)
      const beaconAngle = rotation - Math.PI / 2;
      const beaconX = centerX + Math.cos(beaconAngle) * radius;
      const beaconY = centerY + Math.sin(beaconAngle) * radius;

      ctx.save();
      ctx.beginPath();
      ctx.arc(beaconX, beaconY, 4, 0, Math.PI * 2);
      ctx.fillStyle = layer.color;
      ctx.shadowColor = layer.color;
      ctx.shadowBlur = 8;
      ctx.fill();

      ctx.beginPath();
      ctx.arc(beaconX, beaconY, 1.5, 0, Math.PI * 2);
      ctx.fillStyle = '#ffffff';
      ctx.fill();
      ctx.restore();

      if (this.showLabels) {
        ctx.save();
        ctx.font = '9px JetBrains Mono, monospace';
        ctx.fillStyle = layer.color;
        ctx.textAlign = 'center';
        ctx.fillText(`${sides}`, centerX, centerY - radius - 6);
        ctx.restore();
      }
    }

    // 3. Faz Çakışma Noktası (Saat 12 / Apex)
    const apexY = centerY - maxRadius;
    ctx.save();
    ctx.beginPath();
    ctx.arc(centerX, apexY, 5, 0, Math.PI * 2);
    ctx.fillStyle = '#ffd166';
    ctx.shadowColor = '#ffd166';
    ctx.shadowBlur = 12;
    ctx.fill();

    ctx.beginPath();
    ctx.arc(centerX, apexY, 2, 0, Math.PI * 2);
    ctx.fillStyle = '#ffffff';
    ctx.fill();
    ctx.restore();

    // 4. Tepe Vuruş Dalgası (Ripples)
    for (let r = this.hitRipples.length - 1; r >= 0; r--) {
      const rip = this.hitRipples[r];
      rip.progress += 0.08;
      rip.opacity = Math.max(0, 1.0 - rip.progress);

      const color = layers[rip.layerIndex] ? layers[rip.layerIndex].color : '#00f0ff';
      const curRadius = rip.progress * rip.maxRadius;

      ctx.save();
      ctx.beginPath();
      ctx.arc(centerX, apexY, curRadius, 0, Math.PI * 2);
      ctx.strokeStyle = color;
      ctx.globalAlpha = rip.opacity;
      ctx.lineWidth = 2;
      ctx.shadowColor = color;
      ctx.shadowBlur = 10;
      ctx.stroke();
      ctx.restore();

      if (rip.progress >= 1.0) {
        this.hitRipples.splice(r, 1);
      }
    }
  }

  drawPolygon(ctx, cx, cy, radius, sides, rotation, color, isMuted) {
    ctx.save();
    const alpha = isMuted ? 0.25 : 0.85;

    if (sides < 3) {
      const a1 = -Math.PI / 2 + rotation;
      const a2 = a1 + Math.PI;
      ctx.beginPath();
      ctx.moveTo(cx + Math.cos(a1) * radius, cy + Math.sin(a1) * radius);
      ctx.lineTo(cx + Math.cos(a2) * radius, cy + Math.sin(a2) * radius);
      ctx.strokeStyle = color;
      ctx.globalAlpha = alpha;
      ctx.lineWidth = 2.5;
      ctx.stroke();
      ctx.restore();
      return;
    }

    const angleStep = (Math.PI * 2) / sides;
    const initialOffset = -Math.PI / 2 + rotation;

    ctx.beginPath();
    for (let i = 0; i < sides; i++) {
      const angle = initialOffset + i * angleStep;
      const x = cx + Math.cos(angle) * radius;
      const y = cy + Math.sin(angle) * radius;
      if (i === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    }
    ctx.closePath();

    ctx.strokeStyle = color;
    ctx.globalAlpha = alpha;
    ctx.lineWidth = 2.2;
    ctx.shadowColor = color;
    ctx.shadowBlur = isMuted ? 0 : 7;
    ctx.stroke();

    for (let i = 0; i < sides; i++) {
      const angle = initialOffset + i * angleStep;
      const vx = cx + Math.cos(angle) * radius;
      const vy = cy + Math.sin(angle) * radius;

      ctx.beginPath();
      ctx.arc(vx, vy, (i === 0) ? 3.8 : 2.8, 0, Math.PI * 2);
      ctx.fillStyle = (i === 0) ? '#ffffff' : color;
      ctx.shadowColor = color;
      ctx.shadowBlur = (i === 0) ? 10 : 4;
      ctx.fill();
    }

    ctx.restore();
  }
}

/* ==========================================================================
   6. DOKUNMATİK KOMPAKT JOG WHEEL (Rotary Dial)
   ========================================================================== */

class RotaryJogWheel {
  constructor(element, onBpmDelta) {
    this.el = element;
    this.dial = element.querySelector('#jogDial');
    this.onBpmDelta = onBpmDelta;

    this.isDragging = false;
    this.lastAngle = 0;
    this.visualAngle = 0;
    this.angleAccumulator = 0;
    this.rafPending = false;
    this.activePointerId = null;

    // Hassasiyet Ölçekleme: Her 4.5 derecede 1 BPM (4.5 * PI / 180 = ~0.0785398 radyan)
    this.radPerBpm = (4.5 * Math.PI) / 180;

    // Tarayıcı kaydırmasını kesin olarak engelle
    this.el.style.touchAction = 'none';
    if (this.dial) {
      this.dial.style.touchAction = 'none';
    }

    this.bindEvents();
  }

  bindEvents() {
    const onStart = (clientX, clientY, pointerId) => {
      this.isDragging = true;
      this.activePointerId = pointerId;
      const rect = this.el.getBoundingClientRect();
      const cx = rect.left + rect.width / 2;
      const cy = rect.top + rect.height / 2;
      this.lastAngle = Math.atan2(clientY - cy, clientX - cx);
      this.angleAccumulator = 0;
    };

    const onMove = (clientX, clientY) => {
      if (!this.isDragging) return;
      const rect = this.el.getBoundingClientRect();
      const cx = rect.left + rect.width / 2;
      const cy = rect.top + rect.height / 2;
      const angle = Math.atan2(clientY - cy, clientX - cx);

      let delta = angle - this.lastAngle;
      if (delta > Math.PI) delta -= Math.PI * 2;
      if (delta < -Math.PI) delta += Math.PI * 2;

      this.lastAngle = angle;
      this.visualAngle += delta;
      this.angleAccumulator += delta;

      // Akıcı tepki eğrisi: mikro hareketleri kaybetmeyen kesintisiz akümülasyon
      const bpmSteps = Math.trunc(this.angleAccumulator / this.radPerBpm);
      if (bpmSteps !== 0) {
        this.angleAccumulator -= bpmSteps * this.radPerBpm;
        this.onBpmDelta(bpmSteps);
      }

      // UI güncellemelerini doğrudan requestAnimationFrame içine bağla (gecikmesiz 1:1 takip)
      if (!this.rafPending) {
        this.rafPending = true;
        requestAnimationFrame(() => {
          if (this.dial) {
            this.dial.style.transform = `rotate(${this.visualAngle}rad)`;
          }
          this.rafPending = false;
        });
      }
    };

    const onEnd = (pointerId) => {
      if (this.isDragging && this.activePointerId !== null && pointerId === this.activePointerId) {
        try {
          if (this.el.hasPointerCapture(pointerId)) {
            this.el.releasePointerCapture(pointerId);
          }
        } catch (_) {}
      }
      this.isDragging = false;
      this.activePointerId = null;
    };

    this.el.addEventListener('pointerdown', (e) => {
      e.preventDefault();
      try {
        this.el.setPointerCapture(e.pointerId);
      } catch (_) {}
      onStart(e.clientX, e.clientY, e.pointerId);
    }, { passive: false });

    this.el.addEventListener('pointermove', (e) => {
      if (this.isDragging) {
        e.preventDefault();
        onMove(e.clientX, e.clientY);
      }
    }, { passive: false });

    this.el.addEventListener('pointerup', (e) => {
      e.preventDefault();
      onEnd(e.pointerId);
    }, { passive: false });

    this.el.addEventListener('pointercancel', (e) => {
      onEnd(e.pointerId);
    }, { passive: false });

    // Touch olayları için mobil varsayılan kaydırmasını (scroll) kesin olarak engelle
    this.el.addEventListener('touchstart', (e) => {
      e.preventDefault();
    }, { passive: false });

    this.el.addEventListener('touchmove', (e) => {
      e.preventDefault();
    }, { passive: false });

    this.el.addEventListener('wheel', (e) => {
      e.preventDefault();
      const delta = e.deltaY < 0 ? 1 : -1;
      this.visualAngle += delta * this.radPerBpm;
      if (this.dial) {
        this.dial.style.transform = `rotate(${this.visualAngle}rad)`;
      }
      this.onBpmDelta(delta);
    }, { passive: false });
  }
}

/* ==========================================================================
   7. TAP TEMPO HESAPLAYICI
   ========================================================================== */

class TapTempoCalculator {
  constructor(onTempoCalculated) {
    this.onTempoCalculated = onTempoCalculated;
    this.tapTimes = [];
    this.maxTaps = 5;
    this.resetTimeout = 2500;
    this.timer = null;
  }

  tap() {
    const now = performance.now();
    clearTimeout(this.timer);
    this.timer = setTimeout(() => {
      this.tapTimes = [];
    }, this.resetTimeout);

    this.tapTimes.push(now);
    if (this.tapTimes.length > this.maxTaps) {
      this.tapTimes.shift();
    }

    if (this.tapTimes.length >= 2) {
      let totalInterval = 0;
      for (let i = 1; i < this.tapTimes.length; i++) {
        totalInterval += (this.tapTimes[i] - this.tapTimes[i - 1]);
      }
      const avgIntervalMs = totalInterval / (this.tapTimes.length - 1);
      const bpm = Math.round(60000 / avgIntervalMs);
      if (bpm >= 30 && bpm <= 300) {
        this.onTempoCalculated(bpm);
      }
    }
  }
}

/* ==========================================================================
   8. UYGULAMA VE KULLANICI ARAYÜZÜ (PRO METRONOME MOBIL CONTROLLER)
   ========================================================================== */

class MetronomeApp {
  constructor() {
    this.engine = new AudioEngine();
    this.canvasVis = new CanvasPolygonVisualizer(document.getElementById('phaseCanvas'), this.engine);

    this.activeModalLayerId = null;

    this.initElements();
    this.initJogWheel();
    this.initTapTempo();
    this.initAudioUnlocker();
    this.bindEvents();
    this.renderLayersUI();
    this.updateBpmUI(this.engine.bpm);
  }

  initElements() {
    // Header & Transport
    this.engineStatus = document.getElementById('engineStatus');
    this.statusText = document.getElementById('statusText');
    this.jogBpmDisplay = document.getElementById('jogBpmDisplay');
    this.btnPlayPause = document.getElementById('btnPlayPause');
    this.playIcon = document.getElementById('playIcon');
    this.playLabel = document.getElementById('playLabel');
    this.btnBpmMinus1 = document.getElementById('btnBpmMinus1');
    this.btnBpmPlus1 = document.getElementById('btnBpmPlus1');
    this.btnBpmMinus5 = document.getElementById('btnBpmMinus5');
    this.btnBpmPlus5 = document.getElementById('btnBpmPlus5');
    this.btnTapTempo = document.getElementById('btnTapTempo');
    this.collisionFlash = document.getElementById('collisionFlash');

    // Layers
    this.layersList = document.getElementById('layersList');
    this.btnAddLayer = document.getElementById('btnAddLayer');
    this.btnResetPhase = document.getElementById('btnResetPhase');

    // Bottom Drawer
    this.btnOpenDrawer = document.getElementById('btnOpenDrawer');
    this.btnCloseDrawer = document.getElementById('btnCloseDrawer');
    this.bottomDrawerOverlay = document.getElementById('bottomDrawerOverlay');
    this.drawerSheet = document.getElementById('drawerSheet');

    // Trainers
    this.tempoTrainerEnable = document.getElementById('tempoTrainerEnable');
    this.tempoTrainerStatusBadge = document.getElementById('tempoTrainerStatusBadge');
    this.tempoStartBpm = document.getElementById('tempoStartBpm');
    this.tempoTargetBpm = document.getElementById('tempoTargetBpm');
    this.tempoStepBpm = document.getElementById('tempoStepBpm');
    this.tempoBarsInterval = document.getElementById('tempoBarsInterval');
    this.tempoAutoReverse = document.getElementById('tempoAutoReverse');
    this.tempoProgressBar = document.getElementById('tempoProgressBar');

    this.muteTrainerEnable = document.getElementById('muteTrainerEnable');
    this.muteTrainerStatusBadge = document.getElementById('muteTrainerStatusBadge');
    this.mutePlayBars = document.getElementById('mutePlayBars');
    this.muteSilentBars = document.getElementById('muteSilentBars');
    this.muteStatusDisplay = document.getElementById('muteStatusDisplay');
    this.muteCounterText = document.getElementById('muteCounterText');
    this.muteProgressBar = document.getElementById('muteProgressBar');

    // Master Settings in Drawer
    this.masterTimeSigNum = document.getElementById('masterTimeSigNum');
    this.masterTimeSigDen = document.getElementById('masterTimeSigDen');
    this.masterVolume = document.getElementById('masterVolume');
    this.masterVolumeVal = document.getElementById('masterVolumeVal');
    this.btnToggleLabels = document.getElementById('btnToggleLabels');

    // Layer Settings Modal
    this.layerSettingsModal = document.getElementById('layerSettingsModal');
    this.layerModalTitle = document.getElementById('layerModalTitle');
    this.btnCloseLayerModal = document.getElementById('btnCloseLayerModal');
    this.layerModalBody = document.getElementById('layerModalBody');
  }

  initJogWheel() {
    this.jog = new RotaryJogWheel(document.getElementById('jogWheel'), (delta) => {
      this.engine.setBpm(this.engine.bpm + delta);
    });
  }

  initTapTempo() {
    this.tapCalc = new TapTempoCalculator((newBpm) => {
      this.engine.setBpm(newBpm);
    });
    this.btnTapTempo.addEventListener('pointerdown', (e) => {
      e.preventDefault();
      this.tapCalc.tap();
      this.btnTapTempo.classList.add('active');
      setTimeout(() => this.btnTapTempo.classList.remove('active'), 120);
    });
  }

  initAudioUnlocker() {
    const banner = document.getElementById('audioUnlockBanner');
    const btnUnlock = document.getElementById('btnUnlockAudio');

    const unlock = () => {
      this.engine.ensureAudioContext().then(() => {
        banner.classList.add('hidden');
      }).catch(err => console.warn('Audio unlock warning:', err));
    };

    btnUnlock.addEventListener('click', unlock);

    window.addEventListener('pointerdown', () => {
      if (this.engine.audioCtx && this.engine.audioCtx.state === 'suspended') {
        unlock();
      }
    }, { once: true });
  }

  bindEvents() {
    // Play/Pause
    this.btnPlayPause.addEventListener('click', () => {
      if (this.engine.isPlaying) {
        this.engine.stop();
        this.updatePlayStateUI(false);
      } else {
        this.engine.start();
        this.updatePlayStateUI(true);
      }
    });

    // BPM Steppers
    this.btnBpmMinus1.addEventListener('click', () => this.engine.setBpm(this.engine.bpm - 1));
    this.btnBpmPlus1.addEventListener('click', () => this.engine.setBpm(this.engine.bpm + 1));
    this.btnBpmMinus5.addEventListener('click', () => this.engine.setBpm(this.engine.bpm - 5));
    this.btnBpmPlus5.addEventListener('click', () => this.engine.setBpm(this.engine.bpm + 5));

    // Reset Phases
    this.btnResetPhase.addEventListener('click', () => this.engine.resetPhases());

    // Add Layer
    this.btnAddLayer.addEventListener('click', () => {
      if (this.engine.layers.length >= 6) {
        alert('Maximum of 6 layers supported.');
        return;
      }
      const newId = this.engine.layers.length;
      const newLayer = new Layer(newId, 5, 5, RhythmMode.POLYRHYTHMIC);
      this.engine.layers.push(newLayer);
      this.renderLayersUI();
    });

    // Drawer Aç/Kapat
    this.btnOpenDrawer.addEventListener('click', () => this.openDrawer());
    this.btnCloseDrawer.addEventListener('click', () => this.closeDrawer());
    this.bottomDrawerOverlay.addEventListener('click', (e) => {
      if (e.target === this.bottomDrawerOverlay) {
        this.closeDrawer();
      }
    });

    // Drawer Tabs
    document.querySelectorAll('.drawer-tab').forEach(tab => {
      tab.addEventListener('click', () => {
        document.querySelectorAll('.drawer-tab').forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
        tab.classList.add('active');
        const targetPane = tab.dataset.tab === 'presets' ? 'panePresets' : (tab.dataset.tab === 'trainers' ? 'paneTrainers' : 'paneSettings');
        const el = document.getElementById(targetPane);
        if (el) el.classList.add('active');
      });
    });

    // Presets
    const presetPillList = document.getElementById('presetPillList');
    presetPillList.addEventListener('click', (e) => {
      const tile = e.target.closest('.preset-tile');
      if (!tile) return;
      this.applyPreset(tile.dataset.preset);
      this.closeDrawer();
    });

    // Layer Settings Modal Kapat
    this.btnCloseLayerModal.addEventListener('click', () => this.closeLayerModal());
    this.layerSettingsModal.addEventListener('click', (e) => {
      if (e.target === this.layerSettingsModal) {
        this.closeLayerModal();
      }
    });

    // Engine Callbacks
    this.engine.onBpmChanged = (bpm) => this.updateBpmUI(bpm);

    this.engine.onBeatScheduled = (ev) => {
      this.canvasVis.triggerHitVisual(ev.layerIndex, ev.accent);

      if (ev.accent === BeatAccent.DOWNBEAT) {
        this.collisionFlash.classList.add('flash-active');
        setTimeout(() => this.collisionFlash.classList.remove('flash-active'), 50);
      }

      const pad = document.querySelector(`.pad-dot[data-layer="${ev.layerIndex}"][data-step="${ev.stepIndex}"]`);
      if (pad) {
        pad.classList.add('current-hit');
        setTimeout(() => pad.classList.remove('current-hit'), 100);
      }
    };

    this.engine.onBarCompleted = (barCount, trainers) => {
      this.updateTrainersUI(barCount, trainers);
    };

    // Master Settings Listeners
    this.masterTimeSigNum.addEventListener('change', (e) => {
      this.engine.timeSigNum = Math.max(1, parseInt(e.target.value) || 4);
    });

    this.masterTimeSigDen.addEventListener('change', (e) => {
      this.engine.timeSigDen = parseInt(e.target.value) || 4;
    });

    this.masterVolume.addEventListener('input', (e) => {
      const v = parseFloat(e.target.value);
      this.engine.setMasterVolume(v);
      this.masterVolumeVal.textContent = `${Math.round(v * 100)}%`;
    });

    this.btnToggleLabels.addEventListener('click', () => {
      this.canvasVis.showLabels = !this.canvasVis.showLabels;
      this.btnToggleLabels.classList.toggle('btn-add-highlight', this.canvasVis.showLabels);
    });

    // Trainers Listeners
    this.bindTrainersEvents();
  }

  bindTrainersEvents() {
    this.tempoTrainerEnable.addEventListener('change', (e) => {
      this.engine.trainers.tempoTrainer.enabled = e.target.checked;
      this.tempoTrainerStatusBadge.textContent = e.target.checked ? 'ACTIVE' : 'OFF';
      this.tempoTrainerStatusBadge.classList.toggle('active', e.target.checked);
    });

    this.tempoStartBpm.addEventListener('change', (e) => {
      this.engine.trainers.tempoTrainer.startBpm = parseFloat(e.target.value) || 100;
    });

    this.tempoTargetBpm.addEventListener('change', (e) => {
      this.engine.trainers.tempoTrainer.targetBpm = parseFloat(e.target.value) || 150;
    });

    this.tempoStepBpm.addEventListener('change', (e) => {
      this.engine.trainers.tempoTrainer.stepBpm = parseFloat(e.target.value) || 2;
    });

    this.tempoBarsInterval.addEventListener('change', (e) => {
      this.engine.trainers.tempoTrainer.barsInterval = parseInt(e.target.value) || 4;
    });

    this.tempoAutoReverse.addEventListener('change', (e) => {
      this.engine.trainers.tempoTrainer.autoReverse = e.target.checked;
    });

    this.muteTrainerEnable.addEventListener('change', (e) => {
      this.engine.trainers.muteTrainer.enabled = e.target.checked;
      this.muteTrainerStatusBadge.textContent = e.target.checked ? 'ACTIVE' : 'OFF';
      this.muteTrainerStatusBadge.classList.toggle('active', e.target.checked);
    });

    this.mutePlayBars.addEventListener('change', (e) => {
      this.engine.trainers.muteTrainer.playBars = parseInt(e.target.value) || 4;
    });

    this.muteSilentBars.addEventListener('change', (e) => {
      this.engine.trainers.muteTrainer.silentBars = parseInt(e.target.value) || 2;
    });
  }

  openDrawer() {
    this.bottomDrawerOverlay.classList.remove('hidden');
  }

  closeDrawer() {
    this.bottomDrawerOverlay.classList.add('hidden');
  }

  updatePlayStateUI(isPlaying) {
    if (isPlaying) {
      this.btnPlayPause.classList.add('playing');
      this.playIcon.textContent = '■';
      this.playLabel.textContent = 'STOP';
      this.statusText.textContent = 'PLAYING';
      const dot = this.engineStatus.querySelector('.status-dot');
      if (dot) dot.classList.add('playing');
    } else {
      this.btnPlayPause.classList.remove('playing');
      this.playIcon.textContent = '▶';
      this.playLabel.textContent = 'START';
      this.statusText.textContent = 'READY';
      const dot = this.engineStatus.querySelector('.status-dot');
      if (dot) dot.classList.remove('playing');
    }
  }

  updateBpmUI(bpm) {
    const rounded = Math.round(bpm);
    this.jogBpmDisplay.textContent = rounded;
  }

  updateTrainersUI(barCount, trainers) {
    if (trainers.tempoTrainer.enabled) {
      const interval = Math.max(1, trainers.tempoTrainer.barsInterval);
      const current = trainers.tempoTrainer.barCounter;
      const pct = (current / interval) * 100;
      this.tempoProgressBar.style.width = `${pct}%`;
    } else {
      this.tempoProgressBar.style.width = '0%';
    }

    if (trainers.muteTrainer.enabled) {
      const isMuted = trainers.muteTrainer.isMuted;
      const target = isMuted ? trainers.muteTrainer.silentBars : trainers.muteTrainer.playBars;
      const current = trainers.muteTrainer.barCounter;
      const pct = (current / Math.max(1, target)) * 100;
      this.muteProgressBar.style.width = `${pct}%`;

      const stateLabel = this.muteStatusDisplay.querySelector('.mute-indicator-state');
      if (isMuted) {
        stateLabel.textContent = 'MUTED (TEST)';
        stateLabel.classList.add('muted-active');
      } else {
        stateLabel.textContent = 'ACTIVE (PLAYING)';
        stateLabel.classList.remove('muted-active');
      }
      this.muteCounterText.textContent = `Bars: ${current} / ${target}`;
    } else {
      this.muteProgressBar.style.width = '0%';
    }
  }

  /* --------------------------------------------------------------------------
     KOMPAKT TEK SATIRLIK KATMAN ŞERİTLERİ (Layer Rows)
     -------------------------------------------------------------------------- */
  renderLayersUI() {
    this.layersList.innerHTML = '';

    this.engine.layers.forEach((layer, idx) => {
      const row = document.createElement('div');
      row.className = `layer-row ${layer.isMuted ? 'muted' : ''} ${layer.isSolo ? 'soloed' : ''}`;
      row.style.setProperty('--layer-color', layer.color);

      row.innerHTML = `
        <!-- Sol: Vuruş Stepper -->
        <div class="layer-stepper-group">
          <span class="layer-badge">${idx + 1}</span>
          <button class="btn-pulse-step btn-minus-pulse" data-id="${idx}">−</button>
          <span class="pulse-count-val">${layer.pulses}</span>
          <button class="btn-pulse-step btn-plus-pulse" data-id="${idx}">+</button>
        </div>

        <!-- Orta: Kompakt Step Sequencer Butonları -->
        <div class="layer-pads-scroll" data-layer-idx="${idx}">
          ${layer.accents.map((accent, stepIdx) => `
            <div class="pad-dot accent-${accent}" data-layer="${idx}" data-step="${stepIdx}" title="Beat ${stepIdx + 1}">
              ${stepIdx + 1}
            </div>
          `).join('')}
        </div>

        <!-- Sağ: Ses Kiti Dropdown & Katman Ayar Çarkı -->
        <select class="select-layer-kit" data-id="${idx}" title="Sound Kit">
          <option value="${SoundPreset.SNARE_RIM}" ${layer.soundPreset === SoundPreset.SNARE_RIM ? 'selected' : ''}>Snare</option>
          <option value="${SoundPreset.HIHAT}" ${layer.soundPreset === SoundPreset.HIHAT ? 'selected' : ''}>Hi-Hat</option>
          <option value="${SoundPreset.WOODBLOCK}" ${layer.soundPreset === SoundPreset.WOODBLOCK ? 'selected' : ''}>Wood</option>
          <option value="${SoundPreset.MECHANICAL}" ${layer.soundPreset === SoundPreset.MECHANICAL ? 'selected' : ''}>Mech</option>
          <option value="${SoundPreset.DIGITAL}" ${layer.soundPreset === SoundPreset.DIGITAL ? 'selected' : ''}>Digital</option>
        </select>

        <button class="btn-layer-gear" data-id="${idx}" title="Layer Settings">⚙️</button>
      `;

      this.layersList.appendChild(row);
    });

    this.bindLayerRowEvents();
  }

  bindLayerRowEvents() {
    // Ses Kiti Seçimi
    this.layersList.querySelectorAll('.select-layer-kit').forEach(sel => {
      sel.addEventListener('change', (e) => {
        const id = parseInt(e.currentTarget.dataset.id);
        const preset = parseInt(e.target.value);
        this.engine.layers[id].soundPreset = preset;
        this.engine.ensureAudioContext().then(() => {
          if (this.engine.audioCtx && this.engine.audioCtx.state === 'running') {
            this.engine.synthesizeClick(this.engine.layers[id], BeatAccent.DOWNBEAT, this.engine.audioCtx.currentTime + 0.01);
          }
        });
      });
    });

    // Stepper Eksi
    this.layersList.querySelectorAll('.btn-minus-pulse').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const id = parseInt(e.currentTarget.dataset.id);
        const l = this.engine.layers[id];
        l.pulses = Math.max(1, l.pulses - 1);
        l.updateAccents();
        this.renderLayersUI();
      });
    });

    // Stepper Artı
    this.layersList.querySelectorAll('.btn-plus-pulse').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const id = parseInt(e.currentTarget.dataset.id);
        const l = this.engine.layers[id];
        l.pulses = Math.min(32, l.pulses + 1);
        l.updateAccents();
        this.renderLayersUI();
      });
    });

    // Step Dot Tıklaması (Aksan Döngüsü)
    this.layersList.querySelectorAll('.pad-dot').forEach(dot => {
      dot.addEventListener('click', (e) => {
        const layerIdx = parseInt(e.currentTarget.dataset.layer);
        const stepIdx = parseInt(e.currentTarget.dataset.step);
        this.engine.layers[layerIdx].cycleAccent(stepIdx);
        this.renderLayersUI();
      });
    });

    // Ayar Çarkı Butonu
    this.layersList.querySelectorAll('.btn-layer-gear').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const id = parseInt(e.currentTarget.dataset.id);
        this.openLayerModal(id);
      });
    });
  }

  /* --------------------------------------------------------------------------
     KATMAN AYARLARI MODALI (Popover Card)
     -------------------------------------------------------------------------- */
  openLayerModal(layerId) {
    const layer = this.engine.layers[layerId];
    if (!layer) return;

    this.activeModalLayerId = layerId;
    this.layerModalTitle.textContent = `${layer.name} (${layer.pulses} Beats)`;
    this.layerModalTitle.style.color = layer.color;

    this.layerModalBody.innerHTML = `
      <div class="modal-section">
        <label>Sound Kit:</label>
        <select id="modalLayerSoundPreset" class="modal-select">
          <option value="${SoundPreset.SNARE_RIM}" ${layer.soundPreset === SoundPreset.SNARE_RIM ? 'selected' : ''}>Snare / Rimshot</option>
          <option value="${SoundPreset.HIHAT}" ${layer.soundPreset === SoundPreset.HIHAT ? 'selected' : ''}>Hi-Hat (Closed/Open)</option>
          <option value="${SoundPreset.WOODBLOCK}" ${layer.soundPreset === SoundPreset.WOODBLOCK ? 'selected' : ''}>Woodblock / Claves</option>
          <option value="${SoundPreset.MECHANICAL}" ${layer.soundPreset === SoundPreset.MECHANICAL ? 'selected' : ''}>Mechanical Metronome</option>
          <option value="${SoundPreset.DIGITAL}" ${layer.soundPreset === SoundPreset.DIGITAL ? 'selected' : ''}>Modern Digital Sine</option>
        </select>
      </div>

      <div class="modal-section">
        <label>Rhythm Mode:</label>
        <select id="modalLayerMode" class="modal-select">
          <option value="${RhythmMode.POLYRHYTHMIC}" ${layer.mode === RhythmMode.POLYRHYTHMIC ? 'selected' : ''}>Polyrhythmic (Ratio)</option>
          <option value="${RhythmMode.POLYMETRIC}" ${layer.mode === RhythmMode.POLYMETRIC ? 'selected' : ''}>Polymetric (Meter)</option>
          <option value="${RhythmMode.EUCLIDEAN}" ${layer.mode === RhythmMode.EUCLIDEAN ? 'selected' : ''}>Euclidean (Bjorklund)</option>
        </select>
      </div>

      ${layer.mode !== RhythmMode.POLYRHYTHMIC ? `
      <div class="modal-section">
        <label>Total Steps (n):</label>
        <input type="number" id="modalLayerSteps" class="input-tiny" min="1" max="32" value="${layer.totalSteps}">
      </div>` : ''}

      <div class="modal-row-2col">
        <div class="modal-slider-item">
          <span>Volume:</span>
          <input type="range" id="modalLayerVol" min="0" max="1" step="0.05" value="${layer.volume}">
        </div>
        <div class="modal-slider-item">
          <span>Pan (L/R):</span>
          <input type="range" id="modalLayerPan" min="-1" max="1" step="0.1" value="${layer.pan}">
        </div>
      </div>

      <div class="modal-slider-item">
        <span>Tone / Pitch (Hz):</span>
        <input type="range" id="modalLayerFreq" min="400" max="2400" step="50" value="${layer.synthFreq}">
      </div>

      <div class="modal-actions-bar">
        <button id="modalBtnSolo" class="btn-modal-solo ${layer.isSolo ? 'active' : ''}">SOLO</button>
        <button id="modalBtnMute" class="btn-modal-mute ${layer.isMuted ? 'active' : ''}">MUTE</button>
        ${this.engine.layers.length > 1 ? `<button id="modalBtnRemove" class="btn-modal-remove">Remove</button>` : ''}
      </div>
    `;

    this.bindLayerModalEvents(layerId);
    this.layerSettingsModal.classList.remove('hidden');
  }

  bindLayerModalEvents(layerId) {
    const layer = this.engine.layers[layerId];
    if (!layer) return;

    const presetSel = document.getElementById('modalLayerSoundPreset');
    if (presetSel) {
      presetSel.addEventListener('change', (e) => {
        layer.soundPreset = parseInt(e.target.value);
        this.renderLayersUI();
        this.engine.ensureAudioContext().then(() => {
          if (this.engine.audioCtx && this.engine.audioCtx.state === 'running') {
            this.engine.synthesizeClick(layer, BeatAccent.DOWNBEAT, this.engine.audioCtx.currentTime + 0.01);
          }
        });
      });
    }

    const modeSel = document.getElementById('modalLayerMode');
    if (modeSel) {
      modeSel.addEventListener('change', (e) => {
        layer.mode = parseInt(e.target.value);
        layer.updateAccents();
        this.openLayerModal(layerId); // Refresh modal view
        this.renderLayersUI();
      });
    }

    const stepsInp = document.getElementById('modalLayerSteps');
    if (stepsInp) {
      stepsInp.addEventListener('change', (e) => {
        layer.totalSteps = Math.max(1, parseInt(e.target.value) || 1);
        layer.updateAccents();
        this.renderLayersUI();
      });
    }

    const volSl = document.getElementById('modalLayerVol');
    if (volSl) {
      volSl.addEventListener('input', (e) => {
        layer.volume = parseFloat(e.target.value);
      });
    }

    const panSl = document.getElementById('modalLayerPan');
    if (panSl) {
      panSl.addEventListener('input', (e) => {
        layer.pan = parseFloat(e.target.value);
      });
    }

    const freqSl = document.getElementById('modalLayerFreq');
    if (freqSl) {
      freqSl.addEventListener('input', (e) => {
        layer.synthFreq = parseFloat(e.target.value);
      });
    }

    const btnSolo = document.getElementById('modalBtnSolo');
    if (btnSolo) {
      btnSolo.addEventListener('click', () => {
        layer.isSolo = !layer.isSolo;
        btnSolo.classList.toggle('active', layer.isSolo);
        this.renderLayersUI();
      });
    }

    const btnMute = document.getElementById('modalBtnMute');
    if (btnMute) {
      btnMute.addEventListener('click', () => {
        layer.isMuted = !layer.isMuted;
        btnMute.classList.toggle('active', layer.isMuted);
        this.renderLayersUI();
      });
    }

    const btnRemove = document.getElementById('modalBtnRemove');
    if (btnRemove) {
      btnRemove.addEventListener('click', () => {
        this.engine.layers.splice(layerId, 1);
        this.engine.layers.forEach((l, i) => l.id = i);
        this.closeLayerModal();
        this.renderLayersUI();
      });
    }
  }

  closeLayerModal() {
    this.layerSettingsModal.classList.add('hidden');
    this.activeModalLayerId = null;
  }

  /* --------------------------------------------------------------------------
     HAZIR ÖN AYARLAR (Presets)
     -------------------------------------------------------------------------- */
  applyPreset(presetKey) {
    if (presetKey === 'poly34') {
      this.engine.setBpm(120);
      this.engine.timeSigNum = 4;
      this.engine.timeSigDen = 4;
      const l1 = new Layer(0, 3, 3, RhythmMode.POLYRHYTHMIC);
      const l2 = new Layer(1, 4, 4, RhythmMode.POLYRHYTHMIC);
      this.engine.layers = [l1, l2];
    } else if (presetKey === 'poly23') {
      this.engine.setBpm(108);
      const l1 = new Layer(0, 2, 2, RhythmMode.POLYRHYTHMIC);
      const l2 = new Layer(1, 3, 3, RhythmMode.POLYRHYTHMIC);
      this.engine.layers = [l1, l2];
    } else if (presetKey === 'poly45') {
      this.engine.setBpm(116);
      const l1 = new Layer(0, 4, 4, RhythmMode.POLYRHYTHMIC);
      const l2 = new Layer(1, 5, 5, RhythmMode.POLYRHYTHMIC);
      this.engine.layers = [l1, l2];
    } else if (presetKey === 'poly57') {
      this.engine.setBpm(96);
      const l1 = new Layer(0, 5, 5, RhythmMode.POLYRHYTHMIC);
      const l2 = new Layer(1, 7, 7, RhythmMode.POLYRHYTHMIC);
      this.engine.layers = [l1, l2];
    } else if (presetKey === 'euc38') {
      this.engine.setBpm(124);
      const l1 = new Layer(0, 3, 8, RhythmMode.EUCLIDEAN);
      const l2 = new Layer(1, 4, 8, RhythmMode.POLYRHYTHMIC);
      this.engine.layers = [l1, l2];
    } else if (presetKey === 'euc516') {
      this.engine.setBpm(132);
      const l1 = new Layer(0, 5, 16, RhythmMode.EUCLIDEAN);
      const l2 = new Layer(1, 4, 16, RhythmMode.POLYRHYTHMIC);
      this.engine.layers = [l1, l2];
    } else if (presetKey === 'flamenco12') {
      this.engine.setBpm(160);
      this.engine.timeSigNum = 12;
      this.engine.timeSigDen = 8;
      const l1 = new Layer(0, 12, 12, RhythmMode.POLYRHYTHMIC);
      l1.accents = new Array(12).fill(BeatAccent.NORMAL);
      [0, 2, 5, 7, 9].forEach(i => l1.accents[i] = BeatAccent.DOWNBEAT);
      const l2 = new Layer(1, 4, 12, RhythmMode.POLYRHYTHMIC);
      this.engine.layers = [l1, l2];
    } else if (presetKey === 'brubeck98') {
      this.engine.setBpm(144);
      this.engine.timeSigNum = 9;
      this.engine.timeSigDen = 8;
      const l1 = new Layer(0, 9, 9, RhythmMode.POLYRHYTHMIC);
      l1.accents = [
        BeatAccent.DOWNBEAT, BeatAccent.NORMAL,
        BeatAccent.DOWNBEAT, BeatAccent.NORMAL,
        BeatAccent.DOWNBEAT, BeatAccent.NORMAL,
        BeatAccent.DOWNBEAT, BeatAccent.NORMAL, BeatAccent.NORMAL
      ];
      const l2 = new Layer(1, 3, 9, RhythmMode.POLYRHYTHMIC);
      this.engine.layers = [l1, l2];
    }

    this.engine.resetPhases();
    this.renderLayersUI();
  }
}

window.addEventListener('DOMContentLoaded', () => {
  window.app = new MetronomeApp();
});
