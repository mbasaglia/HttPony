/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright  Mattia Basaglia
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
#ifndef HTTPONY_HTTP_POST_FORM_DATA_HPP
#define HTTPONY_HTTP_POST_FORM_DATA_HPP

#include "post.hpp"
#include "httpony/uri.hpp"
#include "httpony/multipart.hpp"
#include "httpony/http/formatter.hpp"
#include "httpony/http/parser.hpp"

namespace httpony {
namespace post {

/**
 * \see https://tools.ietf.org/html/rfc2388
 * \todo Handle files
 */
class FormData final : public PostFormat
{
private:
    bool do_can_parse(const Request& request) const override
    {
        return request.body.content_type().matches_type("multipart", "form-data");
    }

    bool do_parse(Request& request) const override
    {
        if ( request.body.content_type().parameter().first != "boundary" )
            return false;

        /// \todo Change parser based on the protocol
        Http1Parser parser;

        Multipart form_data(request.body.content_type().parameter().second);
        if ( !parser.multipart(request.body, form_data) )
            return false;

        for ( const auto& part : form_data.parts )
        {
            CompoundHeader disposition;
            if ( !parser.compound_header(part.headers.get("content-disposition"), disposition) )
                return false;
            if ( disposition.value != "form-data" || !disposition.parameters.contains("name") )
                return false;
            request.post.append(disposition.parameters["name"], part.content);
        }

        return true;
    }

    bool do_can_format(const Request& request) const override
    {
        return true;
    }

    bool do_format(Request& request) const override
    {
        std::string boundary = generate_boundary(request.post);
        request.body.start_output(
            MimeType("multipart", "form-data", {"boundary", boundary})
        );
        /// \todo Change formatter based on the protocol
        http::Http1Formatter formatter;

        Multipart form_data(boundary);
        form_data.parts.reserve(request.post.size());
        for ( const auto& item: request.post )
        {
            CompoundHeader disposition;
            disposition.value = "form-data";
            disposition.parameters["name"] = item.first;
            form_data.parts.push_back({
                {{"Content-Disposition", formatter.compound_header(disposition)}},
                item.second
            });
        }

        formatter.multipart(request.body, form_data);
        return true;
    }

private:
    /**
     * \brief Generates a boundary string that does not appear in the input data
     */
    static std::string generate_boundary(const DataMap& input)
    {
        std::string boundary;
        boundary.reserve(input.size());
        for ( const auto& item : input )
        {
            if ( item.second.size() <= boundary.size() )
                boundary.push_back('p');
            else
                boundary.push_back(notchar(item.second[boundary.size()]));
        }

        if ( boundary.empty() )
            return "p0ny";

        return boundary;
    }

    /**
     * \brief Returns a character that is different from the input
     */
    static char notchar(char in)
    {
        if ( melanolib::string::ascii::is_alpha(in) )
            return '0';

        if ( melanolib::string::ascii::is_digit(in) )
            return 'n';

        return 'y';
    }
};

} // namespace post
} // namespace httpony
#endif // HTTPONY_HTTP_POST_FORM_DATA_HPP
