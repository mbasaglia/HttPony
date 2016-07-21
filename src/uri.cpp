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

#include "httpony/uri.hpp"

#include <cctype>
#include <regex>
#include <melanolib/string/stringutils.hpp>

namespace httpony {

Authority::Authority(const std::string& string)
{
    auto at = string.find('@');

    if ( at != std::string::npos )
    {
        auto colon = string.find(':');
        if ( colon != std::string::npos && colon < at )
        {
            user = string.substr(0, colon);
            colon += 1;
            password = string.substr(colon, at - colon);
        }
        else
        {
            user = string.substr(0, at);
        }
        at += 1;
    }
    else
    {
        at = 0;
    }

    auto colon = string.rfind(':');
    if ( colon != std::string::npos && colon > at &&
        std::all_of(string.begin() + colon + 1, string.end(), melanolib::string::ascii::is_digit) )
    {
        host = string.substr(at, colon - at);
        port = std::stoul(string.substr(colon + 1));
    }
    else
    {
        host = string.substr(at);
    }
}

std::string Authority::full() const
{
    std::string result;
    if ( user )
    {
        result += *user;
        if ( password )
            result += ':' + *password;
        result += '@';
    }

    result += host;

    if ( port )
        result += ':' + std::to_string(*port);

    return result;
}

Uri::Uri(const std::string& uri)
{
    static const std::regex uri_regex(
        R"(^(?:([a-zA-Z][-a-zA-Z.+]*):)?(?://([^/?#]*))?(/?[^?#]*)(?:\?([^#]*))?(?:#(.*))?$)",
        std::regex::ECMAScript|std::regex::optimize
    );
    std::smatch match;
    if ( std::regex_match(uri, match, uri_regex) )
    {
        scheme = urldecode(match[1]);
        authority = Authority(match[2]);

        path = Path(match[3], true);

        query = parse_query_string(match[4]);

        fragment = urldecode(match[5]);
    }
}

Uri::Uri(
    std::string scheme,
    std::string authority,
    Path path,
    DataMap query,
    std::string fragment
) : scheme(std::move(scheme)),
    authority(std::move(authority)),
    path(std::move(path)),
    query(std::move(query)),
    fragment(std::move(fragment))
{}


Uri::Uri(
    std::string scheme,
    Authority authority,
    Path path,
    DataMap query,
    std::string fragment
) : scheme(std::move(scheme)),
    authority(std::move(authority)),
    path(std::move(path)),
    query(std::move(query)),
    fragment(std::move(fragment))
{}


std::string Uri::full() const
{
    std::string result;

    if ( !scheme.empty() )
        result += urlencode(scheme) + ':';

    if ( !authority.empty() )
        result += "//" + authority.full();

    result += path.url_encoded();

    result += query_string(true);

    if ( !fragment.empty() )
        result += '#' + urlencode(fragment);

    return result;
}

bool Uri::operator==(const Uri& oth) const
{
    return scheme == oth.scheme &&
           authority == oth.authority &&
           std::equal(path.begin(), path.end(), oth.path.begin(), oth.path.end()) &&
           query == oth.query &&
           fragment == oth.fragment
    ;
}

std::string Uri::query_string(bool question_mark) const
{
    return build_query_string(query, question_mark);
}

std::string urlencode(const std::string& input, bool plus_spaces)
{
    std::string output;
    output.reserve(input.size());

    for ( uint8_t c : input )
    {
        if ( std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' )
        {
            output.push_back(c);
        }
        else if ( plus_spaces && c == ' ' )
        {
            output.push_back('+');
        }
        else
        {
            output.push_back('%');
            output.push_back(melanolib::string::ascii::hex_digit((c & 0xF0) >> 4));
            output.push_back(melanolib::string::ascii::hex_digit(c & 0xF));
        }
    }
    return output;
}

std::string urldecode(const std::string& input, bool plus_spaces)
{
    std::string output;
    output.reserve(input.size());

    for ( auto it = input.begin(); it != input.end(); ++it )
    {
        if ( *it == '%' )
        {
            if ( it + 2 >= input.end() )
                break;

            if ( !std::isxdigit(*(it + 1)) || !std::isxdigit(*(it + 2)) )
            {
                output.push_back(*it);
                continue;
            }
            output.push_back(
                (melanolib::string::ascii::get_hex(*(it + 1)) << 4) |
                melanolib::string::ascii::get_hex(*(it + 2))
            );
            it += 2;
        }
        else if ( plus_spaces && *it == '+' )
        {
            output.push_back(' ');
        }
        else
        {
            output.push_back(*it);
        }
    }
    return output;
}

DataMap parse_query_string(const std::string& str)
{
    DataMap result;

    melanolib::string::QuickStream input(str);
    if ( input.peek() == '?' )
        input.ignore();


    while ( !input.eof() )
    {
        std::string name;
        std::string value;
        name = urldecode(
            input.get_until([](char c){ return c == '&' || c == '='; })
        );
        if ( input.peek_back() == '=' )
            value = urldecode(input.get_line('&'), true);
        result.append(name, value);
    }

    return result;
}

std::string build_query_string(const DataMap& data, bool question_mark)
{
    std::string result;

    for ( const auto& item : data )
    {
        if ( result.empty() )
        {
            if ( question_mark )
                result += '?';
        }
        else
        {
            result += '&';
        }

        result += urlencode(item.first);
        if ( !item.second.empty() )
            result += '=' + urlencode(item.second, true);
    }

    return result;
}

} // namespace httpony
