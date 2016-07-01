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
#include <ostream>

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
    std::string protocol;
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
    enum StatusCode
    {
        Continue = 100,
        SwitchingProtocols = 101,
        Processing = 102,

        OK = 200,
        Created = 201,
        Accepted = 202,
        Non = 203,
        NoContent = 204,
        ResetContent = 205,
        PartialContent = 206,
        Multi = 207,
        AlreadyReported = 208,
        IMUsed = 226,
        MultipleChoices = 300,
        MovedPermanently = 301,
        Found = 302,
        SeeOther = 303,
        NotModified = 304,
        UseProxy = 305,
        SwitchProxy = 306,
        TemporaryRedirect = 307,
        PermanentRedirect = 308,

        BadRequest = 400,
        Unauthorized = 401,
        PaymentRequired = 402,
        Forbidden = 403,
        NotFound = 404,
        MethodNotAllowed = 405,
        NotAcceptable = 406,
        ProxyAuthenticationRequired = 407,
        RequestTimeout = 408,
        Conflict = 409,
        Gone = 410,
        LengthRequired = 411,
        PreconditionFailed = 412,
        PayloadTooLarge = 413,
        URITooLong = 414,
        UnsupportedMediaType = 415,
        RangeNotSatisfiable = 416,
        ExpectationFailed = 417,
        ImaTeapot = 418,
        MisdirectedRequest = 421,
        UnprocessableEntity = 422,
        Locked = 423,
        FailedDependency = 424,
        UpgradeRequired = 426,
        PreconditionRequired = 428,
        TooManyRequests = 429,
        RequestHeaderFieldsTooLarge = 431,
        UnavailableForLegalReasons = 451,

        InternalServerError = 500,
        NotImplemented = 501,
        BadGateway = 502,
        ServiceUnavailable = 503,
        GatewayTimeout = 504,
        HTTPVersionNotSupported = 505,
        VariantAlsoNegotiates = 506,
        InsufficientStorage = 507,
        LoopDetected = 508,
        NotExtended = 510,
        NetworkAuthenticationRequired = 511,
    };

    Response(
        std::string body = "",
        const std::string& content_type = "text/plain",
        unsigned status_code = OK
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
 * \note It reads POST data in a single buffer instead of streaming it
 */
class Server
{
public:
    enum class ErrorCode
    {
        Unknown,            //< Unspecified error
        StartupFailure,     //< start() failed
        BadBody,            //< Could not process a request body
    };

    explicit Server(IPAddress listen);
    explicit Server(uint16_t listen_port);

    ~Server();

    /**
     * \brief Whether the server should accept connections from this address
     */
    virtual bool accept(const IPAddress& address)
    {
        return true;
    }

    /**
     * \brief Listening address
     */
    IPAddress listen_address() const;

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
     * \brief Maximum size of a request body to be accepted
     */
    std::size_t max_request_body() const;

    void set_max_request_body(std::size_t size);

    /**
     * \brief Function handling requests
     */
    virtual Response respond(const Request& request) = 0;


    virtual void on_error(ErrorCode error)
    {
        if ( error == ErrorCode::StartupFailure )
            throw std::runtime_error("Could not start the server");
    }

    /**
     * \brief Writes a line of log into \p output based on format
     */
    void log(const std::string& format, const Request& request, const Response& response, std::ostream& output) const;

    struct Data;
    static constexpr unsigned post_chunk_size = 65536;

protected:
    /**
     * \brief Writes a single log item into \p output
     */
    virtual void process_log_format(
        char label,
        const std::string& argument,
        const Request& request,
        const Response& response,
        std::ostream& output
    ) const;

private:
    std::unique_ptr<Data> data;
};

template<class T>
    struct CommonLogFormatItem
    {
        template<class... U>
            CommonLogFormatItem(U&&... args)
                : value(std::forward<U>(args)...)
            {}
        T value;
    };

template<class T>
    std::ostream& operator<<(std::ostream& out, const CommonLogFormatItem<T>& clf)
{
    if ( clf.value )
        return out << clf.value;
    return out << '-';
}

template<>
    inline std::ostream& operator<<(std::ostream& out, const CommonLogFormatItem<std::string>& clf)
{
    if ( !clf.value.empty() )
        return out << clf.value;
    return out << '-';
}

/**
 * \brief Streams values as Common Log Format
 */
template<class T>
    CommonLogFormatItem<T> clf(T&& item)
    {
        return CommonLogFormatItem<T>(std::forward<T>(item));
    }

} // namespace muhttpd
#endif // MU_HTTPD_PP_HPP
