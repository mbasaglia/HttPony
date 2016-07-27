/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright  Mattia Basaglia
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
#ifndef HTTPONY_HTTP_POST_POST_HPP
#define HTTPONY_HTTP_POST_POST_HPP

#include <melanolib/utils/singleton.hpp>
#include "httpony/http/request.hpp"

namespace httpony {
namespace post {

/// \todo multipart/mixed ?
class PostFormat
{
public:
    virtual ~PostFormat(){}

    /**
     * \brief Whether this format chan handle data provided in the given mime type
     */
    virtual bool matches(const MimeType& mime) const = 0;

    /**
     * \brief Parses the given data according to its mime type
     * \returns \b true on success
     */
    virtual bool parse(Request& request) const = 0;

    /**
     * \brief Formats the request post data
     * \returns \b true on success
     */
    virtual bool format(Request& request) const = 0;
};

class FormatRegistry : public melanolib::Singleton<FormatRegistry>
{
public:
    void register_format(std::unique_ptr<PostFormat>&& format)
    {
        formats.emplace_back(std::move(format));
    }

    template<class Format, class... Args>
        void register_format(Args&&... args)
    {
        formats.emplace_back(melanolib::New<Format>(std::forward<Args>(args)...));
    }

    void load_default();

    bool can_parse(const MimeType& mime)
    {
        if ( formats.empty() )
            load_default();

        for ( const auto& format : formats )
            if ( format->matches(mime) )
                return true;

        return false;
    }

    bool parse(Request& request)
    {
        if ( formats.empty() )
            load_default();

        for ( const auto& format : formats )
            if ( format->matches(request.body.content_type()) )
                return format->parse(request);

        return false;
    }

    bool format(Request& request)
    {
        if ( formats.empty() )
            load_default();

        for ( const auto& format : formats )
            if ( format->matches(request.body.content_type()) )
                return format->format(request);

        return false;
    }

private:
    FormatRegistry(){}
    friend ParentSingleton;

    std::vector<std::unique_ptr<PostFormat>> formats;
};

} // namespace post
} // namespace httpony
#endif // HTTPONY_HTTP_POST_POST_HPP
