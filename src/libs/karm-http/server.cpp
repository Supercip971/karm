module;

#include <karm-async/task.h>
#include <karm-base/rc.h>

#include "karm-sys/socket.h"

export module Karm.Http:server;

import :request;
import :response;

namespace Karm::Http {

export struct Service {
    virtual ~Service() = default;
    virtual Async::Task<> handleAsync(Rc<Request>, Rc<Response::Writer>) = 0;
};

struct ServerConnection {
    Sys::TcpConnection _connection;
};

export struct Server {
    static Rc<Server> simple(Rc<Service>) {
        notImplemented();
    }

    Rc<Service> _srv;
    virtual ~Server() = default;
    virtual Async::Task<> serveAsync() = 0;
};

// MARK: Serverless ------------------------------------------------------------

export Async::Task<> serveAsync(Rc<Service> srv) {
    auto server = Server::simple(srv);
    co_return co_await server->serveAsync();
}

} // namespace Karm::Http
