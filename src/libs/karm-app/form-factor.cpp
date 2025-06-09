module;

#include <karm-sys/context.h>

export module Karm.App:formFactor;

namespace Karm::App {

export enum struct FormFactor {
    DESKTOP,
    MOBILE,
};

export constexpr FormFactor formFactor = FormFactor::MOBILE;

} // namespace Karm::App
