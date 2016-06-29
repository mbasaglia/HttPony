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
#include <memory>

namespace muhttpd {

/**
 * \brief Header name and value pair
 */
struct Header
{
    Header(std::string name, std::string value)
        : name(std::move(name)), value(std::move(value))
    {}

    std::string name;
    std::string value;
};

/**
 * \brief Thin wrapper around a vector of Header object prividing
 * dictionary-like access
 */
struct Headers
{
    /**
     * \brief Whether it has a header matching the given name
     */
    bool has_header(const std::string& name) const
    {
        for ( const auto& header : headers )
        {
            if ( header.name == name )
                return true;
        }
        return false;
    }

    /**
     * \brief Counts the number of headers matching the given name
     */
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

    /**
     * \brief Returns the first occurence of a header with the given name
     *
     * If no header is found, returns the empty string
     */
    std::string operator[](const std::string& name) const
    {
        for ( const auto& header : headers )
        {
            if ( header.name == name )
                return header.value;
        }
        return "";
    }

    /**
     * \brief Returns the first occurence of a header with the given name
     *
     * If no header is found, a new one is added
     */
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

    /**
     * \brief Appends a new header
     */
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

/**
 * \brief IP address and port
 */
struct IPAddress
{
    /**
     * \brief Address type
     */
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
    std::string version;
    std::string body;
    Headers     headers;
    Headers     cookies;
    Headers     get;
    Headers     post;
    IPAddress   from;
    Auth        auth;
};

/**
 * \brief Response data
 */
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

/**
 * \brief Base class for a simple HTTP server
 */
class Server
{
public:
    explicit Server(uint16_t port);

    ~Server();

    /**
     * \brief Whether the server should accept connections from this address
     */
    virtual bool accept(const IPAddress& address)
    {
        return true;
    }

    /**
     * \brief Listening port
     */
    uint16_t port() const;

    /**
     * \brief Starts the server in a background thread
     * \throws runtime_error if it cannot be started
     */
    void start();

    /**
     * \brief Whether the server has been started
     */
    bool started() const;

    /**
     * \brief Stops the background threads
     */
    void stop();

    /**
     * \brief Function handling requests
     */
    virtual Response respond(const Request& request) = 0;

    struct Data;
private:
    std::unique_ptr<Data> data;
};

} // namespace muhttpd
#endif // MU_HTTPD_PP_HPP
