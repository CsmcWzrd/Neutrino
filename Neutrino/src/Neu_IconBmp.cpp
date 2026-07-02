#include "Neutrino/Neutrino.hpp"
#include <fstream>

namespace neutrino {

bool Neu_IconBmp::load(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    unsigned char header[54]{};
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    if (file.gcount() != static_cast<std::streamsize>(sizeof(header)) || header[0] != 'B' || header[1] != 'M') {
        return false;
    }

    const uint32_t dataOffset = *reinterpret_cast<uint32_t*>(&header[10]);
    const int32_t width = *reinterpret_cast<int32_t*>(&header[18]);
    const int32_t height = *reinterpret_cast<int32_t*>(&header[22]);
    const uint16_t bitsPerPixel = *reinterpret_cast<uint16_t*>(&header[28]);

    if (width <= 0 || height == 0 || (bitsPerPixel != 24 && bitsPerPixel != 32)) {
        return false;
    }

    w_ = width;
    h_ = std::abs(height);
    pixels_.assign(static_cast<size_t>(w_ * h_), 0xff000000);

    const int rowStride = ((w_ * bitsPerPixel + 31) / 32) * 4;
    std::vector<unsigned char> row(static_cast<size_t>(rowStride));
    file.seekg(dataOffset, std::ios::beg);

    for (int y = 0; y < h_; ++y) {
        file.read(reinterpret_cast<char*>(row.data()), rowStride);
        const int destinationY = height > 0 ? (h_ - 1 - y) : y;

        for (int x = 0; x < w_; ++x) {
            const int bytesPerPixel = bitsPerPixel / 8;
            const unsigned char blue = row[static_cast<size_t>(x * bytesPerPixel)];
            const unsigned char green = row[static_cast<size_t>(x * bytesPerPixel + 1)];
            const unsigned char red = row[static_cast<size_t>(x * bytesPerPixel + 2)];
            const unsigned char alpha = bitsPerPixel == 32 ? row[static_cast<size_t>(x * 4 + 3)] : 255;
            pixels_[static_cast<size_t>(destinationY * w_ + x)] =
                (static_cast<uint32_t>(alpha) << 24)
                | (static_cast<uint32_t>(red) << 16)
                | (static_cast<uint32_t>(green) << 8)
                | blue;
        }
    }

    return true;
}

} // namespace neutrino
