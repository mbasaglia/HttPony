/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2016 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MUHTTPD_LOGGING_HPP
#define MUHTTPD_LOGGING_HPP

#include "client_connection.hpp"

namespace muhttpd {


/**
 * \brief Writes a line of log into \p output based on format
 */
void log(const std::string& format, const ClientConnection& connection, std::ostream& output);

template<class T>
    struct CommonLogFormatItem
    {
        template<class... U>
            CommonLogFormatItem(U&&... args)
                : value(std::forward<U>(args)...)
            {}
        T value;
    };

template<class T>
    std::ostream& operator<<(std::ostream& out, const CommonLogFormatItem<T>& clf)
{
    if ( clf.value )
        return out << clf.value;
    return out << '-';
}

template<>
    inline std::ostream& operator<<(std::ostream& out, const CommonLogFormatItem<std::string>& clf)
{
    if ( !clf.value.empty() )
        return out << clf.value;
    return out << '-';
}

/**
 * \brief Streams values as Common Log Format
 */
template<class T>
    CommonLogFormatItem<T> clf(T&& item)
    {
        return CommonLogFormatItem<T>(std::forward<T>(item));
    }

} // namespace muhttpd
#endif // MUHTTPD_LOGGING_HPP
