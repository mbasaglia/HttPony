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

/// \cond
#include <melanolib/time/date_time.hpp>
/// \endcond

namespace httpony {
namespace io {

using boost_tcp = boost::asio::ip::tcp;

class SocketWrapper
{
public:
    using raw_socket_type = boost_tcp::socket;

    /**
     * \brief Functor for ASIO calls
     */
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

    virtual ~SocketWrapper() {}

    /**
     * \brief Closes the socket, further IO calls will fail
     *
     * It shouldn't throw an exception on failure
     */
    virtual void close() = 0;

    /**
     * \brief Returns the low-level socket object
     */
    virtual raw_socket_type& raw_socket() = 0;
    /**
     * \brief Returns the low-level socket object
     */
    virtual const raw_socket_type& raw_socket() const = 0;

    /**
     * \brief Async IO call to read from the socket into a buffer
     */
    virtual void async_read_some(boost::asio::mutable_buffers_1& buffer, const AsyncCallback& callback) = 0;

    /**
     * \brief Async IO call to write to the socket from a buffer
     */
    virtual void async_write(boost::asio::const_buffers_1& buffer, const AsyncCallback& callback) = 0;
};

class PlainSocket : public SocketWrapper
{
public:
    explicit PlainSocket(boost::asio::io_service& io_service)
        : socket(io_service)
    {}

    void close() override
    {
        boost::system::error_code error;
        socket.close(error);
    }

    raw_socket_type& raw_socket() override
    {
        return socket;
    }

    const raw_socket_type& raw_socket() const override
    {
        return socket;
    }

    void async_read_some(boost::asio::mutable_buffers_1& buffer, const AsyncCallback& callback) override
    {
        socket.async_read_some(buffer, callback);
    }

    void async_write(boost::asio::const_buffers_1& buffer, const AsyncCallback& callback) override
    {
        boost::asio::async_write(socket, buffer, callback);
    }

private:
    raw_socket_type socket;
};

template<class SocketType>
    struct SocketTag
{
    using type = SocketType;
};

/**
 * \brief A network socket with a timeout
 */
class TimeoutSocket
{
public:
    /**
     * \brief Creates a socket without setting a timeout
     */
    template<class SocketType, class... ExtraArgs>
        explicit TimeoutSocket(SocketTag<SocketType>, ExtraArgs&&... args)
            : _socket(std::make_unique<SocketType>(_io_service, std::forward<ExtraArgs>(args)...))
    {
        clear_timeout();
        check_deadline();
    }

    /**
     * \brief Closes the socket before destucting the object
     */
    ~TimeoutSocket()
    {
        close();
    }

    /**
     * \brief Closes the socket, further IO calls will fail
     */
    void close()
    {
        _socket->close();
    }

    /**
     * \brief Whether the socket timed out
     */
    bool timed_out() const
    {
        return _deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now();
    }

    /**
     * \brief Returns the low-level socket object
     */
    boost_tcp::socket& raw_socket()
    {
        return _socket->raw_socket();
    }

    /**
     * \brief Returns the low-level socket object
     */
    const boost_tcp::socket& raw_socket() const
    {
        return _socket->raw_socket();
    }

    SocketWrapper& socket_wrapper()
    {
        return *_socket;
    }

    /**
     * \brief Sets the timeout
     *
     * timed_out() will be true this many seconds from this call
     * \see clear_timeout()
     */
    void set_timeout(melanolib::time::seconds timeout)
    {
        _deadline.expires_from_now(boost::posix_time::seconds(timeout.count()));
    }

    /**
     * \brief Removes the timeout, IO calls will block indefinitely after this
     * \see set_timeout()
     */
    void clear_timeout()
    {
        _deadline.expires_at(boost::posix_time::pos_infin);
    }

    /**
     * \brief Reads some data to fill the input buffer
     * \returns The number of bytes written to the destination
     */
    template<class MutableBufferSequence>
        std::size_t read_some(MutableBufferSequence&& buffer, boost::system::error_code& error)
    {
        return io_operation(&SocketWrapper::async_read_some, boost::asio::buffer(buffer), error);
    }

    /**
     * \brief writes all data from the given buffer
     * \returns The number of bytes read from the source
     */
    template<class ConstBufferSequence>
        std::size_t write(ConstBufferSequence&& buffer, boost::system::error_code& error)
    {
        return io_operation(&SocketWrapper::async_write, boost::asio::buffer(buffer), error);
    }


    bool connect(
        boost_tcp::resolver::iterator endpoint_iterator,
        boost::system::error_code& error
    )
    {
        error = boost::asio::error::would_block;

        boost::asio::async_connect(raw_socket(), endpoint_iterator,
            [this, &error](
                const boost::system::error_code& error_code,
                boost_tcp::resolver::iterator endpoint_iterator
            )
            {
                error = error_code;
            }
        );

        io_loop(&error);

        return !error;
    }

    boost_tcp::resolver::iterator resolve(
        const boost_tcp::resolver::query& query,
        boost::system::error_code& error
    )
    {
        boost_tcp::resolver resolver(_io_service);

        boost_tcp::resolver::iterator result;
        resolver.async_resolve(
            query,
            [&error, &result](
                const boost::system::error_code& error_code,
                const boost_tcp::resolver::iterator& iterator
            )
            {
                error = error_code;
                result = iterator;
            }
        );

        io_loop(&error);

        return result;
    }

private:
    /**
     * \brief Calls a function for async IO operations, then runs the io service until completion or timeout
     */
    template<class Buffer>
    std::size_t io_operation(
        void (SocketWrapper::*func)(Buffer&, const SocketWrapper::AsyncCallback&),
        Buffer&& buffer,
        boost::system::error_code& error
    )
    {
        std::size_t read_size;

        ((*_socket).*func)(buffer, SocketWrapper::AsyncCallback(error, read_size));

        io_loop(&error);

        return read_size;
    }

    void io_loop(boost::system::error_code* error)
    {
        do
            _io_service.run_one();
        while ( !_io_service.stopped() && *error == boost::asio::error::would_block );
    }

    /**
     * \brief Async wait for the timeout
     */
    void check_deadline()
    {
        if ( timed_out() )
        {
            _deadline.expires_at(boost::posix_time::pos_infin);
            _io_service.stop();
        }

        _deadline.async_wait([this](const boost::system::error_code&){check_deadline();});
    }

    boost::asio::io_service _io_service;
    std::unique_ptr<SocketWrapper> _socket;
    boost::asio::deadline_timer _deadline{_io_service};
};

} // namespace io
} // namespace httpony
#endif // HTTPONY_IO_SOCKET_HPP
