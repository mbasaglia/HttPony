/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2016 Mattia Basaglia
 * \section License
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
#ifndef HTTPONY_MULTIPART_HPP
#define HTTPONY_MULTIPART_HPP

#include <melanolib/string/ascii.hpp>
#include <melanolib/string/simple_stringutils.hpp>

#include "headers.hpp"

#include "http/read.hpp"

namespace httpony {

/// \see https://tools.ietf.org/html/rfc2046#section-5.1
struct Multipart
{
    struct Part
    {
        Headers headers;
        std::string content;
    };

    std::string boundary;
    std::vector<Part> parts;
};

namespace multipart {


inline bool valid_boundary(const std::string& boundary)
{
    if ( boundary.empty() )
        return false;
    for ( auto c : boundary )
        if ( !melanolib::string::ascii::is_print(c) )
            return false;
    if ( boundary.back() == ' ' )
        return false;
    return true;
}

enum class LineType
{
    Boundary,
    Data,
    LastBoundary
};

inline LineType line_type(const std::string& line, const std::string& boundary)
{
    if ( line.size() < boundary.size() + 2 || line[0] != '-' || line[1] != '-' )
        return LineType::Data;

    if ( !std::equal(line.begin() + 2, line.begin() + 2 + boundary.size(),
                     boundary.begin(), boundary.end()) )
        return LineType::Data;

    std::size_t space_start = boundary.size() + 2;
    LineType expected_result = LineType::Boundary;

    if ( line.size() >= space_start + 2 && line[space_start] == '-' && line[space_start+1] == '-' )
    {
        expected_result = LineType::LastBoundary;
        space_start += 2;
    }

    if ( std::all_of(line.begin() + space_start, line.end(), melanolib::string::ascii::is_blank) )
        return expected_result;

    return LineType::Data;
}

bool cleanup_boundary(std::istream& stream, Multipart& output)
{
    if ( stream.get() != '\n' )
        return false;

    if ( output.parts.empty() )
        return true;

    std::string& content = output.parts.back().content;

    if ( content.empty() )
        return true;

    if ( !melanolib::string::ends_with(content, "\r\n") )
        return false;

    content.resize(content.size() - 2);
    return true;
}

inline bool read_multipart(std::istream& stream, Multipart& output)
{
    if ( !valid_boundary(output.boundary) )
        return false;

    while ( true )
    {
        std::string line;
        std::getline(stream, line, '\r');
        if ( !stream )
            return false;

        auto type = line_type(line, output.boundary);

        if ( type == LineType::LastBoundary )
        {
            return cleanup_boundary(stream, output);
        }
        else if ( type == LineType::Boundary )
        {
            if ( !cleanup_boundary(stream, output) )
                return false;

            output.parts.emplace_back();
            if ( !http::read::headers(stream, output.parts.back().headers) )
                return false;
        }
        else if ( !output.parts.empty() )
        {
            output.parts.back().content += line + '\r';
            std::getline(stream, line, '\n');
            output.parts.back().content += line + '\n';
        }
        else
        {
            return false;
        }
    }
}


} // namespace multipart

} // namespace httpony
#endif // HTTPONY_MULTIPART_HPP
