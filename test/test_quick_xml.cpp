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


#define BOOST_TEST_MODULE HttPony_QuickXml
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include "httpony/quick_xml.hpp"

using namespace httpony::quick_xml;

BOOST_AUTO_TEST_CASE( test_text )
{
    Text text("hello");
    BOOST_CHECK( text.contents() == "hello" );
    boost::test_tools::output_test_stream output;
    output << text;
    BOOST_CHECK( output.is_equal( "hello" ) );
}

BOOST_AUTO_TEST_CASE( test_attribute )
{
    Attribute attribute("hello", "world");
    BOOST_CHECK( attribute.name() == "hello" );
    BOOST_CHECK( attribute.value() == "world" );
    boost::test_tools::output_test_stream output;
    output << attribute;
    BOOST_CHECK( output.is_equal( " hello='world'" ) );
}

BOOST_AUTO_TEST_CASE( test_attributes )
{
    Attributes attributes({{"hello", "world"}, {"foo", "bar"}});
    boost::test_tools::output_test_stream output;
    output << attributes;
    BOOST_CHECK( output.is_equal( " hello='world' foo='bar'" ) );
}

BOOST_AUTO_TEST_CASE( test_doctype )
{
    DocType doctype("html");
    BOOST_CHECK( doctype.string() == "html" );
    boost::test_tools::output_test_stream output;
    output << doctype;
    BOOST_CHECK( output.is_equal( "<!DOCTYPE html>" ) );
}

BOOST_AUTO_TEST_CASE( test_xml_declaration )
{
    XmlDeclaration xml_decl;
    BOOST_CHECK( xml_decl.version() == "1.0" );
    BOOST_CHECK( xml_decl.encoding() == "utf-8" );
    boost::test_tools::output_test_stream output;
    output << xml_decl;
    BOOST_CHECK( output.is_equal( "<?xml version='1.0' encoding='utf-8'?>" ) );
}

BOOST_AUTO_TEST_CASE( test_block_element_empty )
{
    BlockElement elem("foo");
    boost::test_tools::output_test_stream output;
    output << elem;
    BOOST_CHECK( output.is_equal( "<foo></foo>" ) );
}

BOOST_AUTO_TEST_CASE( test_element_empty )
{
    Element elem("foo");
    boost::test_tools::output_test_stream output;
    output << elem;
    BOOST_CHECK( output.is_equal( "<foo/>" ) );
}

BOOST_AUTO_TEST_CASE( test_element_attronly )
{
    Element elem("foo", Attribute{"hello", "world"});
    boost::test_tools::output_test_stream output;
    output << elem;
    BOOST_CHECK( output.is_equal( "<foo hello='world'/>" ) );
}

BOOST_AUTO_TEST_CASE( test_element_full )
{
    Element elem("foo", Attribute{"hello", "world"}, Text{"foo"}, Element{"bar"});
    boost::test_tools::output_test_stream output;
    output << elem;
    BOOST_CHECK( output.is_equal( "<foo hello='world'>foo<bar/></foo>" ) );
}

BOOST_AUTO_TEST_CASE( test_block_element_full )
{
    BlockElement elem("foo",  Text{"foo"}, Attribute{"hello", "world"}, Element{"bar"});
    boost::test_tools::output_test_stream output;
    output << elem;
    BOOST_CHECK( output.is_equal( "<foo hello='world'>foo<bar/></foo>" ) );
}

Node html_document()
{
    return Node{
        DocType{"html"},
        Element{"html",
            Element{"head",
                Element{"title", Text{"Hello"}},
            },
            Element{"body",
                Element{"p", Attribute{"id", "content"}, Text{"hello world"}},
            },
        },
    };
}

BOOST_AUTO_TEST_CASE( test_node )
{
    boost::test_tools::output_test_stream output;
    output << html_document();
    BOOST_CHECK( output.is_equal(
        "<!DOCTYPE html><html>"
        "<head><title>Hello</title></head>"
        "<body><p id='content'>hello world</p></body>"
        "</html>"
    ) );
}

BOOST_AUTO_TEST_CASE( test_indent_elements )
{
    boost::test_tools::output_test_stream output;
    html_document().print(output, Indentation{true});
    BOOST_CHECK( output.is_equal(
R"(<!DOCTYPE html>
<html>
    <head>
        <title>Hello</title>
    </head>
    <body>
        <p id='content'>hello world</p>
    </body>
</html>)"
    ) );
}

BOOST_AUTO_TEST_CASE( test_indent_attributes )
{
    boost::test_tools::output_test_stream output;
    html_document().print(output, Indentation{true, true});
    BOOST_CHECK( output.is_equal(
R"(<!DOCTYPE html>
<html>
    <head>
        <title>Hello</title>
    </head>
    <body>
        <p
            id='content'
        >hello world</p>
    </body>
</html>)"
    ) );
}

