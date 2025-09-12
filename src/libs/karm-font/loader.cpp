module;

#include <karm-gfx/font.h>
#include <karm-sys/file.h>
#include <karm-sys/mmap.h>

#include "ttf/fontface.h"

export module Karm.Font:loader;

import Karm.Core;
import :sfnt;
import :woff;

namespace Karm::Font {

export Res<Rc<Ttf::Container>> _loadContainer(Sys::Mmap&& map) {
    if (Sfnt::sniff(map.bytes())) {
        return Sfnt::Container::load(std::move(map));
    } else if (Woff1::sniff(map.bytes())) {
        return Woff1::Container::load(std::move(map));
    } else if (Woff2::sniff(map.bytes())) {
        return Woff2::Container::load(std::move(map));
    } else {
        return Error::invalidData("unknown truetype container");
    }
}

export Res<Rc<Gfx::Fontface>> loadFontface(Sys::Mmap&& map) {
    auto container = try$(_loadContainer(std::move(map)));
    return Ok(try$(Ttf::Fontface::load(container)));
}

export Res<Rc<Gfx::Fontface>> loadFontface(Ref::Url url) {
    auto file = try$(Sys::File::open(url));
    if (url.scheme == "data") {
        auto blob = try$(url.blob);
        auto map = try$(Sys::mmap().size(blob->len()).mapMut());
        copy(bytes(blob->data), map.mutBytes());
        return loadFontface(std::move(map));
    }
    auto map = try$(Sys::mmap().map(file));
    return loadFontface(std::move(map));
}

export Res<Rc<Gfx::Fontface>> loadFontfaceOrFallback(Ref::Url url) {
    if (auto result = loadFontface(url); result) {
        return result;
    }
    return Ok(Gfx::Fontface::fallback());
}

export Res<Gfx::Font> loadFont(f64 size, Ref::Url url) {
    return Ok(Gfx::Font{
        .fontface = try$(loadFontface(url)),
        .fontsize = size,
    });
}

export Res<Gfx::Font> loadFontOrFallback(f64 size, Ref::Url url) {
    return Ok(Gfx::Font{
        .fontface = try$(loadFontfaceOrFallback(url)),
        .fontsize = size,
    });
}

} // namespace Karm::Font
