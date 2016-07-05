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

#include "request.hpp"
#include "uri.hpp"

namespace httpony {

/// \todo Maybe this could use a parser factory based on mime-type
bool Request::can_parse_post() const
{
    return body.has_data() &&
        body.content_type() == MimeType("application/x-www-form-urlencoded")
    ;
}

bool Request::parse_post()
{
    if ( !body.has_data() )
        return false;

    /// \todo Parse urlencoded and multipart/form-data into request.post
    if ( body.content_type() == MimeType("application/x-www-form-urlencoded") )
    {
        post = parse_query_string(body.read_all());
        return true;
    }

    return false;
}


} // namespace httpony
