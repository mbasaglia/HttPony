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

#include "cookie.hpp"
#include "request.hpp"

namespace httpony {

struct AuthChallenge
{
    std::string auth_scheme;
    std::string realm;
    Headers     parameters;
};

/**
 * \brief Response data
 */
struct Response
{
    explicit Response(
        std::string content_type,
        Status status = Status(),
        const Protocol& protocol = Protocol::http_1_1
    )
        : body(std::move(content_type)),
          status(std::move(status)),
          protocol(protocol)
    {
    }

    Response(Status status = Status(), const Protocol& protocol = Protocol::http_1_1)
        : status(std::move(status)),
          protocol(protocol)
    {}

    explicit Response(const Request& request)
        : status(request.suggested_status),
          protocol(request.protocol)
    {}

    static Response redirect(const Uri& location, Status status = StatusCode::Found)
    {
        Response response(status);
        response.headers["Location"] = location.full();
        return response;
    }

    static Response authorization_required(const std::vector<AuthChallenge>& challenges);


    /**
     * \brief Removes the response body when required by HTTP
     */
    void clean_body()
    {
        if ( body.has_data() )
        {
            if ( status.type() == StatusType::Informational ||
                status == StatusCode::NoContent ||
                status == StatusCode::NotModified
            )
            {
                body.stop_data();
            }
        }
    }

    /**
     * \brief Removes the response body when required by HTTP
     */
    void clean_body(const Request& input)
    {
        clean_body();
        if ( body.has_data() )
        {
            if ( status == StatusCode::OK && input.method == "CONNECT" )
            {
                body.stop_data();
            }
            else if ( input.method == "HEAD" )
            {
                headers["Content-Type"] = body.content_type().string();
                headers["Content-Length"] = std::to_string(body.content_length());
                body.stop_data();
            }
        }
    }

    OutputContentStream body;
    Status      status;
    Headers     headers;
    Protocol    protocol;
    CookieJar   cookies;
    melanolib::time::DateTime date;
};

} // namespace httpony
#endif // HTTPONY_RESPONSE_HPP
