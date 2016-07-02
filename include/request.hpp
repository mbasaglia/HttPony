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
#ifndef HTTPONY_REQUEST_HPP
#define HTTPONY_REQUEST_HPP

#include "ip_address.hpp"
#include "headers.hpp"
#include "protocol.hpp"
#include "io.hpp"
#include "uri.hpp"

namespace httpony {


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
    Uri         url;
    std::string method;
    Protocol    protocol;
    Headers     headers;
    DataMap     cookies;
    DataMap     get;
    DataMap     post;
    IPAddress   from;
    Auth        auth;

    /// \todo Parse urlencoded and multipart/form-data into request.post
    NetworkInputStream body;
};

} // namespace httpony
#endif // HTTPONY_REQUEST_HPP
