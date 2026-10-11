#include "runtime.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <map>
#include <system_error>
#include <limits>
#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace elma
{
std::string lower_ascii(std::string text)
{
    for (char& ch : text)
    {
        if (ch >= 'A' && ch <= 'Z')
        {
            ch = char(ch - 'A' + 'a');
        }
    }
    return text;
}
std::filesystem::path safe_relative(const std::string& name)
{
    std::string normalized = name;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    const auto relative = std::filesystem::u8path(normalized);
    if (relative.is_absolute() || relative.has_root_name() || normalized.find(':') != std::string::npos)
    {
        throw std::runtime_error("The legacy engine requested an absolute path: " + name);
    }
    for (const auto& part : relative)
    {
        if (part == "..")
        {
            throw std::runtime_error("Parent traversal is not permitted in game paths: " + name);
        }
    }
    return relative.lexically_normal();
}
std::filesystem::path resolve_case(const std::filesystem::path& root, const std::filesystem::path& relative)
{
    auto current = root;
    for (const auto& part : relative)
    {
        if (part == "." || part.empty())
        {
            continue;
        }
        const auto exact = current / part;
        std::error_code ec;
        if (std::filesystem::exists(exact, ec))
        {
            current = exact;
            continue;
        }
        std::filesystem::path match;
        const std::string folded = lower_ascii(part.u8string());
        for (const auto& item : std::filesystem::directory_iterator(current, ec))
        {
            if (lower_ascii(item.path().filename().u8string()) == folded)
            {
                if (match.empty() == false)
                {
                    throw std::runtime_error("Ambiguous case-insensitive game filename: " + exact.u8string());
                }
                match = item.path();
            }
        }
        current = match.empty() ? exact : match;
    }
    return current;
}
std::filesystem::path Runtime::read_path(const std::string& logical) const
{
    const auto relative = safe_relative(logical);
    const auto user = resolve_case(options.user_dir, relative);
    if (std::filesystem::exists(user) || lower_ascii(relative.u8string()) == "state.dat")
    {
        return user;
    }
    return resolve_case(options.data_dir, relative);
}
std::filesystem::path Runtime::write_path(const std::string& logical, bool copy_original)
{
    const auto relative = safe_relative(logical);
    auto user = resolve_case(options.user_dir, relative);
    // A profile symlink must never turn an edit into a write to the installation
    // (or anywhere else outside the profile). This is not a hostile-user sandbox.
    const auto canonical_user = std::filesystem::weakly_canonical(options.user_dir);
    const auto canonical_data = std::filesystem::weakly_canonical(options.data_dir);
    const auto canonical_target = std::filesystem::weakly_canonical(user);
    const auto inside = [](const std::filesystem::path& parent, const std::filesystem::path& child)
    {
        return std::mismatch(parent.begin(), parent.end(), child.begin(), child.end()).first == parent.end();
    };
    if (inside(canonical_user, canonical_target) == false || inside(canonical_data, canonical_target))
    {
        throw std::runtime_error("Refusing a profile write outside the profile or inside the original installation: " + user.u8string());
    }
    std::filesystem::create_directories(user.parent_path());
    if (copy_original && std::filesystem::exists(user) == false)
    {
        const auto original = resolve_case(options.data_dir, relative);
        if (std::filesystem::is_regular_file(original))
        {
            std::filesystem::copy_file(original, user);
        }
    }
    return user;
}
}

int elma_stricmp(const char* a, const char* b)
{
    const std::string left = elma::lower_ascii(a);
    const std::string right = elma::lower_ascii(b);
    return left.compare(right);
}
int elma_strnicmp(const char* a, const char* b, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        const unsigned char x = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(a[i])));
        const unsigned char y = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(b[i])));
        if (x != y || x == 0)
        {
            return int(x) - int(y);
        }
    }
    return 0;
}
char* elma_strupr(char* text)
{
    for (char* p = text; *p != 0; ++p) { *p = char(std::toupper(static_cast<unsigned char>(*p))); }
    return text;
}
char* elma_strlwr(char* text)
{
    for (char* p = text; *p != 0; ++p) { *p = char(std::tolower(static_cast<unsigned char>(*p))); }
    return text;
}
FILE* elma_fopen(const char* name, const char* mode)
{
    const bool writing = std::strchr(mode, 'w') != nullptr || std::strchr(mode, 'a') != nullptr || std::strchr(mode, '+') != nullptr;
    const auto path = writing ? elma::runtime().write_path(name, mode[0] != 'w') : elma::runtime().read_path(name);
#ifdef _WIN32
    const std::wstring wide_mode(mode, mode + std::strlen(mode));
    return _wfopen(path.c_str(), wide_mode.c_str());
#else
    return std::fopen(path.c_str(), mode);
#endif
}
int elma_access(const char* name, int mode)
{
    const auto path = elma::runtime().read_path(name);
    std::error_code ec;
    if (std::filesystem::exists(path, ec))
    {
        (void)mode;
        return 0;
    }
    errno = ENOENT;
    return -1;
}
int elma_remove(const char* name)
{
    const auto path = elma::runtime().write_path(name, false);
    std::error_code ec;
    const bool removed = std::filesystem::remove(path, ec);
    if (removed && ec.value() == 0) { return 0; }
    errno = ec.value() == 0 ? ENOENT : EIO;
    return -1;
}
int elma_rename(const char* old_name, const char* new_name)
{
    const auto from = elma::runtime().write_path(old_name, true);
    const auto to = elma::runtime().write_path(new_name, false);
    std::error_code ec;
    std::filesystem::rename(from, to, ec);
    if (ec.value() == 0) { return 0; }
    errno = EIO;
    return -1;
}
int elma_mkdir(const char* name)
{
    std::error_code ec;
    std::filesystem::create_directories(elma::runtime().write_path(name, false), ec);
    return ec.value() == 0 ? 0 : -1;
}
int32_t elma_filelength(int handle)
{
#ifdef _WIN32
    const auto length = _filelengthi64(handle);
#else
    struct stat status{};
    if (fstat(handle, &status) != 0) { return -1; }
    const auto length = status.st_size;
#endif
    if (length < 0 || length > std::numeric_limits<int32_t>::max())
    {
        errno = EOVERFLOW;
        return -1;
    }
    return int32_t(length);
}
char* elma_itoa(int value, char* output, int base)
{
    if (base < 2 || base > 36) { output[0] = 0; return output; }
    const bool negative = value < 0 && base == 10;
    uint32_t magnitude = negative ? uint32_t(0) - uint32_t(value) : uint32_t(value);
    char buffer[35]{};
    size_t count = 0;
    do
    {
        const unsigned digit = magnitude % unsigned(base);
        buffer[count++] = char(digit < 10 ? '0' + digit : 'a' + digit - 10);
        magnitude /= unsigned(base);
    }
    while (magnitude != 0);
    size_t pos = 0;
    if (negative) { output[pos++] = '-'; }
    while (count > 0) { output[pos++] = buffer[--count]; }
    output[pos] = 0;
    return output;
}

int finext(char* filename)
{
    auto& rt = elma::runtime();
    if (rt.file_search_open == false) { throw std::runtime_error("finext without fifirst"); }
    filename[0] = 0;
    if (rt.file_index >= rt.file_matches.size()) { return 1; }
    std::strcpy(filename, rt.file_matches[rt.file_index++].c_str());
    return 0;
}
int fifirst(char* pattern, char* filename)
{
    auto& rt = elma::runtime();
    if (rt.file_search_open) { throw std::runtime_error("Nested legacy file enumeration"); }
    rt.file_search_open = true;
    rt.file_matches.clear();
    rt.file_index = 0;
    const auto requested = elma::safe_relative(pattern);
    const auto wanted = elma::lower_ascii(requested.filename().u8string());
    const auto star = wanted.find('*');
    std::map<std::string, std::string> matches;
    for (const auto& root : {rt.options.data_dir, rt.options.user_dir})
    {
        std::error_code ec;
        const auto directory = elma::resolve_case(root, requested.parent_path());
        for (const auto& item : std::filesystem::directory_iterator(directory, ec))
        {
            if (item.is_regular_file() == false) { continue; }
            const auto name = item.path().filename().u8string();
            const auto folded = elma::lower_ascii(name);
            const auto dot = name.find('.');
            if (name.size() > 12 || (dot == std::string::npos ? name.size() : dot) > 8) { continue; }
            bool match = wanted == "*" || wanted == "*.*" || folded == wanted;
            if (star != std::string::npos)
            {
                const auto prefix = wanted.substr(0, star);
                const auto suffix = wanted.substr(star + 1);
                match = match || (folded.size() >= prefix.size() + suffix.size() && folded.compare(0, prefix.size(), prefix) == 0 && folded.compare(folded.size() - suffix.size(), suffix.size(), suffix) == 0);
            }
            if (match) { matches[folded] = name; }
        }
    }
    for (const auto& item : matches) { rt.file_matches.push_back(item.second); }
    return finext(filename);
}
void ficlose()
{
    auto& rt = elma::runtime();
    rt.file_search_open = false;
    rt.file_matches.clear();
    rt.file_index = 0;
}
