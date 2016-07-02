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
#ifndef HTTPONY_URI_HPP
#define HTTPONY_URI_HPP

#include "headers.hpp"

namespace httpony {

struct Uri
{

    Uri(const std::string& uri);
    Uri() = default;

    std::string full() const;

    std::string path_string() const;

    std::string query_string(bool question_mark = false) const;

    std::string scheme;
    std::string authority;
    std::vector<std::string> path;
    DataMap query;
    std::string fragment;

    /**
     * \brief URI equivalence
     */
    bool operator==(const Uri& oth) const;
};

std::string urlencode(const std::string& input, bool plus_spaces = false);
std::string urldecode(const std::string& input, bool plus_spaces = false);

DataMap parse_query_string(const std::string& str);

std::string build_query_string(const DataMap& headers, bool question_mark = false);

} // namespace httpony
#endif // HTTPONY_URI_HPP
