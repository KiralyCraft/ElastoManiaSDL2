#include "runtime.hpp"
#include <algorithm>
#include <cerrno>
#include <cfenv>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>
#include <limits>
#if defined(_MSC_VER)
#include <float.h>
#endif
#include "../ALL.H"
void CustomMixer(void*, int, void*, int, int);
int Mostvoltegynemcurpos = 0;
#undef fopen
#undef remove
#undef rename
#undef rand
#undef srand

namespace elma
{
Runtime* active_runtime = nullptr;
Runtime& runtime()
{
    if (active_runtime == nullptr) { throw std::runtime_error("The SDL runtime has not been initialized."); }
    return *active_runtime;
}
void bind_runtime(Runtime* instance)
{
    active_runtime = instance;
}
const char* compatibility_label()
{
#if defined(ELMA_X87)
#if INTPTR_MAX == INT32_MAX
    return "x86 / x87 compatibility target (parity not certified)";
#else
    return "x86-64 / x87 compatibility target (parity not certified)";
#endif
#else
    return "EXPERIMENTAL architecture / floating-point backend";
#endif
}
void configure_floating_point()
{
    std::fesetround(FE_TONEAREST);
#if defined(ELMA_X87) && (defined(__i386__) || defined(__x86_64__))
    // Elma.exe SHA256 348d1f34... CRT at VA 0x43d7d6 requests _PC_53.
    // Preserve the 53-bit significand, extended exponent, nearest rounding,
    // and masked exceptions. Do not substitute 64-bit significand (0x037f).
    const unsigned short control = 0x027f;
    __asm__ volatile ("fnclex\n\tfldcw %0" : : "m" (control));
#elif defined(_MSC_VER) && defined(_M_IX86)
    unsigned int current = 0;
    _controlfp_s(&current, _PC_53 | _RC_NEAR, _MCW_PC | _MCW_RC);
#endif
}
Runtime::Runtime(Options opts) : options(std::move(opts)), argb(640U * 480U)
{
    for (auto& buffer : framebuffers) { buffer.resize(640U * 480U); }
    palette.fill(0xff000000U);
}
Runtime::~Runtime()
{
    if (audio_device != 0) { SDL_CloseAudioDevice(audio_device); }
    if (texture != nullptr) { SDL_DestroyTexture(texture); }
    if (renderer != nullptr) { SDL_DestroyRenderer(renderer); }
    if (window != nullptr) { SDL_DestroyWindow(window); }
    SDL_Quit();
    active_runtime = nullptr;
}
void Runtime::initialize()
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
    {
        throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
    }
    if (options.user_dir.empty())
    {
        char* preferred = SDL_GetPrefPath("ElmaClassicSDL", "ElmaClassicSDL2");
        if (preferred == nullptr) { throw std::runtime_error("SDL_GetPrefPath failed"); }
        options.user_dir = std::filesystem::u8path(preferred);
        SDL_free(preferred);
    }
    options.data_dir = std::filesystem::absolute(options.data_dir);
    options.user_dir = std::filesystem::absolute(options.user_dir);
    std::filesystem::create_directories(options.user_dir);
    if (std::filesystem::equivalent(options.data_dir, options.user_dir))
    {
        throw std::runtime_error("Data and user directories must be different; original data is read-only.");
    }
    for (const char* folder : {"lev", "rec", "lgr", "screenshots"})
    {
        std::filesystem::create_directories(options.user_dir / folder);
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    const std::string title = std::string("Elasto Mania Classic SDL2 - ") + compatibility_label();
    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640 * options.scale, 480 * options.scale, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == nullptr) { throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError()); }
    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    if (options.vsync) { renderer_flags |= SDL_RENDERER_PRESENTVSYNC; }
    renderer = SDL_CreateRenderer(window, -1, renderer_flags);
    renderer_vsync = renderer != nullptr && options.vsync;
    if (renderer == nullptr)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        renderer_vsync = false;
    }
    if (renderer == nullptr) { throw std::runtime_error(std::string("SDL_CreateRenderer: ") + SDL_GetError()); }
    if (SDL_RenderSetLogicalSize(renderer, 640, 480) != 0) { throw std::runtime_error(SDL_GetError()); }
    SDL_RenderSetIntegerScale(renderer, options.integer_scale ? SDL_TRUE : SDL_FALSE);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);
    if (texture == nullptr) { throw std::runtime_error(std::string("SDL_CreateTexture: ") + SDL_GetError()); }
    if (options.fullscreen)
    {
        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) { throw std::runtime_error(SDL_GetError()); }
        fullscreen = true;
    }
    SDL_ShowCursor(0);
    initialize_keys();
    // Some renderer/audio drivers change the FPU environment during setup.
    configure_floating_point();
    if (options.input_record.empty() == false)
    {
        input_log.open(options.input_record);
        if (input_log.is_open() == false) { throw std::runtime_error("Could not open input-record file"); }
        input_log << "# SDL ticks in milliseconds; snapshot of pressed original DirectInput codes\n";
    }
    SDL_version version{};
    SDL_GetVersion(&version);
    std::cout << "Elasto Mania Classic SDL2\n" << compatibility_label() << "\n"
              << "SDL " << int(version.major) << "." << int(version.minor) << "." << int(version.patch)
              << " on " << SDL_GetPlatform() << "\n"
              << "Data (read-only): " << options.data_dir.u8string() << "\n"
              << "User data: " << options.user_dir.u8string() << "\n" << std::flush;
}
void Runtime::pump(bool queue_characters)
{
    audio_tick();
    SDL_Event event{};
    while (SDL_PollEvent(&event) != 0)
    {
        if (event.type == SDL_QUIT)
        {
            if (Editorban_dialnak && Valtozott)
            {
                if (options.dialogs)
                {
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Unsaved level", "Save or leave the editor before closing the window.", window);
                }
            }
            else
            {
                throw ExitRequest{0};
            }
        }
    }
    const bool now_focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    if (now_focused != focused)
    {
        focused = now_focused;
        keys.fill(0);
        input_queue.clear();
        if (focused) { invalidateegesz(); configure_floating_point(); }
    }
    const Uint8* physical = SDL_GetKeyboardState(nullptr);
    const auto old_keys = keys;
    for (size_t code = 0; code < keys.size(); ++code)
    {
        keys[code] = focused && scancodes[code] != 0 && physical[scancodes[code]] != 0 ? 0x80 : 0;
    }
    // DirectInput polled scan order, not OS key repeat; this is intentional.
    for (size_t code = 0; code < keys.size(); ++code)
    {
        if (keys[code] != 0 && old_keys[code] == 0)
        {
            if (code == 0x58) { screenshot_requested = true; continue; } // F12
            if (code == 0x57) // F11
            {
                fullscreen = fullscreen == false;
                SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                continue;
            }
            int character = characters[code];
            if (character >= 'a' && character <= 'z' && (keys[0x2a] != 0 || keys[0x36] != 0))
            {
                character += 'A' - 'a';
            }
            if (queue_characters && character != 0 && input_queue.size() < 30)
            {
                input_queue.push_back(character);
            }
        }
    }
    // Editor dialogs can wait without repainting. F12 must still work there.
    if (screenshot_requested && lock_kind == 0) { present(); }
    if (input_log.is_open())
    {
        input_log << SDL_GetTicks();
        for (size_t code = 0; code < keys.size(); ++code)
        {
            if (keys[code] != 0) { input_log << ' ' << code; }
        }
        input_log << '\n';
    }
    if (options.quit_after_ms != 0 && SDL_GetTicks64() >= options.quit_after_ms)
    {
        throw ExitRequest{0};
    }
    if (focused == false)
    {
        // Match the original blocking focus behavior. Clock continues advancing.
        SDL_Delay(10);
    }
}
void Runtime::present()
{
    if (renderer_vsync == false && options.frame_rate > 0)
    {
        const uint64_t frequency = SDL_GetPerformanceFrequency();
        const uint64_t period = frequency / uint64_t(options.frame_rate);
        const uint64_t now = SDL_GetPerformanceCounter();
        if (last_present != 0 && now < last_present + period)
        {
            const uint64_t remaining_ms = (last_present + period - now) * 1000U / frequency;
            if (remaining_ms > 0) { SDL_Delay(Uint32(remaining_ms)); }
        }
    }
    const auto& pixels = framebuffers[size_t(front)];
    for (size_t i = 0; i < pixels.size(); ++i) { argb[i] = palette[pixels[i]]; }
    if (SDL_UpdateTexture(texture, nullptr, argb.data(), 640 * int(sizeof(uint32_t))) != 0) { throw std::runtime_error(SDL_GetError()); }
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    if (options.capture_after_ms != 0 && timed_capture_done == false && SDL_GetTicks64() >= options.capture_after_ms)
    {
        screenshot_requested = true;
        timed_capture_done = true;
    }
    if (screenshot_requested)
    {
        capture();
        screenshot_requested = false;
    }
    SDL_RenderPresent(renderer);
    last_present = SDL_GetPerformanceCounter();
    ++frames;
}
void Runtime::capture()
{
    auto output = options.capture_path;
    if (output.empty()) { output = options.user_dir / "screenshots" / ("elma-" + std::to_string(SDL_GetTicks64()) + ".bmp"); }
    int width = 0, height = 0;
    if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0 || width <= 0 || height <= 0)
    {
        throw std::runtime_error("Cannot query capture dimensions");
    }
    std::vector<uint32_t> captured(size_t(width) * size_t(height));
    // Read the real SDL backbuffer after RenderCopy and before RenderPresent.
    if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, captured.data(), width * int(sizeof(uint32_t))) != 0)
    {
        throw std::runtime_error(std::string("SDL_RenderReadPixels: ") + SDL_GetError());
    }
    write_bmp(output, captured.data(), width, height);
    std::cout << "Captured running SDL output: " << output.u8string() << " (" << width << "x" << height << ")\n" << std::flush;
}
void write_bmp(const std::filesystem::path& path, const uint32_t* pixels, int width, int height)
{
    if (path.parent_path().empty() == false) { std::filesystem::create_directories(path.parent_path()); }
    std::ofstream file(path, std::ios::binary);
    if (file.is_open() == false) { throw std::runtime_error("Cannot create screenshot " + path.u8string()); }
    const uint32_t stride = (uint32_t(width) * 3U + 3U) & ~3U;
    const uint32_t size = stride * uint32_t(height);
    auto put16 = [&file](uint16_t value)
    {
        file.put(char(value & 255)); file.put(char(value >> 8));
    };
    auto put32 = [&file](uint32_t value)
    {
        for (unsigned shift = 0; shift < 32; shift += 8) { file.put(char((value >> shift) & 255)); }
    };
    file.write("BM", 2); put32(54U + size); put32(0); put32(54); put32(40);
    put32(uint32_t(width)); put32(uint32_t(height)); put16(1); put16(24);
    put32(0); put32(size); put32(2835); put32(2835); put32(0); put32(0);
    for (int y = height - 1; y >= 0; --y)
    {
        for (int x = 0; x < width; ++x)
        {
            const auto color = pixels[size_t(y) * size_t(width) + size_t(x)];
            file.put(char(color & 255)); file.put(char((color >> 8) & 255)); file.put(char((color >> 16) & 255));
        }
        for (uint32_t i = uint32_t(width) * 3U; i < stride; ++i) { file.put(0); }
    }
    if (file.good() == false) { throw std::runtime_error("Screenshot write failed"); }
}
void Runtime::audio_tick()
{
    if (audio_device == 0 || Hangenabled == 0) { return; }
    constexpr unsigned samples = 512;
    std::array<short, samples> buffer{};
    // Keep the legacy mixer on the game thread. The audio thread reads only SDL's
    // private copied queue, so original globals never race with a callback.
    while (SDL_GetQueuedAudioSize(audio_device) < samples * sizeof(short) * 2U)
    {
        CustomMixer(buffer.data(), int(buffer.size() * sizeof(short)), nullptr, 0, 1);
        if (volume != 1.0F)
        {
            for (short& sample : buffer) { sample = short(std::clamp(float(sample) * volume, -32768.0F, 32767.0F)); }
        }
        if (SDL_QueueAudio(audio_device, buffer.data(), Uint32(buffer.size() * sizeof(short))) != 0)
        {
            std::cerr << "Audio disabled: " << SDL_GetError() << '\n';
            SDL_CloseAudioDevice(audio_device);
            audio_device = 0;
            Hangenabled = 0; Mostvoltegynemcurpos = 1;
            return;
        }
        audio_samples += buffer.size();
    }
}
}

int Win_95 = 1;
int Billint = 0;
int Locky0_alul = 0;
int32_t Inkremens = 0;
void inkremental() { ++Inkremens; }
void init_d_dd() {}
void mk_init() { elma::runtime().initialize_keys(); }
void mv_check(int no_characters)
{
    do { elma::runtime().pump(no_characters == 0); } while (elma::runtime().focused == false);
}
int mk_getextchar()
{
    auto& rt = elma::runtime();
    for (;;)
    {
        mv_check();
        if (rt.input_queue.empty() == false)
        {
            const int character = rt.input_queue.front();
            rt.input_queue.pop_front();
            return character;
        }
        SDL_Delay(1);
    }
}
void mk_emptychar() { mv_check(); elma::runtime().input_queue.clear(); }
int mk_kbhit() { mv_check(); return elma::runtime().input_queue.empty() ? 0 : 1; }
int mk_getstate(int code)
{
    if (code < 0 || code >= 256) { throw std::runtime_error("Invalid original key code"); }
    return elma::runtime().keys[size_t(code)];
}
void mv_exit(char* text)
{
    if (text != nullptr) { std::cout << text << '\n'; }
    throw elma::ExitRequest{0};
}
void hiba(char* first, char* second, char* third)
{
    std::string message = first == nullptr ? "Unknown game error" : first;
    if (second != nullptr) { message += std::string("\n") + second; }
    if (third != nullptr) { message += std::string("\n") + third; }
    throw std::runtime_error(message);
}
void uzenet(char* first, char* second, char* third) { hiba(first, second, third); }
double mv_stopperido()
{
    // Retain the original multiplication/subtraction order and millisecond clock,
    // not a new fixed physics tick or seconds-based approximation.
    return SDL_GetTicks() * 0.182 - elma::runtime().start_time;
}
void mv_startstopper() { elma::runtime().start_time = SDL_GetTicks() * 0.182; }
void getbiostort(unsigned short* bios, unsigned short* fraction)
{
    const uint64_t count = SDL_GetTicks64() * 1193182U / 1000U;
    *bios = static_cast<unsigned short>(count >> 16);
    *fraction = static_cast<unsigned short>(count);
}
int elma_rand()
{
    auto& seed = elma::runtime().random_state;
    seed = seed * 214013U + 2531011U;
    return int((seed >> 16) & 0x7fffU);
}
void elma_srand(unsigned seed) { elma::runtime().random_state = seed; }
int s_random(int maximum)
{
    if (maximum <= 0) { throw std::runtime_error("Nonpositive random range"); }
    return elma_rand() % maximum;
}
void getmou(int* x, int* y)
{
    int wx = 0, wy = 0;
    SDL_GetMouseState(&wx, &wy);
    float lx = 0, ly = 0;
    SDL_RenderWindowToLogical(elma::runtime().renderer, wx, wy, &lx, &ly);
    *x = std::clamp(int(lx), 0, 639);
    *y = std::clamp(int(ly), 0, 479);
}
void setmou(int x, int y)
{
    int wx = 0, wy = 0;
    SDL_RenderLogicalToWindow(elma::runtime().renderer, float(x), float(y), &wx, &wy);
    SDL_WarpMouseInWindow(elma::runtime().window, wx, wy);
}
int getbutbmou() { mv_check(); return (SDL_GetMouseState(nullptr, nullptr) & 1U) != 0; }
int getbutjmou() { mv_check(); return (SDL_GetMouseState(nullptr, nullptr) & 4U) != 0; }
void initmou(int, int) {}
void setresolmou(int) {}
void waitforcathode() { SDL_Delay(1); }

unsigned char** lockbackbuffer(int)
{
    auto& rt = elma::runtime();
    if (rt.lock_kind != 0) { throw std::runtime_error("Nested framebuffer lock"); }
    rt.lock_kind = 1;
    for (int y = 0; y < 480; ++y)
    {
        rt.rows[size_t(y)] = rt.framebuffers[size_t(rt.back)].data() + size_t(Locky0_alul ? 479 - y : y) * 640;
    }
    return rt.rows.data();
}
void unlockbackbuffer()
{
    auto& rt = elma::runtime();
    if (rt.lock_kind != 1) { throw std::runtime_error("Unbalanced backbuffer unlock"); }
    rt.lock_kind = 0;
    rt.front = rt.back;
    rt.back = (rt.back + 1) % rt.options.buffer_count;
    rt.present();
}
unsigned char** lockfrontbuffer()
{
    auto& rt = elma::runtime();
    if (rt.lock_kind != 0) { throw std::runtime_error("Nested framebuffer lock"); }
    rt.lock_kind = 2;
    for (int y = 0; y < 480; ++y)
    {
        rt.rows[size_t(y)] = rt.framebuffers[size_t(rt.front)].data() + size_t(Locky0_alul ? 479 - y : y) * 640;
    }
    return rt.rows.data();
}
void unlockfrontbuffer()
{
    auto& rt = elma::runtime();
    if (rt.lock_kind != 2) { throw std::runtime_error("Unbalanced frontbuffer unlock"); }
    rt.lock_kind = 0;
    rt.present();
}
ddpal::ddpal(unsigned char* source)
{
    auto* colors = new std::array<uint32_t, 256>;
    for (size_t i = 0; i < colors->size(); ++i)
    {
        const uint8_t red = uint8_t(source[i * 3] << 2);
        const uint8_t green = uint8_t(source[i * 3 + 1] << 2);
        const uint8_t blue = uint8_t(source[i * 3 + 2] << 2);
        (*colors)[i] = 0xff000000U | uint32_t(red) << 16 | uint32_t(green) << 8 | uint32_t(blue);
    }
    pddpal = colors;
}
ddpal::~ddpal() { delete static_cast<std::array<uint32_t, 256>*>(pddpal); }
void ddpal::set() { elma::runtime().palette = *static_cast<std::array<uint32_t, 256>*>(pddpal); }

void initdsound(int)
{
    auto& rt = elma::runtime();
    if (rt.audio_opened) { return; }
    rt.audio_opened = true;
    if (rt.options.audio == false) { Hangenabled = 0; Mostvoltegynemcurpos = 1; return; }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
    {
        std::cerr << "Audio unavailable: " << SDL_GetError() << '\n'; Hangenabled = 0; Mostvoltegynemcurpos = 1; return;
    }
    SDL_AudioSpec wanted{};
    wanted.freq = 11025;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 1;
    wanted.samples = 512;
    rt.audio_device = SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);
    if (rt.audio_device == 0)
    {
        std::cerr << "Audio unavailable: " << SDL_GetError() << '\n'; Hangenabled = 0; Mostvoltegynemcurpos = 1; return;
    }
    SDL_PauseAudioDevice(rt.audio_device, 0);
    elma::configure_floating_point();
}
void hang() { elma::runtime().audio_tick(); }
void dsoundfinish()
{
    auto& rt = elma::runtime();
    if (rt.audio_device != 0) { SDL_CloseAudioDevice(rt.audio_device); rt.audio_device = 0; }
}
void nullazhangot()
{
    const auto device = elma::runtime().audio_device;
    if (device != 0) { SDL_ClearQueuedAudio(device); }
}
void setvolume(double value) { elma::runtime().volume = float(std::clamp(value, 0.0, 1.0)); }
void stophang() { dsoundfinish(); }
int starthang(unsigned) { initdsound(0); return Hangenabled; }
