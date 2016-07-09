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
#include "response.hpp"
#include "http/write.hpp"

namespace httpony {

static Response authorization_required(const std::vector<AuthChallenge>& challenges)
{
    Response response(StatusCode::Unauthorized);
    std::string www_authenticate;
    for ( const auto& challenge : challenges )
    {
        if ( !www_authenticate.empty() )
            www_authenticate += ", ";
        www_authenticate += http::write::auth_challenge(challenge);
    }
    response.headers["WWW-Authenticate"] = www_authenticate;
    return response;
}

} // namespace httpony
