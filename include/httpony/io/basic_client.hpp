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
     * \brief Runs the server synchronously
     * \tparam OnSuccess Functor accepting a io::Connectio&
     * \tparam OnFailure Functor accepting a io::Connectio& and a std::string
     * \tparam OnResolveFailure Functor accepting two std::string
     * \tparam CreateConnection Function returning a std::unique_ptr<Connection>
     * \param on_success Functor called on a successful connection
     * \param on_resolve_failure Functor called when the client could not resolve
     *  the target (This happens before the connection is established).
     *  The first argument is \p target and the second is an error message.
     * \param on_failure Functor called on a connection that had some issues
     * \param create_connection Function called to create a new connection object
     */
    template<class OnSuccess, class OnResolveFailure,
             class OnFailure, class CreateConnection>
    void queue_request(
        const std::string& target,
        const OnSuccess& on_success,
        const OnResolveFailure& on_resolve_failure,
        const OnFailure& on_failure,
        const CreateConnection& create_connection
    )
    {
        boost_tcp::resolver::query query(target, "http");
        resolver.async_resolve(query,
            [this, target, on_success, on_resolve_failure, on_failure, create_connection](
                const boost::system::error_code& error_code,
                boost_tcp::resolver::iterator endpoint_iterator
            )
            {
                if ( error_code )
                    on_resolve_failure(target, error_code.message());
                else
                    on_resolve(endpoint_iterator, on_success, on_failure, create_connection);
            }
        );
    }

    void run()
    {
        io_service.run();
    }

private:
    template<class OnSuccess, class OnFailure, class CreateConnection>
    void on_resolve(
        boost_tcp::resolver::iterator endpoint_iterator,
        const OnSuccess& on_success,
        const OnFailure& on_failure,
        const CreateConnection& create_connection
    )
    {
        /// \todo Clean up the pushed connection
        connections.push_back(create_connection());
        auto connection = connections.back().get();

        if ( _timeout )
            connection->socket().set_timeout(*_timeout);

        boost::asio::async_connect(
            connection->socket().raw_socket(),
            endpoint_iterator,
            [this, on_success, on_failure, connection](
                const boost::system::error_code& error_code,
                boost_tcp::resolver::iterator endpoint_iterator
            )
            {
                if ( error_code )
                    on_failure(*connection, error_code.message());
                else
                    on_success(*connection);

            }
        );
    }

    melanolib::Optional<melanolib::time::seconds> _timeout;
    boost::asio::io_service io_service;
    boost_tcp::resolver resolver{io_service};
    std::vector<std::unique_ptr<io::Connection>> connections;
};

} // namespace io
} // namespace httpony
#endif // HTTPONY_IO_BASIC_CLIENT_HPP
