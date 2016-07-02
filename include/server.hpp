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
#ifndef MUHTTPD_SERVER_HPP
#define MUHTTPD_SERVER_HPP

#include <stdexcept>
#include <memory>
#include <ostream>

#include "client_connection.hpp"

namespace muhttpd {

/**
 * \brief Base class for a simple HTTP server
 * \note It reads POST data in a single buffer instead of streaming it
 */
class Server
{
public:

    explicit Server(IPAddress listen);
    explicit Server(uint16_t listen_port);

    ~Server();

    /**
     * \brief Listening address
     */
    IPAddress listen_address() const;

    /**
     * \brief Starts the server in a background thread
     * \throws runtime_error if it cannot be started
     */
    void start();

    /**
     * \brief Whether the server has been started
     */
    bool started() const;

    /**
     * \brief Stops the background threads
     */
    void stop();

    /**
     * \brief Maximum size of a request body to be accepted
     */
    std::size_t max_request_body() const;

    void set_max_request_body(std::size_t size);

    /**
     * \brief Function handling requests
     */
    virtual void respond(ClientConnection& connection) = 0;

    /**
     * \brief Writes a single log item into \p output
     */
    virtual void process_log_format(
        char label,
        const std::string& argument,
        const ClientConnection& connection,
        std::ostream& output
    ) const;

private:
    struct Data;
    std::unique_ptr<Data> data;
};

} // namespace muhttpd
#endif // MUHTTPD_SERVER_HPP
