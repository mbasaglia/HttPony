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
#ifndef MUHTTPD_REQUEST_HPP
#define MUHTTPD_REQUEST_HPP

#include "ip_address.hpp"
#include "headers.hpp"
#include "protocol.hpp"

namespace muhttpd {


/**
 * \brief HTTP Authentication credentials
 */
struct Auth
{
    std::string user;
    std::string password;
};


/**
 * \brief HTTP request data
 */
struct Request
{
    std::string url;
    std::string method;
    Protocol    protocol;
    std::string body;
    Headers     headers;
    Headers     cookies;
    Headers     get;
    Headers     post;
    IPAddress   from;
    Auth        auth;
};

} // namespace muhttpd
#endif // MUHTTPD_REQUEST_HPP
