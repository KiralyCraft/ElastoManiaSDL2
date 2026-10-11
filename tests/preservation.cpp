#include "platform/runtime.hpp"
#include "platform/sampler.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include "ALL.H"

namespace
{
void require(bool condition, const char* message)
{
    if (condition == false) { throw std::runtime_error(message); }
}
std::string read_text(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
void write_text(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary);
    stream << text;
    require(stream.good(), "write fixture");
}
void put32(std::vector<uint8_t>& bytes, size_t offset, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) { bytes.at(offset + i) = uint8_t(value >> (8U * i)); }
}
int32_t signed16(int32_t value)
{
    const uint32_t bits = uint32_t(value) & 65535U;
    return bits >= 32768U ? int32_t(bits) - 65536 : int32_t(bits);
}
void resource_fixture(const std::filesystem::path& path, bool malformed)
{
    std::vector<uint8_t> bytes(3616, 0);
    put32(bytes, 0, 1);
    std::memcpy(bytes.data() + 4, "TEST.DAT", 9);
#ifdef ELMA_SHAREWARE
    put32(bytes, 20, malformed ? 9000 : 3608);
    put32(bytes, 24, 8);
    int32_t b = 9882;
#else
    put32(bytes, 20, 8);
    put32(bytes, 24, malformed ? 9000 : 3608);
    int32_t b = 9982;
#endif
    int32_t a = 23;
    for (size_t i = 4; i < 3604; ++i)
    {
        bytes[i] ^= uint8_t(a);
        a %= 3391;
        b = signed16(b + a * 3391);
        a = signed16(31 * b + 3391);
    }
    put32(bytes, 3604, 1347839);
    std::memcpy(bytes.data() + 3608, "ABCDEFGH", 8);
    write_text(path, std::string(reinterpret_cast<char*>(bytes.data()), bytes.size()));
}
void sampler_tests()
{
    std::array<uint8_t, 65536> texture{};
    for (size_t i = 0; i < texture.size(); ++i) { texture[i] = uint8_t((i * 117U) ^ (i >> 8)); }
    uint32_t random = 1;
    auto next = [&]() -> uint32_t { random = random * 1664525U + 1013904223U; return random; };
    for (unsigned trial = 0; trial < 20000; ++trial)
    {
        const uint32_t x = next(), y = next(), dx = next(), dy = next();
        const uint8_t count = uint8_t(trial), transparent = uint8_t(trial >> 8);
        std::array<uint8_t, 258> actual{}, expected{};
        actual.fill(0xa5); expected.fill(0xa5);
        elma::sample_row(transparent, count, actual.data() + 1, texture.data(), x, y, dx, dy);
        // Independent byte/word register model of the original MSVC assembly:
        // DI/SI low-word ADD; AL/AH upper-byte ADC; CH 8-bit countdown.
        uint16_t di = uint16_t(x), si = uint16_t(y);
        uint8_t al = uint8_t(x >> 16), ah = uint8_t(y >> 16), ch = count;
        unsigned output = 1;
        do
        {
            const uint16_t index = uint16_t(uint16_t(ah) * 256U + al);
            if (texture[index] != transparent) { expected[output] = texture[index]; }
            ++output;
            const uint32_t sum_x = uint32_t(di) + uint16_t(dx);
            di = uint16_t(sum_x); al = uint8_t(unsigned(al) + uint8_t(dx >> 16) + (sum_x >> 16));
            const uint32_t sum_y = uint32_t(si) + uint16_t(dy);
            si = uint16_t(sum_y); ah = uint8_t(unsigned(ah) + uint8_t(dy >> 16) + (sum_y >> 16));
            --ch;
        }
        while (ch != 0);
        require(actual == expected, "texture sampler differs from original assembly register model");
        actual.fill(0xa5); expected.fill(0xa5);
        const int width = int(trial % 257);
        elma::sample_row_clipped(transparent, count, actual.data() + 1, texture.data(), x, y, dx, dy, width);
        uint32_t fx = x, fy = y;
        for (int i = 0; i < count && i < width; ++i)
        {
            const uint16_t index = uint16_t((uint64_t(fy >> 16) * 256U) + (fx >> 16));
            if (texture[index] != transparent) { expected[size_t(i + 1)] = texture[index]; }
            fx += dx; fy += dy;
        }
        require(actual == expected, "clipped texture sampler differs from original C path");
    }
}
}
int main(int argc, char** argv)
{
    const bool malformed = argc == 2 && std::string(argv[1]) == "malformed-resource";
    const auto serial = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() / ("elma-sdl-test-" + std::to_string(serial));
    unsigned passed = 0;
    try
    {
        elma::Options options;
        options.data_dir = root / "data";
        options.user_dir = root / "profile";
        std::filesystem::create_directories(options.data_dir);
        std::filesystem::create_directories(options.user_dir);
        elma::Runtime rt(options);
        elma::bind_runtime(&rt);
        resource_fixture(options.data_dir / "Elma.res", malformed);
        if (malformed)
        {
            bool rejected = false;
            try { initqopen(); } catch (const std::exception&) { rejected = true; }
            require(rejected, "out-of-bounds resource directory was accepted");
            std::cout << "PASS malformed resource directory rejected before asset loading\n";
            std::filesystem::remove_all(root);
            return 0;
        }
        auto test = [&](const char* name, const std::function<void()>& run)
        {
            run(); ++passed; std::cout << "PASS " << name << '\n';
        };
        test("20,000 full + 20,000 clipped sampler cases, including wrap/carry/transparency/count zero", sampler_tests);
        test("x87 environment and numeric primitives", []()
        {
            elma::configure_floating_point();
#ifdef ELMA_X87
            unsigned short control = 0;
            __asm__ volatile ("fnstcw %0" : "=m" (control));
            require((control & 0x0f3f) == 0x023f, "not 53-bit, nearest, masked x87");
            volatile double one = 1.0, half_ulp = 0x1p-53;
            const double rounded = (one + half_ulp) - one;
            require(rounded == 0, "x87 did not round to 53 significand bits");
#endif
            require(elma_math_sqrt(4.0) == 2.0, "sqrt");
            require(elma_math_atan2(0.0, 1.0) == 0.0, "atan2 zero");
            require(std::abs(elma_math_atan2(1.0, 0.0) - 1.5707963267948966) < 1e-15, "atan2 operand order");
            require(elma_math_atan2(-1.0, -1.0) < -2.0, "atan2 quadrant");
            require(elma_math_sin(0.0) == 0.0 && elma_math_cos(0.0) == 1.0, "trigonometry");
        });
        test("legacy MSVC random sequence", []()
        {
            elma_srand(1);
            const int expected[] = {41, 18467, 6334, 26500, 19169, 15724, 11478, 29358};
            for (int value : expected) { require(elma_rand() == value, "MSVC rand sequence mismatch"); }
        });
        test("original disk layouts", []()
        {
            require(sizeof(egyhang) == 16 && offsetof(egyhang, hangero) == 12, "replay event layout");
            require(sizeof(jatekos) == 116 && sizeof(palyaidok) == 688 && sizeof(jatekosopciok) == 32, "state layout");
            require(sizeof(vekt2) == 16, "level vector layout");
        });
        test("profile symlink cannot write into original data", [&]()
        {
            std::filesystem::create_directories(options.data_dir / "protected");
            std::error_code ec;
            std::filesystem::create_directory_symlink(options.data_dir / "protected", options.user_dir / "linked", ec);
            if (ec)
            {
                std::cout << "SKIP symlink setup unavailable on this host: " << ec.message() << '\n';
                return;
            }
            bool rejected = false;
            try { rt.write_path("linked/file.lev", false); } catch (const std::exception&) { rejected = true; }
            require(rejected, "profile symlink could overwrite an original asset");
        });
        test("Windows paths and case-insensitive resource lookup", [&]()
        {
            write_text(options.data_dir / "Lgr" / "DeFaUlT.Lgr", "graphics");
            require(read_text(rt.read_path("lgr\\default.lgr")) == "graphics", "case lookup");
        });
        test("copy-on-write keeps installed data untouched", [&]()
        {
            write_text(options.data_dir / "Lev" / "Example.lev", "original");
            const auto output = rt.write_path("lev\\example.lev", true);
            require(read_text(output) == "original", "copy-on-write lost original bytes");
            write_text(output, "updated");
            require(read_text(rt.read_path("LEV/EXAMPLE.LEV")) == "updated", "profile override");
            require(read_text(options.data_dir / "Lev" / "Example.lev") == "original", "original data modified");
        });
        test("absolute paths and traversal rejected", []()
        {
            for (const char* path : {"../state.dat", "lev/../../x", "C:\\Elma\\state.dat", "/etc/passwd"})
            {
                bool rejected = false;
                try { elma::safe_relative(path); } catch (const std::exception&) { rejected = true; }
                require(rejected, "unsafe game path accepted");
            }
        });
        test("8.3 level enumeration merges profile overrides once", [&]()
        {
            write_text(options.data_dir / "Lev" / "Beta.lev", "b");
            write_text(options.data_dir / "Lev" / "toolonglevel.lev", "ignored");
            char name[13]{};
            int result = fifirst(const_cast<char*>("lev\\*.lev"), name);
            std::vector<std::string> names;
            while (result == 0) { names.push_back(elma::lower_ascii(name)); result = finext(name); }
            ficlose();
            require(names == std::vector<std::string>({"beta.lev", "example.lev"}), "enumeration mismatch");
        });
        test("fresh state is isolated; legacy encrypted state round-trips", [&]()
        {
            write_text(options.data_dir / "state.dat", "do not import me implicitly");
            state_s first;
            first.soundon = 0;
            first.jatekosokszama = 1;
            std::strcpy(first.jatekosok[0].nev, "SDLTEST");
            std::strcpy(first.jatekosa, "SDLTEST");
            first.save();
            state_s second;
            require(second.soundon == 0 && second.jatekosokszama == 1, "state flags round-trip");
            require(std::strcmp(second.jatekosa, "SDLTEST") == 0, "player round-trip");
            require(read_text(options.data_dir / "state.dat") == "do not import me implicitly", "original state changed");
        });
        test("physical key map and original two-player control defaults", [&]()
        {
            rt.initialize_keys();
            require(rt.scancodes[0xc8] == 82 && rt.scancodes[0xcd] == 79 && rt.scancodes[0x39] == 44, "SDL physical mapping");
            require(rt.characters[0x0c] == MK_LEFT && rt.characters[0x4e] == MK_RIGHT, "legacy +/- menu mapping");
            state_s state;
            state.resetcontrols();
            require(state.opciok1.billgaz == 0xc8 && state.opciok2.billgaz == 0x4c && state.opciok2.billfordul == 0x52, "control defaults");
        });
        test("muting does not discard gameplay object events", []()
        {
            Mute = 1;
            resetwavegyujto();
            startwavegyujto(4, 0.75, 7);
            int sound = 0, object = 0; double volume = 0;
            require(getwavegyujto(&sound, &volume, &object) != 0, "muting discarded object event");
            require(sound == 4 && object == 7 && volume == 0.75, "event payload");
            require(getwavegyujto(&sound, &volume, &object) == 0, "event delivered twice");
        });
        test("registered/shareware resource decoding and virtual seeks", []()
        {
            initqopen();
            FILE* file = qopen(const_cast<char*>("test.dat"), const_cast<char*>("rb"));
            char bytes[8]{};
            require(std::fread(bytes, 1, 8, file) == 8 && std::memcmp(bytes, "ABCDEFGH", 8) == 0, "resource payload");
            require(qseek(file, -2, SEEK_END) == 0 && std::fgetc(file) == 'G', "virtual SEEK_END");
            require(qseek(file, 1, SEEK_SET) == 0 && std::fgetc(file) == 'B', "virtual SEEK_SET");
            qclose(file);
        });
        test("BMP output stores the real pixel channel order", [&]()
        {
            const uint32_t pixels[2] = {0xff112233U, 0xff445566U};
            elma::write_bmp(root / "capture.bmp", pixels, 2, 1);
            const auto bytes = read_text(root / "capture.bmp");
            require(bytes.size() == 62 && bytes.substr(0, 2) == "BM", "BMP header/row padding");
            require(uint8_t(bytes[54]) == 0x33 && uint8_t(bytes[55]) == 0x22 && uint8_t(bytes[56]) == 0x11, "BMP channels");
        });
        std::filesystem::remove_all(root);
        std::cout << "All " << passed << " preservation test groups passed. This is not an original-binary physics oracle.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL after " << passed << " groups: " << error.what() << "\nFixture: " << root << '\n';
        return 1;
    }
}
