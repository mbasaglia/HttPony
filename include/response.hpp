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

#ifndef HTTPONY_RESPONSE_HPP
#define HTTPONY_RESPONSE_HPP

#include <melanolib/utils/c++-compat.hpp>
#include <melanolib/time/date_time.hpp>

#include "status.hpp"
#include "protocol.hpp"
#include "io.hpp"
#include "uri.hpp"
#include "cookie.hpp"

namespace httpony {

struct AuthChallenge
{
    std::string auth_scheme;
    std::string realm;
    Headers     parameters;

    std::string header_value() const
    {
        std::string authenticate = auth_scheme;

        if ( !realm.empty() )
            authenticate += " realm=\"" + melanolib::string::add_slashes(realm, "\"\\") + "\";";

        if ( !parameters.empty() )
            authenticate += ' ' + header_parameters(parameters);

        return authenticate;
    }
};

/**
 * \brief Response data
 */
struct Response
{
    explicit Response(std::string content_type, Status status = Status())
        : body(std::move(content_type)),
          status(std::move(status)),
          protocol(Protocol::http_1_1)
    {
        /// \todo Date
    }

    Response(Status status = Status())
        : status(std::move(status)),
          protocol(Protocol::http_1_1)
    {

    }

    OutputContentStream body;
    Status      status;
    Headers     headers;
    Protocol    protocol;
    CookieJar   cookies;
    melanolib::time::DateTime date;

    static Response redirect(const Uri& location, Status status = StatusCode::Found)
    {
        Response response(status);
        response.headers["Location"] = location.full();
        return response;
    }

    static Response authorization_required(const std::vector<AuthChallenge>& challenges)
    {
        Response response(StatusCode::Unauthorized);
        std::string www_authenticate;
        for ( const auto& challenge : challenges )
        {
            if ( !www_authenticate.empty() )
                www_authenticate += ", ";
            www_authenticate += challenge.header_value();
        }
        response.headers["WWW-Authenticate"] = www_authenticate;
        return response;
    }
};

} // namespace httpony
#endif // HTTPONY_RESPONSE_HPP
