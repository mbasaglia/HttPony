/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2016 Mattia Basaglia
 * \section License
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
#ifndef HTTPONY_BASE_ENCODING_HPP
#define HTTPONY_BASE_ENCODING_HPP

#include <melanolib/utils/gsl.hpp>

namespace httpony {

using byte = uint8_t;
using byte_view = melanolib::gsl::array_view<const byte>;

/**
 * \see https://tools.ietf.org/html/rfc4648#section-4
 */
class Base64
{
public:
    bool encode(const std::string& input, std::string& output)
    {
        output.clear();
        output.reserve(input.size() * 4 / 3 + 3);

        bool encoded = encode(
            byte_view(reinterpret_cast<const byte*>(input.data()), input.size()),
            std::back_inserter(output)
        );

        if ( !encoded )
            output.clear();

        return encoded;
    }

    template<class OutputIterator>
        bool encode(byte_view input, OutputIterator output)
    {
        uint32_t group = 0;
        int count = 0;
        for ( auto bin : input )
        {
            group = (group << 8) | bin;
            count++;
            if ( count == 3 )
            {
                if ( !encode_24bits(group, output) )
                    return false;
                group = 0;
                count = 0;
            }
        }
        if ( count )
        {
            if ( !encode_24bits(group, output, count * 8) )
                return false;
            for ( int i = count; i < 3; i++ )
            {
                *output = padding;
                ++output;
            }
        }
        return true;
    }

private:
    byte encode_6bits(byte data)
    {
        if ( data < 26 )
            return 'A' + data;
        if ( data < 52 )
            return 'a' + (data - 26);
        if ( data < 62 )
            return '0' + (data - 52);
        if ( data == 62 )
            return '+';
        if ( data == 63 )
            return '/';
        return invalid_byte;
    }

    template<class OutputIterator>
        bool encode_24bits(uint32_t data, OutputIterator output, int bits = 4*6)
    {
        if ( bits % 6 )
        {
            int diff = 6 - (bits % 6);
            data <<= diff;
            bits += diff;
        }

        for ( ;  bits > 0; bits -= 6 )
        {
            int shift = bits - 6;
            byte bout = encode_6bits((data >> shift) & 0x3F);
            if ( bout == invalid_byte )
                return false;
            *output = bout;
            ++output;
        }
        return true;
    }

    static const byte invalid_byte = 255;
    static const byte padding = '=';

};

} // namespace httpony
#endif // HTTPONY_BASE_ENCODING_HPP
