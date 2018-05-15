#include <rest.hpp>

int main() {
    boost::asio::io_context ioc(1);
    rest::server app(ioc);
    
    app.get("/", [](const rest::request&, rest::response& resp) {
        resp.send("Hello World!");
    });

    // Default handler when no route matched
    app.use([](const rest::request&, rest::response& resp) {
        resp.status(404).send();
    });

    app.listen(8080);
    ioc.run();
}

