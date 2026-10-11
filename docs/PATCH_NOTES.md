# Port notes and provenance — 0.1.0

This is a source-preservation port of the attached `elma-classic-master(1).zip`, not a new physics implementation. The source snapshot was supplied from `KiralyCraft/elma-classic`; it was not re-downloaded or substituted during implementation.

## Source identities

| Input | SHA-256 |
|---|---|
| Attached source ZIP | `e2cd34b3c6bd848604e2001369e133be366d6ce3011f36aa591e07f17dc98032` |
| Attached original-game ZIP, used locally for testing only | `86d88d10accda2adc9447fbc99058f63eb1942d63b11621363b0401e2050401d` |
| Unchanged original `LICENSE.md` | `f214ec24d04bc0de8422ac11b9d4b13a4c9dd4d52a753e7d351187ec19e4be77` |

The original release README is retained as [UPSTREAM_README.md](UPSTREAM_README.md). Original authorship and licensing remain intact; this fork is unofficial. Non-source game assets and the original executable are not distributed here. Documentation screenshots show the game in operation without relicensing its artwork.

[SOURCE_CHANGES.json](SOURCE_CHANGES.json) records original and updated hashes for all original source-snapshot files. It allows changes to be reviewed against the user's exact input ZIP. [MODERNIZATION.json](MODERNIZATION.json) is the initial mechanical-pass count, **not** an exhaustive list of subsequent fixes. [ARRAY_DELETE_AUDIT.json](ARRAY_DELETE_AUDIT.json) lists specifically audited array-deallocation replacements.

## Added platform and build layer

`platform/runtime.cpp` provides SDL2 lifecycle, indexed-buffer presentation, palette handling, millisecond timing, actual-output screenshots, focus handling, mouse coordinates, and queued audio. `platform/input.cpp` preserves original scan-code/control semantics. `platform/filesystem.cpp` provides case/separator compatibility, isolated profiles, resource lookup, explicit legacy random behavior, and CRT replacements. `platform/numeric.hpp` and `platform/sampler.hpp` isolate x87 and texture-sampling compatibility decisions.

CMake builds an explicit set of **59 original translation units**, plus the new platform. The original sound mixer `F_WIN/W_HHIGH.CPP` remains active; DirectDraw, DirectInput, DirectSound device code, alternate physics, and old Windows entry points are not compiled. Historical source files remain in the tree for reference and must not be indiscriminately added to the build.

The default is the registered resource layout; `ELMA_SHAREWARE` selects the original alternate layout. Runtime asset inspection is available through `tools/check_assets.py`. There are no automatic downloads, executable patching, hidden network services, AI models, or bundled game assets.

## Mechanical language and representation changes

Active code uses correct include case and portable separators. Original 32-bit `long` assumptions become `int32_t`/`uint32_t`, with corresponding format conversions. Legacy MSVC loop-variable visibility is preserved by declarations in the original enclosing scope rather than changing loop behavior. Invalid qualifications, old implicit declarations, obsolete headers, and one reserved identifier are adapted for C++17.

The x86 texture-sampling assembly is translated to explicit byte/word and 32-bit carry/wrap semantics. Tagged texture pointers use pointer-width integers; the rendering algorithm is not replaced. Resource, state, sound, and replay structures gain relevant size/offset assertions. No global packing directive is used to disguise a width mismatch.

## Targeted correctness fixes

| Location | Change and reason |
|---|---|
| `ABC8`, `ECSET`, `GOLYOK`, `GYURU`, `KISKEP`, `LGRFILE`, `MAIN`, `MENUKEP`, `PIC8`, `SPRITE`, `SZAKASZ`, `VALASZT2` | Use array deletion for allocations confirmed to be arrays. Object deletion is retained for object allocations. |
| `TOPOL.CPP` | Terminate a ten-byte sprite-name buffer inside its bounds rather than writing index ten. |
| `RECORDER.CPP` | Correct a pointer-arithmetic out-of-bounds read used for replay filler bytes. Packed unused filler bits may differ; saved replay files are not promised byte-identical. |
| `EDITTOLT.CPP` | Enlarge Save As storage to fit an eight-character base, `.lev`, and NUL. Backspace at cursor zero becomes a no-op instead of writing before the name buffer. |
| `STATE.CPP` | Replace a failed-file-reopen loop with an explicit error; avoid treating player names and formatted time strings as `fprintf` format strings. |
| `QOPEN.CPP` | Validate resource directory counts, names, magic, duplicate entries, and entry extents before exposing reads. |
| `TELJES.CPP` | Do not alter the installed intro resource's printed version lettering. The port's own version/status is displayed outside the original artwork. |

These fixes remove undefined behavior and unsafe error handling. They do not justify claiming exact original-binary equivalence. They are deliberately identified rather than hidden as “no behavioral changes.”

## Deliberate user-facing changes

The game is windowed/resizable by default, with original-ratio nearest-neighbor presentation, optional desktop fullscreen, F11 fullscreen switching, and F12 real-frame capture. Settings, results, replays, and edited levels go to a separate profile. State import is explicit and never overwrites an existing profile. The original editor progress requirement is not bypassed.

Window close saves normal state and exits through the runtime. A dirty editor refuses an external close rather than silently discarding work. Disabling audio preserves gameplay events. Errors are reported to stderr and optionally an SDL dialog. Diagnostic clock/input logging and timed exit/capture are opt-in; no deterministic external stepping API is claimed.

See [COMPATIBILITY.md](COMPATIBILITY.md) for all numerical, timing, source-version, filesystem, platform, and validation limitations.
