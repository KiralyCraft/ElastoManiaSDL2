#include "runtime.hpp"
#include <cstring>
#include <utility>
#include "../ALL.H"

namespace elma
{
void Runtime::initialize_keys()
{
    scancodes.fill(0);
    characters.fill(0);
    keys.fill(0);
    input_queue.clear();
    // Pairs are original DirectInput (PC set-1) codes and SDL2 physical scancodes.
    // Keeping the original codes preserves existing state.dat control bindings.
    const std::pair<int, int> mapping[] =
    {
        {0x01,41}, {0x02,30}, {0x03,31}, {0x04,32}, {0x05,33}, {0x06,34},
        {0x07,35}, {0x08,36}, {0x09,37}, {0x0a,38}, {0x0b,39}, {0x0c,45},
        {0x0d,46}, {0x0e,42}, {0x0f,43}, {0x10,20}, {0x11,26}, {0x12,8},
        {0x13,21}, {0x14,23}, {0x15,28}, {0x16,24}, {0x17,12}, {0x18,18},
        {0x19,19}, {0x1a,47}, {0x1b,48}, {0x1c,40}, {0x1d,224}, {0x1e,4},
        {0x1f,22}, {0x20,7}, {0x21,9}, {0x22,10}, {0x23,11}, {0x24,13},
        {0x25,14}, {0x26,15}, {0x27,51}, {0x28,52}, {0x29,53}, {0x2a,225},
        {0x2b,49}, {0x2c,29}, {0x2d,27}, {0x2e,6}, {0x2f,25}, {0x30,5},
        {0x31,17}, {0x32,16}, {0x33,54}, {0x34,55}, {0x35,56}, {0x36,229},
        {0x37,85}, {0x38,226}, {0x39,44}, {0x3a,57}, {0x3b,58}, {0x3c,59},
        {0x3d,60}, {0x3e,61}, {0x3f,62}, {0x40,63}, {0x41,64}, {0x42,65},
        {0x43,66}, {0x44,67}, {0x45,83}, {0x46,71}, {0x47,95}, {0x48,96},
        {0x49,97}, {0x4a,86}, {0x4b,92}, {0x4c,93}, {0x4d,94}, {0x4e,87},
        {0x4f,89}, {0x50,90}, {0x51,91}, {0x52,98}, {0x53,99}, {0x56,100},
        {0x57,68}, {0x58,69}, {0x9c,88}, {0x9d,228}, {0xb5,84}, {0xb7,70},
        {0xb8,230}, {0xc5,72}, {0xc7,74}, {0xc8,82}, {0xc9,75}, {0xcb,80},
        {0xcd,79}, {0xcf,77}, {0xd0,81}, {0xd1,78}, {0xd2,73}, {0xd3,76},
        {0xdb,227}, {0xdc,231}, {0xdd,101}
    };
    for (const auto& pair : mapping) { scancodes[size_t(pair.first)] = pair.second; }
    for (size_t code = 0; code < scancodes.size(); ++code)
    {
        const int scan = scancodes[code];
        if (scan >= 4 && scan <= 29) { characters[code] = 'a' + scan - 4; }
        if (scan >= 30 && scan <= 38) { characters[code] = '1' + scan - 30; }
        if (scan == 39) { characters[code] = '0'; }
    }
    characters[0x39] = ' ';
    characters[0x34] = '.';
    characters[0x01] = MK_ESC;
    characters[0x1c] = characters[0x9c] = MK_ENTER;
    characters[0xc8] = characters[0x48] = MK_UP;
    characters[0xd0] = characters[0x50] = MK_DOWN;
    characters[0xcb] = characters[0x4b] = MK_LEFT;
    characters[0xcd] = characters[0x4d] = MK_RIGHT;
    characters[0xc9] = characters[0x49] = MK_PGUP;
    characters[0xd1] = characters[0x51] = MK_PGDOWN;
    characters[0xd3] = characters[0x53] = MK_DEL;
    characters[0x0e] = MK_BACKSPACE;
    // Original W_K.CPP overrides +/- to left/right for menu adjustments.
    characters[0x0c] = characters[0x4a] = MK_LEFT;
    characters[0x0d] = characters[0x4e] = MK_RIGHT;
}
}
void di_reset_state(state_s* state)
{
    state->opciok1 = {0xc8, 0xd0, 0xcd, 0xcb, 0x39, 0x2f, 0x14, 0x02};
    state->opciok2 = {0x4c, 0x50, 0x51, 0x4f, 0x52, 0x30, 0x15, 0x03};
    state->billplussz = 0x0d;
    state->billminusz = 0x0c;
    state->billsnap = 0x17;
}
char* kodtobillnev(int code)
{
    if (code <= 0 || code >= 256) { return nullptr; }
    const int scan = elma::runtime().scancodes[size_t(code)];
    if (scan == 0) { return nullptr; }
    // The original API does not mutate this borrowed name.
    return const_cast<char*>(SDL_GetScancodeName(static_cast<SDL_Scancode>(scan)));
}
int controlaltnyomva() { return mk_getstate(0x38) != 0 && mk_getstate(0x1d) != 0; }
