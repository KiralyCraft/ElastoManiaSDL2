#pragma once
#include <cstdint>
namespace elma
{
inline void sample_row(uint8_t transparent, uint8_t count, uint8_t* out, const uint8_t* texture, uint32_t x, uint32_t y, uint32_t dx, uint32_t dy)
{
    // The assembly's 8-bit count executes 256 iterations when its input is zero.
    const unsigned iterations = count == 0 ? 256U : unsigned(count);
    for (unsigned i = 0; i < iterations; ++i)
    {
        const uint32_t index = ((y >> 8) & 0xff00U) | ((x >> 16) & 0xffU);
        const uint8_t pixel = texture[index];
        if (pixel != transparent)
        {
            out[i] = pixel;
        }
        x += dx;
        y += dy;
    }
}
inline void sample_row_clipped(uint8_t transparent, int count, uint8_t* out, const uint8_t* texture, uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, int width)
{
    for (int i = 0; i < count && i < width; ++i)
    {
        const uint16_t index = uint16_t((uint16_t(y >> 16) << 8) + uint16_t(x >> 16));
        const uint8_t pixel = texture[index];
        if (pixel != transparent)
        {
            out[i] = pixel;
        }
        x += dx;
        y += dy;
    }
}
}
