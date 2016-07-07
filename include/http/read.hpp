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

#ifndef HTTPONY_HTTP_PARSER_HPP
#define HTTPONY_HTTP_PARSER_HPP

#include <istream>

#include "request.hpp"
#include "response.hpp"

namespace httpony {
/**
 * \brief Namespace with utilities that implement the HTTP text-based protocol
 */
namespace http {
/**
 * \brief Namespace with functions to read HTTP from an input stream
 */
namespace read {

enum HttpParserFlag
{
    /// Whether to accept (and parse) requests containing folded headers
    ParseFoldedHeaders  = 0x001,
    /// Whether to parse Cookie header into request.cookie
    /// If not set, cookies will still be accessible as headers
    ParseCookies        = 0x002,

    ParseDefault        = ParseCookies,
};

using HttpParserFlags = unsigned;


/**
 * \brief Reads a full request object from the stream
 * \param stream    Input stream
 * \param request   Request object to update
 * \param parse_folded_headers
 * \return Recommended status code
 * \note If it returns an error code, it's likely the request contain only
 *       partially parsed data
 */
Status request(std::istream& stream, Request& request, HttpParserFlags flags = ParseDefault);

/**
 * \brief Reads a string delimited by a specific character and ignores following spaces
 * \param stream Input to read from
 * \param output Output string
 * \param delim  Delimiting character
 * \param at_end Whether the value can be at the end of the line
 * \returns \b true on success
 */
bool delimited(std::istream& stream, std::string& output,
                    char delim = ':', bool at_end = false);

/**
 * \brief Skips spaces (except \r)
 * \param stream Input to read from
 * \param at_end Whether the value can be at the end of the line
 * \returns \b true on success
 */
bool skip_spaces(std::istream& stream, bool at_end = false);

/**
 * \brief Ignores all before "\n"
 */
void skip_line(std::istream& stream);

/**
 * \brief Reads all headers and the empty line following them
 * \param stream        Input stream
 * \param headers       Header container to update
 * \param parse_folded  Whether to parse or reject folded headers
 * \returns \b true on success
 */
bool headers(std::istream& stream, httpony::Headers& headers, bool parse_folded = false);

/**
 * \brief Reads a "quoted" header value
 * \returns \b true on success
 */
bool quoted_header_value(std::istream& stream, std::string& value);

/**
 * \brief Reads the request line (GET /url HTTP/1.1)
 * \param stream    Input stream
 * \param request   Request object to update
 * \returns \b true on success
 */
bool request_line(std::istream& stream, Request& request);

} // namespace read
} // namespace http
} // namespace httpony
#endif // HTTPONY_HTTP_PARSER_HPP
