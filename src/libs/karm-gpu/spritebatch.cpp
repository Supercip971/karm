module;

#include <karm-math/rect.h>

export module Karm.Gpu:spritebatch;

import Karm.Core;
import Karm.Cg;
import :device;

namespace Karm::Gpu {

export struct Sprite {
    Rc<Texture> _texture;
    Math::Recti _source;
};

export struct Spritebatch {
    void begin();

    void draw(Sprite const& spr, Math::Recti dest, Cg::Color color);

    void end();
};

} // namespace Karm::Gpu
