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
#ifndef HTTPONY_CONNECTION_HPP
#define HTTPONY_CONNECTION_HPP

#include "httpony/http/format/write.hpp"
#include "httpony/http/format/read.hpp"

namespace httpony {
namespace io {

class Connection
{
public:
    template<class... SocketArgs>
        explicit Connection(SocketArgs&&... args)
            : _socket(std::forward<SocketArgs>(args)...)
    {}

    NetworkInputBuffer& input_buffer()
    {
        return _input_buffer;
    }

    NetworkOutputBuffer& output_buffer()
    {
        return _output_buffer;
    }

    bool commit_output()
    {
        boost::system::error_code error;
        _socket.write(_output_buffer.data(), error);
        _output_buffer.consume(_output_buffer.size());
        return !error;
    }

    void close()
    {
        _socket.close();
    }

    IPAddress remote_address() const
    {
        boost::system::error_code error;
        auto endpoint = _socket.raw_socket().remote_endpoint(error);
        if ( error )
            return {};
        return endpoint_to_ip(endpoint);
    }

    IPAddress local_address() const
    {
        boost::system::error_code error;
        auto endpoint = _socket.raw_socket().local_endpoint(error);
        if ( error )
            return {};
        return endpoint_to_ip(endpoint);
    }

    bool send_response(Response& response)
    {
        std::ostream stream(&_output_buffer);
        http::write::response(stream, response);
        return commit_output();
    }

    bool send_response(Response&& resp)
    {
        return send_response(resp);
    }

    bool send_request(Request& request)
    {
        std::ostream stream(&_output_buffer);
        http::write::request(stream, request);
        return commit_output();
    }

    Status read_request(Request& output, http::read::HttpParserFlags parse_flags = http::read::ParseDefault)
    {
        output = Request();

        Status status;

        boost::system::error_code error;
        /// \todo Avoid magic number, keep reading if needed
        auto sz = _input_buffer.read_some(1024, error);
        if ( !error && sz > 0 )
        {
            std::istream stream(&_input_buffer);
            output = http::read::request(stream, parse_flags, status);
            if ( output.body.has_data() )
                _input_buffer.expect_input(output.body.content_length());
        }
        else if ( _socket.timed_out() )
        {
            return StatusCode::RequestTimeout;
        }
        else
        {
            return StatusCode::BadRequest;
        }

        return status;
    }

    ClientStatus read_response(Response& output, http::read::HttpParserFlags parse_flags = http::read::ParseDefault)
    {
        output = Response();

        boost::system::error_code error;
        /// \todo Avoid magic number, keep reading if needed
        auto sz = _input_buffer.read_some(1024, error);

        if ( !error && sz > 0 )
        {
            std::istream stream(&_input_buffer);

            if ( !http::read::response(stream, parse_flags, output) );
                return "malformed response";

            if ( output.body.has_data() )
                _input_buffer.expect_input(output.body.content_length());
        }
        else if ( _socket.timed_out() )
        {
            return "timeout";
        }
        else if ( error )
        {
            return error.message();
        }
        else if ( sz > 0 )
        {
            return "no content";
        }

        return {};
    }


    TimeoutSocket& socket()
    {
        return _socket;
    }

    /**
     * \brief Reads request data from the socket
     *
     * Sets \c status() to an error code if the request is malformed
     */
    Request read_request(http::read::HttpParserFlags parse_flags = http::read::ParseDefault)
    {
        Request output;
        set_status(read_request(output, parse_flags));
        return output;
    }

    Status status() const
    {
        return _status;
    }

    void set_status(const Status& status)
    {
        _status = status;
    }

    /**
     * \brief Reads response data from the socket
     *
     * Sets \c client_status() to an error if the response is malformed
     */
    Response read_response(http::read::HttpParserFlags parse_flags = http::read::ParseDefault)
    {
        Response output;
        set_client_status(read_response(output, parse_flags));
        return output;
    }

    ClientStatus client_status() const
    {
        return _client_status;
    }

    void set_client_status(const ClientStatus& status)
    {
        _client_status = status;
    }

private:
    /**
     * \brief Converts a boost endpoint to an IPAddress object
     */
    static IPAddress endpoint_to_ip(const boost_tcp::endpoint& endpoint)
    {
        return IPAddress(
            endpoint.address().is_v6() ? IPAddress::Type::IPv6 : IPAddress::Type::IPv4,
            endpoint.address().to_string(),
            endpoint.port()
        );
    }

    TimeoutSocket       _socket;
    NetworkInputBuffer  _input_buffer{_socket};
    NetworkOutputBuffer _output_buffer;
    Status              _status;
    ClientStatus        _client_status;
};


} // namespace io
} // namespace httpony
#endif // HTTPONY_CONNECTION_HPP
