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


#include "asio_request_parser.hpp"

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
            [this](boost::system::error_code error_code)
            {
                if ( !acceptor.is_open() )
                    return;

                parse_request(error_code);

                accept();
            }
        );
    }

    void parse_request(boost::system::error_code error_code)
    {
        /// \todo This could spawn async reads (and writes)
        asio::AsioRequestParser parser(std::move(socket));
        if ( error_code )
            return;
        owner->respond(parser.read_request());
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
