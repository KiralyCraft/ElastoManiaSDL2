# Delivery-container ABI declarations only

`SDL.h` contains the minimal declarations used to link the delivery-container builds
against the installed **real SDL2 2.32.4 shared library**. It is not the official
SDL2 SDK, not an SDL implementation, and not a supported replacement for the SDL2
development package. It is retained to make the test environment transparent.

Normal CMake builds do not search this directory. Install the official SDL2 headers
and use the normal CMake dependency discovery. The supplied GitHub workflow targets
that normal route, but has not run during this delivery.

This header is qualified only for the tested little-endian Linux x86-64 ABI. It must
not be used to claim cross-platform header/ABI validation. It provides no video,
audio, clock, or input implementation: all those calls went to the installed SDL
runtime. The screenshots are actual SDL-renderer readbacks, not generated fixtures.
