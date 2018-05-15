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

#ifndef REST_SERVER_HPP
#define REST_SERVER_HPP

#include <rest/router.hpp>
#include <rest/error.hpp>

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

#endif // REST_SERVER_HPP

