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

/// \cond
#include <melanolib/data_structures/ordered_multimap.hpp>
#include <melanolib/string/quickstream.hpp>
#include <melanolib/string/ascii.hpp>
#include <melanolib/string/stringutils.hpp>
/// \endcond

namespace httpony {

using Headers = melanolib::OrderedMultimap<std::string, std::string, melanolib::ICaseComparator>;
using DataMap = melanolib::OrderedMultimap<>;

struct CompoundHeader
{
    std::string value;
    Headers parameters;
};

} // namespace httpony
#endif // HTTPONY_HEADERS_HPP
