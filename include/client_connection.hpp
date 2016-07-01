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


struct NetworkInputStream
{

    explicit NetworkInputStream(boost::asio::ip::tcp::socket socket)
        : socket(std::move(socket))
    {
    }

    std::size_t read_some(std::size_t size, boost::system::error_code& error)
    {
        /// \todo timeout
        auto prev_size = buffer.size();
        if ( size <= prev_size )
            return size;
        size -= prev_size;
        auto in_buffer = buffer.prepare(size);
        auto read_size = socket.read_some(boost::asio::buffer(in_buffer), error);
        buffer.commit(read_size);
        return read_size + prev_size;
    }

    boost::asio::ip::tcp::socket socket;
    boost::asio::streambuf buffer;
    std::istream stream{&buffer};
};

class ClientConnection
{
public:
    ClientConnection(boost::asio::ip::tcp::socket socket, Server* server)
        : input(std::move(socket)), server(server)
    {}

    ~ClientConnection()
    {
        input.socket.close();
    }

    void read_request()
    {
        boost::system::error_code error;

        // boost::asio::streambuf buffer_read;
        // auto sz = boost::asio::read(socket, buffer_read, error);

        /// \todo Avoid magic number, keep reading if needed
        auto sz = input.read_some(1024, error);

        request = Request();
        status_code = StatusCode::OK;

        request.from = endpoint_to_ip(input.socket.remote_endpoint());


        if ( error || sz == 0 )
        {
            status_code = StatusCode::BadRequest;
            return;
        }

        if ( !read_request_line(input.stream) || !read_headers(input.stream) )
        {
            status_code = StatusCode::BadRequest;
            return;
        }

        status_code = StatusCode::OK;

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

        boost::asio::write(input.socket, buffer_write);
    }

    /**
     * \brief Reads the request body into the request object
     */
    void read_body(std::size_t max_size = 0);

    Request request;
    Response response;
    StatusCode status_code = StatusCode::OK;
    NetworkInputStream input;
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


/**
 * \brief Gives access to the request body from a connection object
 */
class BodyStream
{
public:
    explicit BodyStream(ClientConnection& connection, std::size_t max_size = 0);

    bool ok()
    {
        return _ok && connection->input.stream.good();
    }

    std::size_t content_length() const
    {
        return _content_length;
    }

    std::string read_all()
    {
        std::string all(_content_length, ' ');
        connection->input.stream.read(&all[0], _content_length);
        all.resize(connection->input.stream.gcount());
        return all;
    }

    template<class T>
    friend BodyStream& operator>>(BodyStream& bs, T& obj)
    {
        bs.connection->input.stream >> obj;
        return bs;
    }

private:
    bool _ok = false;
    std::size_t _content_length = 0;
    ClientConnection* connection;
};

} // namespace muhttpd
#endif // MUHTTPD_CLIENT_CONNECTION_HPP
