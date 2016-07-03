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

#include "io.hpp"

namespace httpony {


NetworkInputStream::NetworkInputStream(NetworkBuffer& buffer, const Headers& headers)
    : std::istream(&buffer)
{
    get_data(buffer, headers);
}

bool NetworkInputStream::get_data(NetworkBuffer& buffer, const Headers& headers)
{
    rdbuf(&buffer);

    /// \todo handle Transfer-Encoding
    std::string length = headers.get("Content-Length");
    std::string content_type = headers.get("Content-Type");

    if ( length.empty() || !std::isdigit(length[0]) || content_type.empty() )
    {
        _content_length = 0;
        _content_type = {};
        rdbuf(nullptr);
        _error = boost::system::errc::make_error_code(
            boost::system::errc::bad_message
        );
        return false;
    }

    _content_length = std::stoul(length);
    _content_type = std::move(content_type);

    buffer.expect_input(_content_length);

    _error = boost::system::error_code();
    return true;
}

void NetworkOutputStream::copy_from(NetworkOutputStream& other)
{
    other.flush();
    for ( const auto& buf : other.buffer.data() )
    {
        auto data = boost::asio::buffer_cast<const char*>(buf);
        auto size = boost::asio::buffer_size(buf);
        write(data, size);
    }
}


} // namespace httpony
