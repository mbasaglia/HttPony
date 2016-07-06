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
#include "multipart.hpp"

namespace httpony {

/// \todo Maybe this could use a parser factory based on mime-type
bool Request::can_parse_post() const
{
    return body.has_data() && (
        body.content_type().matches_type("application", "x-www-form-urlencoded") ||
        body.content_type().matches_type("multipart", "form-data")
    );
}


bool Request::parse_post()
{
    if ( !body.has_data() )
        return false;

    if ( body.content_type().matches_type("application", "x-www-form-urlencoded") )
    {
        post = parse_query_string(body.read_all());
        return true;
    }
    /// \see https://tools.ietf.org/html/rfc2388
    else if ( body.content_type().matches_type("multipart", "form-data") )
    {
        if ( body.content_type().parameter().first != "boundary" )
            return false;

        Multipart form_data;
        form_data.boundary = body.content_type().parameter().second;
        if ( !multipart::read_multipart(body, form_data) )
            return false;

        for ( const auto& part : form_data.parts )
        {
            CompoundHeader disposition(part.headers.get("content-disposition"));
            if ( disposition.value != "form-data" || !disposition.parameters.contains("name") )
                return false;
            post.append(disposition.parameters["name"], part.content);
        }

        return true;

    }
    /// \todo multipart/mixed ?

    return false;
}


} // namespace httpony
