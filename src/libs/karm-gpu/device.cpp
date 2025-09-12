module;

#include <karm-core/macros.h>
#include <karm-math/vec.h>

export module Karm.Gpu:device;

import Karm.Core;

namespace Karm::Gpu {

// MARK: Objects ---------------------------------------------------------------

export struct Texture {
    struct Props {
        Math::Vec2i size;
    };

    virtual ~Texture() = default;
};

export enum struct ShaderStage {
    VERTEX,
    FRAGMENT,
    COMPUTE
};

export struct Shader {
    struct Props {
        ShaderStage stage;
    };

    virtual ~Shader() = default;
};

export struct Buffer {
    enum struct Usage {
        VERTEX = 1 << 0,
        INDEX = 1 << 1,
        READ = 1 << 2,
        WRITE = 1 << 3,
        RENDER = 1 << 4,
        COMPUTE = 1 << 5,
    };

    using enum Usage;

    struct Props {
        Flags<Usage> usages;
        usize size;
    };

    struct Region {
        Rc<Buffer> buffer;
        urange range;
    };

    virtual ~Buffer() = default;
};

export struct Transfer {
    enum struct Usage {
        UPLOAD,
        DOWNLOAD
    };

    using enum Usage;

    struct Props {
        Usage usage;
        usize size;
    };

    struct Location {
        Rc<Transfer> transfer;
        usize offset;
    };

    virtual ~Transfer() = default;

    virtual Res<MutBytes> _map() = 0;

    virtual void _unmap() = 0;

    struct Mapped : Meta::Pinned {
        Transfer* transfer{};
        MutBytes data{};

        ~Mapped() {
            unmap();
        }

        void unmap() {
            if (transfer) {
                transfer->_unmap();
                transfer = nullptr;
                data = {};
            }
        }
    };

    Res<Mapped> map() {
        return Ok<Mapped>(this, try$(_map()));
    }
};

export enum struct AddressMode {
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE
};

export struct Sampler {
    enum struct Filter {
        NEAREST,
        LINEAR,
    };

    using enum Filter;

    struct Props {

        Filter minFilter = NEAREST;
        Filter magFilter = NEAREST;

        AddressMode uAddressMode = AddressMode::CLAMP_TO_EDGE;
        AddressMode vAddressMode = AddressMode::CLAMP_TO_EDGE;
        AddressMode wAddressMode = AddressMode::CLAMP_TO_EDGE;
    };

    virtual ~Sampler() = default;
};

export struct Fence {
    virtual ~Fence() = default;
};

// MARK: Pipeline --------------------------------------------------------------

export struct ComputePipeline {
    struct Props {
        Rc<Shader> shader;
    };

    virtual ~ComputePipeline() = default;
};

export struct RenderPipeline {
    struct Props {
        Rc<Shader> vertexShader;
        Rc<Shader> fragmentShader;
    };

    virtual ~RenderPipeline() = default;
};

// MARK: Passes ----------------------------------------------------------------

export struct CopyPass {
    virtual ~CopyPass() = default;

    virtual Res<> upload(Transfer::Location src, Buffer::Region dest) = 0;

    virtual Res<> download(Buffer::Region src, Transfer::Location dest) = 0;
};

export struct ComputePass {
    virtual ~ComputePass() = default;
};

export struct RenderPass {
    virtual ~RenderPass() = default;
};

export struct Batch {
    virtual ~Batch() = default;

    virtual Res<Rc<CopyPass>> beginCopy() = 0;

    virtual Res<Rc<ComputePass>> beginCompute() = 0;

    virtual Res<Rc<RenderPass>> beginRender() = 0;

    virtual Res<Rc<Fence>> submit() = 0;
};

// MARK: Device ----------------------------------------------------------------

export struct Device {
    static Res<Rc<Device>> openDefault();

    virtual Res<Rc<Texture>> createTexture(Texture::Props props) = 0;

    virtual Res<Rc<Shader>> createShader(Shader::Props props) = 0;

    virtual Res<Rc<Buffer>> createBuffer(Buffer::Props props) = 0;

    virtual Res<Rc<Transfer>> createTransfer(Transfer::Props props);

    virtual Res<Rc<Sampler>> createSampler(Sampler::Props props) = 0;

    virtual Res<Rc<ComputePipeline>> createComputePipeline(ComputePipeline::Props props) = 0;

    virtual Res<Rc<RenderPipeline>> createRenderPipeline(RenderPipeline::Props props) = 0;

    virtual Res<Rc<Batch>> createBatch() = 0;
};

} // namespace Karm::Gpu