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


#define BOOST_TEST_MODULE HttPony_MimeType
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include "httpony/http/protocol.hpp"

using namespace httpony;

BOOST_AUTO_TEST_CASE( test_cmp )
{
    BOOST_CHECK( Protocol("FOO", 1, 2) == Protocol("FOO", 1, 2) );
    BOOST_CHECK( Protocol("FOO", 1, 2) >= Protocol("FOO", 1, 2) );
    BOOST_CHECK( Protocol("FOO", 1, 2) <= Protocol("FOO", 1, 2) );
    BOOST_CHECK( !(Protocol("FOO", 1, 2) != Protocol("FOO", 1, 2)) );
    BOOST_CHECK( !(Protocol("FOO", 1, 2) > Protocol("FOO", 1, 2)) );
    BOOST_CHECK( !(Protocol("FOO", 1, 2) < Protocol("FOO", 1, 2)) );

    BOOST_CHECK( !(Protocol("FOO", 1, 3) == Protocol("FOO", 1, 2)) );
    BOOST_CHECK( Protocol("FOO", 1, 3) >= Protocol("FOO", 1, 2) );
    BOOST_CHECK( !(Protocol("FOO", 1, 3) <= Protocol("FOO", 1, 2)) );
    BOOST_CHECK( Protocol("FOO", 1, 3) != Protocol("FOO", 1, 2) );
    BOOST_CHECK( Protocol("FOO", 1, 3) > Protocol("FOO", 1, 2) );
    BOOST_CHECK( !(Protocol("FOO", 1, 3) < Protocol("FOO", 1, 2)) );

    BOOST_CHECK( !(Protocol("FOO", 1, 2) == Protocol("FOO", 1, 3)) );
    BOOST_CHECK( !(Protocol("FOO", 1, 2) >= Protocol("FOO", 1, 3)) );
    BOOST_CHECK( Protocol("FOO", 1, 2) <= Protocol("FOO", 1, 3) );
    BOOST_CHECK( Protocol("FOO", 1, 2) != Protocol("FOO", 1, 3) );
    BOOST_CHECK( !(Protocol("FOO", 1, 2) > Protocol("FOO", 1, 3)) );
    BOOST_CHECK( Protocol("FOO", 1, 2) < Protocol("FOO", 1, 3) );

    BOOST_CHECK( !(Protocol("FOO", 1, 2) == Protocol("BAR", 1, 2)) );
    BOOST_CHECK( !(Protocol("FOO", 1, 2) >= Protocol("BAR", 1, 2)) );
    BOOST_CHECK( !(Protocol("FOO", 1, 2) <= Protocol("BAR", 1, 2)) );
    BOOST_CHECK( Protocol("FOO", 1, 2) != Protocol("BAR", 1, 2) );
    BOOST_CHECK( !(Protocol("FOO", 1, 2) > Protocol("BAR", 1, 2)) );
    BOOST_CHECK( !(Protocol("FOO", 1, 2) < Protocol("BAR", 1, 2)) );
}

BOOST_AUTO_TEST_CASE( test_valid )
{
    BOOST_CHECK( Protocol("FOO", 1, 2).valid() );
    BOOST_CHECK( !Protocol().valid() );
}

BOOST_AUTO_TEST_CASE( test_ctor )
{
    BOOST_CHECK( Protocol("FOO/1.2") == Protocol("FOO", 1, 2) );
    BOOST_CHECK( !Protocol("FOO").valid() );
    BOOST_CHECK( !Protocol("FOO/bar").valid() );
    BOOST_CHECK( !Protocol("FOO/b.r").valid() );
    BOOST_CHECK( !Protocol("FOO/.").valid() );
}

BOOST_AUTO_TEST_CASE( test_stream_out )
{
    boost::test_tools::output_test_stream output;

    output << Protocol("FOO", 1, 2);
    BOOST_CHECK( output.is_equal( "FOO/1.2" ) );

    output << Protocol("FOO/");
    BOOST_CHECK( output.is_equal( "" ) );
}

BOOST_AUTO_TEST_CASE( test_stream_in )
{
    std::istringstream input("FOO/1.2");
    Protocol proto;
    BOOST_CHECK( input >> proto );
    BOOST_CHECK( proto == Protocol("FOO", 1, 2) );
}

BOOST_AUTO_TEST_CASE( test_constants )
{
    BOOST_CHECK( Protocol::http_1_0 == Protocol("HTTP", 1, 0) );
    BOOST_CHECK( Protocol::http_1_1 == Protocol("HTTP", 1, 1) );
}
