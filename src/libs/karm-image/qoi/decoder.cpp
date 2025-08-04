module;

#include <karm-gfx/buffer.h>
#include <karm-gfx/colors.h>

export module Karm.Image:qoi.decoder;

import Karm.Core;
import :qoi.base;

namespace Karm::Image::Qoi {

struct Header {
    Array<u8, 4> magic;
    i32be width;
    i32be height;
    i8be channels;
    i8be colorSpace;
};

export struct Decoder : Io::BChunk {
    Header _header;

    Decoder(Bytes slice, Header header)
        : BChunk(slice), _header(header) {}

    static bool sniff(Bytes slice) {
        return slice.len() >= 4 and slice[0] == 0x71 and slice[1] == 0x6F and
               slice[2] == 0x69 and slice[3] == 0x66;
    }

    static Res<Decoder> init(Bytes slice) {
        if (slice.len() < 14)
            return Error::invalidData("image too small");

        Io::BScan s{slice};
        auto header = try$(s.next<Header>());

        if (header.magic != MAGIC)
            return Error::invalidData("invalid magic");

        if (not(header.channels == 4 or header.channels == 3))
            return Error::invalidData("invalid number of channels");

        if (not(header.colorSpace == 0 or header.colorSpace == 1))
            return Error::invalidData("invalid color space");

        return Ok(Decoder{slice, header});
    }

    Res<> decode(Gfx::MutPixels dest) {
        usize run = 0;
        Array<Gfx::Color, 64> index{};
        Gfx::Color pixel = Gfx::BLACK;

        auto s = begin().skip(14); // skip the header

        for (isize y = 0; y < _header.height; y++) {
            for (isize x = 0; x < _header.width; x++) {
                if (s.ended()) {
                    return Error::invalidData("unexpected end of file");
                }

                if (run > 0) {
                    run--;
                    dest.store({x, y}, pixel);
                    continue;
                }

                auto b1 = try$(s.next<u8be>());
                if (b1 == Chunk::RGB) {
                    auto [red, green, blue] = try$((s.next<Tuple<u8be, u8be, u8be>>()));
                    pixel.red = red;
                    pixel.green = green;
                    pixel.blue = blue;
                } else if (b1 == Chunk::RGBA) {
                    auto [red, green, blue, alpha] = try$((s.next<Tuple<u8be, u8be, u8be, u8be>>()));
                    pixel.red = red;
                    pixel.green = green;
                    pixel.blue = blue;
                    pixel.blue = alpha;
                } else if ((b1 & Chunk::MASK) == Chunk::INDEX) {
                    pixel = index[b1 & 0x3f];
                } else if ((b1 & Chunk::MASK) == Chunk::DIFF) {
                    pixel.red += ((b1 >> 4) & 0x03) - 2;
                    pixel.green += ((b1 >> 2) & 0x03) - 2;
                    pixel.blue += (b1 & 0x03) - 2;
                } else if ((b1 & Chunk::MASK) == Chunk::LUMA) {
                    auto b2 = try$(s.next<u8be>());
                    auto vg = (b1 & 0x3f) - 32;
                    pixel.red += vg - 8 + ((b2 >> 4) & 0x0f);
                    pixel.green += vg;
                    pixel.blue += vg - 8 + (b2 & 0x0f);
                } else if ((b1 & Chunk::MASK) == Chunk::RUN) {
                    run = b1 & (~Chunk::MASK);
                } else {
                    return Error::invalidData("invalid chunk");
                }

                index[hashColor(pixel) % index.len()] = pixel;
                dest.store({x, y}, pixel);
            }
        }

        if (try$(s.nextBytes(8)) != END)
            return Error::invalidData("missing end marker");

        return Ok();
    }
};

} // namespace Karm::Image::Qoi
