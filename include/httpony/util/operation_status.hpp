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

#ifndef HTTPONY_OPERATION_STATUS_HPP
#define HTTPONY_OPERATION_STATUS_HPP

/// \cond
#include <string>
#include <ostream>
/// \endcond

namespace httpony {


class OperationStatus
{
public:
    OperationStatus() = default;

    OperationStatus(std::string message)
        : _message(std::move(message))
    {}

    OperationStatus(const char* message)
        : _message(message)
    {}

    const std::string& message() const
    {
        return _message;
    }

    explicit operator bool() const
    {
        return !error();
    }

    bool operator!() const
    {
        return error();
    }

    bool error() const
    {
        return !_message.empty();
    }

private:
    std::string _message;
};

inline std::ostream& operator<<(std::ostream& stream, const OperationStatus& status)
{
    return stream << status.message();
}

} // namespace httpony
#endif // HTTPONY_OPERATION_STATUS_HPP
