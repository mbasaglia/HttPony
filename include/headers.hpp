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
#ifndef MUHTTPD_HEADERS_HPP
#define MUHTTPD_HEADERS_HPP

#include <string>
#include <vector>

#include <melanolib/string/simple_stringutils.hpp>

namespace muhttpd {

/**
 * \brief Header name and value pair
 */
struct Header
{
    Header(std::string name, std::string value)
        : name(std::move(name)), value(std::move(value))
    {}

    bool operator==(const Header& oth) const
    {
        return name == oth.name && value == oth.value;
    }

    std::string name;
    std::string value;
};

/**
 * \brief Case-insensitive string comparator class
 */
struct ICaseComparator
{
    bool operator()(const std::string& a, const std::string& b) const
    {
        return melanolib::string::icase_equal(a, b);
    }
};

/**
 * \brief Thin wrapper around a vector of Header object prividing
 * dictionary-like access
 * \todo Hide the vector and implement the container concept
 */
template<class Comparator = std::equal_to<std::string>>
    struct OrderedMultimap
{
    /**
     * \brief Whether it has a header matching the given name
     */
    bool has_header(const std::string& name) const
    {
        for ( const auto& header : headers )
        {
            if ( Comparator()(header.name, name) )
                return true;
        }
        return false;
    }

    /**
     * \brief Counts the number of headers matching the given name
     */
    std::size_t count(const std::string& name) const
    {
        std::size_t n = 0;
        for ( const auto& header : headers )
        {
            if ( Comparator()(header.name, name) )
                n++;
        }
        return n;
    }

    /**
     * \see get()
     */
    std::string operator[](const std::string& name) const
    {
        return get(name);
    }

    /**
     * \brief Returns the first occurence of a header with the given name
     *
     * If no header is found, returns the empty string
     */
    std::string get(const std::string& name) const
    {
        for ( const auto& header : headers )
        {
            if ( Comparator()(header.name, name) )
                return header.value;
        }
        return "";
    }

    /**
     * \brief Returns the first occurence of a header with the given name
     *
     * If no header is found, a new one is added
     */
    std::string& operator[](const std::string& name)
    {
        for ( auto& header : headers )
        {
            if ( Comparator()(header.name, name) )
                return header.value;
        }
        headers.push_back({name, ""});
        return headers.back().value;
    }

    /**
     * \brief Appends a new header
     */
    void append(std::string name, std::string value)
    {
        headers.emplace_back(name, value);
    }

    auto begin() const
    {
        return headers.begin();
    }

    auto end() const
    {
        return headers.end();
    }

    std::vector<Header> headers;
};

using Headers = OrderedMultimap<ICaseComparator>;
using DataMap = OrderedMultimap<>;

} // namespace muhttpd
#endif // MUHTTPD_HEADERS_HPP
