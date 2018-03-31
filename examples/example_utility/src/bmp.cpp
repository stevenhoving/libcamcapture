/* Copyright(c) 2018 Steven Hoving
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "example_utility/bmp.h"
#include <libcamcapture/capture_sample.h>
#include <windows.h>
#include <algorithm>
#include <cstdint>

namespace example_utility
{

int align_up(int size, int align)
{
    return ((size + (align - 1)) & ~(align - 1));
}

void save_bmp(const capture_sample &sample, const char *filename)
{
    const int pixel_size = sample.bpp() / 8;
    int width_padded = align_up(sample.width(), 4);

    BITMAPFILEHEADER header = {};
    header.bfType = ('B' | 'M' << 8);
    header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    header.bfSize = header.bfOffBits + width_padded * sample.height() * 3;

    BITMAPINFOHEADER info_header = { 0 };
    info_header.biSize = sizeof(BITMAPINFOHEADER);
    info_header.biWidth = sample.width();
    info_header.biHeight = sample.height();
    info_header.biPlanes = 1;
    info_header.biBitCount = 24;
    info_header.biCompression = BI_RGB;

    FILE *fp = nullptr;
    auto ret = fopen_s(&fp, filename, "wb");
    if (ret != 0 || fp == nullptr)
    {
        if (fp != nullptr)
            fclose(fp);

        printf("unable to open file for writing: %s\n", filename);
        abort();
    }

    fwrite(&header, 1, sizeof(header), fp);
    fwrite(&info_header, 1, sizeof(info_header), fp);

    for (int y = 0; y < sample.height(); ++y)
    {
        auto *row = &sample.data()[y * sample.width() * pixel_size];
        for (int x = 0; x < width_padded; ++x)
        {
            if (x < sample.width())
            {
                fwrite(&row[x * pixel_size], 1, 3, fp);
            }
            else
            {
                uint8_t pixel4[4] = { 0 };
                const int padding = std::min<int>(width_padded - sample.width(), 4);
                fwrite(&pixel4, padding, 1, fp);
                x += padding;
            }
        }
    }

    fclose(fp);
}

} // namespace example_utility
