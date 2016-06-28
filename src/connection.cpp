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

Request& _request(Server* sv)
{
    return sv->request;
}

static int collect_header(
    void *cls,
    MHD_ValueKind kind,
    const char *key,
    const char *value
)
{
    auto headers = reinterpret_cast<Headers*>(cls);
    headers->append(key, value);
    return MHD_YES;
}

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
    auto self = (Server*)cls;

    Request& request = _request(self);
    request.url = url;
    request.method = method;
    request.version = version;
    request.body = std::string(upload_data, upload_data + *upload_data_size);

    char* password = nullptr;
    char* user = MHD_basic_auth_get_username_password (connection, &password);
    if ( user )
    {
        request.auth.user = user;
        request.auth.password = password;
    }

    MHD_get_connection_values(connection, MHD_HEADER_KIND, &collect_header, &request.headers);

    return send(self->respond(request), connection);
}

static void request_completed(
    void *cls,
    MHD_Connection *connection,
    void **con_cls,
    MHD_RequestTerminationCode toe
)
{
    auto self = (Server*)cls;
    _request(self) = Request();
}

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

static int on_client_connect (void *cls, const sockaddr *addr, socklen_t addrlen)
{
    auto self = (Server*)cls;
    _request(self) = Request();
    _request(self).from = address(addr);
    return self->accept(_request(self).from) ? MHD_YES : MHD_NO;
}

void Server::start()
{
    daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY,
        _port,
        &on_client_connect, this,
        &receive, this,
        MHD_OPTION_NOTIFY_COMPLETED, &request_completed, this, NULL,
        MHD_OPTION_END
    );

    if ( !daemon )
        throw std::runtime_error("Could not start the server");

}


void Server::stop()
{
    if ( daemon )
    {
        MHD_stop_daemon(daemon);
        daemon = nullptr;
    }
}

} // namespace muhttpd
