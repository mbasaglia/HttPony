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
#ifndef HTTPONY_IO_BUFFER_HPP
#define HTTPONY_IO_BUFFER_HPP

#include "httpony/io/socket.hpp"

namespace httpony {
namespace io {


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

        auto read_size = _socket.read_some(in_buffer, error);

        commit(read_size);

        return read_size + prev_size;
    }

    /**
     * \brief Expect at least \p byte_count to be available in the socket
     */
    void expect_input(std::size_t byte_count)
    {
        if ( byte_count > size() )
            _expected_input = byte_count - size();
        else
            _expected_input = 0;
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

} // namespace io
} // namespace httpony
#endif // HTTPONY_IO_BUFFER_HPP
