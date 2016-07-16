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
#ifndef HTTPONY_IO_SOCKET_HPP
#define HTTPONY_IO_SOCKET_HPP

#include <boost/asio.hpp>
#include <melanolib/time/date_time.hpp>

namespace httpony {
namespace io {

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
        return io_operation(&TimeoutSocket::do_async_read_some, boost::asio::buffer(buffer), error);
    }

    template<class ConstBufferSequence>
        std::size_t write(ConstBufferSequence&& buffer, boost::system::error_code& error)
    {
        return io_operation(&TimeoutSocket::do_async_write, boost::asio::buffer(buffer), error);
    }

private:
    class AsyncCallback
    {
    public:
        AsyncCallback(boost::system::error_code& error_code, std::size_t& bytes_transferred)
            : error_code(&error_code), bytes_transferred(&bytes_transferred)
        {
            *this->error_code = boost::asio::error::would_block;
            *this->bytes_transferred = 0;
        }

        void operator()(const boost::system::error_code& ec, std::size_t bt) const
        {
            *error_code = ec;
            *bytes_transferred += bt;
        }

    private:
        boost::system::error_code* error_code;
        std::size_t* bytes_transferred;
    };

    template<class Buffer>
        std::size_t io_operation(void (TimeoutSocket::*func)(Buffer&, const AsyncCallback&), Buffer&& buffer, boost::system::error_code& error)
    {
        std::size_t read_size;

        (this->*func)(buffer, AsyncCallback(error, read_size));

        do
            _io_service.run_one();
        while ( !_io_service.stopped() && error == boost::asio::error::would_block );

        return read_size;
    }

    void do_async_read_some(boost::asio::mutable_buffers_1& buffer, const AsyncCallback& callback)
    {
        _socket.async_read_some(buffer, callback);
    }

    void do_async_write(boost::asio::const_buffers_1& buffer, const AsyncCallback& callback)
    {
        boost::asio::async_write(_socket, buffer, callback);
    }

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

} // namespace io
} // namespace httpony
#endif // HTTPONY_IO_SOCKET_HPP
