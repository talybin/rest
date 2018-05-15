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

#ifndef REST_MIME_TYPE_HPP
#define REST_MIME_TYPE_HPP

#include <rest/forward.hpp>

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

#endif // REST_MIME_TYPE_HPP

