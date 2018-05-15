// MIT License
// 
// Copyright (c) 2018 Vladimir Talybin
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// This file was generated with a script.
// Generated 2018-05-15 18:02:44.541123 UTC

#ifndef REST_SINGLE_INCLUDE_HPP
#define REST_SINGLE_INCLUDE_HPP

// beginning of rest.hpp

// beginning of rest/forward.hpp

#define REST_VERSION_MAJOR 1
#define REST_VERSION_MINOR 0
#define REST_VERSION_PATCH 0

#include <boost/beast.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/filesystem.hpp>

#include <map>

namespace rest {

    namespace fs = boost::filesystem;

    namespace http = boost::beast::http;
    using tcp = boost::asio::ip::tcp;

    using name_value_map = std::map<const char*, boost::string_view>;

} // namespace rest

// end of rest/forward.hpp

// beginning of rest/error.hpp

#include <boost/system/error_code.hpp>

namespace rest {

    struct error {
    private:
        std::string err;

    public:
        error() noexcept = default;

        error(const char* s) noexcept
        : err(s)
        { }

        error(std::string s) noexcept
        : err(std::move(s))
        { }

        error(const std::exception& ex) noexcept
        : err(ex.what())
        { }

        error(const boost::system::error_code& ec) noexcept
        : err(ec.message())
        { }

        operator bool() const noexcept {
            return !err.empty();
        }

        const std::string& what() const noexcept {
            return err;
        }

    };

} // namespace rest

// end of rest/error.hpp

// beginning of rest/mime_type.hpp

namespace rest {

    struct mime_type {
        using table_type = std::map<boost::string_view, boost::string_view>;

        // Filename extension to mime type map.
        // Extend example:
        //   rest::mime_type::table().emplace(".mp4", "video/mp4");
        // Modify example:
        //   rest::mime_type::table()[".js"] = "text/javascript";
        static table_type& table() noexcept {
            static table_type t {
                { ".bmp",   "image/bmp" },
                { ".css",   "text/css" },
                { ".flv",   "video/x-flv" },
                { ".gif",   "image/gif" },
                { ".htm",   "text/html" },
                { ".html",  "text/html" },
                { ".ico",   "image/vnd.microsoft.icon" },
                { ".jpe",   "image/jpeg" },
                { ".jpeg",  "image/jpeg" },
                { ".jpg",   "image/jpeg" },
                { ".js",    "application/javascript" },
                { ".json",  "application/json" },
                { ".php",   "text/html" },
                { ".png",   "image/png" },
                { ".svg",   "image/svg+xml" },
                { ".svgz",  "image/svg+xml" },
                { ".swf",   "application/x-shockwave-flash" },
                { ".tif",   "image/tiff" },
                { ".tiff",  "image/tiff" },
                { ".txt",   "text/plain" },
                { ".xml",   "application/xml" },
            };
            return t;
        }

        // Default type to be returned on no match. Change it by
        // assigning, example:
        //   rest::mime_type::default_type() = "text/plain";
        static boost::string_view& default_type() noexcept {
            static boost::string_view t { "application/octet-stream" };
            return t;
        }

        // Return a reasonable mime type based on the extension of a file
        static const boost::string_view& resolve(const fs::path& filename) noexcept
        {
            auto ext = filename.extension();
            if (!ext.empty()) {
                const table_type& t = table();
                auto it = t.find(ext.native());
                if (it != t.end())
                    return it->second;
            }
            return default_type();
        }

        // Pre-initialize static structures (optional)
        mime_type() noexcept {
            table();
            default_type();
        }
    };

} // namespace rest

// end of rest/mime_type.hpp

// beginning of rest/next.hpp

namespace rest {

    struct next {
    private:
        struct impl {
            bool proceed = false;
            error err;
        };

        std::shared_ptr<impl> self;

    public:
        next()
        : self(std::make_shared<impl>())
        { }

        void operator()() noexcept {
            self->proceed = true;
        }
        void operator()(error&& err) noexcept {
            self->proceed = true;
            self->err = std::move(err);
        }

        template <class F>
        bool safe_invoke(F&& f) noexcept {
            self->proceed = false;

            try {
                std::forward<F>(f)();
            }
            catch (const std::exception& ex) { operator()(ex); }
            catch (const boost::system::error_code& ec) { operator()(ec); }
            catch (...) { operator()("unknown error"); }

            return self->proceed;
        }

        const error& err() const noexcept {
            return self->err;
        }
    };

} // namespace rest

// end of rest/next.hpp

// beginning of rest/path.hpp

#include <regex>

namespace rest {

    struct path {
    private:
        std::string regex;
        std::vector<std::pair<size_t, std::string>> params;

    public:
        path(const boost::string_view& uri) noexcept
        {
            size_t distance = 0;
            std::string name;

            for (auto it = uri.begin(); it != uri.end();) {
                regex.push_back(*it);
                if (*it++ != '/')
                    continue;

                ++distance;
                if (it != uri.end() && *it == ':') {
                    // Replace parameter with \w+
                    regex.append("\\w+", 3);
                    while (++it != uri.end() && *it != '/')
                        name.push_back(*it);
                    // Store parameter
                    params.emplace_back(distance, std::move(name));
                    distance = 0;
                }
            }
        }

        bool match(const boost::string_view& uri, std::cmatch& cm) const
        {
            std::regex rx(regex);
            if (!std::regex_search(uri.begin(), uri.end(), cm, rx))
                return false;
            // Match must occur at the beginning
            return (cm.position(0) == 0);
        }

        name_value_map get_params(const boost::string_view& uri) const noexcept
        {
            name_value_map pm;
            size_t start = 1;

            for (auto& p : params) {
                // Find value by counting '/' in current uri
                for (auto dist = p.first; dist > 1; --dist)
                    start = uri.find('/', start) + 1;
                size_t stop = start;
                for (; stop < uri.size() && uri[stop] != '/'; ++stop);

                pm.emplace(p.second.c_str(),
                    boost::string_view(&uri[start], stop - start));
                start = stop + 1;
            }
            return pm;
        }
    };

} // namespace rest

// end of rest/path.hpp

// beginning of rest/router.hpp

// beginning of rest/request.hpp

namespace rest {

    struct server;

    struct request {
    private:
        using socket_ptr = std::shared_ptr<tcp::socket>;

        struct impl {
            impl(const socket_ptr& s) noexcept
            : socket(s)
            { }

            socket_ptr socket;
            http::request<http::dynamic_body> req;
            boost::beast::flat_buffer buffer { 8192 };

            const server* app = nullptr;
            name_value_map params;
            boost::string_view base_url;
            boost::string_view path;
        };

        std::shared_ptr<impl> self;

    public:
        request(const socket_ptr& s)
        : self(std::make_shared<impl>(s))
        { }

        // Asynchronous receive of request
        template <class C>
        static void receive(const socket_ptr& sock, C&& cb) {
            request req(sock);
            http::async_read(*sock, req.self->buffer, req.self->req,
                [req, cb](boost::beast::error_code err, size_t) mutable
            {
                if (!err) cb(req);
            });
        }

        const socket_ptr& socket() const noexcept {
            return self->socket;
        }

        // Root route (server generated this request)
        const server& app() const noexcept {
            return *self->app;
        }
        void app(const server* val) noexcept {
            self->app = val;
        }

        // The url path on which a router instance was mounted
        const boost::string_view& base_url() const noexcept {
            return self->base_url;
        }
        void base_url(const boost::string_view& val) noexcept {
            self->base_url = val;
        }

        // The remaining path part of the router mount point
        const boost::string_view& path() const noexcept {
            return self->path;
        }
        void path(const boost::string_view& val) noexcept {
            self->path = val;
        }

        // Named route parameters
        //name_value_map& params() const noexcept {
        name_value_map& params() const noexcept {
            return self->params;
        }

        // Contains a string corresponding to the HTTP method
        // of the request: GET, POST, PUT, and so on
        boost::string_view method_string() const noexcept {
            return self->req.method_string();
        }

        // HTTP method of the request as a boost::beast::http::verb enum
        http::verb method() const noexcept {
            return self->req.method();
        }

        // The original request url
        boost::string_view original_url() const noexcept {
            return self->req.target();
        }

        // Get header by name. T is a case-insensitive matching field
        // name or a rest::http::field value.
        template <class T>
        boost::optional<boost::string_view> get(T name) const noexcept {
            auto it = self->req.find(name);
            if (it != self->req.end())
                return it->value();
            return { };
        }
    };

} // namespace rest

// end of rest/request.hpp

// beginning of rest/response.hpp

namespace rest {

    struct response {
    private:
        struct impl {
            impl(const request& r) noexcept
            : req(r)
            { }

            request req;
            http::response<http::dynamic_body> resp;
            bool is_sent = false;
        };

        std::shared_ptr<impl> self;

    public:
        response(const request& req)
        : self(std::make_shared<impl>(req))
        { }

        // Set a header field value, removing any other instances of that field.
        // T is a case-insensitive matching field name or a rest::http::field value.
        template <class T>
        void set(T name, const boost::beast::string_param& value) {
            self->resp.set(name, value);
        }

        // Sets the HTTP status for the response
        response& status(unsigned code) {
            self->resp.result(code);
            return *this;
        }

        // Gets the HTTP status for the response
        unsigned status() const noexcept {
            return self->resp.result_int();
        }

        // Send file content
        void send_file(const fs::path& fname) {
            std::ifstream f(fname.native(), std::ios::binary);
            if (f.is_open()) {
                send(f.rdbuf());
            }
        }

        // Sends the HTTP response with a body
        template <class T>
        void send(const T& body) {
            boost::beast::ostream(self->resp.body()) << body;
            set(http::field::content_length, self->resp.body().size());
            send();
        }

        // Sends the HTTP response. Correspond to end() method in nodejs express.
        void send() {
            self->resp.keep_alive(false);

            auto scopy = self;
            http::async_write(*self->req.socket(), self->resp,
                [scopy](boost::beast::error_code err, size_t)
            {
                scopy->req.socket()->shutdown(tcp::socket::shutdown_send, err);
            });

            self->is_sent = true;
        }

        // Return true if response queing to be sent or is already sent
        bool is_sent() const noexcept {
            return self->is_sent;
        }

        // Sets the response Location HTTP header to the specified path
        // parameter. Currently supports absolute path only.
        void location(const boost::string_view& path) {
            set(http::field::location, path);
        }

        // Redirects to the URL derived from the specified path. If not
        // specified, status defaults to "302 Found".
        void redirect(unsigned code, const boost::string_view& path) {
            location(path);
            status(code).send();
        }
        void redirect(const boost::string_view& path) {
            redirect(302, path);
        }
    };

} // namespace rest

// end of rest/response.hpp

#include <boost/variant.hpp>

namespace rest {

    template <class... Args>
    using callback = std::function<void(Args...)>;

    struct router;
    struct route;

    using req_resp = callback<const request&, response&>;
    using req_resp_next = callback<const request&, response&, next&>;

    using param_handler = callback<const request&, response&, next&, const boost::string_view&>;
    using error_handler = callback<const error&, const request&, response&, next&>;
    using route_handler = boost::variant<req_resp, req_resp_next, router>;

    enum class route_type {
        use,
        method,
        all
    };

    struct router {
    private:
        struct visitor : boost::static_visitor<> {
        private:
            request& req;
            response& resp;
            next& n;

        public:
            visitor(request& req, response& resp, next& n) noexcept
            : req(req), resp(resp), n(n)
            { }

            void operator()(req_resp& cb) const {
                cb(req, resp);
            }
            void operator()(req_resp_next& cb) const {
                cb(req, resp, n);
            }
            void operator()(router& r) const {
                r.handle_request(req, resp, n);
            }
        };

        struct impl {
            std::vector<route> routes;
            std::vector<error_handler> errors;
            std::map<std::string, param_handler> params;
        };

        std::shared_ptr<impl> self;

        void handle_error(request&, response&, next&);

    protected:
        void handle_request(request&, response&, next&);

    public:
        router() noexcept
        : self(std::make_shared<impl>())
        { }

        virtual ~router() { }

        // Error handler called on next(error) or in case of exception
        void use(error_handler&& handler) noexcept {
            self->errors.emplace_back(std::forward<error_handler>(handler));
        }

        // Matches all HTTP methods and any path
        void use(route_handler&& handler) noexcept {
            self->routes.emplace_back(
                route_type::use, std::forward<route_handler>(handler));
        }

        // Matches all HTTP methods on specified mount (leading) path
        void use(boost::string_view path, route_handler&& handler) noexcept {
            self->routes.emplace_back(
                route_type::use,
                std::forward<route_handler>(handler),
                path);
        }

        // Matches all HTTP methods (verbs). Same as 'use' but matches path exactly.
        void all(boost::string_view path, route_handler&& handler) noexcept {
            self->routes.emplace_back(
                route_type::all,
                std::forward<route_handler>(handler),
                path);
        }

        // Matches HTTP GET
        void get(boost::string_view path, route_handler&& handler) noexcept {
            self->routes.emplace_back(
                route_type::method,
                std::forward<route_handler>(handler),
                path,
                http::verb::get);
        }

        // Matches HTTP PUT
        void put(boost::string_view path, route_handler&& handler) noexcept {
            self->routes.emplace_back(
                route_type::method,
                std::forward<route_handler>(handler),
                path,
                http::verb::put);
        }

        // Matches HTTP POST
        void post(boost::string_view path, route_handler&& handler) noexcept {
            self->routes.emplace_back(
                route_type::method,
                std::forward<route_handler>(handler),
                path,
                http::verb::post);
        }

        // Matches HTTP DELETE
        void del(boost::string_view path, route_handler&& handler) noexcept {
            self->routes.emplace_back(
                route_type::method,
                std::forward<route_handler>(handler),
                path,
                http::verb::delete_);
        }

        // Parameteter handler called whenever triggered path contains
        // matched named parameter
        void param(std::string name, param_handler&& handler) noexcept {
            self->params.emplace(
                std::move(name), std::forward<param_handler>(handler));
        }
    };

    struct route
    {
        route_type type;
        http::verb method; // used only for route_type::method
        path uri;
        route_handler handler;

        route(route_type rt, route_handler&& rh,
            boost::string_view u = "/", http::verb mtd = http::verb::unknown) noexcept
        : type(rt)
        , method(mtd)
        , uri(u)
        , handler(std::forward<route_handler>(rh))
        { }

        boost::string_view match(const request& req) const {
            // Check method if handler type is method
            if (type == route_type::method && method != req.method())
                return { };

            std::cmatch cm;
            if (!uri.match(req.path(), cm))
                return { };

            size_t msize = cm.length(0);
            if (type != route_type::use && msize != req.path().size()) {
                // For non-use routes uri must match exactly
                return { };
            }

            return req.path().substr(0, msize);
        }
    };

    void router::handle_error(request& req, response& resp, next& next)
    {
        for (auto& handler : self->errors) {
            if (!next.safe_invoke([&] { handler(next.err(), req, resp, next); }))
                break;
        }
    }

    void router::handle_request(request& req, response& resp, next& next)
    {
        for (auto& route : self->routes) {
            boost::string_view uri_match = route.match(req);
            if (uri_match.empty())
                continue;

            // Update request fields
            req.base_url(uri_match);
            // Make sure leading '/' is present
            req.path(req.path().substr(
                uri_match.size() == 1 ? 0 : uri_match.size()));
            if (req.path().empty())
                req.path({ "/", 1 });

            // Add params to request
            for (auto& p : route.uri.get_params(uri_match)) {
                if (req.params().emplace(p).second) {
                    // The insertion took place, use this fact to call param
                    // callback once per request-response cycle
                    auto it = self->params.find(p.first);
                    if (it != self->params.end()) {
                        if (!next.safe_invoke([&] {
                            it->second(req, resp, next, p.second); }))
                            return;
                        if (next.err()) {
                            handle_error(req, resp, next);
                            return;
                        }
                    }
                }
            }

            if (!next.safe_invoke([&] {
                boost::apply_visitor(visitor(req, resp, next), route.handler); }))
                return;
            if (next.err()) {
                handle_error(req, resp, next);
                return;
            }
        }
    }

} // namespace rest

// end of rest/router.hpp

// beginning of rest/server.hpp

namespace rest {

    struct server : router {
    private:
        tcp::acceptor acceptor;
        tcp::socket socket;

        // Loop forever accepting new connections
        void accept() {
            acceptor.async_accept(socket,
                [this](boost::beast::error_code ec) {
                    if (!ec)
                        process_client();
                    accept();
                });
        }

        // Receive and process a request
        void process_client()
        {
            auto sock = std::make_shared<tcp::socket>(std::move(socket));
            request::receive(sock, [this](request& req) {
                req.app(this);
                req.path(req.original_url());

                response resp(req);
                next n;

                handle_request(req, resp, n);
            });
        }

    public:
        server(boost::asio::io_context& ioc) noexcept
        : acceptor(ioc)
        , socket(ioc)
        { }

        void listen(tcp::endpoint endp) {
            acceptor.open(endp.protocol());
            acceptor.set_option(tcp::acceptor::reuse_address(true));
            acceptor.bind(endp);
            acceptor.listen();
            accept();
        }

        void listen(uint16_t port) {
            listen(tcp::endpoint(boost::asio::ip::address(), port));
        }

        void listen(uint16_t port, const char* ip) {
            listen(tcp::endpoint(boost::asio::ip::make_address(ip), port));
        }
    };

} // namespace rest

// end of rest/server.hpp

// beginning of rest/static_files.hpp

namespace rest {

    struct static_files_options {
        using set_headers_type = void (response&, const fs::path&, const fs::file_status&);

        // Sends the specified directory index file. Set to empty to
        // disable directory indexing.
        boost::string_view index { "index.html" };

        // Function for setting HTTP headers to serve with the file
        std::function<set_headers_type> set_headers {
            [](response& r, const fs::path& f, const fs::file_status&) {
                r.set(http::field::content_type, mime_type::resolve(f));
            }};
    };

    struct static_files {
    private:
        const fs::path fpath;
        const static_files_options opt;

    public:
        static_files(fs::path filepath, static_files_options options = { }) noexcept
        : fpath(std::move(filepath))
        , opt(std::move(options))
        { }

        void operator()(const request& req, response& resp, next& parent_next) const
        {
            if (req.method() == http::verb::get) {
                const auto& rp = req.path();
                auto fname = fpath / fs::path(rp.begin(), rp.end());
                auto stat = status(fname);

                if (fs::is_directory(stat)) {
                    fname.append(opt.index.begin(), opt.index.end());
                    stat = status(fname);
                }

                if (fs::exists(stat)) {
                    opt.set_headers(resp, fname, stat);
                    resp.send_file(fname);
                    return;
                }
            }
            // Call next if method not matched or file not found
            parent_next();
        }
    };

} // namespace rest

// end of rest/static_files.hpp

// end of rest.hpp

#endif // REST_SINGLE_INCLUDE_HPP

