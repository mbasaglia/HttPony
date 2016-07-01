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

#include <iostream>
#include <algorithm>

#include "muhttpd.hpp"


class MyServer : public muhttpd::Server
{
public:
    using Server::Server;
    std::string log_format = "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";

    template<class Map>
        void show_headers(const std::string& title, const Map& headers)
    {
        std::cout << title << ":\n";
        for ( const auto& head : headers )
            std::cout << '\t' << head.name << ' ' << head.value << '\n';
    }

    void respond(muhttpd::ClientConnection& connection) override
    {
        connection.read_body();

        if ( connection.ok() )
        {
            connection.response = muhttpd::Response(
                muhttpd::Content::text("Hello, world!\n")
            );
        }
        else
        {
            muhttpd::Status status(connection.status_code);
            connection.response = muhttpd::Response(
                muhttpd::Content::text(status.message + '\n'),
                status
            );
        }

        log(log_format, connection, std::cout);

        show_headers("Headers", connection.request.headers);
        show_headers("Cookies", connection.request.cookies);
        show_headers("Get", connection.request.get);
        show_headers("Post", connection.request.post);

        if ( connection.request.body )
        {
            std::string body = connection.request.body->data;
            std::replace_if(body.begin(), body.end(), [](char c){return c < ' ' && c != '\n';}, ' ');
            std::cout << '\n' << body << '\n';
        }

        connection.send_response();
    }
};

int main()
{
    MyServer server(8888);

    server.start();

    std::cout << "Server started, hit enter to quit\n";
    std::cin.get();

    return 0;
}
