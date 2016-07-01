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

namespace muhttpd {

class Server;

class ClientConnection
{
public:
    ClientConnection(boost::asio::ip::tcp::socket socket, Server* server)
        : socket(std::move(socket)), server(server)
    {}

    ~ClientConnection()
    {
        socket.close();
    }

    void read_request()
    {
        boost::system::error_code error;

        // boost::asio::streambuf buffer_read;
        // auto sz = boost::asio::read(socket, buffer_read, error);

        boost::asio::streambuf buffer_read;
        auto buffer = buffer_read.prepare(1024);
        auto sz = socket.read_some(boost::asio::buffer(buffer), error);

        request = Request();
        status_code = StatusCode::OK;

        request.from = endpoint_to_ip(socket.remote_endpoint());

        if ( error || sz == 0 )
        {
            status_code = StatusCode::BadRequest;
            return;
        }

        buffer_read.commit(sz);
        std::istream buffer_stream(&buffer_read);

        if ( !read_request_line(buffer_stream) || !read_headers(buffer_stream) )
        {
            status_code = StatusCode::BadRequest;
            return;
        }

        /// \todo If the body has already been sent handle it here
    }

    static IPAddress endpoint_to_ip(const boost::asio::ip::tcp::endpoint& endpoint)
    {
        return IPAddress(
            endpoint.address().is_v6() ? IPAddress::Type::IPv6 : IPAddress::Type::IPv4,
            endpoint.address().to_string(),
            endpoint.port()
        );
    }

    bool ok() const
    {
        return status_code == StatusCode::OK;
    }

    void send_response()
    {
        boost::asio::streambuf buffer_write;
        std::ostream buffer_stream(&buffer_write);
        buffer_stream << response.protocol << ' '
                      << response.status.code << ' '
                      << response.status.message << "\r\n";

        for ( const auto& header : response.headers )
            buffer_stream << header.name << ": " << header.value << "\r\n";
        buffer_stream << "\r\n";

        buffer_stream << response.body;

        boost::asio::write(socket, buffer_write);
    }

    void read_body()
    {
        /// \todo Read body checking content-length and stuff
        /// \todo Parse urlencoded and multipart/form-data into request.post
        /// \todo Functionality to read it asyncronously
    }

    Request request;
    Response response;
    StatusCode status_code = StatusCode::OK;
    boost::asio::ip::tcp::socket socket;
    Server* server;

private:
    // Ignore all before \n
    void skip_line(std::istream& buffer_stream)
    {
        while ( buffer_stream && buffer_stream.get() != '\n' );
    }

    bool read_request_line(std::istream& buffer_stream)
    {
        buffer_stream >> request.method >> request.url >> request.protocol;
        skip_line(buffer_stream);

        return request.protocol.valid() && buffer_stream;
    }

    bool read_headers(std::istream& buffer_stream)
    {
        std::string name, value;
        while ( buffer_stream && buffer_stream.peek() != '\r' )
        {
            if ( !read_header_name(buffer_stream, name) )
                return false;

            std::getline(buffer_stream, value, '\r');
            buffer_stream.ignore(1, '\n');

            if ( !buffer_stream )
            {
                status_code = StatusCode::BadRequest;
                return false;
            }
            /// \todo validate header name/value
            request.headers.append(name, value);
        }

        skip_line(buffer_stream);

        return true;
    }

    bool read_header_name(std::istream& buffer_stream, std::string& name)
    {
        name.clear();
        while ( true )
        {
            auto c = buffer_stream.get();
            if ( c == std::char_traits<char>::eof() || c == '\r' )
            {
                status_code = StatusCode::BadRequest;
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

};

} // namespace muhttpd
#endif // MUHTTPD_CLIENT_CONNECTION_HPP
