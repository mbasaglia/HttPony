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

/// \cond
#include <iostream>
/// \endcond

#include "httpony/io/buffer.hpp"
#include "httpony/ip_address.hpp"

namespace httpony {
namespace io {

class Connection
{
public:
    /**
     * \brief Stream used to send data through the connection
     * \note There should be only one send stream per connection at a given time
     * \todo Better async streaming
     */
    class SendStream : public std::ostream
    {
    public:
        SendStream(SendStream&& oth)
            : std::ostream(oth.rdbuf()),
              connection(oth.connection)
        {
            oth.unset();
        }

        SendStream& operator=(SendStream&& oth)
        {
            rdbuf(oth.rdbuf());
            connection = oth.connection;
            oth.unset();
            return *this;
        }

        ~SendStream()
        {
            send();
        }

        /**
         * \brief Ensures all data is being sent,
         * \returns \b true on success
         * \note If not called, the data will still be sent but you might not be
         * able to detect whether that has been successful
         */
        bool send()
        {
            if ( connection && connection->commit_output() )
                return true;
            setstate(badbit);
            return false;
        }

    private:
        void unset()
        {
            connection = nullptr;
            rdbuf(nullptr);
        }

        SendStream(Connection* connection)
            : std::ostream(&connection->output_buffer()),
             connection(connection)
        {}

        friend Connection;
        Connection* connection;
    };

    class ReceiveStream : public std::istream
    {
    public:
        ReceiveStream(ReceiveStream&& oth)
            : std::istream(oth.rdbuf()),
              connection(oth.connection)
        {
            oth.unset();
        }

        ReceiveStream& operator=(ReceiveStream&& oth)
        {
            rdbuf(oth.rdbuf());
            connection = oth.connection;
            oth.unset();
            return *this;
        }

        bool timed_out() const
        {
            return connection->_socket.timed_out();
        }

    private:
        void unset()
        {
            connection = nullptr;
            rdbuf(nullptr);
        }

        ReceiveStream(Connection* connection)
            : std::istream(&connection->input_buffer()),
             connection(connection)
        {
            boost::system::error_code error;
            /// \todo Avoid magic number, keep reading if needed
            auto sz = connection->_input_buffer.read_some(1024, error);
            if ( error || sz <= 0 )
                setstate(badbit);
        }

        friend Connection;
        Connection* connection;
    };


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
        if ( _output_buffer.size() == 0 )
            return true;
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
        return _socket.remote_address();
    }

    IPAddress local_address() const
    {
        return _socket.local_address();
    }

    SendStream send_stream()
    {
        return SendStream(this);
    }

    bool connected() const
    {
        return _socket.is_open();
    }

    ReceiveStream receive_stream()
    {
        return ReceiveStream(this);
    }

    TimeoutSocket& socket()
    {
        return _socket;
    }


private:

    TimeoutSocket       _socket;
    NetworkInputBuffer  _input_buffer{_socket};
    NetworkOutputBuffer _output_buffer;
};


} // namespace io
} // namespace httpony
#endif // HTTPONY_CONNECTION_HPP
