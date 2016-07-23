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
#include "httpony.hpp"

void print_response(httpony::Response&& response)
{
    /// \todo Make sure Http1Formatter::response() works properly for input responses as well
    httpony::http::Http1Formatter("\n").response(std::cout, response);
    std::cout << response.body.read_all() << '\n';
}
int main(int argc, char** argv)
{
    httpony::Client client;

    print_response(client.query(httpony::Request("GET", "http://example.com")));
    return 0;
}
