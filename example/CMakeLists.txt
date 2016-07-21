#
# Copyright 2016 Mattia Basaglia
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

set(LIBRARY_NAME httpony) # repeated otherwise kdevelop messes up


function(example name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} ${LIBRARY_NAME})
endfunction()


example(hello_server)
example(hello_client)

set(Boot_FOUND ON)
set(Magic_FOUND ON)
find_package(Boost COMPONENTS filesystem QUIET)
find_package(Magic QUIET)
if(Boot_FOUND AND Magic_FOUND)
    example(file_browser)
    target_link_libraries(file_browser ${Boost_LIBRARIES} ${Magic_LIBRARY})
else()
    message(STATUS "file_browser example disabled (You need libmagic and Boost::filesystem for this)")
endif()

set(OPENSSL_FOUND ON)
find_package(OpenSSL QUIET)
if(OPENSSL_FOUND)
    example(ssl_hello_server)
    target_link_libraries(ssl_hello_server ${OPENSSL_LIBRARIES})
else()
    message(STATUS "ssl_hello_server example disabled (You need OpenSSL for this)")
endif()

