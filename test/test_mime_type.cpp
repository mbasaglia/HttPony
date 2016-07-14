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

#include "httpony/mime_type.hpp"

using namespace httpony;

BOOST_AUTO_TEST_CASE( test_cmp )
{
    BOOST_CHECK( MimeType("text", "plain") == MimeType("text", "plain") );
    BOOST_CHECK( MimeType("text", "plain") != MimeType("text", "x-c") );
    BOOST_CHECK( MimeType("text", "plain") != MimeType("image", "plain") );
    BOOST_CHECK( MimeType("text", "plain") != MimeType("text", "plain", {"charset", "utf-8"}) );
    BOOST_CHECK( MimeType("text", "plain", {"charset", "utf-8"}) == MimeType("text", "plain", {"charset", "utf-8"}) );
    BOOST_CHECK( MimeType("text", "plain", {"charset", "utf-8"}) == MimeType("TEXT", "Plain", {"Charset", "UTF-8"}) );
    BOOST_CHECK( MimeType("text", "plain", {"foo", "bar"}) == MimeType("TEXT", "Plain", {"Foo", "bar"}) );
    BOOST_CHECK( MimeType("text", "plain", {"foo", "bar"}) != MimeType("TEXT", "Plain", {"Foo", "BAR"}) );
}

BOOST_AUTO_TEST_CASE( test_ctor )
{
    BOOST_CHECK( MimeType("text/plain") == MimeType("text", "plain") );
    BOOST_CHECK( MimeType("Text/Plain") == MimeType("text", "plain") );
    BOOST_CHECK( MimeType("text/plain;charset=utf-8") == MimeType("text", "plain", {"charset", "utf-8"}) );
    BOOST_CHECK( MimeType("text/plain ;charset=utf-8") == MimeType("text", "plain", {"charset", "utf-8"}) );
    BOOST_CHECK( MimeType("text/plain; charset=utf-8") == MimeType("text", "plain", {"charset", "utf-8"}) );
    BOOST_CHECK( MimeType("text/plain ; charset=utf-8") == MimeType("text", "plain", {"charset", "utf-8"}) );
    BOOST_CHECK( MimeType("text/plain;charset=\"utf-8\"") == MimeType("text", "plain", {"charset", "utf-8"}) );
    BOOST_CHECK( MimeType("text/plain;CHARSET=UTF-8") == MimeType("text", "plain", {"charset", "utf-8"}) );
}

BOOST_AUTO_TEST_CASE( test_getters )
{
    MimeType mime("text", "plain", {"charset", "utf-8"});
    BOOST_CHECK( mime.type() == "text" );
    BOOST_CHECK( mime.subtype() == "plain" );
    BOOST_CHECK( mime.parameter().first == "charset" );
    BOOST_CHECK( mime.parameter().second == "utf-8" );
}

BOOST_AUTO_TEST_CASE( test_stream )
{
    boost::test_tools::output_test_stream output;
    output << MimeType("text/plain");
    BOOST_CHECK( output.is_equal( "text/plain" ) );
    output << MimeType("text/plain;charset=utf-8");
    BOOST_CHECK( output.is_equal( "text/plain;charset=utf-8" ) );
}

BOOST_AUTO_TEST_CASE( test_set_parameter )
{
    MimeType mime("text", "plain");

    mime.set_parameter({"charset", "utf-8"});
    BOOST_CHECK( mime.parameter().first == "charset" );
    BOOST_CHECK( mime.parameter().second == "utf-8" );

    mime.set_parameter({"", "utf-8"});
    BOOST_CHECK( mime.parameter().first == "" );
    BOOST_CHECK( mime.parameter().second == "" );

    mime.set_parameter({"charset", ""});
    BOOST_CHECK( mime.parameter().first == "" );
    BOOST_CHECK( mime.parameter().second == "" );
}

BOOST_AUTO_TEST_CASE( test_valid )
{
    BOOST_CHECK( MimeType("text", "plain", {"charset", "utf-8"}).valid() );
    BOOST_CHECK( MimeType("text", "plain").valid() );
    BOOST_CHECK( !MimeType("text", "").valid() );
    BOOST_CHECK( !MimeType("", "plain").valid() );
    BOOST_CHECK( !MimeType().valid() );
}

BOOST_AUTO_TEST_CASE( test_matches_type )
{
    BOOST_CHECK( MimeType("text", "plain", {"charset", "utf-8"}).matches_type("text", "plain") );
    BOOST_CHECK( MimeType("text", "plain").matches_type("text", "plain") );
    BOOST_CHECK( !MimeType("text", "css").matches_type("text", "plain") );
    BOOST_CHECK( !MimeType("application", "xml").matches_type("text", "plain") );
    BOOST_CHECK( MimeType("text", "plain", {"charset", "utf-8"}).matches_type(MimeType("text", "plain")) );
    BOOST_CHECK( MimeType("text", "plain", {"charset", "utf-8"}).matches_type(MimeType("text", "plain", {"charset", "ascii"})) );
}

