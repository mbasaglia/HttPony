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
#ifndef MuHTTPD_SERVER_DATA_HPP
#define MuHTTPD_SERVER_DATA_HPP

#include "muhttpd.hpp"

namespace muhttpd {

class Server::Data
{
public:
    Data(Server* owner, IPAddress listen)
        : owner(owner), listen(listen), max_request_body(std::string().max_size())
    {}

    virtual ~Data(){}

    Server* owner;
    IPAddress listen;
    std::size_t max_request_body;
};

std::unique_ptr<Server::Data> make_data(Server* owner, IPAddress listen);

} // namespace muhttpd
#endif // MuHTTPD_SERVER_DATA_HPP
