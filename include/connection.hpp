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

#include "http/write.hpp"
#include "http/read.hpp"

namespace httpony {

class TimeoutSocket
{
public:
    explicit TimeoutSocket(melanolib::time::seconds timeout)
    {
        set_timeout(timeout);
        check_deadline();
    }

    TimeoutSocket()
    {
        clear_timeout();
        check_deadline();
    }

    ~TimeoutSocket()
    {
        close();
    }

    void close()
    {
        boost::system::error_code error;
        _socket.close(error);
    }

    bool timed_out() const
    {
        return _timed_out;
    }

    boost::asio::ip::tcp::socket& socket()
    {
        return _socket;
    }

    const boost::asio::ip::tcp::socket& socket() const
    {
        return _socket;
    }

    void set_timeout(melanolib::time::seconds timeout)
    {
        _deadline.expires_from_now(boost::posix_time::seconds(timeout.count()));
    }

    void clear_timeout()
    {
        _deadline.expires_at(boost::posix_time::pos_infin);
    }

    template<class MutableBufferSequence>
        std::size_t read_some(MutableBufferSequence&& buffer, boost::system::error_code& error)
    {
        std::size_t read_size = 0;
        error = boost::asio::error::would_block;
        _socket.async_read_some(std::forward<MutableBufferSequence>(buffer),
            [&error, &read_size](const boost::system::error_code& ec, std::size_t bytes_read)
            {
                error = ec;
                read_size += bytes_read;
            }
        );

        do
            _io_service.run_one();
        while ( !_io_service.stopped() && error == boost::asio::error::would_block );

        return read_size;
    }

    template<class ConstBufferSequence>
        std::size_t write(ConstBufferSequence&& buffer, boost::system::error_code& error)
    {
        std::size_t written_size = 0;
        error = boost::asio::error::would_block;

        boost::asio::async_write(_socket, buffer,
            [&error, &written_size](const boost::system::error_code& ec, std::size_t bytes_written)
            {
                error = ec;
                written_size += bytes_written;
            }
        );

        do
            _io_service.run_one();
        while ( !_io_service.stopped() && error == boost::asio::error::would_block );

        return written_size;
    }

private:
    void check_deadline()
    {
        if ( _deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now() )
        {
            _deadline.expires_at(boost::posix_time::pos_infin);

            _timed_out = true;
            _io_service.stop();
        }

        _deadline.async_wait([this](const boost::system::error_code&){check_deadline();});
    }

    boost::asio::io_service _io_service;
    boost::asio::ip::tcp::socket _socket{_io_service};
    boost::asio::deadline_timer _deadline{_io_service};
    bool _timed_out = false;
};

/**
 * \brief Stream buffer linked to a socket for reading
 */
class NetworkInputBuffer : public boost::asio::streambuf
{
public:
    explicit NetworkInputBuffer(TimeoutSocket& socket)
        : _socket(socket)
    {
    }

    /**
     * \brief Reads up to size from the socket
     */
    std::size_t read_some(std::size_t size, boost::system::error_code& error)
    {
        auto prev_size = this->size();
        if ( size <= prev_size )
            return size;
        size -= prev_size;

        auto in_buffer = prepare(size);

        auto read_size = _socket.read_some(boost::asio::buffer(in_buffer), error);

        commit(read_size);

        return read_size + prev_size;
    }

    /**
     * \brief Expect at least \p byte_count to be available in the socket
     */
    void expect_input(std::size_t byte_count)
    {
        _expected_input = byte_count;
    }

    std::size_t expected_input() const
    {
        return _expected_input;
    }

    boost::system::error_code error() const
    {
        return _error;
    }

protected:
    int_type underflow() override
    {
        int_type ret = boost::asio::streambuf::underflow();
        if ( ret == traits_type::eof() && _expected_input > 0 )
        {
            auto read_size = read_some(_expected_input, _error);

            if ( read_size <= _expected_input )
                _expected_input -= read_size;
            else
                /// \todo This should trigger a bad request
                _error = boost::system::errc::make_error_code(
                    boost::system::errc::file_too_large
                );

            ret = boost::asio::streambuf::underflow();
        }
        return ret;
    }

private:

    TimeoutSocket& _socket;
    std::size_t _expected_input = 0;
    boost::system::error_code _error;
};

using NetworkOutputBuffer = boost::asio::streambuf;

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
        _socket.write(_output_buffer, error);
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


} // namespace httpony
#endif // HTTPONY_CONNECTION_HPP
