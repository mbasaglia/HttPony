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

#include "httpony/multipart.hpp"

#include "httpony/http/format/read.hpp"

namespace httpony {

bool Multipart::valid_boundary(const std::string& boundary)
{
    if ( boundary.empty() )
        return false;

    /// \note This is a bit more permissive than it should
    for ( auto c : boundary )
        if ( !melanolib::string::ascii::is_print(c) )
            return false;

    if ( boundary.back() == ' ' )
        return false;

    return true;
}

enum class LineType
{
    Boundary,       ///< A --boundary line
    LastBoundary,   ///< A --boundary-- line (Marking the end of multipart data)
    Data,           ///< A non-boundary line
};

/**
 * \brief Scans the input line to determine its type
 */
static LineType line_type(const std::string& line, const std::string& boundary)
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

/**
 * \brief Cleans up after a boundary line has been found
 *
 * The \r\n found before a boundary are considered part of the boundary line so
 * they need to be stripped from the data.
 */
static bool cleanup_boundary(std::istream& stream, Multipart& output)
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

bool Multipart::read(std::istream& stream)
{
    if ( !valid_boundary(boundary) )
        return false;

    while ( true )
    {
        std::string line;
        std::getline(stream, line, '\r');
        if ( !stream )
            return false;

        auto type = line_type(line, boundary);

        if ( type == LineType::LastBoundary )
        {
            return cleanup_boundary(stream, *this);
        }
        else if ( type == LineType::Boundary )
        {
            if ( !cleanup_boundary(stream, *this) )
                return false;

            parts.emplace_back();
            if ( !http::read::headers(stream, parts.back().headers) )
                return false;
        }
        else if ( !parts.empty() )
        {
            parts.back().content += line + '\r';
            std::getline(stream, line, '\n');
            parts.back().content += line + '\n';
        }
        else
        {
            return false;
        }
    }
}

} // namespace httpony
