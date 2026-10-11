# Building the SDL2 port

## Common build

Use a C++17 compiler, CMake >= 3.18, and SDL2 >= 2.0.18 development files for the **same architecture as the executable**. The native executable does not require Python. Python is optional for asset inspection and the X11 smoke driver.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/elma-sdl2 --data-dir /absolute/path/to/Elma
```

SDL2 is found through its CMake config package or `pkg-config`. This build does not download dependencies or game data. Do not add every `.CPP` file to a new project: the original tree includes incompatible alternative implementations. `cmake/LegacySources.cmake` is the authoritative list.

## Linux distributions

Debian/Ubuntu:

```sh
sudo apt-get install build-essential cmake ninja-build pkg-config libsdl2-dev
```

Arch Linux with its current packaged compatibility implementation:

```sh
sudo pacman -S --needed base-devel cmake ninja pkgconf sdl2-compat
```

The Arch dependency is an SDL2 API layer implemented over SDL3. It has not been tested with this release. Do not silently treat its behavior as evidence about the genuine SDL2 runtime. For original-behavior investigations, build real SDL2 into a separate prefix and use the instructions below.

For Linux ARM64, the same native build commands select portable arithmetic and print **EXPERIMENTAL**. This is not an ARM64 validation certificate. No x86 intrinsics are compiled on ARM.

## Genuine SDL2 in a private prefix

Obtain an official SDL2 source release separately. Do not use the SDL3 source tree for these instructions. With that source already unpacked:

```sh
cmake -S /path/to/SDL2-source -B /path/to/sdl2-build \
  -DCMAKE_BUILD_TYPE=Release \
  -DSDL_SHARED=ON -DSDL_STATIC=OFF \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/elma-sdl2"
cmake --build /path/to/sdl2-build --parallel
cmake --install /path/to/sdl2-build

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/.local/elma-sdl2"
cmake --build build --parallel
```

The installed SDL library must also be discoverable by the platform's runtime loader. On Linux, a temporary development launch can use:

```sh
LD_LIBRARY_PATH="$HOME/.local/elma-sdl2/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
  ./build/elma-sdl2 --data-dir /path/to/Elma
```

Use `ldd build/elma-sdl2` on Linux or `otool -L build/elma-sdl2` on macOS to inspect the actual selected library. These commands diagnose dependencies; they do not establish game compatibility.

## 32-bit x86

The original Windows reference executable is 32-bit. A GCC i686 build is an important compatibility target, separate from the tested x86-64 build. It was not possible to run an i686 build in the delivery environment.

On a Debian/Ubuntu x86-64 development machine, 32-bit compilation requires multilib compiler support and a matching i386 SDL2 development library. In an appropriate disposable build environment:

```sh
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install g++-multilib cmake ninja-build libsdl2-dev:i386
cmake -S . -B build-i686 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/i686-linux.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DELMA_SDL2_INCLUDE_DIR=/usr/include/SDL2 \
  -DELMA_SDL2_LIBRARY=/usr/lib/i386-linux-gnu/libSDL2.so
cmake --build build-i686 --parallel
ctest --test-dir build-i686 --output-on-failure
```

The explicit SDL paths above are Debian multiarch paths, not universal locations. Do not link a 64-bit SDL library into the 32-bit build. A native 32-bit distribution can use the common build instead. These instructions are supplied for validation, not reported as executed results.

## Windows / MSYS2

Use the **UCRT64 shell**, not the plain MSYS shell:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-SDL2
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/elma-sdl2.exe --data-dir /c/Games/Elma --user-dir /c/Games/ElmaSDLProfile
```

The shell supplies the SDL2 and GCC runtime DLL search path. Packaging for launch outside that shell must include the appropriate runtime dependencies. Do not copy unrelated DLLs from another architecture.

GCC x86/x86-64 uses the x87 backend. The MSVC branch uses conservative floating-point flags, but it has not been compiled here and is not a numerical-equivalence target in this release. Prefer MinGW/GCC for the first Windows validation. `SDL_MAIN_HANDLED` plus `SDL_SetMainReady` replaces SDL's main trampoline; there is no DirectX initialization in the active platform code.

## macOS

Homebrew's current SDL2 provider is `sdl2-compat`, also known as `sdl2`. To test that explicitly experimental configuration:

```sh
brew install cmake ninja pkgconf sdl2-compat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DELMA_X86_COMPAT=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/elma-sdl2 --data-dir /path/to/Elma
```

For genuine SDL2, use the private-prefix build above instead. Native macOS execution and `.app` packaging have not been validated here. A universal build would require both architecture slices of every dependency; build and test each architecture separately first.

## CMake switches

| Option | Default | Meaning |
|---|---|---|
| `ELMA_X86_COMPAT` | `ON` | Select x87 only for supported GCC x86 targets; other combinations remain explicitly experimental. |
| `ELMA_SHAREWARE` | `OFF` | Select the original SARVARI resource layout instead of the registered layout. |
| `ELMA_BUILD_TESTS` | `ON` | Build the asset-free preservation tests. |
| `ELMA_SANITIZERS` | `OFF` | Enable AddressSanitizer and UndefinedBehaviorSanitizer on GCC/Clang. |
| `ELMA_SDL2_INCLUDE_DIR` + `ELMA_SDL2_LIBRARY` | empty | Expert override for an existing SDL2 SDK/library pair; both must be supplied together. |

Do not add `-ffast-math`, `-Ofast`, FMA contraction, or architecture-wide vectorization overrides for compatibility runs. Record all compiler flags, including environment-provided flags. The `-fexcess-precision=fast` flag in the x87 configuration is a specific legacy-intermediate-format choice, **not** `-ffast-math`.

## Platform limits

This is currently a desktop-style application with the original blocking game/menu loops and one global game instance per process. There is no Android activity, iOS lifecycle integration, browser main-loop adaptation, touch layout, gamepad mapping, or console packaging. These must be added and tested explicitly. Big-endian targets fail early rather than silently misread the remaining legacy on-disk structures.

## Dependency references

Checked during development on 11 October 2026:

- [SDL2 CMake integration](https://wiki.libsdl.org/SDL2/README-cmake)
- [SDL_SetMainReady](https://wiki.libsdl.org/SDL2/SDL_SetMainReady)
- [SDL_RenderWindowToLogical, available since SDL 2.0.18](https://wiki.libsdl.org/SDL2/SDL_RenderWindowToLogical)
- [Arch sdl2-compat package](https://archlinux.org/packages/extra/x86_64/sdl2-compat/)
- [MSYS2 UCRT64 SDL2 package](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-SDL2)
- [Homebrew sdl2-compat formula](https://formulae.brew.sh/formula/sdl2-compat)
- [GitHub runner labels](https://docs.github.com/en/actions/reference/runners/github-hosted-runners)
- [MSYS2 setup action](https://github.com/msys2/setup-msys2)
