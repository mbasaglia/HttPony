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

#include "httpony/mime_type.hpp"

#include "httpony/http/format/read.hpp"


namespace httpony {


MimeType::MimeType(const std::string& string)
{
    melanolib::string::QuickStream stream(string);
    set_type(stream.get_line('/'));
    set_subtype(stream.get_until(
        [](char c){ return melanolib::string::ascii::is_space(c) || c ==';'; }
    ));

    Headers parameters;
    http::read::header_parameters(stream, parameters);
    if ( !parameters.empty() )
        set_parameter(parameters.front());
}

} // namespace httpony
