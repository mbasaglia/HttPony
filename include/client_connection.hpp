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
 *
 * Implements most of the HTTP protocol in parsing requests and sending responses.
 *
 * Server implementation must still generate valid responses that conform to the
 * semantics as described by the protocol, this class will simply format the
 * response correctly and add headers whose value can be determined by the
 * response object.
 *
 * \see https://tools.ietf.org/html/rfc7230
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

    void send_response(Response& resp);

    void send_response(Response&& resp)
    {
        send_response(resp);
    }

    /**
     * \brief Sends the response to the socket
     */
    void send_response()
    {
        send_response(response);
    }


    Request request;
    Response response;
    IPAddress local;
    IPAddress remote;
    Status status = StatusCode::OK;
    NetworkBuffer input;
    Server* const server;
    /// Whether to accept (and parse) requests containing folded headers
    bool parse_folded_headers = false;
    /// Whether to parse Cookie header into request.cookie
    /// If false, cookies will be accessible as headers
    bool parse_cookies = true;

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
     * \brief Reads a string delimited by a specific character and ignores following spaces
     * \param stream Input to read from
     * \param output Output string
     * \param delim  Delimiting character
     * \param at_end Whether the value can be at the end of the line
     * \returns \b true on success
     */
    bool read_delimited(std::istream& stream, std::string& output,
                        char delim = ':', bool at_end = false);

    /**
     * \brief Skips spaces (except \r)
     * \param stream Input to read from
     * \param at_end Whether the value can be at the end of the line
     * \returns \b true on success
     */
    bool skip_spaces(std::istream& stream, bool at_end = false);

    /**
     * \brief Reads a "quoted" header value
     * \returns \b true on success
     */
    bool read_quoted_header_value(std::istream& buffer_stream, std::string& value);

    template<class String, class Value>
        void write_header(std::ostream& stream, String&& name, Value&& value)
    {
        stream << std::forward<String>(name) << ": " << std::forward<Value>(value) << "\r\n";
    }

    bool read_cookie(std::istream& stream);
};


} // namespace httpony
#endif // HTTPONY_CLIENT_CONNECTION_HPP
