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
#ifndef MUHTTPD_HPP
#define MUHTTPD_HPP

#include <string>
#include <melanolib/utils/c++-compat.hpp>

namespace muhttpd {

struct Content
{
    Content(std::string data, std::string mime_type)
        : data(std::move(data)), mime_type(std::move(mime_type))
    {}

    std::string data;
    std::string mime_type;

    static Content text(const std::string& data)
    {
        return Content(data, "text/plain");
    }
};

struct Body : public melanolib::Optional<Content>
{
    using melanolib::Optional<Content>::Optional;

    std::size_t content_length() const
    {
        return *this ? (*this)->data.size() : 0;
    }
};

} // namespace muhttpd
#endif // MUHTTPD_HPP
