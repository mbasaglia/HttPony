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

#include "client_connection.hpp"

#include <melanolib/time/time_string.hpp>

namespace httpony {

void ClientConnection::read_request()
{
    boost::system::error_code error;

    request = Request();
    request.from = remote;

    /// \todo Avoid magic number, keep reading if needed
    auto sz = input.read_some(1024, error);

    status = StatusCode::BadRequest;

    if ( error || sz == 0 )
        return;

    std::istream stream(&input);

    if ( !read_request_line(stream) || !read_headers(stream) )
        return;
    
    response.protocol = request.protocol;

    if ( request.headers.contains("Content-Length") )
    {
        if ( !request.body.get_data(input, request.headers) )
            return;

        if ( request.protocol == Protocol("HTTP", 1, 1) && request.headers.get("Expect") == "100-continue" )
        {
            status = StatusCode::Continue;
            return;
        }
    }
    else if ( input.size() )
    {
        status = StatusCode::LengthRequired;
        return;
    }

    if ( request.protocol == Protocol("HTTP", 1, 1) && request.headers.contains("Expect") )
    {
        status = StatusCode::ExpectationFailed;
        return;
    }

    status = StatusCode::OK;
}

void ClientConnection::send_response(Response& response)
{
    boost::asio::streambuf buffer_write;
    std::ostream buffer_stream(&buffer_write);
    buffer_stream << response.protocol << ' '
                    << response.status.code << ' '
                    << response.status.message << "\r\n";

    write_header(buffer_stream, "Date", melanolib::time::strftime(response.date, "%r GMT"));

    for ( const auto& header : response.headers )
        write_header(buffer_stream, header.first, header.second);

    for ( const auto& cookie : response.cookies )
        write_header(buffer_stream, "Set-Cookie", cookie);


    if ( response.body.has_data() )
    {
        write_header(buffer_stream, "Content-Type", response.body.content_type());
        write_header(buffer_stream, "Content-Length", response.body.content_length());
    }

    buffer_stream << "\r\n";
    boost::asio::write(input.socket(), buffer_write);

    if ( response.body.has_data() )
        response.body.net_write(input.socket());
}


void ClientConnection::skip_line(std::istream& buffer_stream)
{
    while ( buffer_stream && buffer_stream.get() != '\n' );
}

bool ClientConnection::read_request_line(std::istream& buffer_stream)
{
    std::string uri;
    buffer_stream >> request.method >> uri >> request.protocol;
    request.url = uri;
    request.get = request.url.query;
    skip_line(buffer_stream);

    return request.protocol.valid() && buffer_stream;
}

bool ClientConnection::read_headers(std::istream& buffer_stream)
{
    std::string name, value;
    while ( buffer_stream && buffer_stream.peek() != '\r' )
    {
        // (obsolete) header folding
        // it's compliant to either return 400 or parse them
        if ( std::isspace(buffer_stream.peek()) )
        {
            if ( !parse_folded_headers )
                return false;

            if ( !skip_spaces(buffer_stream) || request.headers.empty() )
                return false;

            std::getline(buffer_stream, value, '\r');
            buffer_stream.ignore(1, '\n');
            if ( !buffer_stream )
            {
                return false;
            }
            request.headers.back().second += ' ' + value;
            continue;
        }

        if ( !read_delimited(buffer_stream, name, ':') )
            return false;

        if ( name == "Cookie" )
        {
            if ( !read_cookie(buffer_stream) )
                return false;
            continue;
        }

        if ( buffer_stream.peek() == '"' )
        {
            read_quoted_header_value(buffer_stream, value);
        }
        else
        {
            std::getline(buffer_stream, value, '\r');
            buffer_stream.ignore(1, '\n');
        }
        /// \todo Read header comments

        if ( !buffer_stream )
        {
            return false;
        }
        /// \todo validate header name
        request.headers.append(name, value);
    }

    skip_line(buffer_stream);

    return true;
}

bool ClientConnection::read_delimited(std::istream& stream, std::string& output, char delim, bool at_end)
{
    output.clear();
    while ( true )
    {
        auto c = stream.get();
        if ( c == std::istream::traits_type::eof() || c == '\r' )
        {
            stream.unget();
            return at_end;
        }
        if ( c == delim )
            break;
        output += c;
    }

    return skip_spaces(stream, at_end);
}

bool ClientConnection::skip_spaces(std::istream& stream, bool at_end )
{
    while ( true )
    {
        auto c = stream.peek();
        if ( c == '\r' || c == std::istream::traits_type::eof() )
            return at_end;
        if ( !std::isspace(c) )
            break;
        stream.ignore(1);
    }
    return true;
}

bool ClientConnection::read_quoted_header_value(std::istream& buffer_stream, std::string& value)
{
    buffer_stream.ignore(1, '"');
    value.clear();
    bool last_slash = false;
    while ( true )
    {
        auto c = buffer_stream.get();
        if ( c == std::char_traits<char>::eof() || c == '\r' || c == '\n' )
        {
            return false;
        }

        if ( !last_slash )
        {
            if ( c == '"'  )
                break;
            if ( c == '\\' )
            {
                last_slash = true;
                continue;
            }
        }
        else
        {
            last_slash = false;
        }

        value += c;

    }

    skip_line(buffer_stream);

    return true;
}

bool ClientConnection::read_cookie(std::istream& stream)
{
    do
    {
        std::string name;
        if ( !read_delimited(stream, name, '=') )
            return false;

        std::string value;
        if ( !read_delimited(stream, value, ';', true) )
            return false;

        request.cookies.append(name, value);
    }
    while ( stream.peek() != '\r' );

    skip_line(stream);

    return true;
}


} // namespace httpony
