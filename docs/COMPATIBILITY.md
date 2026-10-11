# Compatibility contract

## Goal versus evidence

**x86 is the 1:1 compatibility target, not a certified equivalence claim.** The tested implementation is native Linux x86-64 with GCC and the x87 backend. ARM64 and other architectures are experimental. Clang and MSVC builds are also experimental for numerical equivalence, including when the CPU is x86.

The port is playable, and the original renderer, physics algorithms, collision rules, menus, editor, and replay logic are retained. That does not establish that every state, pixel, sound, or result matches an original Windows executable. The [test report](TEST_REPORT.md) separates executed tests from outstanding acceptance work.

## Reference identity and source-version caveat

The supplied executable is the registered 1.0 distribution's 32-bit x86 `Elma.exe`:

```text
SHA-256 348d1f3480c001956493f67f159d815bbe5bc51d8a16f8805cc2052039c1c44d
```

Its supplied `Elma.res` validates with the registered resource layout, which is the default build configuration. The source snapshot, however, contains a cosmetic routine replacing the intro's 1.0 lettering with **1.11a**. Matching the resource layout does not demonstrate that this published source is the same engine revision as that executable. The port leaves the installed resource lettering unchanged and identifies itself separately in its window title. No attempt is made to present an unverified source/executable match as established fact.

The executable was inspected statically, not executed in this delivery environment. The archive contains no copy of it.

## Preserved engine behavior

The active source list is explicit. In particular, the simplified alternate `LEPTET_D.CPP` is not built. Physics constants, suspension equations, collision ordering and special cases are not replaced with another engine or deliberately retuned. Original comments warn that apparently imperfect collision choices affect established behavior; those choices are retained.

The game still renders into indexed 640 × 480 software buffers. SDL2 presents an expanded ARGB texture; it does not redraw terrain or bike geometry. Palette channels use the original left shift by two, and the legacy row orientation and framebuffer pages are preserved. Nearest-neighbor scaling does not widen the camera view.

Original game-object events remain active when audio output is disabled. The sound-event queue is not used as an excuse to drop apple, death, or finish processing.

## x86 numerical backend

The original executable includes x87 transcendental instructions. Its startup code at `0x43d7d6` calls the CRT control routine with the 53-bit precision selection. The GNU x86 backend configures the x87 control word to `0x027f`: masked exceptions, nearest rounding, 53-bit significand precision. It reapplies the setting after SDL initialization, audio initialization, and focus restoration.

GCC x86 builds use `-mfpmath=387`, disable fast math and floating-point contraction, and disable automatic tree and SLP vectorization. Wrappers use hardware `fsin`, `fcos`, `fsqrt`, and `fpatan` for the corresponding original math operations. Original 32-bit signed/unsigned integer and wraparound assumptions are made explicit. The original MSVC random generator is reproduced rather than using the host C library's different sequence.

These measures improve the compatibility baseline, but they do **not** reproduce the original compiler's register allocation, spill points, instruction ordering, extended exponent range at every temporary, or all runtime-library behavior. The tested executable is x86-64, not the original 32-bit ABI. A 32-bit build remains an important next comparison. Other library operations, including audio-related exponentials, have not been certified against the old CRT.

The `-fexcess-precision=fast` setting permits legacy x87 intermediate evaluation; it is not a synonym for `-ffast-math`. Do not change the flags and retain an equivalence claim without rerunning comparisons.

## Clock and input

The original stopwatch conversion is retained as `milliseconds * 0.182 - start_time`. The gameplay loop still computes its target with the original `0.0024` factor and advances with steps capped at `0.0055`, shortening the last step to reach the target. There is no replacement 60 Hz physics loop.

SDL's millisecond clock replaces Win32 `timeGetTime`. The different clock origin, clock granularity, presentation timing, and input-polling schedule can still change floating-point differences and the actions applied to an update. `--fps` controls presentation only. Vsync, frame cap, focus changes, and machine load must be recorded when comparing runs. Losing focus clears input and waits for focus, but does not invent a new paused simulation clock.

Keyboard bindings retain the original DirectInput-style physical scan codes and the original state-file mapping. F11 and F12 are newly reserved for fullscreen and screenshots. Menus do not use SDL text input or OS repeat. Short presses during a busy original polling loop may not be sampled.

`--record-input` is a diagnostic log of polled keys and SDL timestamps. It is **not** a deterministic replay oracle, a complete trace of simulator states, or an externally controlled stepping API.

## Files, rendering, and deliberate portability differences

On-disk widths and important record sizes are asserted. Little-endian layout is required. Several formats still use original fixed-layout structures, so this is not a fully endian-independent serializer. Big-endian builds fail explicitly rather than silently corrupt data.

Legacy paths accept Windows separators and case-insensitive names on case-sensitive filesystems. Ambiguous case-insensitive matches are rejected. Original 8.3 filename restrictions are retained in game menus. Writable profile files override installed files; reads otherwise fall back to the installation. Writes, renames, and removals target the profile, with copy-on-write for existing assets. Removing an installed-only external file from an original menu does not delete it from the installation; manage installed assets outside the game. Original state is imported only on explicit request.

Legacy write-path traversal and symlinks escaping into the installation are checked, but the profile layer is not a security boundary against a hostile process modifying paths concurrently. Use trusted local assets and a distinct profile outside the installation. Malformed resource directories are validated; the many remaining legacy level, replay, and graphics parsers are not comprehensively hardened against malicious input.

Presentation through SDL, window resizing, desktop fullscreen, safer quit handling, and profile isolation necessarily differ from exclusive DirectDraw. Audio uses the original mixer with an SDL queued device; buffering/latency and audible equivalence still need testing on real output hardware.

The [patch notes](PATCH_NOTES.md) identify fixes to undefined memory behavior. Those fixes intentionally remove crashes, out-of-bounds accesses, and invalid deallocations. Replay filler bytes and editor error cases are not promised to reproduce the original program's undefined behavior.

## Replay and editor evidence

The supplied replay and an embedded demonstration were decoded and displayed by the real port. Original replay files contain sampled positions and event information, so successful playback is a renderer/file-format test, **not proof that native simulation follows the same trajectory**.

The editor loaded an installed external level, saved it to an eight-character name in a separate profile, and reopened it. A regression exercised backspace at position zero in Save As. The original progress requirement remains enforced. This is not exhaustive testing of topology tools, all editor operations, custom graphics packages, or multiplayer.

## Platform classification

GNU x86 builds select the compatibility backend and print “parity not certified.” Non-x86 and unsupported compiler/backend combinations print **EXPERIMENTAL**. Little-endian ARM64 is a porting target, not a tested release claim. The supplied CI workflow is intended to expand coverage; it has not run as part of this delivery.

Desktop-style blocking loops remain. Android/iOS application lifecycles, browser main-loop integration, touch controls, gamepad mapping, and console packaging are not implemented. SDL2 support on an OS alone does not make those integrations complete.

## What is required before claiming 1:1

Establish that the source corresponds to the selected original executable, then compare original and port under controlled clock/input samples. Record intermediate bike, wheel, collision, object, and finish/death states; report the **first divergent update**, not just the final time. Compare native-resolution indexed frames and palettes separately from window scaling. Include acceleration, braking, rotation reversals, direction changes, hard/narrow collisions, gravity objects, apple collection, finish/death ordering, difficult levels, and multiplayer.

Repeat across 32-bit x86 and x86-64, compiler versions, optimization levels, and presentation settings. Only promote an ARM64 configuration after a separately specified acceptance suite; until then it remains experimental. Replay playback, an attractive screenshot, a few successful levels, and a passing sampler test are not substitutes for this acceptance work.
