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
#ifndef HTTPONY_MIME_TYPE_HPP
#define HTTPONY_MIME_TYPE_HPP

#include <ostream>

#include <melanolib/string/quickstream.hpp>
#include <melanolib/string/simple_stringutils.hpp>

#include "headers.hpp"

namespace httpony {

class MimeType
{
public:
    MimeType(const std::string& string)
    {
        melanolib::string::QuickStream stream(string);
        set_type(stream.get_line('/'));
        set_subtype(stream.get_until(
            [](char c){ return std::isspace(c) || c ==';'; }
        ));

        if ( !stream.eof() )
        {
            stream.get_until(
                [](char c){ return !std::isspace(c) && c != ';'; },
                false
            );

            Header parameter;
            parameter.name = stream.get_line('=');

            if ( stream.peek() == '"' )
            {
                stream.ignore(1);
                parameter.value = stream.get_line('"');
            }
            else
            {
                parameter.value = stream.get_line();
            }

            set_parameter(parameter);

        }
    }

    MimeType(){}

    MimeType(const std::string& type, const std::string& subtype, const Header& param = {})
    {
        set_type(type);
        set_subtype(subtype);
        set_parameter(param);
    }

    bool valid() const
    {
        return !_type.empty() && !_subtype.empty();
    }

    bool operator==(const MimeType& oth) const
    {
        return _type == oth._type && _subtype == oth._subtype &&
            _parameter.name == oth._parameter.name && _parameter.value == oth._parameter.value;
    }

    bool operator!=(const MimeType& oth) const
    {
        return !(*this == oth);
    }

    void set_type(const std::string& type)
    {
        _type = melanolib::string::strtolower(type);
    }

    void set_subtype(const std::string& subtype)
    {
        _subtype = melanolib::string::strtolower(subtype);
    }

    void set_parameter(const Header& param)
    {
        if ( param.name.empty() || param.value.empty() )
        {
            _parameter = Header();
            return;
        }

        _parameter.name = melanolib::string::strtolower(param.name);

        if ( _parameter.name == "charset" )
            _parameter.value = melanolib::string::strtolower(param.value);
        else
            _parameter.value = param.value;
    }

    std::string type() const
    {
        return _type;
    }

    std::string subtype() const
    {
        return _subtype;
    }

    Header parameter() const
    {
        return _parameter;
    }

    friend std::ostream& operator<<(std::ostream& os, const MimeType& mime)
    {
        os << mime._type << '/' << mime._subtype;
        if ( !mime._parameter.name.empty() )
            os << ';' << mime._parameter.name << '=' << mime._parameter.value;
        return os;
    }

private:
    std::string _type;
    std::string _subtype;
    Header _parameter;
};

} // namespace httpony
#endif // HTTPONY_MIME_TYPE_HPP
