## REST
Rest is a header only C\+\+11 library for building high performance RESTful web services. It is based on [Boost](https://www.boost.org) library to provide cross-platform compability and support for older compilers that does not support higher than C\+\+11.

### Design Goals
Unlike other similar libraries, Rest has it's main goal to have as close interface to [Node.js Express](https://expressjs.com) framework as possible with exception to avoid camel caps style.
Also staying in C++11 makes it easier to integrate into corporate code where not always latest compilers can be used.

### Requirements
Rest using Beast library (among others) from Boost. Beast was introduced in version 1.66 which makes it minimum required version of Boost.

### Getting Started
This example starts a server and listens on any interface and port 8080 for connections. The app responds with “Hello World!” for requests to the root URL (/). For every other path, it will respond with a **404 Not Found**.
```cpp
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
```
If your compiler support C++14 or later it is simpler to use `auto` keyword
```cpp
    app.get("/", [](auto&, auto& resp) {
        resp.send("Hello World!");
    });
```

### Routing
Since Rest trying to mimic Express library it is suggested to read about routing [here](https://expressjs.com/en/guide/routing.html). Here some examples using C++ syntax.
```cpp
// POST method route
app.post("/", [](auto&, auto& resp) {
    resp.send("POST request to the homepage");
});
```
```cpp
// All used to match any supported HTTP request method
app.all("/secret", [](auto&, auto&, auto& next) {
    std::cout << "Accessing the secret section ...\n";
    next(); // pass control to the next handler
});
```
```cpp
// This route path will match /abe and /abcde
app.get("/ab(cd)?e", [](auto&, auto& resp) {
    resp.send("ab(cd)?e");
});
```
```cpp
// Route parameters
app.get("/users/:userId/books/:bookId", [](auto& req, auto&) {
    // Parameters stored in std::map
    for (auto& p : req.params())
        std::cout << p.first << ": " << p.second << "\n";
});
```

### Router
Use `rest::router` to create modular, mountable route handlers. A router instance is a complete middleware and routing system.
The following example creates a router as a module, loads a middleware function in it, defines some routes, and mounts the router module on a path in the main app.
```cpp
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
```

### Parameter handler
Parameter handler called whenever router matches named parameter in it's uri. A param callback will be called only once in a request-response cycle, even if the parameter is matched in multiple routes, as shown in the following example.
```cpp
router.param("id", [](
    const rest::request&, rest::response&, rest::next& next,
    const boost::string_view& value)
{
    std::cout << "CALLED ONLY ONCE\n";
    next();
});

router.get("/user/:id", [](
    const rest::request&, rest::response&, rest::next& next)
{
    std::cout << "although this matches\n";
    next();
});

router.get("/user/:id", [](const rest::request&, rest::response& resp) {
    std::cout << "and this matches too\n";
    resp.send(); // send 200 OK
});
```

### Static files
This middleware serves static files. Following example will serve files from local **./html** directory matching **/files** in request uri (ex. the request **/files/index.html** will be respond with local file **./html/index.html**).
```cpp
app.use("/files", rest::static_files("html"));
```
It is possible to customize `rest::static_files` by providing options.
```cpp
rest::static_files_options options;
options.set_headers = [](
    rest::response& r, const rest::fs::path& f, const rest::fs::file_status&)
{
    // Override default mime type resolving
    r.set(rest::http::field::content_type, "text/plain");
};
app.use(rest::static_files("html", std::move(options)));
```
Available options
| Option | Type | Description |
| ------ | ---- | ----------- |
| index | boost::string_view | Sends the specified directory index file. Set to empty to disable directory indexing. Default is `index.html`. |
| set_headers | std::function | Function for setting HTTP headers to serve with the file. By default sets Content-Type header with mime type resolved by file extension. |

### Mime types
Mime type (or Content type) used to identify type of HTTP payload (ex. embedded file). Rest library has a predefined list of most common types. This list is fully modifiable. 
```cpp
// Extend example
rest::mime_type::table().emplace(".mp4", "video/mp4");

// Modify example
rest::mime_type::table()[".js"] = "text/javascript";
```
Resolving mime types by filename extension is used by `static_files` middleware. In case type is not in list, the default type, **application/octet-stream**, is used. This can be changed by
```cpp
// Default type to be returned on no match
rest::mime_type::default_type() = "text/plain";
```
### Error handling
Each router may have one or more error handler that will be executed (in order provided) in case of any exception or by manually calling `rest::next` with an error. If current router (where exception occurred) does not have an error handler it will be routed to parent router.
```cpp
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
```

### About Json support
Right now there is no support for Json in Rest library. Either Boost can be explored for Json support or to use third party library, such as [Nlohmann Json](https://github.com/nlohmann/json). The later can be used like this.
```cpp
#include <rest.hpp>
#include <json.hpp>

using json = nlohmann::json;

int main() {
    boost::asio::io_context ioc(1);
    rest::server app(ioc);

    app.get("/", [](const rest::request&, rest::response& resp) {
        resp.set(rest::http::field::content_type, "application/json");
        resp.send(json { { "pi", 3.141 }, { "library", "rest" } });
    });

    app.listen(8080);
    ioc.run();
}
```

### TODO
* Add missing methods and functionality of Express library
* Add support for web sockets
* Add support for SSL/TSL
* Orginize and complete documentation
* Add unit tests

### Contributing
The code is not perfect and there a lot of TODOs. Any pull requests are welcome.

