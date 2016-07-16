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
    NetworkInputBuffer& input_buffer()
    {
        return _input_buffer;
    }

    boost::asio::streambuf& output_buffer()
    {
        return _output_buffer;
    }

    bool commit_output()
    {
        boost::system::error_code error;
        _socket.write(_output_buffer.data(), error);
        return !error;
    }

    void close()
    {
        _socket.close();
    }

    IPAddress remote_address() const
    {
        return endpoint_to_ip(_socket.socket().remote_endpoint());
    }

    IPAddress local_address() const
    {
        return endpoint_to_ip(_socket.socket().local_endpoint());
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

    /**
     * \brief Reads request data from the socket
     *
     * Sets \c output.status to an error code if the request is malformed
     */
    Request read_request(http::read::HttpParserFlags parse_flags = http::read::ParseDefault)
    {
        Request output;

        boost::system::error_code error;
        /// \todo Avoid magic number, keep reading if needed
        auto sz = _input_buffer.read_some(1024, error);
        if ( !error && sz > 0 )
        {
            std::istream stream(&_input_buffer);
            output = http::read::request(stream, parse_flags);
            if ( output.body.has_data() )
                _input_buffer.expect_input(output.body.content_length());
        }
        else if ( _socket.timed_out() )
        {
            output.suggested_status = StatusCode::RequestTimeout;
        }
        else
        {
            output.suggested_status = StatusCode::BadRequest;
        }

        output.from = remote_address();

        return output;
    }

    TimeoutSocket& socket()
    {
        return _socket;
    }

private:
    /**
     * \brief Converts a boost endpoint to an IPAddress object
     */
    static IPAddress endpoint_to_ip(const boost::asio::ip::tcp::endpoint& endpoint)
    {
        return IPAddress(
            endpoint.address().is_v6() ? IPAddress::Type::IPv6 : IPAddress::Type::IPv4,
            endpoint.address().to_string(),
            endpoint.port()
        );
    }

    TimeoutSocket _socket;
    NetworkInputBuffer  _input_buffer{_socket};
    NetworkOutputBuffer _output_buffer;

};

} // namespace io
} // namespace httpony
#endif // HTTPONY_CONNECTION_HPP
