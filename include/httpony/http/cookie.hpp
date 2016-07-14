/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HTTPONT_COOKIE_HPP
#define HTTPONT_COOKIE_HPP

#include <melanolib/time/date_time.hpp>
#include <melanolib/time/time_string.hpp>
#include <melanolib/utils/c++-compat.hpp>

#include "httpony/uri.hpp"

namespace httpony {

struct Cookie
{
    Cookie(
        std::string value = {},
        melanolib::Optional<melanolib::time::DateTime> expires = {},
        melanolib::Optional<melanolib::time::seconds> max_age = {},
        std::string domain = {},
        std::string path = {},
        bool secure = false,
        bool http_only = false,
        std::vector<std::string> extension = {}

    )
        : value (std::move(value)),
          expires(std::move(expires)),
          max_age(std::move(max_age)),
          domain(std::move(domain)),
          path(std::move(path)),
          secure(secure),
          http_only(http_only),
          extension(std::move(extension))
    {}

    Cookie& operator=(const std::string& value)
    {
        this->value = value;
        return *this;
    }

    std::string value;
    melanolib::Optional<melanolib::time::DateTime> expires;
    melanolib::Optional<melanolib::time::seconds> max_age;
    std::string domain;
    std::string path;
    bool secure = false;
    bool http_only = false;
    std::vector<std::string> extension;
};

using CookieJar = melanolib::OrderedMultimap<std::string, Cookie>;

inline std::ostream& operator<<(std::ostream& os, CookieJar::const_reference cookie)
{
    os << cookie.first << '=' << cookie.second.value;

    if ( cookie.second.expires )
        os << "; Expires=" << melanolib::time::strftime(*cookie.second.expires, "%r GMT");

    if ( cookie.second.max_age )
        os << "; Max-Age=" << cookie.second.max_age->count();

    if ( !cookie.second.domain.empty() )
        os << "; Domain=" << cookie.second.domain;

    if ( !cookie.second.path.empty() )
        os << "; Path=" << urlencode(cookie.second.path);

    if ( cookie.second.secure )
        os << "; Secure";

    if ( cookie.second.http_only )
        os << "; HttpOnly";

    if ( !cookie.second.extension.empty() )
        for ( const auto extension : cookie.second.extension )
            os << "; " << extension;

    return os;
}


} // namespace httpony
#endif // HTTPONT_COOKIE_HPP
