/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2016 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MUHTTPD_CLIENT_CONNECTION_HPP
#define MUHTTPD_CLIENT_CONNECTION_HPP

#include <iostream>

#include <boost/asio.hpp>

#include "request.hpp"
#include "response.hpp"
#include "io.hpp"

namespace muhttpd {

class Server;

/**
 * \brief Represents a single connection to a client for a request/response interaction
 */
class ClientConnection
{
public:
    ClientConnection(boost::asio::ip::tcp::socket socket, Server* server)
        : input(std::move(socket)), server(server)
    {

        request.from = remote = endpoint_to_ip(input.socket().remote_endpoint());
        local = endpoint_to_ip(input.socket().local_endpoint());
    }

    ~ClientConnection()
    {
        input.socket().close();
    }

    /**
     * \brief Reads request data from the socket
     *
     * Sets \c status to an error code if the request is malformed
     */
    void read_request()
    {
        boost::system::error_code error;

        request = Request();
        request.from = remote;

        // boost::asio::streambuf buffer_read;
        // auto sz = boost::asio::read(socket, buffer_read, error);

        /// \todo Avoid magic number, keep reading if needed
        auto sz = input.read_some(1024, error);

        status = StatusCode::BadRequest;

        if ( error || sz == 0 )
            return;

        std::istream stream(&input);

        if ( !read_request_line(stream) || !read_headers(stream) )
            return;

        if ( request.headers.has_header("Content-Length") )
        {
            if ( !request.body.get_data(input, request.headers) )
                return;
        }
        else if ( input.size() )
        {
            status = StatusCode::LengthRequired;
            return;
        }

        status = StatusCode::OK;
    }

    /**
     * \brief Whether the status represents a successful read
     */
    bool ok() const
    {
        return status == StatusCode::OK;
    }

    /**
     * \brief Removes the response body when required by HTTP
     */
    void clean_response_body()
    {
        if ( response.body.has_data() )
        {
            if ( response.status.type() == StatusType::Informational ||
                response.status == StatusCode::NoContent ||
                response.status == StatusCode::NotModified ||
                ( response.status == StatusCode::OK && request.method == "CONNECT" )
            )
            {
                response.body.stop_data();
            }
        }
    }

    /**
     * \brief Sends the response to the socket
     */
    void send_response()
    {
        boost::asio::streambuf buffer_write;
        std::ostream buffer_stream(&buffer_write);
        buffer_stream << response.protocol << ' '
                      << response.status.code << ' '
                      << response.status.message << "\r\n";

        for ( const auto& header : response.headers )
            buffer_stream << header.name << ": " << header.value << "\r\n";

        if ( response.body.has_data() )
        {
            buffer_stream << "Content-Type" << ": " << response.body.content_type() << "\r\n";
            buffer_stream << "Content-Length" << ": " << response.body.content_length() << "\r\n";
        }

        buffer_stream << "\r\n";
        boost::asio::write(input.socket(), buffer_write);

        if ( response.body.has_data() )
            response.body.net_write(input.socket());
    }


    Request request;
    Response response;
    IPAddress local;
    IPAddress remote;
    Status status = StatusCode::OK;
    NetworkBuffer input;
    Server* const server;

private:
    /**
     * \brief Converts a boost endpoint to an IPAddress object
     */
    static IPAddress endpoint_to_ip(const boost::asio::ip::tcp::endpoint& endpoint)
    {
        return IPAddress(
            endpoint.address().is_v6() ? IPAddress::Type::IPv6 : IPAddress::Type::IPv4,
            endpoint.address().to_string(),
            endpoint.port()
        );
    }

    /**
     * \brief Ignores all before "\n"
     */
    void skip_line(std::istream& buffer_stream)
    {
        while ( buffer_stream && buffer_stream.get() != '\n' );
    }

    /**
     * \brief Reads the request line (GET /url HTTP/1.1)
     * \returns \b true on success
     */
    bool read_request_line(std::istream& buffer_stream)
    {
        std::string uri;
        buffer_stream >> request.method >> uri >> request.protocol;
        request.url = uri;
        request.get = request.url.query;
        skip_line(buffer_stream);

        return request.protocol.valid() && buffer_stream;
    }

    /**
     * \brief Reads all headers and the empty line following them
     * \returns \b true on success
     */
    bool read_headers(std::istream& buffer_stream)
    {
        std::string name, value;
        while ( buffer_stream && buffer_stream.peek() != '\r' )
        {
            // Discard deprecated header folding
            if ( std::isspace(buffer_stream.peek()) )
                return false;

            if ( !read_header_name(buffer_stream, name) )
                return false;

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

    /**
     * \brief Reads a header name and the following colon, including optional spaces
     * \returns \b true on success
     */
    bool read_header_name(std::istream& buffer_stream, std::string& name)
    {
        name.clear();
        while ( true )
        {
            auto c = buffer_stream.get();
            if ( c == std::char_traits<char>::eof() || c == '\r' )
            {
                return false;
            }
            if ( c == ':' )
                break;
            name += c;
        }

        while ( std::isspace(buffer_stream.peek()) )
            buffer_stream.ignore(1);

        return true;
    }

    /**
     * \brief Reads a "quoted" header value
     * \returns \b true on success
     */
    bool read_quoted_header_value(std::istream& buffer_stream, std::string& value)
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

};


} // namespace muhttpd
#endif // MUHTTPD_CLIENT_CONNECTION_HPP
