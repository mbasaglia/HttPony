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


#define BOOST_TEST_MODULE HttPony_BaseEncoding
#include <boost/test/unit_test.hpp>

#include "base_encoding.hpp"

using namespace httpony;

BOOST_AUTO_TEST_CASE( test_base64_encode )
{
    std::string output;
    BOOST_CHECK( Base64().encode("EUP", output) );
    BOOST_CHECK_EQUAL( output, "RVVQ" );

    BOOST_CHECK( Base64().encode("Hello!", output) );
    BOOST_CHECK_EQUAL( output, "SGVsbG8h" );

    BOOST_CHECK( Base64().encode("1", output) );
    BOOST_CHECK_EQUAL( output, "MQ==" );

    BOOST_CHECK( Base64().encode("x", output) );
    BOOST_CHECK_EQUAL( output, "eA==" );

    BOOST_CHECK( Base64().encode("Hello world", output) );
    BOOST_CHECK_EQUAL( output, "SGVsbG8gd29ybGQ=" );

    BOOST_CHECK( Base64().encode("HttPony", output) );
    BOOST_CHECK_EQUAL( output, "SHR0UG9ueQ==" );

    BOOST_CHECK_EQUAL( Base64().encode("~~>~~?"), "fn4+fn4/" );
    BOOST_CHECK_EQUAL( Base64('-', '_').encode("~~>~~?"), "fn4-fn4_" );
    BOOST_CHECK_EQUAL( Base64(false).encode("x"), "eA" );
}
