#include <rest.hpp>
#include <iostream>

int main() {
    boost::asio::io_context ioc(1);

    rest::server app(ioc);
    rest::router birds;

    // Match on /birds/tweet
    birds.get("/tweet", [](const rest::request&, rest::response& resp) {
        resp.send("tweet-tweet");
    });

    // Match on /birds/search
    birds.get("/search", [](const rest::request&, rest::response& resp) {
        resp.redirect("https://www.google.com/search?q=birds");
    });

    // Allways match on any request
    app.use([](const rest::request& req, rest::response&, rest::next& next) {
        std::cout << "Got a request: "
            << req.method_string() << " " << req.original_url() << "\n";
        next();
    });

    // Look for any uri starting with "/birds"
    app.use("/birds", birds);

    app.listen(8080);
    ioc.run();
}

