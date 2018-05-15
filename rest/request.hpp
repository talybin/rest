// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// Copyright (c) 2018 Vladimir Talybin.

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef REST_REQUEST_HPP
#define REST_REQUEST_HPP

#include <rest/forward.hpp>

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

#endif // REST_REQUEST_HPP

