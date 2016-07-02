/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2016 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MUHTTPD_IO_HPP
#define MUHTTPD_IO_HPP

#include <iostream>
#include <boost/asio.hpp>

#include "headers.hpp"

namespace muhttpd {


struct NetworkBuffer : boost::asio::streambuf
{
    explicit NetworkBuffer(boost::asio::ip::tcp::socket socket)
        : _socket(std::move(socket))
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

    boost::asio::ip::tcp::socket& socket()
    {
        return _socket;
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
    boost::asio::ip::tcp::socket _socket;
    std::size_t _expected_input = 0;
    boost::system::error_code _error;
};

/**
 * \brief Gives access to the request body from a connection object
 */
class NetworkInputStream : std::istream
{
public:
    explicit NetworkInputStream(NetworkBuffer& buffer, const Headers& headers);
//     NetworkInputStream(NetworkInputStream&& other) = default;
//     NetworkInputStream& operator=(NetworkInputStream&& other) = default;

//     NetworkInputStream()
//         : std::istream(nullptr)
//     {
//         _error = boost::system::errc::make_error_code(
//             boost::system::errc::no_message
//         );
//     }

    boost::system::error_code error() const
    {
        return _error;
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

    std::string read_all()
    {
        std::string all(_content_length, ' ');
        read(&all[0], _content_length);
        all.resize(gcount());
        return all;
    }


    std::string content_type() const
    {
        return _content_type;
    }

private:
    std::size_t _content_length = 0;
    std::string _content_type;
    boost::system::error_code _error;
};

} // namespace muhttpd
#endif // MUHTTPD_IO_HPP
