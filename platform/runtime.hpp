#pragma once
#include <SDL.h>
#include <array>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>

namespace elma
{
struct ExitRequest
{
    int code;
};
struct Options
{
    std::filesystem::path data_dir;
    std::filesystem::path user_dir;
    std::filesystem::path capture_path;
    std::filesystem::path input_record;
    int scale = 2;
    int frame_rate = 60;
    int buffer_count = 3;
    uint64_t capture_after_ms = 0;
    uint64_t quit_after_ms = 0;
    bool audio = true;
    bool fullscreen = false;
    bool vsync = true;
    bool integer_scale = false;
    bool dialogs = true;
};
struct Runtime
{
    Options options;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_AudioDeviceID audio_device = 0;
    std::array<std::vector<uint8_t>, 3> framebuffers;
    std::array<uint8_t*, 480> rows{};
    std::array<uint32_t, 256> palette{};
    std::vector<uint32_t> argb;
    std::array<uint8_t, 256> keys{};
    std::array<int, 256> scancodes{};
    std::array<int, 256> characters{};
    std::deque<int> input_queue;
    std::vector<std::string> file_matches;
    size_t file_index = 0;
    bool file_search_open = false;
    int front = 0;
    int back = 1;
    int lock_kind = 0;
    bool fullscreen = false;
    bool focused = true;
    bool screenshot_requested = false;
    bool timed_capture_done = false;
    bool renderer_vsync = false;
    bool audio_opened = false;
    double start_time = 0.0;
    uint64_t last_present = 0;
    uint64_t frames = 0;
    uint64_t audio_samples = 0;
    uint32_t random_state = 1;
    float volume = 1.0F;
    std::ofstream input_log;
    explicit Runtime(Options opts);
    ~Runtime();
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    void initialize();
    void pump(bool queue_characters);
    void present();
    void capture();
    void initialize_keys();
    void audio_tick();
    std::filesystem::path read_path(const std::string& logical) const;
    std::filesystem::path write_path(const std::string& logical, bool copy_original);
};
Runtime& runtime();
void bind_runtime(Runtime* instance);
std::string lower_ascii(std::string text);
std::filesystem::path safe_relative(const std::string& name);
std::filesystem::path resolve_case(const std::filesystem::path& root, const std::filesystem::path& relative);
void write_bmp(const std::filesystem::path& path, const uint32_t* pixels, int width, int height);
void configure_floating_point();
const char* compatibility_label();
}
