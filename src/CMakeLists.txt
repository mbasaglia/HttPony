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

set(SOURCES
server.cpp
status.cpp
logging.cpp
io/network_stream.cpp
uri.cpp
request.cpp
http_read.cpp
protocol.cpp
multipart.cpp
mime_type.cpp
)

add_library(${LIBRARY_NAME} ${SOURCES})


find_package (Boost COMPONENTS system thread REQUIRED)
target_link_libraries(${LIBRARY_NAME} ${Boost_LIBRARIES})
include_directories(${Boost_INCLUDE_DIRS})


target_link_libraries(${LIBRARY_NAME} melano_stringutils melano_time)
