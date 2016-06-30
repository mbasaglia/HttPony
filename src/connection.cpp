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
#include<algorithm>

#include <arpa/inet.h>

#include <microhttpd.h>

#include "muhttpd.hpp"


namespace muhttpd {

struct Server::Data
{
    Data(Server* owner, uint16_t port)
        : owner(owner), port(port), max_request_body(std::string().max_size())
    {}

    Server* owner;
    uint16_t port;
    struct MHD_Daemon* daemon = nullptr;
    Request request;
    std::size_t max_request_body;
};

/**
 * \brief Gathers post key/value pairs
 */
static int collect_post(
    void *cls,
    MHD_ValueKind kind,
    const char *key,
    const char *filename,
    const char *content_type,
    const char *transfer_encoding,
    const char *data,
    uint64_t off,
    size_t size
)
{
    auto request = reinterpret_cast<Request*>(cls);
    std::string string_data = std::string(data, data+size);
    request->post.append(key, string_data);
    return MHD_YES;
}


/**
 * \brief Populates an IPAddress object from a sockaddr
 */
static IPAddress address(const sockaddr *addr)
{
    IPAddress result;
    void* source = nullptr;
    if ( addr->sa_family == AF_INET )
    {
        auto in = (sockaddr_in*)addr;
        source = &in->sin_addr;
        result.port = in->sin_port;
        result.type = IPAddress::Type::IPv4;
    }
    else if ( addr->sa_family == AF_INET6 )
    {
        auto in = (sockaddr_in6*) addr;
        source = &in->sin6_addr;
        result.port = in->sin6_port;
        result.type = IPAddress::Type::IPv6;
    }
    else
    {
        return {};
    }

    char buffer[40];
    if ( inet_ntop(addr->sa_family, source, buffer, sizeof(buffer)) )
        result.string = buffer;
    return result;
}

/**
 * \brief Callback used to retrieve headers
 * \param cls A pointer to a Headers object
 */
static int collect_headers(void *cls, MHD_ValueKind kind, const char *key, const char *value)
{
    auto headers = reinterpret_cast<Headers*>(cls);
    headers->append(key, value);
    return MHD_YES;
}

/**
 * \brief Sends a Response object to a Microhttpd connection
 */
static int send(const Response& resp, MHD_Connection* connection)
{
    MHD_Response *response;
    int ret;
    response = MHD_create_response_from_buffer(
        resp.body.size(),
        (void*)resp.body.data(),
        MHD_RESPMEM_MUST_COPY
    );

    for ( const auto& head : resp.headers )
        MHD_add_response_header(response, head.name.c_str(), head.value.c_str());

    ret = MHD_queue_response(connection, resp.status_code, response);
    MHD_destroy_response(response);
    return ret;
}

/**
 * \brief Callback for receiving a request
 * \param cls A pointer to a Server object
 */
static int receive(
    void *cls,
    MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
)
{
    auto self = (Server::Data*)cls;

    Request& request = self->request;

    if ( request.method.empty() )
        request.method = method;

    if ( request.method == "POST" && self->max_request_body )
    {
        MHD_PostProcessor* post_processor = (MHD_PostProcessor*)*con_cls;

        if ( !post_processor )
        {
            // First call
            post_processor = MHD_create_post_processor(
                connection,
                Server::post_chunk_size,
                collect_post,
                (void*)&request
            );
            *con_cls = post_processor;
            if ( post_processor )
                return MHD_YES;
            self->owner->on_error(Server::ErrorCode::BadBody);
            return MHD_NO;
        }
        else if ( *upload_data_size )
        {
            // Chunk
            if ( request.body.size() + *upload_data_size > self->max_request_body )
            {
                self->owner->on_error(Server::ErrorCode::BadBody);
                return MHD_NO;
            }
            request.body += std::string(upload_data, upload_data + *upload_data_size);
            MHD_post_process(post_processor, upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        }
        else
        {
            // Last Call
            // NOTE: If a urlencoded body doesn't end with \n,
            // MHD will call the cursor again with the same key as the last
            // time and empty data...
            MHD_destroy_post_processor(post_processor);
        }
    }

    request.url = url;
    request.version = version;

    char* password = nullptr;
    char* user = MHD_basic_auth_get_username_password (connection, &password);
    if ( user )
    {
        request.auth.user = user;
        request.auth.password = password;
    }

    MHD_get_connection_values(connection, MHD_HEADER_KIND, &collect_headers, &request.headers);
    MHD_get_connection_values(connection, MHD_COOKIE_KIND, &collect_headers, &request.cookies);
    MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, &collect_headers, &request.get);

    const sockaddr *addr = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS)->client_addr;
    self->request.from = address(addr);

    return send(self->owner->respond(self->request), connection);
}

/**
 * \brief Callback called when a request has been completed
 * \param cls A pointer to a Server object
 */
static void request_completed(
    void *cls,
    MHD_Connection *connection,
    void **con_cls,
    MHD_RequestTerminationCode toe
)
{
    auto self = (Server::Data*)cls;
    self->request = Request();
}

/**
 * \brief Callback called to determine whether a connection should be accepted
 * \param cls A pointer to a Server object
 */
static int accept_policy(void *cls, const sockaddr *addr, socklen_t addrlen)
{
    auto self = (Server::Data*)cls;
    return self->owner->accept(address(addr)) ? MHD_YES : MHD_NO;
}

Server::Server(uint16_t port)
    : data(std::make_unique<Server::Data>(this, port))
{}

Server::~Server()
{
    stop();
}

uint16_t Server::port() const
{
    return data->port;
}

bool Server::started() const
{
    return data->daemon;
}

void Server::start()
{
    data->daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY | MHD_USE_DUAL_STACK,
        data->port,
        &accept_policy, data.get(),
        &receive, data.get(),
        MHD_OPTION_NOTIFY_COMPLETED, &request_completed, data.get(), NULL,
        MHD_OPTION_END
    );

    if ( !data->daemon )
        on_error(ErrorCode::StartupFailure);
}

void Server::stop()
{
    if ( data->daemon )
    {
        MHD_stop_daemon(data->daemon);
        data->daemon = nullptr;
    }
}


std::size_t Server::max_request_body() const
{
    return data->max_request_body;
}

void Server::set_max_request_body(std::size_t size)
{
    data->max_request_body = size;
}

void Server::log(
        const std::string& format,
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
        process_log_format(label, argument, request, response, output);

    }
    output << std::endl;
}


void Server::process_log_format(
        char label,
        const std::string& argument,
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
            // TODO
            break;
        case 'B': // Size of response in bytes, excluding HTTP headers.
            output << response.body.size();
            break;
        case 'b': // Size of response in bytes, excluding HTTP headers. In CLF format, i.e. a '-' rather than a 0 when no bytes are sent.
            output << clf(response.body.size());
            break;
        case 'C': // The contents of cookie Foobar in the request sent to the server.
            output << request.cookies[argument];
            break;
        case 'D': // The time taken to serve the request, in microseconds.
            // TODO
            break;
        case 'e': // The contents of the environment variable FOOBAR
            // TODO
            break;
        case 'f': // Filename
            // TODO
            break;
        case 'H': // The request protocol
            output << request.version;
            break;
        case 'i': // The contents of header line in the request sent to the server.
            /// \todo Handle multiple headers with the same name
            output << request.headers[argument];
            break;
        case 'k': // Number of keepalive requests handled on this connection.
            // TODO ?
            output << 0;
            break;
        case 'l': // Remote logname
            // TODO ?
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
            else
                output << data->port;
            break;
        case 'P': // The process ID or thread id of the child that serviced the request. Valid formats are pid, tid, and hextid.
            // TODO ?
            break;
        case 'q': // The query string (prepended with a ? if a query string exists, otherwise an empty string)
            // TODO
            break;
        case 'r': // First line of request
            /// \todo TODO check if this is correct (eg: query string)
            output << request.version << ' ' << request.method << request.url;
            break;
        case 'R': // The handler generating the response (if any).
            // TODO ?
            break;
        case 's': // Status
            output << int(response.status_code);
            break;
        case 't': // The time, in the format given by argument
            // TODO
            break;
        case 'T': // The time taken to serve the request, in a time unit given by argument (default seconds).
            // TODO
            break;
        case 'u': // Remote user (from auth; may be bogus if return status (%s) is 401)
            output << request.auth.user;
            break;
        case 'U': // The URL path requested, not including any query string.
            output << request.url;
            break;
        case 'v': // The canonical ServerName of the server serving the request.
            // TODO ?
            break;
        case 'V': // The server name according to the UseCanonicalName setting.
            // TODO ?
            break;
        case 'X': // Connection status when response is completed. X = aborted before response; + = Maybe keep alive; - = Close after response.
            // TODO
            output << '-';
            break;
    }
}

} // namespace muhttpd
