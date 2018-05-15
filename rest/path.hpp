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

#ifndef REST_PATH_HPP
#define REST_PATH_HPP

#include <rest/forward.hpp>
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

#endif // REST_PATH_HPP

