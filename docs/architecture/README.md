# XMMS Classic — Architecture

This directory documents the internal architecture of XMMS Classic: UI, audio
pipeline, plugins, external control, playlist/streaming side paths, and
build/test layout.

| Document | Contents |
| --- | --- |
| [UI interaction](ui-interaction.md) | Windows, skinned widgets, core APIs, idle loop, remote control — start here if you are new to the codebase |
| [Processing pipeline](processing-pipeline.md) | End-to-end PCM path, plugin roles, threading, visualization side-channel |
| [Plugin system](plugin-system.md) | Discovery, loading, vtables, enable/disable lifecycle |
| [External control](external-control.md) | libxmms, wmxmms, General plugins, control socket clients |
| [Playlist and streaming](playlist-and-streaming.md) | Get-info thread, lazy metadata, HTTP/Icecast inside Input plugins |
| [Build and test](build-and-test.md) | Autotools layout, `make check`, packaging, CI |
| [Skins](skins.md) | Hard requirements for WinAmp 2 / XMMS skin files (artists + developers) |

Diagrams use [Mermaid](https://mermaid.js.org/) and render on GitHub and most
Markdown previewers.

## Quick map of the tree

```
xmms/                 Core player (GTK UI, playlist, plugin glue)
libxmms/              Shared helpers (config, titles, remote control API)
Input/                Input plugins  (decode → PCM; some embed HTTP)
Output/               Output plugins (PCM → device / file)
Effect/               Effect plugins (in-place PCM transforms)
Visualization/        Vis plugins    (PCM / spectrum renderers)
General/              General plugins (IR, joystick, song change; not on PCM path)
wmxmms/               Dockapp client (libxmms remote API)
tests/                Regression suite (make check)
packaging/ tools/     Debian packaging and release helpers
```

## Two-sentence overview

**UI is a skinned client of core services** (playlist, input/output glue,
config, control socket)—not the media engine itself. See
[UI interaction](ui-interaction.md).

**Playlist picks a file → an Input plugin decodes it to PCM → the selected
Output plugin runs enabled Effect plugins, converts format if needed, and
writes to the audio device, while a time-stamped PCM tap feeds Visualization
and the built-in analyzer/scope.**

## Suggested reading paths

| If you want to… | Read |
| --- | --- |
| Learn the desktop UI | [UI interaction](ui-interaction.md) → [processing pipeline](processing-pipeline.md) |
| Write or debug a plugin | [Plugin system](plugin-system.md) → pipeline or [external control](external-control.md) for General |
| Drive XMMS from another program | [External control](external-control.md) |
| Understand playlist titles / HTTP URLs | [Playlist and streaming](playlist-and-streaming.md) |
| Run tests or change the build | [Build and test](build-and-test.md), [CONTRIBUTING.md](../../CONTRIBUTING.md) |
| Draw or package a skin | [Skins](skins.md), [manual §2.3](../manual.md#23-skin-installation) |
