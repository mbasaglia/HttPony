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

#ifndef HTTPONY_NETWORK_STREAM_HPP
#define HTTPONY_NETWORK_STREAM_HPP

#include <iostream>

/// \todo Limit asio dependencies to connection.hpp and server.cpp
#include <boost/asio.hpp>

#include "mime_type.hpp"
#include "headers.hpp"

namespace httpony {


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

    InputContentStream(InputContentStream&& other)
        : std::istream(other.rdbuf()),
          _content_length(other._content_length),
          _content_type(std::move(other._content_type)),
          _error(other._error)
        {}

    InputContentStream& operator=(InputContentStream&& other)
    {
        rdbuf(other.rdbuf());
        _content_length = other._content_length;
        std::swap(_content_type, other._content_type);
        std::swap(_error, other._error);
        return *this;
    }

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
    bool _error = false;
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

    OutputContentStream(OutputContentStream&& other)
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
    void start_data(const MimeType& content_type)
    {
        rdbuf(&buffer);
        _content_type = content_type;
    }

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
#endif // HTTPONY_NETWORK_STREAM_HPP
