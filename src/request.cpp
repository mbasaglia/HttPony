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
#include "base_encoding.hpp"

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

        Multipart form_data(body.content_type().parameter().second);
        if ( !form_data.read(body) )
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

Auth::Auth(const std::string& header_contents)
{
    melanolib::string::QuickStream stream(header_contents);
    auth_scheme = stream.get_until(melanolib::string::ascii::is_space);
    stream.ignore_if(melanolib::string::ascii::is_space);
    auth_string = stream.get_until(melanolib::string::ascii::is_space);
    parse_header_parameters(stream, parameters);
    realm = parameters.get("realm");
    parameters.erase("realm");

    if ( auth_scheme == "Basic" )
    {
        stream.str(Base64().decode(auth_string));
        user = stream.get_line(':');
        password = stream.get_remaining();
    }
}

} // namespace httpony
