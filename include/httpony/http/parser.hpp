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

#include "httpony/http/response.hpp"
#include "httpony/multipart.hpp"
#include "httpony/http/client_status.hpp"

namespace httpony {


class Parser
{
public:
    virtual ~Parser() {}
    /**
     * \brief Reads a full request object from the stream
     * \param[in]  stream   Input stream
     * \param[out] request  Object to populate
     * \return Recommended status code
     * \note If it returns an error code, it's likely the request contain only
     *       partially parsed data
     */
    virtual Status request(std::istream& stream, Request& request) const = 0;

    /**
     * \brief Reads a full response object from the stream
     * \param[in]  stream   Input stream
     * \param[out] response Object to populate
     * \return Recommended status code
     * \note If it returns an error code, it's likely the response contain only
     *       partially parsed data
     */
    virtual ClientStatus response(std::istream& stream, Response& response) const = 0;

    /**
     * \brief Reads all headers and the empty line following them
     * \param stream        Input stream
     * \param headers       Header container to update
     * \returns \b true on success
     */
    virtual bool headers(std::istream& stream, httpony::Headers& headers) const = 0;

    /**
     * \brief Reads the request line (GET /url HTTP/1.1)
     * \param stream    Input stream
     * \param request   Request object to update
     * \returns \b true on success
     */
    virtual bool request_line(std::istream& stream, Request& request) const = 0;

    /**
     * \brief Reads the response line (HTTP/1.1 200 OK)
     * \param stream    Input stream
     * \param response  Response object to update
     * \returns \b true on success
     */
    virtual bool response_line(std::istream& stream, Response& response) const = 0;

    /**
     * \brief Converts a string into a compound header
     */
    virtual bool compound_header(const std::string& header_value, CompoundHeader& header) const = 0;

    /**
     * \brief Populates the multipart data from a stream
     * \pre \p multipart.boundary has been initialized with the proper value
     * \returns \b true on success
     */
    virtual bool multipart(std::istream& stream, Multipart& multipart) const = 0;
};


class Http1Parser : public Parser
{
public:
    enum ParserFlag
    {
        /// Whether to accept (and parse) requests containing folded headers
        ParseFoldedHeaders  = 0x001,
        /// Whether to parse Cookie header into request.cookie
        /// If not set, cookies will still be accessible as headers
        ParseCookies        = 0x002,

        ParseDefault        = ParseCookies,
    };

    using ParserFlags = unsigned;

    explicit Http1Parser(ParserFlags flags = ParseDefault)
        : flags(flags)
    {}

    Status request(std::istream& stream, Request& request) const override;


    ClientStatus response(std::istream& stream, Response& response) const override;

    bool headers(std::istream& stream, httpony::Headers& headers) const override;

    bool request_line(std::istream& stream, Request& request) const override;

    bool response_line(std::istream& stream, Response& response) const override;

    /**
     * \brief Reads header parameters (param1=foo param2=bar)
     * \param stream    Input stream
     * \param output    Container to update
     * \param delimiter Character delimitng the arguments
     * \returns \b true on success
     */
    template<class OrderedContainer>
        static bool header_parameters(
        melanolib::string::QuickStream& stream,
        OrderedContainer& output,
        char delimiter = ';'
    )
    {
        using melanolib::string::ascii::is_space;

        auto is_boundary_char = [delimiter](char c){ return is_space(c) || c == delimiter; };

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

    /**
     * \brief Converts a string into a compound header
     */
    bool compound_header(const std::string& header_value, CompoundHeader& header) const override
    {
        melanolib::string::QuickStream stream(header_value);
        header.value = stream.get_until(
            [](char c){ return std::isspace(c) || c ==';'; }
        );
        return header_parameters(stream, header.parameters);
    }


    /**
     * \brief Populates the multipart data from a stream
     * \pre \p multipart.boundary has been initialized with the proper value
     * \returns \b true on success
     */
    bool multipart(std::istream& stream, Multipart& multipart) const override;


    bool auth(const std::string& header_contents, Auth& auth) const;

private:
    /**
     * \brief Reads a string delimited by a specific character and ignores following spaces
     * \param stream Input to read from
     * \param output Output string
     * \param delim  Delimiting character
     * \param at_end Whether the value can be at the end of the line
     * \returns \b true on success
     */
    bool delimited(std::istream& stream, std::string& output,
                        char delim = ':', bool at_end = false) const;

    /**
     * \brief Skips spaces (except "\r")
     * \param stream Input to read from
     * \param at_end Whether the value can be at the end of the line
     * \returns \b true on success
     */
    bool skip_spaces(std::istream& stream, bool at_end = false) const;

    /**
     * \brief Ignores all before "\n"
     */
    void skip_line(std::istream& stream) const;

    /**
     * \brief Reads a "quoted" header value
     * \returns \b true on success
     */
    bool quoted_header_value(std::istream& stream, std::string& value) const;

    /**
     * \brief Whether the string is a valid boundary
     */
    bool multipart_valid_boundary(const std::string& boundary) const;

    ParserFlags flags;
};

} // namespace httpony

#endif // HTTPONY_HTTP_PARSER_HPP
