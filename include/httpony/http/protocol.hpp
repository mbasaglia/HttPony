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

#ifndef HTTPONY_PROTOCOL_HPP
#define HTTPONY_PROTOCOL_HPP

/// \cond
#include <string>
#include <cctype>
#include <iostream>
/// \endcond

namespace httpony {

/**
 * \brief Network protocol and version
 */
struct Protocol
{
    Protocol(std::string name,  unsigned version_major, unsigned version_minor)
        : name(std::move(name)), version_major(version_major), version_minor(version_minor)
    {}

    Protocol(const std::string& string)
    {
        auto slash = string.find('/');
        if ( slash == std::string::npos || slash > string.size() - 1 || !std::isdigit(string[slash+1]) )
            return;

        name = string.substr(0, slash);

        auto dot = string.find('.', slash);
        if ( dot == std::string::npos )
        {
            version_major = std::stoul(string.substr(slash+1));
            version_minor = 0;
        }
        else if ( dot > string.size() - 1 || dot < slash + 2  || !std::isdigit(string[dot+1]) )
        {
            name.clear();
        }
        else
        {
            version_major = std::stoul(string.substr(slash+1, dot-slash-1));
            version_minor = std::stoul(string.substr(dot+1));
        }
    }

    Protocol() = default;

    bool valid() const
    {
        return !name.empty();
    }

    bool operator==(const Protocol& oth) const
    {
        return name == oth.name && version_major == oth.version_major && version_minor == oth.version_minor;
    }

    bool operator!=(const Protocol& oth) const
    {
        return !(*this == oth);
    }


    bool operator>=(const Protocol& oth) const
    {
        return name == oth.name && (
            version_major > oth.version_major ||
            (version_major == oth.version_major && version_minor >= oth.version_minor)
        );
    }

    bool operator<=(const Protocol& oth) const
    {
        return name == oth.name && (
            version_major < oth.version_major ||
            (version_major == oth.version_major && version_minor <= oth.version_minor)
        );
    }

    bool operator>(const Protocol& oth) const
    {
        return name == oth.name && (
            version_major > oth.version_major ||
            (version_major == oth.version_major && version_minor > oth.version_minor)
        );
    }

    bool operator<(const Protocol& oth) const
    {
        return name == oth.name && (
            version_major < oth.version_major ||
            (version_major == oth.version_major && version_minor < oth.version_minor)
        );
    }


    std::string name;
    unsigned version_major = 0;
    unsigned version_minor = 0;

    static const Protocol http_1_0;
    static const Protocol http_1_1;
};

inline std::ostream& operator<<(std::ostream& os, const Protocol& protocol)
{
    if ( !protocol.valid() )
        return os;
    return os << protocol.name << '/' << protocol.version_major << '.' << protocol.version_minor;
}

inline std::istream& operator>>(std::istream& is, Protocol& protocol)
{
    std::string str;
    is >> str;
    protocol = Protocol(str);
    return is;
}

} // namespace httpony
#endif // HTTPONY_PROTOCOL_HPP
