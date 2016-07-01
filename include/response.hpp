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
#ifndef MUHTTPD_RESPONSE_HPP
#define MUHTTPD_RESPONSE_HPP

#include "status.hpp"
#include "headers.hpp"
#include "protocol.hpp"

namespace muhttpd {

/**
 * \brief Response data
 */
struct Response
{
    Response(
        std::string body = "",
        const std::string& content_type = "text/plain",
        Status status = Status()
    )
        : body(std::move(body)), status(std::move(status))
    {
        /// \todo Date
        headers.append("Content-Type", content_type);
        /// \todo Content length
    }

    std::string body;
    Status      status;
    Headers     headers;
    Protocol    protocol = Protocol("HTTP", 1, 1);
};

} // namespace muhttpd
#endif // MUHTTPD_RESPONSE_HPP
