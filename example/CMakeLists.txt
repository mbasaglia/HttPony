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

set(Boot_FOUND ON)
find_package (Boost COMPONENTS filesystem QUIET)
if(Boot_FOUND)
    example(file_browser)
    target_link_libraries(file_browser ${Boost_LIBRARIES})
endif()

