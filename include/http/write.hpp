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

#include "response.hpp"

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

/**
 * \brief Writes all response headers, including the blank line at the end
 */
inline void response_headers(std::ostream& stream, const Response& response)
{
    header(stream, "Date", melanolib::time::strftime(response.date, "%r GMT"));

    headers(stream, response.headers);

    for ( const auto& cookie : response.cookies )
        header(stream, "Set-Cookie", cookie);

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


inline std::string header_parameter(const std::string& name, const std::string& value)
{
    using namespace melanolib::string;
    static const char* const slashable = "\" \t\\";
    std::string result = name;
    result +=  '=';
    if ( contains_any(value, slashable) )
    {
        result +=  '"';
        result += add_slashes(value, slashable);
        result += '"';
    }
    else
    {
        result += value;
    }
    return result;
}

template<class OrderedContainer>
    inline std::string header_parameters(const OrderedContainer& input, char delimiter = ',')
{
    std::string result;
    for ( const auto& param : input )
        result += header_parameter(param.first, param.second) + delimiter + ' ';
    if ( !result.empty() )
        result.pop_back();
    return result;
}

inline std::string auth_challenge(const AuthChallenge& challenge)
{
    std::string authenticate = challenge.auth_scheme;

    if ( !challenge.realm.empty() )
        authenticate += " realm=\"" + melanolib::string::add_slashes(challenge.realm, "\"\\") + "\";";

    if ( !challenge.parameters.empty() )
        authenticate += ' ' + header_parameters(challenge.parameters);

    return authenticate;
}
} // namespace write
} // namespace http
} // namespace httpony
#endif // HTTPONY_HTTP_WRITE_HPP
