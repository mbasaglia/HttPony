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
#include <melanolib/string/ascii.hpp>

namespace httpony {

using Headers = melanolib::OrderedMultimap<std::string, std::string, melanolib::ICaseComparator>;
using DataMap = melanolib::OrderedMultimap<>;


/**
 * \brief Reads header parameters (param1=foo param2=bar)
 * \param stream    Input stream
 * \param output    Container to update
 * \returns \b true on success
 */
template<class OrderedContainer>
    bool parse_header_parameters(
    melanolib::string::QuickStream& stream,
    OrderedContainer& output
)
{
    using melanolib::string::ascii::is_space;

    auto is_boundary_char = [](char c){ return is_space(c) || c == ';'; };

    while ( !stream.eof() )
    {
        stream.ignore_if(is_boundary_char);

        std::string param_name = stream.get_line('=');
        std::string param_value;

        if ( stream.peek() == '"' )
        {
            stream.ignore(1);
            bool last_slash = false;
            while ( true )
            {
                auto c = stream.next();
                if ( stream.eof() && (c != '"' || last_slash) )
                    return false;

                if ( !last_slash )
                {
                    if ( c == '"'  )
                        break;
                    if ( c == '\\' )
                    {
                        last_slash = true;
                        continue;
                    }
                }
                else
                {
                    last_slash = false;
                }

                param_value += c;
            }
        }
        else
        {
            param_value = stream.get_until(is_boundary_char);
        }
        output.append(param_name, param_value);
    }
    return true;
}

struct CompoundHeader
{
    CompoundHeader(const std::string& header_value)
    {
        melanolib::string::QuickStream stream(header_value);
        value = stream.get_until(
            [](char c){ return std::isspace(c) || c ==';'; }
        );
        parse_header_parameters(stream, parameters);
    }

    std::string value;
    Headers parameters;
};

} // namespace httpony
#endif // HTTPONY_HEADERS_HPP
