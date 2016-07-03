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

#ifndef HTTPONY_CLIENT_CONNECTION_HPP
#define HTTPONY_CLIENT_CONNECTION_HPP

#include "request.hpp"
#include "response.hpp"

namespace httpony {

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
    void read_request();

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
    void send_response();


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
    void skip_line(std::istream& buffer_stream);

    /**
     * \brief Reads the request line (GET /url HTTP/1.1)
     * \returns \b true on success
     */
    bool read_request_line(std::istream& buffer_stream);

    /**
     * \brief Reads all headers and the empty line following them
     * \returns \b true on success
     */
    bool read_headers(std::istream& buffer_stream);

    /**
     * \brief Reads a header name and the following colon, including optional spaces
     * \returns \b true on success
     */
    bool read_header_name(std::istream& buffer_stream, std::string& name);

    /**
     * \brief Reads a "quoted" header value
     * \returns \b true on success
     */
    bool read_quoted_header_value(std::istream& buffer_stream, std::string& value);

};


} // namespace httpony
#endif // HTTPONY_CLIENT_CONNECTION_HPP
