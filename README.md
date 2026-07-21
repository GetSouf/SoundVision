# SoundVision

Open-source **VST3** spatial visualiser for FL Studio (and other JUCE hosts).

Place **Sender** instances on mixer tracks (vocals, guitars, drums…). Put one **Receiver** on the master bus. The Receiver draws a top-down sound field around a virtual head — Insight-style density cloud or a growing-dot grid.

## Download (Windows)

Prebuilt VST3 builds are on the [Releases](https://github.com/GetSouf/SoundVision/releases) page.

1. Download `SoundVision-vX.Y.Z-windows-x64.zip`
2. Close your DAW
3. Unzip and copy `SoundVision.vst3` into:
   - `C:\Program Files\Common Files\VST3\` (recommended), or
   - any folder you already scan for VST3 plugins
4. In FL Studio: **Options → Manage plugins → Find plugins**
5. Insert **SoundVision** on tracks (Sender) and master (Receiver)

Current release: **v0.3.0**

## Why Sender / Receiver?

Once tracks are summed on the master, individual sources cannot be separated. Senders publish lightweight analysis frames into a process-wide hub; the Receiver only visualises.

One plugin binary, mode switch inside the UI.

## Features

- Sender / Receiver modes in a single plugin
- Mid/Side polar sound field (Insight-style) + optional Grid view
- Pan centroid + correlation meter from the bus stereo image
- Frequency range filter (Low / High Hz) with Detail control
- Pass-through insert (does not change the audio)

## Requirements (using a release)

- Windows 10/11 x64
- A VST3 host (FL Studio, etc.)

## Build from source

- Windows 10/11
- Visual Studio 2022 or newer with C++ workload
- [JUCE](https://juce.com/) 8.x (this repo expects `C:\dev\JUCE` by default)
- CMake 3.22+ (ships with Visual Studio)

From PowerShell in the repo root:

```powershell
.\scripts\configure.ps1
.\scripts\build.ps1
.\scripts\package-release.ps1
```

Artefact:

`build\SoundVision_artefacts\Release\VST3\SoundVision.vst3`

Release zip (gitignored `dist/`):

`dist\SoundVision-v0.3.0-windows-x64.zip`

Standalone test app target: `SoundVision_Standalone`.

### Open in Visual Studio

```powershell
start build\SoundVision.sln
```

## FL Studio setup

1. Install the VST3 (from a Release or your own build) and refresh the plugin list.
2. On each track of interest: insert **SoundVision** → Mode **Sender** → set name/colour.
3. On **Master**: insert **SoundVision** → Mode **Receiver**.
4. Use **View** (Insight / Grid), **Low Hz** / **High Hz**, and **Detail**.
5. Pan tracks in the mixer — the field and pan markers should follow the bus image.

> All instances must run in the same host process (normal for FL Studio). Bridged/out-of-process hosts will not share the hub.

## Project layout

```
Source/
  PluginProcessor.*     Audio callback, mode logic, hub publish
  PluginEditor.*        UI wiring
  dsp/                  Band filter, Mid/Side polar analysis
  ipc/                  Shared SoundVisionHub + snapshots
  params/               APVTS parameter layout
  ui/                   Spatial view, FieldRenderer, legend
```

## Roadmap ideas

- Per-source mute / solo in the Receiver UI
- Shared-memory IPC for bridged hosts
- HRTF / true 3D azimuth from more advanced spatial cues
- Preset bands (kick / snare / vocal presence)

## License

MIT — see [LICENSE](LICENSE).

JUCE is licensed separately; comply with the JUCE license for distribution.
