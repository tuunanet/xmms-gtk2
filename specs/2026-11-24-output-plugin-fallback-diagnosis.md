# Investigation: no sound + racing seek bar on Ubuntu 26.04 build

Date: 2026-11-24
Symptom: On Ubuntu 26.04, starting playback makes the seek bar race through
the song at decode speed and no audio is heard. The same tree reportedly
plays fine on a Kubuntu 26.04 machine.

## Outcome

Not a playback-code bug in the common case. Two build/configuration issues
combined to leave XMMS on the Disk Writer output plugin:

1. `./configure` ran before `libasound2-dev` was installed, so the ALSA
   output plugin was never built (only OSS and disk_writer). OSS cannot
   open on modern Ubuntu (no `/dev/dsp`).
2. With no usable device plugin, `xmms/main.c`'s fallback chain (see its
   own FIXME about "usually the disk writer plugin") ends up on
   disk_writer and persists the choice to `~/.xmms/config`.

Disk writer violates the output-plugin pacing contract by design:
`buffer_free()` is unbounded and `output_time()` returns the written time,
so the decode loop runs at full speed and the UI clock races.

## Fix applied

- Installed `libasound2-dev` (was already done by the user), re-ran
  `./configure`, rebuilt: `Output/alsa/.libs/libALSA.so` now exists.
- Switched `~/.xmms/config` `output_plugin` to the ALSA plugin (backup at
  `~/.xmms/config.bak-20261124`).

## Feedback loop (kept as a regression test)

`tests/test-output-pacing.c` loads a real output plugin `.so`, drives the
input-plugin contract (open_audio, buffer_free/write_audio loop), and
asserts:

- `output_time()` never runs ahead of wall clock + slack,
- wall clock exceeds the plugin's buffering headroom (8 s of audio vs
  ~4 s default buffering),
- the device clock advances (stream actually started).

Wired into `make check` behind an existence gate so trees without ALSA
headers still pass. Verified:

- disk_writer: RED — 3000 ms of audio "played" in 0.3 ms, `output_time`
  3018 ms.
- ALSA via PipeWire: GREEN — 3988 ms wall for 8000 ms of audio after
  buffering, `output_time` tracks wall within ~60 ms.
- End to end in the real binary under xvfb (tonegen playlist, remote
  control): before the fix `output_time` advanced 1,040,742 ms per 1000 ms
  of wall clock; after the fix exactly 1000 ms per 1000 ms.

## Follow-up recommendations (not implemented)

1. `xmms/main.c` still silently lands on disk_writer when no real output
   plugin is available (FIXME acknowledged in source). Consider refusing
   to auto-select a non-device plugin and warning instead.
2. The disk_writer `output_time()` racing behavior also breaks the vis
   queue timestamp contract; if disk rendering is ever used deliberately
   it should pace `written_time()` to the decode rate at minimum.
