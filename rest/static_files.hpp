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

#ifndef REST_STATIC_FILES_HPP
#define REST_STATIC_FILES_HPP

#include <rest/mime_type.hpp>

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

#endif // REST_STATIC_FILES_HPP

