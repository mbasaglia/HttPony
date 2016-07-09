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

#include <thread>

#include <melanolib/time/time_string.hpp>

#include "server.hpp"
#include "logging.hpp"
#include "connection.hpp"


namespace httpony {

struct Server::Data
{
    Data(Server* owner, IPAddress listen)
        : owner(owner), listen(listen), max_request_body(std::string().max_size())
    {}

    virtual ~Data(){}

    Server* owner;
    IPAddress listen;
    std::size_t max_request_body;

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

                /// \todo Keep the connection alive if needed
                Connection connection(std::move(socket));
                if ( !error_code )
                {
                    owner->respond(connection, connection.read_request());
                }
                /// \todo Log error if error_code

                accept();
            }
        );
    }
};

Server::Server(IPAddress listen)
    : data(std::make_unique<Data>(this, listen))
{}

Server::Server(uint16_t port)
    : Server(IPAddress(IPAddress::Type::IPv4, "0.0.0.0", port))
{}

Server::~Server()
{
    stop();
}

IPAddress Server::listen_address() const
{
    return data->listen;
}

std::size_t Server::max_request_body() const
{
    return data->max_request_body;
}

void Server::set_max_request_body(std::size_t size)
{
    data->max_request_body = size;
}

void Server::process_log_format(
        char label,
        const std::string& argument,
        const Connection& connection,
        const Request& request,
        const Response& response,
        std::ostream& output
    ) const
{
    // Formats as from the Apache docs, not all formats are supported (Yet)
    /// \todo Check which ones should use clf
    switch ( label )
    {
        case '%': // Escaped %
            output << '%';
            break;
        case 'h': // Remote host
        case 'a': // Remote IP-address
            output << request.from.string;
            break;
        case 'A': // Local IP-address
            output << connection.local_address().string;
            break;
        case 'B': // Size of response in bytes, excluding HTTP headers.
            output << response.body.content_length();
            break;
        case 'b': // Size of response in bytes, excluding HTTP headers. In CLF format, i.e. a '-' rather than a 0 when no bytes are sent.
            output << clf(response.body.content_length());
            break;
        case 'C': // The contents of cookie Foobar in the request sent to the server.
            output << request.cookies[argument];
            break;
        case 'D': // The time taken to serve the request, in microseconds.
            std::cout << std::chrono::duration_cast<std::chrono::microseconds>(
                response.date - request.received_date
            ).count();
            break;
        case 'e': // The contents of the environment variable FOOBAR
            // TODO
            break;
        case 'f': // Filename
            // TODO
            break;
        case 'H': // The request protocol
            output << request.protocol;
            break;
        case 'i': // The contents of header line in the request sent to the server.
            /// \todo Handle multiple headers with the same name
            output << request.headers[argument];
            break;
        case 'k': // Number of keepalive requests handled on this connection.
            // TODO ?
            output << 0;
            break;
        case 'l': // Remote logname (something to do with Apache modules)
            output << '-';
            break;
        case 'm':
            output << request.method;
            break;
        case 'o': // The contents of header line(s) in the reply.
            /// \todo Handle multiple headers with the same name
            output << response.headers[argument];
            break;
        case 'p':
            if ( argument == "remote" )
                output << request.from.port;
            else if ( argument == "local" )
                output << connection.local_address().port;
            else // canonical
                output << data->listen.port;
            break;
        case 'P': // The process ID or thread id of the child that serviced the request. Valid formats are pid, tid, and hextid.
            // TODO ?
            break;
        case 'q': // The query string (prepended with a ? if a query string exists, otherwise an empty string)
            output << request.url.query_string(true);
            break;
        case 'r': // First line of request
            output << request.method << ' ' << request.url.full() << ' ' << request.protocol;
            break;
        case 'R': // The handler generating the response (if any).
            // TODO ?
            break;
        case 's': // Status
            output << response.status.code;
            break;
        case 't': // The time, in the format given by argument
            output << melanolib::time::strftime(melanolib::time::DateTime(), argument);
            break;
        case 'T': // The time taken to serve the request, in a time unit given by argument (default seconds).
        {
            auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
                response.date - request.received_date
            ).count();
            if ( argument == "ms" )
                microseconds /= 1000;
            else if ( argument != "us" )
                microseconds /= 1'000'000;
            output << microseconds;
            break;
        }
        case 'u': // Remote user (from auth; may be bogus if return status (%s) is 401)
            output << request.auth.user;
            break;
        case 'U': // The URL path requested, not including any query string.
            output << request.url.path_string();
            break;
        case 'v': // The canonical ServerName of the server serving the request.
            // TODO ?
            break;
        case 'V': // The server name according to the UseCanonicalName setting.
            // TODO ?
            break;
        case 'X': // Connection status when response is completed. X = aborted before response; + = Maybe keep alive; - = Close after response.
            // TODO keep alive?
            output << (request.suggested_status.is_error() ? 'X' : '-');
            break;
    }
}

bool Server::started() const
{
    return data->thread.joinable();
}

void Server::start()
{
    boost::asio::ip::tcp::resolver resolver(data->io_service);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({
        data->listen.string,
        std::to_string(data->listen.port)
    });
    data->acceptor.open(endpoint.protocol());
    data->acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    data->acceptor.bind(endpoint);
    data->acceptor.listen();
    data->thread = std::thread([this](){
        data->accept();
        data->io_service.run();
    });
}

void Server::stop()
{
    if ( started() )
    {
        /// \todo Needs locking?
        data->acceptor.close();
        data->io_service.stop();
        data->thread.join();
    }
}

void Server::log_response(
        const std::string& format,
        const Connection& connection,
        const Request& request,
        const Response& response,
        std::ostream& output
    ) const
{
    auto start = format.begin();
    auto finish = format.begin();
    while ( start < format.end() )
    {
        finish = std::find(start, format.end(), '%');
        output << std::string(start, finish);

        // No more % (clean exit)
        if ( finish == format.end() )
            break;

        start = finish + 1;
        // stray % at the end
        if ( start == format.end() )
            break;

        char label = *start;
        start++;
        std::string argument;

        if ( label == '{' )
        {
            finish = std::find(start, format.end(), '}');
            // Unterminated %{
            if ( finish + 1 >= format.end() )
                break;
            argument = std::string(start, finish);
            start = finish + 1;
            label = *start;
            start++;
        }

        process_log_format(label, argument, connection, request, response, output);

    }
    output << std::endl;
}

} // namespace httpony
