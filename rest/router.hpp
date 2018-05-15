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

#ifndef REST_ROUTER_HPP
#define REST_ROUTER_HPP

#include <rest/request.hpp>
#include <rest/response.hpp>
#include <rest/path.hpp>
#include <rest/next.hpp>

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

#endif // REST_ROUTER_HPP

