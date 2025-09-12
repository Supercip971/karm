#include <karm-math/vec.h>
#include <karm-sys/entry.h>

import Karm.Gpu;
import Karm.App;

using namespace Karm;

struct Vertex {
    Math::Vec3f pos;
    Math::Vec3f col;
};

Async::Task<> entryPointAsync(Sys::Context&) {
    auto device = co_try$(Gpu::Device::openDefault());

    auto vs = co_try$(device->createShader({.stage = Gpu::ShaderStage::VERTEX}));
    auto fs = co_try$(device->createShader({.stage = Gpu::ShaderStage::FRAGMENT}));

    auto pipeline = co_try$(device->createRenderPipeline({
        .vertexShader = vs,
        .fragmentShader = fs,
    }));

    auto transfer = co_try$(device->createTransfer({
        .usage = Gpu::Transfer::UPLOAD,
        .size = sizeof(Vertex) * 3,
    }));

    auto map = co_try$(transfer->map());
    Array verts{
        Vertex{},
        Vertex{},
        Vertex{},
    };
    copy(bytes(verts), map.data);
    map.unmap();

    auto batch = co_try$(device->createBatch());

    auto copyPass = co_try$(batch->beginCopy());

    co_return Ok();
}
