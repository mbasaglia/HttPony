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

/// \cond
#include <melanolib/time/time_string.hpp>
#include <melanolib/utils/c++-compat.hpp>
/// \endcond

#include "httpony/uri.hpp"

namespace httpony {

/**
 * \brief Set-Cookie value
 * \see https://tools.ietf.org/html/rfc6265
 */
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

/**
 * \brief Cookie as stored on the client
 */
class ClientCookie
{
public:
    ClientCookie(const Cookie& cookie)
        : domain(cookie.domain),
          path(cookie.path),
          secure(cookie.secure),
          http_only(cookie.http_only)
    {
        if ( cookie.max_age )
        {
            if ( cookie.max_age->count() <= 0 )
                expiry_time = melanolib::time::DateTime{
                    melanolib::time::DateTime::Time::min()
                };
            else
                expiry_time = melanolib::time::DateTime{} + *cookie.max_age;
        }
        else if ( cookie.expires )
        {
            expiry_time = *cookie.expires;
        }
    }

    /**
     * \brief Whether the cookie can be sent to the given uri
     */
    bool matches_uri(const Uri& uri) const
    {
        return matches_domain(uri.authority.host) && matches_path(path);
    }

    /**
     * \brief Whether the domain string matches
     * \see https://tools.ietf.org/html/rfc6265#section-5.1.3
     */
    bool matches_domain(const std::string& string) const
    {
        /// \todo canonicalize domains ( https://tools.ietf.org/html/rfc6265#section-5.1.2 )
        if ( domain == string )
            return true;
        if ( !melanolib::string::ends_with(string, '.' + domain) )
            return false;
        /// \todo Check if the string is an IP address
        return true;
    }

    /**
     * \brief Whether the path matches
     * \see https://tools.ietf.org/html/rfc6265#section-5.1.4
     */
    bool matches_path(const Path& other_path) const
    {
        if ( other_path.size() < path.size() )
            return false;
        return std::equal(
            path.begin(),
            path.end(),
            other_path.begin(),
            other_path.begin() + path.size()
        );
    }

    bool expired(const melanolib::time::DateTime& date = {}) const
    {
        return expiry_time < date;
    }

    bool is_session() const
    {
        return !expiry_time;
    }

    void update_access()
    {
        last_access = {};
    }

    std::string value;
    melanolib::Optional<melanolib::time::DateTime> expiry_time;
    std::string domain;
    Path path;
    bool secure = false;
    bool http_only = false;
    melanolib::time::DateTime creation_time;
    melanolib::time::DateTime last_access;
};

/**
 * \todo Order as from https://tools.ietf.org/html/rfc6265#section-5.4 (2.)
 */
using ClientCookieJar = melanolib::OrderedMultimap<std::string, ClientCookie>;



} // namespace httpony
#endif // HTTPONT_COOKIE_HPP
