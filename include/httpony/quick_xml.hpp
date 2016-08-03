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
#ifndef HTTPONY_QUICK_XML_HPP
#define HTTPONY_QUICK_XML_HPP

#include <ostream>
#include <vector>

#include <melanolib/utils/c++-compat.hpp>

namespace httpony {
namespace quick_xml {

class Indentation
{
public:
    explicit Indentation(
        bool elements = false,
        bool attributes = false,
        int  depth = 4,
        char character = ' ',
        int  level = 0)
        : elements(elements),
        attributes(attributes),
        depth(depth),
        character(character),
        level(level)
    {}

    void indent(std::ostream& out, bool attribute = false) const
    {
        if ( attribute && !attributes )
        {
            out << ' ';
        }
        else if ( elements )
        {
            out << '\n' << std::string(level*depth, character);
        }
    }

    Indentation next() const
    {
        return Indentation{elements, attributes, depth, character, level+1};
    }

    bool indents_attributes() const
    {
        return attributes;
    }

private:
    bool elements;
    bool attributes;
    int  depth;
    char character;
    int  level;
};

class Node
{
public:

    template<class... Args>
    Node(Args&&... args)
    {
        _children.reserve(sizeof...(args));
        append(std::forward<Args>(args)...);
    }

    const std::vector<std::shared_ptr<Node>>& children() const
    {
        return _children;
    }

    virtual void print(std::ostream& out, const Indentation& indent) const
    {
        for ( const auto& child : _children )
            child->print(out, indent);
    }

    virtual bool is_attribute() const
    {
        return false;
    }

    virtual bool is_element() const
    {
        return false;
    }

protected:
    template<class Head, class... Tail>
        void append(Head&& head, Tail&&... tail)
        {
            _children.push_back(
                std::make_shared<std::remove_cv_t<std::remove_reference_t<Head>>>(
                    std::forward<Head>(head)
            ));
            append(std::forward<Tail>(tail)...);
        }

    template<class ForwardIterator>
        void append_range(ForwardIterator begin, ForwardIterator end)
        {
            for ( ; begin != end; ++ begin )
                append(*begin);
        }

    template<class Sequence>
        void append_range(Sequence sequence)
        {
            _children.reserve(sequence.size());
            append_range(sequence.begin(), sequence.end());
        }

private:
    void append()
    {}

    std::vector<std::shared_ptr<Node>> _children;
};

inline std::ostream& operator<<(std::ostream& stream, const Node& node)
{
    node.print(stream, Indentation{});
    return stream;
}

class BlockElement : public Node
{
public:
    template<class... Args>
        BlockElement(std::string tag_name, Args&&... args)
            : Node(std::forward<Args>(args)...),
              _tag_name(std::move(tag_name))
        {}

    std::string tag_name() const
    {
        return _tag_name;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        indent.indent(out);
        out << '<' << tag_name();
        bool has_attribute = false;
        for ( const auto& child : children() )
        {
            if ( child->is_attribute() )
            {
                child->print(out, indent.next());
                has_attribute = true;
            }
        }
        if ( has_attribute && indent.indents_attributes() )
            indent.indent(out);
        out << '>';

        bool has_element = false;
        for ( const auto& child : children() )
        {
            if ( !child->is_attribute() )
            {
                child->print(out, indent.next());
                if ( child->is_element() )
                    has_element = true;
            }
        }
        if ( has_element )
            indent.indent(out);

        out << "</" << tag_name() << '>';
    }

    bool is_element() const override
    {
        return true;
    }

private:
    std::string _tag_name;
};

class Element : public BlockElement
{
public:
    using BlockElement::BlockElement;


    void print(std::ostream& out, const Indentation& indent) const override
    {
        for ( const auto& child : children() )
            if ( !child->is_attribute() )
                return BlockElement::print(out, indent);

        indent.indent(out);
        out << '<' << tag_name();
        for ( const auto& child : children() )
            if ( child->is_attribute() )
                child->print(out, indent.next());
        out << "/>";
    }
};

class Attribute : public Node
{
public:
    Attribute(std::string name, std::string value)
        : _name(std::move(name)),
          _value(std::move(value))
    {}

    std::string name() const
    {
        return _name;
    }

    std::string value() const
    {
        return _value;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        indent.indent(out, true);
        /// \todo Escape special characters
        out << _name << "='" << _value << '\'';
    }

    bool is_attribute() const override
    {
        return true;
    }

private:
    std::string _name;
    std::string _value;
};

class Attributes : public Node
{
public:
    Attributes(const std::initializer_list<Attribute>& attributes)
    {
        append_range(attributes);
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        if ( children().empty() )
            return;
        auto iter = children().begin();
        (**iter).print(out, indent.next());
        for ( ++iter; iter != children().end(); ++iter )
                (**iter).print(out, indent.next());
    }

    bool is_attribute() const override
    {
        return true;
    }
};

class Text : public Node
{
public:
    Text(std::string contents)
        : _contents(std::move(contents))
    {}

    std::string contents() const
    {
        return _contents;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        /// \todo Escape special characters
        out << _contents;
    }

private:
    std::string _contents;
};

class XmlDeclaration : public Node
{
public:
    XmlDeclaration(std::string version="1.0", std::string encoding="utf-8")
        : _version(std::move(version)),
          _encoding(std::move(encoding))
    {}

    std::string version() const
    {
        return _version;
    }

    std::string encoding() const
    {
        return _encoding;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        out << "<?xml version='" << _version << '\'';
        if ( !_encoding.empty() )
            out << " encoding='" << _encoding << '\'';
        out << "?>";
    }

private:
    std::string _version;
    std::string _encoding;
};

class DocType : public Node
{
public:
    DocType(std::string string)
        : _string(std::move(string))
    {}

    std::string string() const
    {
        return _string;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        out << "<!DOCTYPE " << _string << ">";
    }

private:
    std::string _string;
};

class Comment : public Node
{
public:
    Comment(std::string contents)
        : _contents(std::move(contents))
    {}

    std::string contents() const
    {
        return _contents;
    }

    void print(std::ostream& out, const Indentation& indent) const override
    {
        /// \todo Remove/escape --
        out << "<!--" << _contents << "-->";
    }

private:
    std::string _contents;
};

} // namespace quick_xml
} // namespace httpony
#endif // HTTPONY_QUICK_XML_HPP
