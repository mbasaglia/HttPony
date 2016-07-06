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

#ifndef HTTPONY_HEADERS_HPP
#define HTTPONY_HEADERS_HPP

#include <melanolib/data_structures/ordered_multimap.hpp>
#include <melanolib/string/quickstream.hpp>

namespace httpony {

using Headers = melanolib::OrderedMultimap<std::string, std::string, melanolib::ICaseComparator>;
using DataMap = melanolib::OrderedMultimap<>;

struct CompoundHeader
{
    /// \todo Can share code with MimeType
    CompoundHeader(const std::string& value)
    {
        melanolib::string::QuickStream stream(value);
        this->value = stream.get_until(
            [](char c){ return std::isspace(c) || c ==';'; }
        );

        while ( !stream.eof() )
        {
            stream.get_until(
                [](char c){ return !std::isspace(c) && c != ';'; },
                false
            );

            std::string param_name = stream.get_line('=');
            std::string param_value;

            if ( stream.peek() == '"' )
            {
                stream.ignore(1);
                param_value = stream.get_line('"');
            }
            else
            {
                param_value = stream.get_until(
                    [](char c){ return std::isspace(c) || c ==';'; }
                );
            }
            parameters.append(param_name, param_value);
        }
    }

    std::string value;
    Headers parameters;
};

} // namespace httpony
#endif // HTTPONY_HEADERS_HPP
