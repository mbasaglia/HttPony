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

#ifndef HTTPONY_HEADERS_HPP
#define HTTPONY_HEADERS_HPP

#include <string>
#include <vector>

#include <melanolib/string/simple_stringutils.hpp>

namespace httpony {

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
 * \brief Associative container which allows multiple items with the same
 * key and preserves insertion order
 */
template<
    class Key = std::string, class
    Mapped = std::string,
    class Compare = std::equal_to<std::string>,
    class MappedCompare = std::equal_to<std::string>
>
    class OrderedMultimap
{
public:
    using key_type = Key;
    using key_compare = Compare;
    using mapped_type = Mapped;
    using value_type = std::pair<key_type, mapped_type>;
    using container_type = std::vector<value_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using reverse_iterator = typename container_type::reverse_iterator;
    using const_reverse_iterator = typename container_type::const_reverse_iterator;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using pointer = typename container_type::pointer;
    using const_pointer = typename container_type::const_pointer;
    class value_compare
    {
    public:
        value_compare(const key_compare& key = key_compare()) : key(key) {}

        bool operator()(const value_type& a, const value_type& b) const
        {
            return key(a.first, b.first) && MappedCompare()(a.second, b.second);
        }

        key_compare key;
        MappedCompare value;
    };

    OrderedMultimap() = default;
    OrderedMultimap(const OrderedMultimap&) = default;
    OrderedMultimap(OrderedMultimap&&) = default;

    explicit OrderedMultimap(const key_compare& comp)
        : compare(comp)
    {}

    OrderedMultimap(container_type data)
        : data(std::move(data))
    {}

    OrderedMultimap(std::initializer_list<value_type> data, const key_compare& comp = Compare())
        : data(data),
          compare(comp)
    {}

    template<class InputIterator>
    OrderedMultimap(InputIterator&& first, InputIterator&& last, const key_compare& comp = Compare())
        : data(std::forward(first), std::forward(last)),
        compare(comp)
    {}

    OrderedMultimap& operator=(const OrderedMultimap&) = default;
    OrderedMultimap& operator=(OrderedMultimap&&) = default;
    OrderedMultimap& operator=(std::initializer_list<value_type> data)
    {
        this->data = data;
        return *this;
    }

    Mapped& at(const key_type& key)
    {
        for ( auto& item : data )
        {
            if ( compare.key(item.first, key) )
                return item.second;
        }
        throw std::out_of_range("Item not found");
    }

    const Mapped& at(const key_type& key) const
    {
        for ( const auto& item : data )
        {
            if ( compare.key(item.first, key) )
                return item.second;
        }
        throw std::out_of_range("Item not found");
    }


    /**
     * \see get()
     */
    Mapped operator[](const key_type& key) const
    {
        return get(key);
    }

    /**
     * \brief Returns the first occurence of a header with the given name
     *
     * If no header is found, a new one is added
     */
    Mapped& operator[](const key_type& key)
    {
        for ( auto& item : data )
        {
            if ( compare.key(item.first, key) )
                return item.second;
        }
        data.push_back({key, ""});
        return data.back().second;
    }


    auto begin()        { return data.begin(); }
    auto end()          { return data.end();   }
    auto begin() const  { return data.begin(); }
    auto end() const    { return data.end();   }
    auto cbegin() const { return data.cbegin();}
    auto cend() const   { return data.cend();  }

    auto rbegin()        { return data.rbegin(); }
    auto rend()          { return data.rend();   }
    auto rbegin() const  { return data.rbegin(); }
    auto rend() const    { return data.rend();   }
    auto crbegin() const { return data.crbegin();}
    auto crend() const   { return data.crend();  }

    auto empty() const { return data.empty(); }
    auto size() const { return data.size(); }
    auto max_size() const { return data.max_size(); }

    template<class... Args>
        auto insert(Args&&... args)
    {
        return data.insert(std::forward<Args>(args)...);
    }

    template<class... Args>
        auto emplace(Args&&... args)
    {
        return data.emplace(std::forward<Args>(args)...);
    }

    template<class... Args>
        auto erase(Args&&... args)
    {
        return data.erase(std::forward<Args>(args)...);
    }

    size_type erase(const key_type& key)
    {
        auto last = std::remove_if(begin(), end(),
            [this, &key](const_reference item) {
                return compare.key(item.key, key);
            }
        );
        auto count = end() - last;
        data.erase(last, end());
        return count;
    }

    void swap(OrderedMultimap& oth)
    {
        swap(data, oth.data);
        swap(compare, oth.compare);
    }

    /**
     * \brief Counts the number of headers matching the given name
     */
    size_type count(const key_type& key) const
    {
        size_type n = 0;
        for ( const auto& item : data )
        {
            if ( compare.key(item.first, key) )
                n++;
        }
        return n;
    }

    iterator find(const key_type& key)
    {
        for( auto iter = begin(); iter != end(); ++iter )
            if ( compare.key(iter->first, key) )
                return iter;
        return end();
    }

    const_iterator find(const key_type& key) const
    {
        for( auto iter = begin(); iter != end(); ++iter )
            if ( compare.key(iter->first, key) )
                return iter;
        return end();
    }

    key_compare key_comp() const
    {
        return compare.key;
    }

    value_compare value_comp() const
    {
        return compare;
    }

// end of associative container

    /**
     * \brief Whether it has a header matching the given name
     */
    bool contains(const key_type& key) const
    {
        for ( const auto& item : data )
        {
            if ( compare.key(item.first, key) )
                return true;
        }
        return false;
    }

    /**
     * \brief Returns the first occurence of a header with the given name
     *
     * If no header is found, returns the empty string
     */
    std::string get(const key_type& key) const
    {
        for( const auto& item : data )
        {
            if ( compare.key(item.first, key) )
                return item.second;
        }
        return "";
    }

    /**
     * \brief Appends a new header
     */
    void append(std::string name, std::string value)
    {
        data.emplace_back(name, value);
    }

    bool operator==(const OrderedMultimap& oth) const
    {
        return std::equal(begin(), end(), oth.begin(), oth.end(), compare);
    }

    bool operator!=(const OrderedMultimap& oth) const
    {
        return !(*this == oth);
    }

private:
    container_type data;
    value_compare  compare;
};

using Headers = OrderedMultimap<std::string, std::string, ICaseComparator>;
using DataMap = OrderedMultimap<>;

} // namespace httpony
#endif // HTTPONY_HEADERS_HPP
