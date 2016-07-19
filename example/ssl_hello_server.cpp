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

#include "httpony/ssl/ssl.hpp"

/**
 * \brief Simple example server
 *
 * This server logs the contents of incoming requests to stdout and
 * returns simple "Hello World" responses to the client
 */
class SslHelloServer : public httpony::ssl::SslServer
{
public:
    explicit SslHelloServer(
        httpony::io::ListenAddress listen,
        const std::string& cert_file,
        const std::string& key_file,
        const std::string& dh_file = {}
    )
        : SslServer(listen, cert_file, key_file, dh_file)
    {
        set_timeout(melanolib::time::seconds(16));
    }

    void respond(httpony::io::Connection& connection, httpony::Request&& request) override
    {
        try
        {
            std::string body = get_body(connection, request);

            httpony::Response response = send_response(connection, request);

            print_info(connection, request, response, body);
        }
        catch ( const std::exception& )
        {
            // Create a server error response if an exception happened
            // while handling the request
            request.suggested_status = httpony::StatusCode::InternalServerError;
            httpony::Response response = send_response(connection, request);

            log_response(log_format, connection, request, response, std::cout);
        }
    }

private:
    /**
     * \brief Returns the whole request body as a string
     */
    std::string get_body(httpony::io::Connection& connection, httpony::Request& request) const
    {
        // Discard requests with a too long of a payload
        if ( request.body.content_length() > max_size )
        {
            request.suggested_status = httpony::StatusCode::PayloadTooLarge;
            return "";
        }

        // Handle HTTP/1.1 requests with an Expect: 100-continue header
        if ( request.suggested_status == httpony::StatusCode::Continue )
        {
            connection.send_response(request.suggested_status);
            request.suggested_status = httpony::StatusCode::OK;
        }

        // Parse form data
        if ( request.can_parse_post() )
        {
            if ( !request.parse_post() )
                request.suggested_status = httpony::StatusCode::BadRequest;

            return "";
        }

        // Check if we have something to read
        if ( request.body.has_data() )
        {
            std::string body = request.body.read_all();

            // Handle read errors (eg: wrong Content-Length)
            if ( request.body.has_error() )
                request.suggested_status = httpony::StatusCode::BadRequest;

            return body;
        }

        return "";
    }

    /**
     * \brief Sets up the response object and sends it to the client
     */
    httpony::Response send_response(httpony::io::Connection& connection, const httpony::Request& request) const
    {
        httpony::Response response(request);

        // The response is in plain text
        response.body.start_data("text/plain");

        if ( !response.status.is_error() )
        {
            // On success, send Hello world
            response.body << "Hello, world!\n";
        }
        else
        {
            // On failure send the error message
            response.body << response.status.message << '\n';
        }

        // We are not going to keep the connection open
        if ( response.protocol >= httpony::Protocol::http_1_1 )
        {
            response.headers["Connection"] = "close";
        }

        // Ensure the response isn't cached
        response.headers["Expires"] = "0";

        // This removes the response body when mandated by HTTP
        response.clean_body(request);

        // Drop the connection if there is some network error on the response
        if ( !connection.send_response(response) )
            connection.close();

        return response;
    }

    /**
     * \brief Prints out dictionary data
     */
    template<class Map>
        void show_headers(const std::string& title, const Map& data) const
    {
        std::cout << title << ":\n";
        for ( const auto& item : data )
            std::cout << '\t' << item.first << " : " << item.second << '\n';
    }

    /**
     * \brief Logs info on the current request/response
     */
    void print_info(httpony::io::Connection& connection,
                    const httpony::Request& request,
                    const httpony::Response& response,
                    std::string& body) const
    {
        std::cout << '\n';
        log_response(log_format, connection, request, response, std::cout);

        show_headers("Headers", request.headers);
        show_headers("Cookies", request.cookies);
        show_headers("Get", request.get);
        show_headers("Post", request.post);

        if ( request.body.has_data() )
        {
            std::replace_if(body.begin(), body.end(), [](char c){return c < ' ' && c != '\n';}, ' ');
            std::cout << '\n' << body << '\n';
        }
    }


    std::string log_format = "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";
    std::size_t max_size = 8192;
};

/**
 * The executable accepts the following (optional) arguments:
 *  port
 *      Port number to listen to (default=8083)
 *  cert_file
 *      Certificate file in PEM format (default=server.pem)
 *  key_file
 *      Private key file in PEM format (default=server.key)
 *  dh_file
 *      Diffie-Hellman paramaters file in PEM format (default=empty)
 *
 * To generate the various PEM files you can use the following commands:
 *
 * Generate RSA key:
 *      openssl genrsa -out server.key 1024
 *
 * Generate self-signed certificate from the key file:
 *      openssl req -days 365 -out server.pem -new -x509 -key server.key
 *
 * (Optional) Generate DH parameters:
 *      openssl dhparam -out dh512.pem 512
 *
 * Since these files are in the PEM format, you can also concatenate them and
 * keep a single file that will work for all three
 */
int main(int argc, char** argv)
{
    uint16_t port = 8083;
    std::string cert_file = "server.pem";
    std::string key_file = "server.key";
    std::string dh_file;

    if ( argc > 1 )
        port = std::stoul(argv[1]);

    if ( argc > 2 )
        cert_file = argv[2];

    if ( argc > 3 )
        key_file = argv[3];

    if ( argc > 4 )
        dh_file = argv[4];

    // This creates a server that listens to both IPv4 and IPv6
    // on the given port
    SslHelloServer server(port, cert_file, key_file, dh_file);

    // This starts the server on a separate thread
    server.start();

    std::cout << "Server started on port "<< server.listen_address().port << ", hit enter to quit\n";
    // Pause the main thread listening to standard input
    std::cin.get();
    std::cout << "Server stopped\n";

    // The MyServer destructor will stop the server and join the thread
    return 0;
}
