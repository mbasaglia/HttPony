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

#include <iostream>
#include <algorithm>

#include "httpony.hpp"


class MyServer : public httpony::Server
{
public:
    using Server::Server;

    void respond(httpony::ClientConnection& connection) override
    {
        try
        {
            std::string body = get_body(connection);

            create_response(connection);

            print_info(connection, body);
        }
        catch ( const std::exception& )
        {
            // Create a server error response if an exception happened
            // while handling the request
            connection.response = httpony::Response();
            connection.status = httpony::StatusCode::InternalServerError;
            create_response(connection);
        }

        send_response(connection);;
    }

private:
    /**
     * \brief Returns the whole request body as a string
     */
    std::string get_body(httpony::ClientConnection& connection) const
    {
        // Handle HTTP/1.1 requests with an Expect: 100-continue header
        if ( connection.status == httpony::StatusCode::Continue )
        {
            connection.send_response(connection.status);
            connection.status = httpony::StatusCode::OK;
        }

        // Check if we have something to read
        if ( connection.request.body.has_data() )
        {
            std::string body = connection.request.body.read_all();

            // Handle read errors (eg: wrong Content-Length)
            if ( connection.request.body.error() )
                connection.status = httpony::StatusCode::BadRequest;

            return body;
        }

        return "";
    }

    /**
     * \brief Sets up the response object
     */
    void create_response(httpony::ClientConnection& connection) const
    {
        // The response is in plain text
        connection.response.body.start_data("text/plain");

        if ( connection.ok() )
        {
            // On success, send Hello world
            connection.response.body << "Hello, world!\n";
        }
        else
        {
            // On failure send the error message
            connection.response.status = connection.status;
            connection.response.body << connection.response.status.message << '\n';
        }

        // We are not going to keep the connection open
        if ( connection.response.protocol == httpony::Protocol("HTTP", 1, 1) )
        {
            connection.response.headers["Connection"] = "close";
        }
    }

    /**
     * \brief Prints out dictionary data
     */
    template<class Map>
        void show_headers(const std::string& title, const Map& headers) const
    {
        std::cout << title << ":\n";
        for ( const auto& head : headers )
            std::cout << '\t' << head.name << " : " << head.value << '\n';
    }

    /**
     * \brief Logs info on the current request/response
     */
    void print_info(httpony::ClientConnection& connection, std::string& body) const
    {
        std::cout << '\n';
        log(log_format, connection, std::cout);

        show_headers("Headers", connection.request.headers);
        show_headers("Cookies", connection.request.cookies);
        show_headers("Get", connection.request.get);
        show_headers("Post", connection.request.post);

        if ( connection.request.body.has_data() )
        {
            std::replace_if(body.begin(), body.end(), [](char c){return c < ' ' && c != '\n';}, ' ');
            std::cout << '\n' << body << '\n';
        }
    }

    /**
     * \brief Send the connection to the client
     */
    void send_response(httpony::ClientConnection& connection) const
    {
        // This removes the response body when mandated by HTTP
        connection.clean_response_body();
        connection.send_response();
    }


    std::string log_format = "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";
};

int main()
{
    MyServer server(8888);

    server.start();

    std::cout << "Server started, hit enter to quit\n";
    std::cin.get();

    return 0;
}
