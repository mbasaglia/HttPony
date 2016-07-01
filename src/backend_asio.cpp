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
 *
 */
#include "server_data.hpp"

#include <thread>

#include <boost/asio.hpp>

#include <iostream>
#include <sstream>

namespace muhttpd {


class AsioServerData : public Server::Data
{
public:
//     using Server::Data::Data;
    AsioServerData(Server* owner, IPAddress listen) : Data(owner, listen) {}

    boost::asio::io_service             io_service;
    boost::asio::ip::tcp::socket        socket{io_service};
    boost::asio::ip::tcp::acceptor      acceptor{io_service};

    std::thread thread;

    void accept()
    {
        acceptor.async_accept(
            socket,
            [this](boost::system::error_code ec)
            {
                if (!acceptor.is_open())
                {
                    return;
                }

                if ( !ec )
                {
                    /// \todo This could spawn async reads (and writes)
                    boost::system::error_code error;

                    // boost::asio::streambuf buffer_read;
                    // auto sz = boost::asio::read(socket, buffer_read, error);

                    boost::asio::streambuf buffer_read;
                    auto buffer = buffer_read.prepare(1024);
                    auto sz = socket.read_some(boost::asio::buffer(buffer), error);

                    if ( error || sz == 0 )
                        return;

                    Request request;

                    request.from = IPAddress(
                        socket.remote_endpoint().address().is_v6() ? IPAddress::Type::IPv6 : IPAddress::Type::IPv4,
                        socket.remote_endpoint().address().to_string(),
                        socket.remote_endpoint().port()
                    );

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

                socket.close();

                accept();
            }
        );
    }

};

std::unique_ptr<Server::Data> make_data(Server* owner, IPAddress listen)
{
    return std::make_unique<AsioServerData>(owner, listen);
}

bool Server::started() const
{
    AsioServerData* asiodata = reinterpret_cast<AsioServerData*>(data.get());
    return asiodata->thread.joinable();
}

void Server::start()
{
    AsioServerData* asiodata = reinterpret_cast<AsioServerData*>(data.get());
    boost::asio::ip::tcp::resolver resolver(asiodata->io_service);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({
        asiodata->listen.string,
        std::to_string(asiodata->listen.port)
    });
    asiodata->acceptor.open(endpoint.protocol());
    asiodata->acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    asiodata->acceptor.bind(endpoint);
    asiodata->acceptor.listen();
    asiodata->thread = std::thread([asiodata](){
        asiodata->accept();
        asiodata->io_service.run();
    });
}

void Server::stop()
{
    AsioServerData* asiodata = reinterpret_cast<AsioServerData*>(data.get());
    if ( started() )
    {
        /// \todo Needs locking?
        asiodata->acceptor.close();
        asiodata->io_service.stop();
        asiodata->thread.join();
    }
}

} // namespace muhttpd
