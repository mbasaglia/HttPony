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
 *
 */

#include "client_connection.hpp"

namespace muhttpd {


void ClientConnection::read_body(std::size_t max_size)
{
    /// \todo Parse urlencoded and multipart/form-data into request.post
    request.body = Content(
        NetworkInputStream(*this, max_size).read_all(),
        request.headers["Content-Type"]
    );
}

NetworkInputStream::NetworkInputStream(ClientConnection& connection, std::size_t max_size)
    : std::istream(&connection.input)
{
    /// \todo handle Transfer-Encoding
    /// \todo Functionality to read it asyncronously
    std::string length = connection.request.headers.get("Content-Length");
    std::string content_type = connection.request.headers.get("Content-Type");

    if ( length.empty() || !std::isdigit(length[0]) || content_type.empty() )
        return;

    _content_length = std::stoul(length);
    _content_type = std::move(content_type);

    if ( max_size && _content_length > max_size )
    {
        connection.status_code = StatusCode::PayloadTooLarge;
        return;
    }

    connection.input.expect_input(_content_length);

    _ok = true;
}

} // namespace muhttpd
