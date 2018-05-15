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

#ifndef REST_ERROR_HPP
#define REST_ERROR_HPP

#include <boost/system/error_code.hpp>
#include <rest/forward.hpp>

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

#endif // REST_ERROR_HPP

