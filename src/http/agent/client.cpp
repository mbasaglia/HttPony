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

#include "httpony/http/agent/client.hpp"
#include "httpony/http/formatter.hpp"

namespace httpony {


Response Client::get_response(io::Connection& connection, Request&& request) const
{
    process_request(request);
    auto stream = connection.send_stream();
    http::Http1Formatter().request(stream, request);
    stream.send();
    /// \todo Report send() errors
    /// \todo Follow redirects
    return connection.read_response();
}

} // namespace httpony
