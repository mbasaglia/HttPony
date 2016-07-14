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
#include <fstream>

#include <boost/filesystem.hpp>
#include <magic.h>

#include "httpony.hpp"


class ServeFiles : public httpony::Server
{
public:
    explicit ServeFiles(httpony::ListenAddress listen)
        : Server(listen)
    {
        magic_cookie = magic_open(MAGIC_SYMLINK|MAGIC_MIME_TYPE);
        magic_load(magic_cookie, nullptr);
        set_timeout(melanolib::time::seconds(16));
    }

    ~ServeFiles()
    {
        if ( magic_cookie )
            magic_close(magic_cookie);
    }

    void respond(httpony::io::Connection& connection, httpony::Request&& request) override
    {
        httpony::Response response = build_response(connection, request);
        log_response(log_format, connection, request, response, std::cout);
        send_response(connection, request, response);
    }

protected:
    httpony::Response build_response(httpony::io::Connection& connection, httpony::Request& request) const
    {
        try
        {
            if ( request.method != "GET"  && request.method != "HEAD")
                request.suggested_status = httpony::StatusCode::MethodNotAllowed;

            if ( request.suggested_status.is_error() )
                return simple_response(request);

            auto file = root;
            for ( const auto & dir : request.url.path )
                file /= dir;

            if ( boost::filesystem::is_directory(file) )
            {
                httpony::Response response(request);
                response.body.start_data("text/html");
                response.body << "<!DOCTYPE html>\n<html>\n"
                    << "<head><title>" << file << "</title></head>\n"
                    << "<body><ul>";

                if ( !request.url.path.empty() )
                {
                    response.body << "<li><a href='..'>Parent</a></li>\n";
                }

                for ( const auto& item : boost::filesystem::directory_iterator(file) )
                {
                    std::string basename = item.path().filename().string();
                    response.body << "<li><a href='"
                        << (request.url.path / basename).url_encoded()
                        << "'>" << basename << "</a></li>\n";
                }

                response.body << "</ul></body>\n</html>";
                return response;
            }
            else if ( boost::filesystem::is_regular(file) )
            {
                httpony::Response response(request);
                response.body.start_data(mime_type(file.string()));
                std::ifstream input(file.string());
                while ( input )
                {
                    char chunk[1024];
                    input.read(chunk, 1024);
                    response.body.write(chunk, input.gcount());
                }
                return response;
            }

            request.suggested_status = httpony::StatusCode::NotFound;
            return simple_response(request);
        }
        catch ( const std::exception& )
        {
            // Create a server error response if an exception happened
            // while handling the request
            request.suggested_status = httpony::StatusCode::InternalServerError;
            return simple_response(request);
        }
    }

    /**
     * \brief Creates a simple text response containing just the status message
     */
    httpony::Response simple_response(const httpony::Request& request) const
    {
        httpony::Response response(request);
        response.body.start_data("text/plain");
        response.body << response.status.message << '\n';
        return response;
    }

    /**
     * \brief Sends the response back to the client
     */
    void send_response(httpony::io::Connection& connection,
                       httpony::Request& request,
                       httpony::Response& response) const
    {
        // We are not going to keep the connection open
        if ( response.protocol >= httpony::Protocol::http_1_1 )
        {
            response.headers["Connection"] = "close";
        }


        // Ensure the response isn't cached
        response.headers["Expires"] = "0";

        // This removes the response body when mandated by HTTP
        response.clean_body(request);

        if ( !connection.send_response(response) )
            connection.close();
    }

    httpony::MimeType mime_type(const std::string& filename) const
    {
        if ( magic_cookie )
        {
            const char* mime = magic_file(magic_cookie, filename.c_str());
            if ( mime )
                return httpony::MimeType(mime);
        }
        return httpony::MimeType("application", "octet-stream");
    }

private:
    boost::filesystem::path root = "/home";
    std::string log_format = "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";
    magic_t magic_cookie;
};


int main()
{
    ServeFiles server(8082);

    server.start();

    std::cout << "Server started on port "<< server.listen_address().port << ", hit enter to quit\n";
    std::cin.get();
    std::cout << "Stopped\n";

    return 0;
}
