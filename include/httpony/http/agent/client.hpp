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

#include "httpony/io/basic_client.hpp"

namespace httpony {

class Client
{
public:
    void start()
    {
        _thread = std::thread([this](){
            _basic_client.run();
        });
    }

    bool started() const
    {
        return _thread.joinable();
    }

    void stop()
    {
        if ( started() )
        {
            _thread.join();
        }
    }

    void queue_request(const Uri& target)
    {
        _basic_client.queue_request(
            target.authority,
            [this](io::Connection& connection){
                /// \todo
                std::cout << "Connected to " << connection.remote_address() << '\n';
            },
            [](const std::string& target, const std::string& message){
                /// \todo Virtual function
                std::cerr << "Could resolve " << target << ": " << message << std::endl;
            },
            [this](const io::Connection& connection, const std::string& message){
                error(connection, message);
            },
            [this]{
                /// \todo Handle SSL based on target.scheme
                return create_connection();
            }
        );
    }

protected:
    /**
     * \brief Handles connection errors
     */
    virtual void error(const io::Connection& connection, const std::string& what) const
    {
        std::cerr << "Error " << connection.remote_address() << ' ' << what << std::endl;
    }

private:
    /**
     * \brief Creates a new connection object
     */
    std::unique_ptr<io::Connection> create_connection()
    {
        return melanolib::New<io::Connection>(io::SocketTag<io::PlainSocket>{});
    }

    io::BasicClient _basic_client;
    std::thread _thread;
};

} // namespace httpony
#endif // HTTPONY_HTTP_AGENT_CLIENT_HPP
