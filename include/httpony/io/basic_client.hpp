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
#ifndef HTTPONY_IO_BASIC_CLIENT_HPP
#define HTTPONY_IO_BASIC_CLIENT_HPP

#include "connection.hpp"
#include "httpony/http/response.hpp"

namespace httpony {
namespace io {

class BasicClient
{
public:
    /**
     * \tparam CreateConnection Function returning a std::unique_ptr<Connection>
     * \param create_connection Function called to create a new connection object
     */
    template<class CreateConnection>
    std::pair<std::unique_ptr<io::Connection>, std::string>
        connect(const Uri& target, const CreateConnection& create_connection)
    {
        std::string service = target.scheme;
        if ( target.authority.port )
            service = std::to_string(*target.authority.port);
        boost_tcp::resolver::query query(target.authority.host, service);

        boost::system::error_code error_code;
        auto endpoint_iterator = resolver.resolve(query, error_code);

        if ( error_code )
        {
            return std::make_pair<std::unique_ptr<io::Connection>, std::string>({}, error_code.message());
        }

        /// \todo Clean up the pushed connection
        std::unique_ptr<io::Connection> connection = create_connection();

        if ( _timeout )
            connection->socket().set_timeout(*_timeout);

        error_code.clear();
        connection->socket().connect(endpoint_iterator, error_code);
        if ( error_code )
            return {std::move(connection), error_code.message()};

        return {std::move(connection), ""};
    }

private:


    melanolib::Optional<melanolib::time::seconds> _timeout;
    boost::asio::io_service io_service;
    boost_tcp::resolver resolver{io_service};
};

} // namespace io
} // namespace httpony
#endif // HTTPONY_IO_BASIC_CLIENT_HPP
