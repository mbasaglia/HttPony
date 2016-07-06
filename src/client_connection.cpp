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

#include "client_connection.hpp"

#include <melanolib/time/time_string.hpp>

namespace httpony {

void ClientConnection::read_request()
{
    boost::system::error_code error;

    request = Request();
    request.from = remote;

    /// \todo Avoid magic number, keep reading if needed
    auto sz = input.read_some(1024, error);

    status = StatusCode::BadRequest;

    if ( error || sz == 0 )
        return;

    std::istream stream(&input);
    status = httpony::read_request(stream, request, parse_flags);
    response.protocol = request.protocol;
    request.from = remote;
}

void ClientConnection::send_response(Response& response)
{
    boost::asio::streambuf buffer_write;
    std::ostream buffer_stream(&buffer_write);
    buffer_stream << response.protocol << ' '
                    << response.status.code << ' '
                    << response.status.message << "\r\n";

    write_header(buffer_stream, "Date", melanolib::time::strftime(response.date, "%r GMT"));

    for ( const auto& header : response.headers )
        write_header(buffer_stream, header.first, header.second);

    for ( const auto& cookie : response.cookies )
        write_header(buffer_stream, "Set-Cookie", cookie);


    if ( response.body.has_data() )
    {
        write_header(buffer_stream, "Content-Type", response.body.content_type());
        write_header(buffer_stream, "Content-Length", response.body.content_length());
    }

    buffer_stream << "\r\n";
    boost::asio::write(input.socket(), buffer_write);

    if ( response.body.has_data() )
        response.body.net_write(input.socket());
}

} // namespace httpony
