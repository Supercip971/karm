module;

#include <karm-gfx/buffer.h>
#include <karm-logger/logger.h>

export module Karm.Image:tga.decoder;

import Karm.Core;
import :tga.base;

// TGA image decoder
// References:
//  - http://www.paulbourke.net/dataformats/tga/
//  - https://en.wikipedia.org/wiki/Truevision_TGA

namespace Karm::Image::Tga {

export struct Decoder {
    Header _header;
    Bytes _data;

    static bool sniff(Bytes slice) {
        if (slice.len() < sizeof(Header))
            return false;

        // NOTE: Sadly TGA don't have a magic number, so we have to read the header
        //       and check it looks like a valid TGA header.
        Io::BScan s{slice};

        // We check that the buffer is already big enough, this should be file
        Header header = s.next<Header>().unwrap();

        if (header.width == 0 or header.height == 0)
            return false;

        if (header.bpp != 8 and
            header.bpp != 15 and
            header.bpp != 16 and
            header.bpp != 24 &&
            header.bpp != 32)
            return false;

        if (header.imageType != UNC_CM and
            header.imageType != UNC_TC and
            header.imageType != UNC_GS and
            header.imageType != RLE_CM and
            header.imageType != RLE_TC and
            header.imageType != RLE_GS)
            return false;

        return true;
    }

    static Res<Decoder> init(Bytes slice) {
        Io::BScan s{slice};
        Header header = try$(s.next<Header>());
        return Ok(Decoder{header, slice});
    }

    Decoder(Header header, Bytes data)
        : _header{header}, _data{data} {}

    isize width() const { return _header.width; }

    isize height() const { return _header.height; }

    static Res<Gfx::Color> readColor(Io::BScan& s, usize bpp) {
        if (bpp == 8) {
            u8 d = try$(s.next<u8le>());
            return Ok(Gfx::Color{d, d, d, 255});
        } else if (bpp == 15 or bpp == 16) {
            auto [l, h] = try$((s.next<Tuple<u8le, u8le>>()));

            return Ok(Gfx::Color{
                static_cast<u8>((h & 0x3e) << 2),
                static_cast<u8>((l & 0xf8)),
                static_cast<u8>((l << 5) | ((h & 0xc0) >> 2)),
                255,
            });
        } else if (bpp == 24) {
            auto [b, g, r] = try$((s.next<Tuple<u8le, u8le, u8le>>()));
            return Ok(Gfx::Color{r, g, b, 255});
        } else if (bpp == 32) {
            auto [b, g, r, a] = try$((s.next<Tuple<u8le, u8le, u8le, u8le>>()));
            return Ok(Gfx::Color{r, g, b, a});
        } else {
            return Ok(Gfx::Color{255, 0, 255, 255});
        }
    }

    bool _hasColorMap = false;
    Vec<Gfx::Color> _colorMap = {};

    usize colorMapSize() const {
        return _header.cmSize * (alignUp(_header.cmBpc, 8) / 8);
    }

    Res<> readColorMap(Io::BScan& s) {
        if (not _header.clrmapType)
            return Ok();
        _hasColorMap = true;
        for (usize i = 0; i < _header.cmSize; ++i)
            _colorMap.pushBack(try$(readColor(s, _header.cmBpc)));
        return Ok();
    }

    Res<Gfx::Color> decodePixel(Io::BScan& s) {
        if (_hasColorMap) {
            auto index = try$(s.next<u8le>());
            if (index >= _colorMap.len())
                return Ok(Gfx::Color{255, 0, 255, 255});
            return Ok(_colorMap[index]);
        }

        return readColor(s, _header.bpp);
    }

    void storePixel(Gfx::MutPixels pixels, Math::Vec2i pos, Gfx::Color color) {
        if (_header.desc & ORDER_R2L)
            pos.x = width() - pos.x - 1;

        if (not(_header.desc & ORDER_T2B))
            pos.y = height() - pos.y - 1;

        pixels.store(pos, color);
    }

    void storePixel(Gfx::MutPixels pixels, isize index, Gfx::Color color) {
        storePixel(pixels, {index % width(), index / width()}, color);
    }

    Res<> decodeUncompress(Io::BScan& s, Gfx::MutPixels pixels) {
        for (isize y = 0; y < height(); ++y)
            for (isize x = 0; x < width(); ++x)
                storePixel(pixels, {x, y}, try$(decodePixel(s)));

        return Ok();
    }

    Res<> decodeRle(Io::BScan& s, Gfx::MutPixels pixels) {
        usize index = 0;
        usize end = _header.width * _header.height;

        while (index < end) {
            u8 packet = try$(s.next<u8le>());
            usize len = (packet & PACKET_LEN) + 1;

            if (packet & PACKET_RLE) {
                auto color = try$(decodePixel(s));
                for (usize i = 0; i < len; ++i)
                    storePixel(pixels, index++, color);
            } else {
                for (usize i = 0; i < len; ++i)
                    storePixel(pixels, index++, try$(decodePixel(s)));
            }
        }

        return Ok();
    }

    Res<> decode(Gfx::MutPixels pixels) {
        Io::BScan s{_data};
        s.skip(sizeof(Header));
        s.skip(_header.idLen);

        try$(readColorMap(s));

        if (_header.imageType == UNC_CM or
            _header.imageType == UNC_TC or
            _header.imageType == UNC_GS) {
            try$(decodeUncompress(s, pixels));
        } else if (_header.imageType == RLE_CM or
                   _header.imageType == RLE_TC or
                   _header.imageType == RLE_GS) {
            try$(decodeRle(s, pixels));
        } else {
            return Error::invalidData("unsupported TGA image type");
        }

        return Ok();
    }

    void repr(Io::Emit& e) const {
        e("TGA Image");
        e.indentNewline();

        e.ln("width: {}", width());
        e.ln("height: {}", height());

        e.deindent();
    }
};

} // namespace Karm::Image::Tga
