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

#ifndef REST_RESPONSE_HPP
#define REST_RESPONSE_HPP

#include <rest/request.hpp>

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

#endif // REST_RESPONSE_HPP

