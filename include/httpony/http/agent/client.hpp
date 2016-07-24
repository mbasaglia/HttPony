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

/// \cond
#include <list>

#include <melanolib/utils/movable.hpp>
/// \endcond

#include "httpony/io/basic_client.hpp"
#include "httpony/http/response.hpp"

namespace httpony {

class Client
{
public:

    /**
     * \todo Add more semantic properties to user_agent, and add it as a request attribute
     */
    explicit Client(const std::string& user_agent)
        : _user_agent(user_agent)
    {}

    Client() : Client("HttPony/1.0") {}

    virtual ~Client()
    {
    }

    /**
     * \brief Creates a connection to the target of the geiven Uri
     * \param[in]  target URI of the server to connect to
     * \param[out] status Status of the operation
     */
    melanolib::Movable<io::Connection> connect(Uri target, ClientStatus& status) const
    {
        if ( target.scheme.empty() )
            target.scheme = "http";

        auto connection = create_connection(target);
        status = _basic_client.connect(target, *connection);

        return connection;
    }

    ClientStatus query(Request& request, Response& response) const
    {
        ClientStatus status;
        auto connection = connect(request.url, status);

        if ( status.error() )
            return status;

        return get_response(connection, request, response);
    }

    ClientStatus get_response(io::Connection& connection, Request& request, Response& response) const;

    /**
     * \brief The timeout for network I/O operations
     */
    melanolib::Optional<melanolib::time::seconds> timeout() const
    {
        return _basic_client.timeout();
    }

    void set_timeout(melanolib::time::seconds timeout)
    {
        _basic_client.set_timeout(timeout);
    }

    void clear_timeout()
    {
        _basic_client.clear_timeout();
    }


protected:

    virtual void process_request(Request& request) const
    {
        request.headers["User-Agent"] = _user_agent;
    }

    /**
     * \brief Creates a new connection object
     */
    virtual melanolib::Movable<io::Connection> create_connection(const Uri& target) const
    {
        /// \todo Handle SSL based on target.scheme
        return melanolib::Movable<io::Connection>(io::SocketTag<io::PlainSocket>{});
    }

private:
    template<class ClientT>
        friend class BasicAsyncClient;


    io::BasicClient _basic_client;
    std::string _user_agent;
};

template<class ClientT>
class BasicAsyncClient : public ClientT
{
private:
    struct AsyncItem
    {
        AsyncItem(
            melanolib::Movable<io::Connection> connection,
            melanolib::Optional<Request> request = {}
        ) : connection(std::move(connection)),
            request(std::move(request))
        {}

        io::Connection* connection_ptr()
        {
            return &(io::Connection&)connection;
        }

        melanolib::Movable<io::Connection> connection;
        melanolib::Optional<Request> request;
    };

public:
    template<class... Args>
        BasicAsyncClient(Args&&... args)
            : ClientT(std::forward<Args>(args)...),
              should_run(true)
        {}

    ~BasicAsyncClient()
    {
        stop();
    }

    void start()
    {
        if ( !started() )
        {
            should_run = true;
            thread = std::move(std::thread([this]{ run(); }));
        }
    }

    bool started() const
    {
        return thread.joinable();
    }

    void stop()
    {
        if ( started() )
        {
            should_run = false;
            condition.notify_all();
            thread.join();
        }
    }

    void run()
    {
        while( should_run )
        {
            std::unique_lock<std::mutex> lock(mutex);
            while ( items.empty() && should_run )
                condition.wait(lock);

            if ( !should_run )
                return;

            std::vector<io::Connection*> connections;
            connections.reserve(items.size());

            for ( auto& item : items )
                connections.push_back(item.connection_ptr());

            lock.unlock();
            for ( auto connection : connections )
                connection->socket().process_async();
            lock.lock();

            std::unique_lock<std::mutex> old_lock(old_connections_mutex);
            for ( auto it_conn = items.begin(); it_conn != items.end(); )
            {
                it_conn = remove_old(it_conn);
            }

        }
    }

    template<class GetRequest, class OnResponse, class OnError>
    void async_query(Uri target, const GetRequest& get_request,
                       const OnResponse& on_response, const OnError& on_error)
    {
        auto connection = ClientT::create_connection(target);

        std::unique_lock<std::mutex> lock(mutex);
        items.push_back(std::move(connection));
        auto& item = items.back();
        lock.unlock();

        ClientT::_basic_client.async_connect(target, item.connection,
            [this, target, get_request, on_response, on_error, &item](){
                Response response;
                auto status = ClientT::get_response(item.connection, get_request(), response);
                if ( status.error() )
                    on_error(target, status);
                else
                    on_response(target, response);
                drop_connection(item.connection_ptr());
            },
            [this, on_error, target, &item](const ClientStatus& status)
            {
                on_error(target, status);
                drop_connection(item.connection_ptr());
            }
        );

        condition.notify_one();
    }

    template<class OnResponse, class OnError>
    void async_query(Request&& request,
                     const OnResponse& on_response,
                     const OnError& on_error)
    {
        auto connection = ClientT::create_connection(request.url);

        std::unique_lock<std::mutex> lock(mutex);
        items.emplace_back(std::move(connection), std::move(request));
        auto& item = items.back();
        lock.unlock();

        ClientT::_basic_client.async_connect(item.request->url, item.connection,
            [this, on_response, on_error, &item](){
                Response response;
                auto status = ClientT::get_response(item.connection, *item.request, response);
                if ( status.error() )
                    on_error(*item.request, status);
                else
                    on_response(*item.request, response);
                drop_connection(item.connection_ptr());
            },
            [this, on_error, &item](const ClientStatus& status)
            {
                on_error(*item.request, status);
                drop_connection(item.connection_ptr());
            }
        );

        condition.notify_one();
    }

private:
    using item_iterator = typename std::list<AsyncItem>::iterator;

    void drop_connection(io::Connection* connection)
    {
        std::unique_lock<std::mutex> lock(old_connections_mutex);
        old_connections.push_back(connection);
        lock.unlock();
        condition.notify_one();
    }

    item_iterator remove_old(item_iterator iter)
    {
        for ( auto it_old = old_connections.begin(); it_old != old_connections.end(); )
        {
            if ( *it_old == iter->connection_ptr() )
            {
                old_connections.erase(it_old);
                iter->connection->close();
                return items.erase(iter);
            }
        }
        return ++iter;
    }

    std::thread thread;
    std::condition_variable condition;

    std::mutex mutex;
    std::atomic<bool> should_run = true;
    std::list<AsyncItem> items;

    std::mutex old_connections_mutex;
    std::list<io::Connection*> old_connections;
};

using AsyncClient = BasicAsyncClient<Client>;

} // namespace httpony
#endif // HTTPONY_HTTP_AGENT_CLIENT_HPP
