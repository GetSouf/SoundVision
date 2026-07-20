# SoundVision

Open-source **VST3** spatial visualiser for FL Studio (and other JUCE hosts).

Place **Sender** instances on mixer tracks (vocals, guitars, drums…). Put one **Receiver** on the master bus. The Receiver draws a top-down scene around a virtual head: each source is a coloured point, with particles showing how energy radiates in the selected frequency band.

## Why Sender / Receiver?

Once tracks are summed on the master, individual sources cannot be separated. Senders publish lightweight analysis frames into a process-wide hub; the Receiver only visualises.

One plugin binary, mode switch inside the UI.

## Features

- Sender / Receiver modes in a single plugin
- Stereo pan + depth estimate from Mid/Side balance
- Band-pass + FFT focus around a chosen centre frequency (e.g. 500 Hz)
- Particle field coloured per source
- Pass-through insert (does not change the audio)

## Requirements

- Windows 10/11
- Visual Studio 2022 or newer with C++ workload
- [JUCE](https://juce.com/) 8.x (this repo expects `C:\dev\JUCE` by default)
- CMake 3.22+ (ships with Visual Studio)

## Build (Visual Studio / CMake)

From PowerShell in the repo root:

```powershell
.\scripts\configure.ps1
.\scripts\build.ps1
```

Or manually:

```powershell
$cmake = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

& $cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DJUCE_PATH=C:/dev/JUCE
& $cmake --build build --config Release --target SoundVision_VST3
```

After a successful build the VST3 is copied to:

`%LOCALAPPDATA%\Programs\Common\VST3\SoundVision.vst3`

Add that folder in FL Studio → Options → File settings → VST plugins extra search folder (if needed), then refresh plugins.

Artefact also stays in:

`build\SoundVision_artefacts\Release\VST3\SoundVision.vst3`

Standalone test app target: `SoundVision_Standalone`.

### Open in Visual Studio

```powershell
start build\SoundVision.sln
```

## FL Studio setup

1. Build the VST3 and refresh the plugin list (or point FL to your VST3 folder).
2. On each track of interest: insert **SoundVision** → Mode **Sender** → set name/colour.
3. On **Master**: insert **SoundVision** → Mode **Receiver**.
4. Dial **Centre Hz** / **Bandwidth** to inspect a frequency region (e.g. 500 Hz ± 100 Hz).
5. Pan tracks in the mixer — source points should move around the head.

> All instances must run in the same host process (normal for FL Studio). Bridged/out-of-process hosts will not share the hub.

## Project layout

```
Source/
  PluginProcessor.*     Audio callback, mode logic, hub publish
  PluginEditor.*        UI wiring
  dsp/                  Band filter, stereo/FFT analysis, particles
  ipc/                  Shared SoundVisionHub + snapshots
  params/               APVTS parameter layout
  ui/                   Spatial view + look and feel
```

## Roadmap ideas

- Per-source mute / solo in the Receiver UI
- Shared-memory IPC for bridged hosts
- HRTF / true 3D azimuth from more advanced spatial cues
- Preset bands (kick / snare / vocal presence)

## License

MIT — see [LICENSE](LICENSE).

JUCE is licensed separately; comply with the JUCE license for distribution.
