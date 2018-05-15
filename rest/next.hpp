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

#ifndef REST_NEXT_HPP
#define REST_NEXT_HPP

#include <rest/error.hpp>

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

#endif // REST_NEXT_HPP

