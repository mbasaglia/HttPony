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
 *
 */

#include "logging.hpp"
#include "server.hpp"

namespace httpony {

void log(
        const std::string& format,
        const ClientConnection& connection,
        std::ostream& output
    )
{
    auto start = format.begin();
    auto finish = format.begin();
    while ( start < format.end() )
    {
        finish = std::find(start, format.end(), '%');
        output << std::string(start, finish);

        // No more % (clean exit)
        if ( finish == format.end() )
            break;

        start = finish + 1;
        // stray % at the end
        if ( start == format.end() )
            break;

        char label = *start;
        start++;
        std::string argument;

        if ( label == '{' )
        {
            finish = std::find(start, format.end(), '}');
            // Unterminated %{
            if ( finish + 1 >= format.end() )
                break;
            argument = std::string(start, finish);
            start = finish + 1;
            label = *start;
            start++;
        }
        connection.server->process_log_format(label, argument, connection, output);

    }
    output << std::endl;
}

} // namespace httpony
