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
#ifndef HTTPONY_HTTP_AGENT_CLIENT_HPP
#define HTTPONY_HTTP_AGENT_CLIENT_HPP

#include <list>

#include "httpony/io/basic_client.hpp"

namespace httpony {

class Client
{
public:
    virtual ~Client()
    {
        stop();
    }

    void start()
    {
        /*thread = std::thread([this](){
            basic_client.run();
        });*/
    }

    bool started() const
    {
        return false; //thread.joinable();
    }

    void stop()
    {
        if ( started() )
        {
//             thread.join();
        }
    }

    /// \todo Overload taking a uri and a callback to generate the request only as needed
    /// (Or make resolve/connect syncronous)
    Response send_request(Request&& request)
    {
        Uri target = request.url;
        if ( target.scheme.empty() )
            target.scheme = "http";

        auto connection_result = basic_client.connect(
            target,
            [this, target] { return create_connection(target); }
        );

        /// \todo Handle this better
        if ( !connection_result.first )
        {
            std::cerr << "Could not resolve " << target.full() << ": " << connection_result.second << std::endl;
            return Response();
        }

        auto& connection = *connection_result.first;

        /// \todo Handle this better
        if ( !connection_result.second.empty() )
        {
            error(connection, connection_result.second);
            return Response();
        }

        std::cout << "Connected to " << connection.remote_address() << '\n';
        connection.send_request(request);
        return connection.read_response();
    }

protected:
    /**
     * \brief Handles connection errors
     */
    virtual void error(const io::Connection& connection, const std::string& what) const
    {
        std::cerr << "Error on " << connection.remote_address() << ": " << what << std::endl;
    }

private:
//     void cleanup_request(Request* request)
//     {
//         auto it = std::find_if(pending.begin(), pending.end(),
//             [request](const auto& c) { return c.get() == request; }
//         );
//
//         if ( it != pending.end() )
//             pending.erase(it);
//     }

    /**
     * \brief Creates a new connection object
     */
    std::unique_ptr<io::Connection> create_connection(const Uri& target)
    {
        /// \todo Handle SSL based on target.scheme
        return melanolib::New<io::Connection>(io::SocketTag<io::PlainSocket>{});
    }

    io::BasicClient basic_client;
//     std::thread thread;
//     std::list<std::unique_ptr<Request>> pending;
};

} // namespace httpony
#endif // HTTPONY_HTTP_AGENT_CLIENT_HPP
