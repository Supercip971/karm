#pragma once

#include "_embed.h"
#include "time.h"
#include "fd.h"

namespace Karm::Sys {

static inline Res<> sleep(Duration span) {
    return _Embed::sleep(span);
}

static inline Res<> sleepUntil(Instant until) {
    return _Embed::sleepUntil(until);
}

static inline Res<Mime::Url> pwd() {
    return _Embed::pwd();
}

// MARK: Sandboxing ------------------------------------------------------------

Res<> enterSandbox();

Res<> ensureUnrestricted();

// MARK: Process ---------------------------------------------------------------

enum struct ProcessState : u8 {
    STOPPED,
    RUNNING,
    EXITED,
    TERMINATED,
};

enum struct ProcessFlags : u8 {
    TRACE = 1 << 0, // Allow tracing of the process
};

struct Pid {
    virtual ~Pid() = default;
};

struct Control {
    Rc<Pid> _pid;

    Res<> resume();

    Res<> suspend();

    Res<> terminate();
};

struct Child {};
static inline constexpr Child CHILD{};

struct Process {
    Rc<Pid> _pid;

    static Res<Process> attach(Sys::Handle id);

    Res<Control> control();

    ProcessState state() const;

    Sys::Handle handle() const;

    Res<usize> wait();

    Async::Task<usize> waitAsync(Sched& sched = globalSched());

};


struct Launchpad {
    Flags<ProcessFlags> flags{};
    Vec<String> args{};
    Map<String, String> env{};
    Opt<Mime::Url> cwd = NONE;
    Opt<Rc<Fd>> stdin = NONE;
    Opt<Rc<Fd>> stdout = NONE;
    Opt<Rc<Fd>> stderr = NONE;


    Res<Process> launch(Mime::Url path);

    Res<Union<Process, Child>> fork();
};



} // namespace Karm::Sys
