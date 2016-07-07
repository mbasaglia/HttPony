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

#ifndef HTTPONY_IO_HPP
#define HTTPONY_IO_HPP

#include <iostream>
#include <boost/asio.hpp>

#include "mime_type.hpp"
#include "ip_address.hpp"

namespace httpony {

/**
 * \brief Stream buffer linked to a socket for reading
 */
class NetworkInputBuffer : public boost::asio::streambuf
{
public:
    explicit NetworkInputBuffer(boost::asio::ip::tcp::socket& socket)
        : _socket(socket)
    {
    }

    /**
     * \brief Reads up to size from the socket
     */
    std::size_t read_some(std::size_t size, boost::system::error_code& error)
    {
        /// \todo timeout
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
    boost::asio::ip::tcp::socket& _socket;
    std::size_t _expected_input = 0;
    boost::system::error_code _error;
};

using NetworkOutputBuffer = boost::asio::streambuf;

class NetworkIO
{
public:
    explicit NetworkIO(boost::asio::ip::tcp::socket socket)
        : _socket(std::move(socket)), _input_buffer(_socket)
    {
    }

    NetworkInputBuffer& input_buffer()
    {
        return _input_buffer;
    }

    boost::asio::streambuf& output_buffer()
    {
        return _output_buffer;
    }

    void commit_output()
    {
        boost::asio::write(_socket, _output_buffer);
    }

    void close()
    {
        _socket.close();
    }

    IPAddress remote_address() const
    {
        return endpoint_to_ip(_socket.remote_endpoint());
    }

    IPAddress local_address() const
    {
        return endpoint_to_ip(_socket.local_endpoint());
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

    boost::asio::ip::tcp::socket _socket;
    NetworkInputBuffer  _input_buffer;
    NetworkOutputBuffer _output_buffer;

};


/**
 * \brief Reads an incoming message payload
 */
class InputContentStream : public std::istream
{
public:
    explicit InputContentStream(std::streambuf* buffer, const Headers& headers);

    InputContentStream()
        : std::istream(nullptr)
    {}

    bool get_data(std::streambuf* buffer, const Headers& headers);

    bool has_data() const
    {
        return !_error && rdbuf();
    }

    bool has_error() const
    {
        return fail() || _error;
    }

    explicit operator bool() const
    {
        return !_error && !fail();
    }

    bool operator!() const
    {
        return _error || fail();
    }

    std::size_t content_length() const
    {
        return _content_length;
    }

    std::string read_all();

    MimeType content_type() const
    {
        return _content_type;
    }

private:
    std::size_t _content_length = 0;
    MimeType _content_type;
    boost::system::error_code _error;
};

/**
 * \brief Writes an outgoing message payload
 */
class OutputContentStream: public std::ostream
{
public:
    explicit OutputContentStream(MimeType content_type)
        : std::ostream(&buffer), _content_type(std::move(content_type))
    {
    }

    explicit OutputContentStream(OutputContentStream&& other)
        : std::ostream(other.rdbuf() ? &buffer : nullptr),
          _content_type(std::move(other._content_type))
    {
        if ( has_data() )
            copy_from(other);
    }

    OutputContentStream& operator=(OutputContentStream&& other)
    {
        stop_data();
        if ( other.has_data() )
        {
            rdbuf(other.rdbuf() ? &buffer : nullptr);
            copy_from(other);
        }
        _content_type = std::move(other._content_type);
        /// \todo Transfer data?
        return *this;
    }

    OutputContentStream()
        : std::ostream(nullptr)
    {
    }

    ~OutputContentStream()
    {
        flush();
        rdbuf(nullptr);
    }

    /**
     * \brief Sets up the stream to contain data in the specified encoding
     */
    void start_data(const std::string& content_type)
    {
        rdbuf(&buffer);
        _content_type = content_type;
    }

    /**
     * \brief Removes all data from the stream, call start() to re-introduce it
     */
    void stop_data()
    {
        flush();
        buffer.consume(buffer.size());
        rdbuf(nullptr);
    }

    /**
     * \brief Whether there is some data to send (which might have 0 length) or no data at all
     */
    bool has_data() const
    {
        return rdbuf() && _content_type.valid();
    }

    std::size_t content_length() const
    {
        return buffer.size();
    }

    MimeType content_type() const
    {
        return _content_type;
    }

    /**
     * \brief Writes the payload to a socket
     */
    void net_write(boost::asio::ip::tcp::socket& socket)
    {
        if ( has_data() )
            boost::asio::write(socket, buffer);
    }

    /**
     * \brief Writes the payload to a stream
     */
    void write_to(std::ostream& output)
    {
        if ( has_data() )
            output << &buffer;
    }

private:
    void copy_from(OutputContentStream& other);

    boost::asio::streambuf buffer;
    MimeType _content_type;
};

} // namespace httpony
#endif // HTTPONY_IO_HPP
