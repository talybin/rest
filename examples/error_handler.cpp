#include <rest.hpp>
#include <iostream>

int main() {
    boost::asio::io_context ioc(1);

    rest::server app(ioc);
    rest::router errors;

    // Match on /errors/unexpected
    errors.get("/unexpected", [](const rest::request&, rest::response&) {
        std::string().at(1);
    });

    // Match on /errors/expected
    errors.get("/expected",
        [](const rest::request&, rest::response&, rest::next& next)
    {
        next("expected exception");
    });

    errors.use([](const rest::error& err,
        const rest::request&, rest::response&, rest::next& next)
    {
        std::cout << "oops, got: " << err.what() << "\n";
        next(); // call parent error handler
    });

    app.use([](const rest::error& err,
        const rest::request&, rest::response& resp, rest::next&)
    {
        resp.status(503).send(err.what());
    });

    app.use("/errors", errors);

    app.listen(8080);
    ioc.run();
}

