#include "runtime.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include "../ALL.H"

void initmenukep1();
void initmenukep2();

namespace elma
{
void print_help()
{
    std::cout <<
        "Elasto Mania Classic SDL2\n"
        "Usage: elma-sdl2 --data-dir /path/to/Elma [options]\n\n"
        "  --user-dir PATH         Separate writable profile (default: SDL_GetPrefPath)\n"
        "  --import-state FILE     Copy a state.dat into a new, empty profile\n"
        "  --fullscreen            Desktop fullscreen; original 640x480 logical view\n"
        "  --scale 1..4             Initial window scale (default 2)\n"
        "  --integer-scale         Integer-only nearest-neighbor scaling\n"
        "  --no-vsync              Disable presentation vsync, not the physics clock\n"
        "  --fps N                 Fallback frame cap without vsync (default 60; 0 uncapped)\n"
        "  --buffers 2|3           Legacy framebuffer page count (default 3)\n"
        "  --no-audio              Disable audio output; gameplay events remain active\n"
        "  --no-dialogs            Report errors on stderr instead of modal dialogs\n"
        "  --capture PATH.bmp      Destination for F12 or timed SDL output capture\n"
        "  --capture-after-ms N    Capture on a presented frame after N milliseconds\n"
        "  --quit-after-ms N       Development smoke-test timeout; normal default is off\n"
        "  --record-input FILE     Log polled original key codes with SDL millisecond timestamps\n"
        "  --version               Show compatibility status\n\n"
        "F11 toggles fullscreen. F12 captures the actual SDL-rendered output.\n"
        "Original assets are required and are not included. Other architectures are experimental.\n"
        "x86 targets original behavior; 1:1 equivalence to Elma.exe is not yet certified.\n";
}
uint64_t parse_number(const std::string& text, const std::string& name)
{
    if (text.empty() || text[0] == '-') { throw std::runtime_error("Invalid value for " + name); }
    size_t end = 0;
    const auto number = std::stoull(text, &end);
    if (end != text.size()) { throw std::runtime_error("Invalid value for " + name); }
    return number;
}
}
int main(int argc, char** argv)
{
    elma::Options options;
    std::filesystem::path import_state;
    const char* data_env = std::getenv("ELMA_DATA_DIR");
    options.data_dir = data_env == nullptr ? std::filesystem::current_path() : std::filesystem::u8path(data_env);
    try
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            auto value = [&]() -> std::string
            {
                if (++i >= argc) { throw std::runtime_error("Missing value for " + arg); }
                return argv[i];
            };
            if (arg == "--help" || arg == "-h") { elma::print_help(); return 0; }
            if (arg == "--version") { std::cout << "Elma Classic SDL2 0.1.0\n" << elma::compatibility_label() << '\n'; return 0; }
            if (arg == "--data-dir") { options.data_dir = std::filesystem::u8path(value()); }
            else if (arg == "--user-dir") { options.user_dir = std::filesystem::u8path(value()); }
            else if (arg == "--import-state") { import_state = std::filesystem::u8path(value()); }
            else if (arg == "--capture") { options.capture_path = std::filesystem::u8path(value()); }
            else if (arg == "--record-input") { options.input_record = std::filesystem::u8path(value()); }
            else if (arg == "--scale") { const auto n = elma::parse_number(value(), arg); if (n < 1 || n > 4) { throw std::runtime_error("Scale must be 1..4"); } options.scale = int(n); }
            else if (arg == "--fps") { const auto n = elma::parse_number(value(), arg); if (n > 1000) { throw std::runtime_error("FPS must be 0..1000"); } options.frame_rate = int(n); }
            else if (arg == "--buffers") { const auto n = elma::parse_number(value(), arg); if (n != 2 && n != 3) { throw std::runtime_error("Buffers must be 2 or 3"); } options.buffer_count = int(n); }
            else if (arg == "--capture-after-ms") { options.capture_after_ms = elma::parse_number(value(), arg); }
            else if (arg == "--quit-after-ms") { options.quit_after_ms = elma::parse_number(value(), arg); }
            else if (arg == "--fullscreen") { options.fullscreen = true; }
            else if (arg == "--integer-scale") { options.integer_scale = true; }
            else if (arg == "--no-vsync") { options.vsync = false; }
            else if (arg == "--no-audio") { options.audio = false; }
            else if (arg == "--no-dialogs") { options.dialogs = false; }
            else { throw std::runtime_error("Unknown option: " + arg); }
        }
        if (std::filesystem::is_directory(options.data_dir) == false) { throw std::runtime_error("Data directory does not exist"); }
        elma::Runtime runtime(std::move(options));
        elma::bind_runtime(&runtime);
        try
        {
            runtime.initialize();
            if (import_state.empty() == false)
            {
                const auto destination = runtime.write_path("state.dat", false);
                if (std::filesystem::exists(destination)) { throw std::runtime_error("Profile already has state.dat; refusing to overwrite it"); }
                std::filesystem::copy_file(import_state, destination);
            }
            initqopen();
            initmenukep1();
            initmenukep2();
            Mute = 1;
            mv_main();
            throw std::runtime_error("The legacy main function unexpectedly returned");
        }
        catch (const elma::ExitRequest& request)
        {
            if (State != nullptr) { State->save(); }
            std::cout << "Clean exit: " << runtime.frames << " presented frames, " << runtime.audio_samples << " queued audio samples\n";
            return request.code;
        }
        catch (const std::exception& error)
        {
            std::cerr << "Elma SDL2 error: " << error.what() << '\n';
            if (runtime.options.user_dir.empty() == false)
            {
                std::ofstream log(runtime.options.user_dir / "error.txt");
                log << error.what() << '\n';
            }
            if (runtime.options.dialogs && runtime.window != nullptr)
            {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Elma Classic SDL2", error.what(), runtime.window);
            }
            return 1;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "Elma SDL2: " << error.what() << "\nUse --help for usage.\n";
        return 1;
    }
}
