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

#include <boost/asio.hpp>

#include "request.hpp"
#include "response.hpp"

#include <iostream>

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
            return ;
        }

        buffer_read.commit(sz);
        std::istream buffer_stream(&buffer_read);
        buffer_stream >> request.method >> request.url >> request.protocol;

        std::cout << "BEGIN "
            << request.from.string << ' ' << request.from.port
            << '\n' << request.method << ' ' <<  request.url << ' ' <<  request.protocol
            << "\n===\n";
        std::string line;
        while ( buffer_stream && buffer_stream.get() != '\n' ); // Ignore all before \n
        while ( true )
        {
            std::getline(buffer_stream, line, '\r');
            buffer_stream.ignore(1, '\n');
            if ( !buffer_stream )
                break;
            std::cout << "> " << line << '\n';
        }
        std::cout << "END\n\n";
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
        // TODO
    }

    Request request;
    Response response;
    StatusCode status_code = StatusCode::OK;
    boost::asio::ip::tcp::socket socket;
    Server* server;
};

} // namespace muhttpd
#endif // MUHTTPD_CLIENT_CONNECTION_HPP
