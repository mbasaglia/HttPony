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


#define BOOST_TEST_MODULE HttPony_Uri
#include <boost/test/unit_test.hpp>

#include "uri.hpp"

using namespace httpony;

BOOST_AUTO_TEST_CASE( test_urlencode )
{
    BOOST_CHECK( urlencode("fo0.-_~ ?&/#:+%") == "fo0.-_~%20%3F%26%2F%23%3A%2B%25" );
    BOOST_CHECK( urlencode("fo0.-_~ ?&/#:+%", true) == "fo0.-_~+%3F%26%2F%23%3A%2B%25" );
}

BOOST_AUTO_TEST_CASE( test_urldecode )
{
    BOOST_CHECK( urldecode("fo0.-_~ ?&/#:+") == "fo0.-_~ ?&/#:+" );
    BOOST_CHECK( urldecode("fo0.-_~%20%3f%26%2F%23%3A%2B%25") == "fo0.-_~ ?&/#:+%" );
    BOOST_CHECK( urldecode("fo0.-_~+%3F%26%2F%23%3A%2B%25", true) == "fo0.-_~ ?&/#:+%" );
}

BOOST_AUTO_TEST_CASE( test_parse_query_string )
{
    BOOST_CHECK( parse_query_string("foo=bar")          == DataMap({{"foo", "bar"}})                );
    BOOST_CHECK( parse_query_string("?foo=bar")         == DataMap({{"foo", "bar"}})                );
    BOOST_CHECK( parse_query_string("foo=foo&bar=bar")  == DataMap({{"foo", "foo"}, {"bar", "bar"}}));
    BOOST_CHECK( parse_query_string("hello")            == DataMap({{"hello", ""}})                 );
    BOOST_CHECK( parse_query_string("test=1%2b1=2")     == DataMap({{"test", "1+1=2"}})             );
    BOOST_CHECK( parse_query_string("2%2b2=4")          == DataMap({{"2+2", "4"}})                  );
    BOOST_CHECK( parse_query_string("q=hello+world")    == DataMap({{"q", "hello world"}})          );
}

BOOST_AUTO_TEST_CASE( test_build_query_string )
{
    BOOST_CHECK( build_query_string(DataMap({{"foo", "bar"}}))                  == "foo=bar"        );
    BOOST_CHECK( build_query_string(DataMap({{"foo", "bar"}}), true)            == "?foo=bar"       );
    BOOST_CHECK( build_query_string(DataMap({{"foo", "foo"}, {"bar", "bar"}}))  == "foo=foo&bar=bar");
    BOOST_CHECK( build_query_string(DataMap({{"hello", ""}}))                   == "hello"          );
    BOOST_CHECK( build_query_string(DataMap({{"test", "1+1=2"}}))               == "test=1%2B1%3D2" );
    BOOST_CHECK( build_query_string(DataMap({{"2+2", "4"}}))                    == "2%2B2=4"        );
    BOOST_CHECK( build_query_string(DataMap({{"q", "hello world"}}))            == "q=hello+world"  );
}

BOOST_AUTO_TEST_CASE( test_uri_cmp )
{
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) == Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) != Uri({"a", "b", {"c"}, {{"d", "e"}}, "X"}) );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) != Uri({"a", "b", {"c"}, {{"d", "X"}}, "f"}) );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) != Uri({"a", "b", {"X"}, {{"d", "e"}}, "f"}) );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) != Uri({"a", "X", {"c"}, {{"d", "e"}}, "f"}) );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) != Uri({"X", "b", {"c"}, {{"d", "e"}}, "f"}) );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_scheme )
{
    BOOST_CHECK( Uri("foo:") == Uri({"foo", "", {}, {}, ""}) );

    BOOST_CHECK( Uri("foo://bar") == Uri({"foo", "bar", {}, {}, ""}) );
    BOOST_CHECK( Uri("foo:/bar") == Uri({"foo", "", {"bar"}, {}, ""}) );
    BOOST_CHECK( Uri("foo:?bar") == Uri({"foo", "", {}, {{"bar", ""}}, ""}) );
    BOOST_CHECK( Uri("foo:#bar") == Uri({"foo", "", {}, {}, "bar"}) );

    BOOST_CHECK( Uri("foo://a/b?c=d#e") == Uri({"foo", "a", {"b"}, {{"c", "d"}}, "e"}) );

    BOOST_CHECK( Uri("//a/b?c=d#e") == Uri({"", "a", {"b"}, {{"c", "d"}}, "e"}) );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_authority )
{
    BOOST_CHECK( Uri("//foo") == Uri({"", "foo", {}, {}, ""}) );

    BOOST_CHECK( Uri("//foo/bar") == Uri({"", "foo", {"bar"}, {}, ""}) );
    BOOST_CHECK( Uri("//foo?bar") == Uri({"", "foo", {}, {{"bar", ""}}, ""}) );
    BOOST_CHECK( Uri("//foo#bar") == Uri({"", "foo", {}, {}, "bar"}) );

    BOOST_CHECK( Uri("a:/b?c=d#e") == Uri({"a", "", {"b"}, {{"c", "d"}}, "e"}) );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_path )
{
    BOOST_CHECK( Uri("/foo") == Uri({"", "", {"foo"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo/bar") == Uri({"", "", {"foo", "bar"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo/bar/") == Uri({"", "", {"foo", "bar"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo//bar") == Uri({"", "", {"foo", "bar"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo/./bar") == Uri({"", "", {"foo", "bar"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo/../bar") == Uri({"", "", {"bar"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo/../../../bar") == Uri({"", "", {"bar"}, {}, ""}) );
    BOOST_CHECK( Uri("foo") == Uri({"", "", {"foo"}, {}, ""}) );

    BOOST_CHECK( Uri("/foo?bar") == Uri({"", "", {"foo"}, {{"bar", ""}}, ""}) );
    BOOST_CHECK( Uri("/foo#bar") == Uri({"", "", {"foo"}, {}, "bar"}) );

    BOOST_CHECK( Uri("a://b?c=d#e") == Uri({"a", "b", {}, {{"c", "d"}}, "e"}) );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_query )
{
    BOOST_CHECK( Uri("?foo") == Uri({"", "", {}, {{"foo", ""}}, ""}) );
    BOOST_CHECK( Uri("?foo=bar") == Uri({"", "", {}, {{"foo", "bar"}}, ""}) );
    BOOST_CHECK( Uri("?foo&bar") == Uri({"", "", {}, {{"foo", ""}, {"bar", ""}}, ""}) );

    BOOST_CHECK( Uri("?foo#bar") == Uri({"", "", {}, {{"foo", ""}}, "bar"}) );

    BOOST_CHECK( Uri("a://b/c/d#e") == Uri({"a", "b", {"c", "d"}, {}, "e"}) );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_fragment )
{
    BOOST_CHECK( Uri("#foo") == Uri({"", "", {}, {}, "foo"}) );

    BOOST_CHECK( Uri("a://b/c?d=e") == Uri({"a", "b", {"c"}, {{"d", "e"}}, ""}) );
}
