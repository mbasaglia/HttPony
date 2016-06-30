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
        MHD_USE_SELECT_INTERNALLY,
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

} // namespace muhttpd
