# Executed validation — 11 October 2026

## Scope and environment qualification

This release was built and run as a native **Linux x86-64 SDL2 application**. The game screenshots came from the port, not from Wine, an emulator, a replay video, or an image generator. Original game assets were used locally and are not included in the source archive.

The environment supplied GCC 14.2, Clang, CMake, Ninja, Xvfb, and the genuine **SDL2 2.32.4 shared runtime**. It did **not** contain the SDL2 development package and could not retrieve it through the container network. Therefore these local builds used a small, manually supplied ABI declaration header for the SDL calls used by the port, linked against the real installed library. That header is retained in [evidence/test-sdk](../evidence/test-sdk/README.md) so this limitation is auditable. It implements no SDL functionality and is not used by normal builds.

**Compiling against the complete official SDL2 SDK through the normal dependency-discovery path remains an unexecuted check in this environment.** The normal CMake build and supplied GitHub workflow use official development headers, not the test declaration directory. They should be run on the target machine before publishing binaries. Do not confuse successful real-runtime execution with validation of every platform's SDK or ABI.

[environment.txt](../evidence/environment.txt) records the actual compilers, executable dependencies, and startup status. No usable Wine/Proton/Windows runtime, 32-bit multilib compiler, ARM hardware/toolchain, or real audio-output device was available. No original-binary physics parity, ARM performance, audible equivalence, or universally supported SDL platform claim is made.

## Build and asset-free regression results

| Configuration | Executed result |
|---|---|
| GCC Release, x87 compatibility backend | Built; 2/2 CTest processes passed, comprising 14 preservation groups and a separate malformed-resource rejection. |
| GCC Debug, AddressSanitizer + UndefinedBehaviorSanitizer | Built; the same 2/2 CTest processes passed. |
| Clang Release, experimental portable arithmetic | Built; the same 2/2 CTest processes passed. |
| Python asset-inspection tests | 7 tests passed. |
| CLI argument checks | 10 expected success/error exit codes passed. |

The 14 preservation groups cover 20,000 full and 20,000 clipped texture-sampler cases against an independently expressed byte/word register model; x87 precision/rounding and numeric primitives; the original MSVC random sequence; original disk layouts; profile symlink write rejection; Windows separator/case lookup; copy-on-write isolation; traversal/absolute-path rejection; merged 8.3 enumeration; fresh-profile isolation and encrypted state round-trip; physical control mapping; muted gameplay-event delivery; registered/shareware directory decoding and virtual seeks; and BMP pixel-channel order.

The malformed-resource process rejects an entry extending outside the resource file before loading assets. The Python suite separately tests registered/shareware inspection, truncated directories, invalid extents, duplicate names, wrong magic, and case-insensitive paths. These checks are not a comprehensive audit of every legacy parser.

Logs: [GCC Release](../evidence/ctest-release.txt), [sanitizers](../evidence/ctest-asan.txt), [Clang](../evidence/ctest-clang.txt), [Python](../evidence/python-tests.txt), [CLI](../evidence/cli-checks.json). Build/configuration logs are retained alongside them. The Clang build retains warnings from legacy audio expressions; its successful compile is not numerical certification.

Sanitizer tests used `ASAN_OPTIONS=detect_leaks=0` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. Leak checking was disabled because the original code retains process-lifetime allocations. The covered runs produced no AddressSanitizer or UndefinedBehaviorSanitizer diagnostics. This does not establish that unvisited paths are free of memory errors or that all allocations are leak-free.

## Real running-game checks

`tests/smoke_x11.py` starts the native binary on Xvfb, supplies keyboard/mouse events through XTest, takes actual SDL-renderer readbacks, and requests a normal window close. It checks clean exit and hashes the installed asset tree before and after. It does not render synthetic game frames or replace the simulator.

| Final run | Observed coverage | Presented frames | Queued audio samples |
|---|---|---:|---:|
| GCC Release, `play` | Original menus, Warm Up, direction change, held throttle, rotation/brake inputs, return to menu | 940 | 142,848 |
| GCC + sanitizers, `demo` | Original embedded demonstration playback | 583 | 81,920 |
| GCC + sanitizers, `replay` | Supplied `Leivo.rec` playback | 558 | 70,656 |
| GCC + sanitizers, `external` | External-level selection and native game display | 488 | 38,912 |
| GCC + sanitizers, `editor` | Original progress-gated editor, load external level, Save As `SDLTEST1.LEV`, cursor-zero backspace, reopen saved level | 412 | 0 |
| Clang experimental, `play`, audio disabled | Original menus and controlled Warm Up gameplay with no audio device | 900 | 0 |

**All six final runs exited with code 0 and left every original installation file unchanged.** The editor wrote only into its separate test profile. Imported progress was copied, not modified in place. The editor scenario does not change level geometry and is not exhaustive testing of the editing tools. No successful level finish, leaderboard result, complete-game playthrough, or learned AI policy is claimed.

Audio-enabled scenarios used SDL's actual **dummy audio device**. Nonzero queued samples demonstrate execution of the original mixer and SDL queue, not sound quality, real-device latency, or audible matching. Editor screens did not require audio. Frame/sample counts are diagnostic counters, **not throughput benchmarks**, and vary with timing and capture work.

Per-run binary hashes, exit results, capture hashes, dimensions, and logs are in [runtime-summary.json](../evidence/runtime-summary.json) and `evidence/runtime/`. Captures were visually inspected to confirm actual gameplay, demo/replay content, external-level display, and the editor's reopened eight-character filename. Failed development runs that led to fixes are not passed off as final results.

## README screenshot provenance

The README image is [images/gameplay.png](images/gameplay.png), from the **final GCC Release `play` run**, after a direction change and three-second throttle input. Its original screenshot name was `warm-up-playing.png`.

```text
Dimensions: 1280 × 960
Method: SDL_RenderReadPixels → BMP → lossless PNG
Image SHA-256: 8f99c05fa9503ba89e22124d82e2b63d59255e785fa6c57c0035eba884dcf44d
Executable SHA-256: e2bd6d16a43f376fc29ee1d9157097b5ff88eeeeb820a8a11c5d3af210d5d7c4
```

No overlay, retouching, synthesized art, or renderer substitution was added. The game internally rendered the original 640 × 480 view, presented at 2× scale. [Editor](images/editor.png) and [replay](images/replay.png) captures are also included. The archive does not include the test profiles, original artwork files, resource package, replay, original executable, or font assets.

## Local asset checks

The supplied installation was identified as the **registered** layout with 82 resource entries. Resource and default-graphics hashes are recorded in [asset-inspection.json](../evidence/asset-inspection.json). All entry extents in that resource directory validated. These tests select the correct build edition; they do not prove that the source and executable are the same revision.

The original executable's x87 precision setup was examined statically; a narrow annotated excerpt is in [reference-x87.txt](../evidence/reference-x87.txt). The executable itself was **not run**, and there is no reference state trajectory or 1:1 comparison certificate.

## Reproduce with your own assets

Build normally with official SDL2 development files first. For the optional Linux GUI checks, install Xvfb, libX11, libXtst, and Pillow. Each output directory must be new:

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 tools/check_assets.py /path/to/Elma

python3 tests/smoke_x11.py --binary build/elma-sdl2 \
  --data-dir /path/to/Elma --output /tmp/elma-smoke-play \
  --scenario play --throttle-seconds 3 --seconds 2

ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  python3 tests/smoke_x11.py --binary build-asan/elma-sdl2 \
  --data-dir /path/to/Elma --output /tmp/elma-smoke-editor \
  --scenario editor --import-state /path/to/Elma/state.dat
```

The editor command needs legitimate existing progress sufficient to unlock the original editor. No unlock or profile manipulation is performed by the driver. Other scenarios are `demo`, `replay`, and `external`; the replay scenario expects the supplied installation's suitable replay list. These menu macros are smoke fixtures for this installation, not a generic automation API for arbitrary game data.

The delivery-container build used the manual CMake SDK/library override described above. The commands, logs, hashes, and test-only declarations are retained for transparency, not a recommendation to replace SDL's official headers.

## Relocated source check

The complete source tree was copied to a new directory whose path contains spaces,
then configured and rebuilt from scratch as GCC Release using the same real SDL2
runtime and qualified test-only declaration header. Both CTest processes and all
seven Python tests passed again. The rebuilt executable printed the expected x87
compatibility-target status. This validates source relocation and quoting for that
build path, not the unavailable official SDK or other architectures.

Logs: [configure](../evidence/relocation-configure.txt),
[build](../evidence/relocation-build.txt),
[CTest](../evidence/relocation-tests.txt), and
[Python](../evidence/relocation-python.txt).

## Remaining acceptance work

Run the normal full-SDK build and CI; test native 32-bit x86, Windows, macOS, and ARM64; exercise actual sound hardware; test multiplayer, all editor operations, and broader/custom content; and build a controlled state/input/clock comparison against the exact original executable. Verify source revision correspondence before claiming identical physics. A played replay does not validate simulated physics, since replays carry sampled positions.

The code still uses one original global game instance per process. Mobile/web lifecycle integration and a deterministic external stepping interface are separate future work. See [COMPATIBILITY.md](COMPATIBILITY.md). Exact x86 parity remains the stated goal; all non-x86 targets remain experimental.
