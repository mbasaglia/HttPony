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
#ifndef MU_HTTPD_PP_HPP
#define MU_HTTPD_PP_HPP
#include <string>
#include <vector>
#include <stdexcept>

namespace muhttpd {

struct Header
{
    Header(std::string name, std::string value)
        : name(std::move(name)), value(std::move(value))
    {}

    std::string name;
    std::string value;
};

struct Headers
{
    bool has_header(const std::string& name) const
    {
        for ( const auto& header : headers )
        {
            if ( header.name == name )
                return true;
        }
        return false;
    }

    std::size_t count(const std::string& name) const
    {
        std::size_t n = 0;
        for ( const auto& header : headers )
        {
            if ( header.name == name )
                n++;
        }
        return n;
    }

    std::string operator[](const std::string& name) const
    {
        for ( const auto& header : headers )
        {
            if ( header.name == name )
                return header.value;
        }
        return "";
    }

    std::string& operator[](const std::string& name)
    {
        for ( auto& header : headers )
        {
            if ( header.name == name )
                return header.value;
        }
        headers.push_back({name, ""});
        return headers.back().value;
    }

    void append(std::string name, std::string value)
    {
        headers.emplace_back(name, value);
    }

    auto begin() const
    {
        return headers.begin();
    }

    auto end() const
    {
        return headers.end();
    }

    std::vector<Header> headers;
};


struct IPAddress
{
    enum class Type
    {
        Invalid,
        IPv4 = 4,
        IPv6 = 6,
    };

    IPAddress() = default;

    IPAddress(Type type, std::string string, uint16_t port)
        : type(type), string(std::move(string)), port(port)
    {}


    Type type = Type::Invalid;
    std::string string;
    uint16_t port = 0;

};

struct Auth
{
    std::string user;
    std::string password;
};

struct Request
{
    std::string url;
    std::string method;
    std::string version;
    std::string body;
    Headers     headers;
    IPAddress   from;
    Auth        auth;
};

struct Response
{
    Response(
        std::string body = "",
        const std::string& content_type = "text/plain",
        unsigned status_code = 200
    )
        : body(std::move(body)), status_code(status_code)
    {
        headers.append("Content-Type", content_type);
    }

    std::string body;
    unsigned    status_code;
    Headers     headers;
};

class Server
{
public:
    explicit Server(uint16_t port)
        : _port(port)
    {
    }

    ~Server()
    {
        stop();
    }

    virtual bool accept(const IPAddress& address)
    {
        return true;
    }

    uint16_t port() const
    {
        return _port;
    }

    void start();

    bool started() const
    {
        return daemon;
    }

    void stop();

    virtual Response respond(const Request& request) = 0;

private:
    uint16_t _port;
    struct MHD_Daemon* daemon = nullptr;
    Request request;

    friend Request& _request(Server* sv);
};

} // namespace muhttpd
#endif // MU_HTTPD_PP_HPP
