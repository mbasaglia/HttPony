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

#ifndef HTTPONY_HTTP_WRITE_HPP
#define HTTPONY_HTTP_WRITE_HPP

#include "httpony/http/response.hpp"

namespace httpony {
namespace http {
/**
 * \brief Namespace with functions to write HTTP to an output stream
 */
namespace write {

/**
 * \brief Writes a line terminator to the stream
 * \note Can be used as a stream manipulator
 */
inline std::ostream& endl(std::ostream& stream)
{
    return stream << "\r\n";
}

/**
 * \brief Writes a header line
 */
template<class String, class Value>
    void header(std::ostream& stream, String&& name, Value&& value)
{
    stream << std::forward<String>(name) << ": " << std::forward<Value>(value) << endl;
}

/**
 * \brief Writes out a block of headers
 * \note This does not output a blank line at the end
 */
inline void headers(std::ostream& stream, const Headers& headers)
{
    for ( const auto& header_obj : headers )
        header(stream, header_obj.first, header_obj.second);
}

/**
 * \brief Writes a response line
 */
inline void response_line(std::ostream& stream, const Response& response)
{
    stream << response.protocol << ' '
           << response.status.code << ' '
           << response.status.message << endl;
}

inline void header_parameter(std::ostream& stream, const std::string& name, const std::string& value)
{
    using namespace melanolib::string;
    static const char* const slashable = "\" \t\\";
    stream << name <<  '=';
    if ( contains_any(value, slashable) )
    {
        stream << '"' << add_slashes(value, slashable) << '"';
    }
    else
    {
        stream << value;
    }
}

template<class OrderedContainer>
    inline void header_parameters(std::ostream& stream, const OrderedContainer& input, const std::string& delimiter = ", ")
{
    header_parameter(stream, input.front().first, input.front().second);

    for ( auto it = input.begin() + 1; it != input.end(); ++it )
    {
        stream << delimiter;
        header_parameter(stream, it->first, it->second);
    }
}

inline void auth_challenge(std::ostream& stream, const AuthChallenge& challenge)
{
    stream << challenge.auth_scheme;

    if ( !challenge.realm.empty() )
        stream << " realm=\""
               << melanolib::string::add_slashes(challenge.realm, "\"\\")
               << "\";";

    if ( !challenge.parameters.empty() )
    {
        stream << ' ';
        header_parameters(stream, challenge.parameters);
    }
}

inline void authenticate_header(std::ostream& stream, const std::string& name, const std::vector<AuthChallenge>& challenges)
{
    if ( challenges.empty() )
        return;

    stream << name << ": ";

    /// \todo merge this loop with header_parameters
    auth_challenge(stream, challenges.front());

    for ( auto it = challenges.begin() + 1; it != challenges.end(); ++it )
    {
        stream << ", ";
        auth_challenge(stream, *it);
    }

    stream << endl;
}

/**
 * \brief Writes all response headers, including the blank line at the end
 */
inline void response_headers(std::ostream& stream, const Response& response)
{
    header(stream, "Date", melanolib::time::strftime(response.date, "%r GMT"));

    headers(stream, response.headers);

    for ( const auto& cookie : response.cookies )
        header(stream, "Set-Cookie", cookie);

    authenticate_header(stream, "WWW-Authenticate", response.www_authenticate);
    authenticate_header(stream, "Proxy-Authenticate", response.proxy_authenticate);

    if ( response.body.has_data() )
    {
        header(stream, "Content-Type", response.body.content_type());
        header(stream, "Content-Length", response.body.content_length());
    }

    stream << endl;
}

/**
 * \brief Writes the entire response to the stream
 * \note \p response is passed by non-const reference because
 *       writing the body could modify the state underlying buffer
 */
inline void response(std::ostream& stream, Response& response)
{
    response_line(stream, response);
    response_headers(stream, response);
    response.body.write_to(stream);
}

inline void request_line(std::ostream& stream, Request& request)
{
    stream << request.method << ' ' << request.url.path.url_encoded()
           << request.url.query_string(true) << ' ' << request.protocol << endl;
}

inline void request(std::ostream& stream, Request& request)
{
    request_line(stream, request);
    headers(stream, request.headers);
    stream << endl;
    request.body.write_to(stream);
}

} // namespace write
} // namespace http
} // namespace httpony
#endif // HTTPONY_HTTP_WRITE_HPP
