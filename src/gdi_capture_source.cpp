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

#include "libcamcapture/gdi_capture_source.h"
#include "libcamcapture/capture_sample.h"

constexpr int GDI_CAPTURE_BPP = 32;

gdi_capture_source::gdi_capture_source(HWND hwnd)
    : bitmap_info_({})
    , bitmap_frame_{ nullptr }
    , hwnd_(hwnd)
    , desktop_dc_{ ::GetDC(hwnd_) }
    , memory_dc_{ ::CreateCompatibleDC(desktop_dc_) }
    , width_{ 0 }
    , height_{ 0 }
    , x_{ 0 }
    , y_{ 0 }
{
    if (hwnd == 0)
    {
        width_ = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
        height_ = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
        x_ = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
        y_ = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
    }
    else
    {
        RECT window_rect = {};
        BOOL ret = ::GetWindowRect(hwnd, &window_rect);
        assert(ret != 0 && "Failed to get window rect.");

        width_ = window_rect.right - window_rect.left;
        height_ = window_rect.bottom - window_rect.top;
        x_ = 0;
        y_ = 0;
    }

    bitmap_frame_ = ::CreateCompatibleBitmap(desktop_dc_, width_, height_);

    bitmap_info_.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_info_.biWidth = width_;
    bitmap_info_.biHeight = height_;
    bitmap_info_.biPlanes = 1;
    bitmap_info_.biBitCount = GDI_CAPTURE_BPP;
    bitmap_info_.biCompression = BI_RGB;
    bitmap_info_.biSizeImage = 0;
    bitmap_info_.biXPelsPerMeter = 0;
    bitmap_info_.biYPelsPerMeter = 0;
    bitmap_info_.biClrUsed = 0;
    bitmap_info_.biClrImportant = 0;
}

gdi_capture_source::~gdi_capture_source()
{
    ::DeleteDC(memory_dc_);
    ::ReleaseDC(hwnd_, desktop_dc_);
}

bool gdi_capture_source::capture_frame(capture_sample &sample) const
{
    assert(width_ == sample.width()
        && height_ == sample.height()
        && sample.bpp() == GDI_CAPTURE_BPP);

    const auto previous_object = ::SelectObject(memory_dc_, bitmap_frame_);

    BOOL b = ::BitBlt(memory_dc_,
        0, 0, width_, height_,
        desktop_dc_,
        x_, y_,
        SRCCOPY | CAPTUREBLT);
    assert(b);

    ::SelectObject(memory_dc_, previous_object);

    const auto *pbitmap_info = reinterpret_cast<const BITMAPINFO *>(&bitmap_info_);
    int ret = ::GetDIBits(desktop_dc_, bitmap_frame_, 0u, static_cast<UINT>(height_), sample.data(),
        const_cast<BITMAPINFO *>(pbitmap_info), DIB_RGB_COLORS);

    /*
     * From: https://msdn.microsoft.com/en-us/library/windows/desktop/dd144879(v=vs.85).aspx
     *
     * Return value
     * If the lpvBits parameter is non-NULL and the function succeeds, the return value is the
     * number of scan lines copied from the bitmap.
     * If the lpvBits parameter is NULL and GetDIBits successfully fills the BITMAPINFO structure,
     * the return value is nonzero.
     * If the function fails, the return value is zero.
     *
     * This function can return the following value.
     * | error code              | description                                     |
     * | ERROR_INVALID_PARAMETER | One or more of the input parameters is invalid. |
     */
    if (ret == 0 || ret == ERROR_INVALID_PARAMETER)
        return false;

    return true;
}

int gdi_capture_source::width() const noexcept
{
    return width_;
}

int gdi_capture_source::height() const noexcept
{
    return height_;
}

int gdi_capture_source::bpp() const noexcept
{
    return GDI_CAPTURE_BPP;
}
