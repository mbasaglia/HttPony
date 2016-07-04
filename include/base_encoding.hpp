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
 * \brief Base 64 encoding
 * \see https://tools.ietf.org/html/rfc4648#section-4
 */
class Base64
{
public:
    /**
     * \brief Encodes \p input
     * \returns The Base64-encoded string
     */
    std::string encode(const std::string& input)
    {
        std::string output;
        encode(input, output);
        return output;
    }

    /**
     * \brief Encodes \p input into \p output
     * \param input  String to encode
     * \param output String holding the result
     * \return \b true on succees (cannot fail)
     */
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

    /**
     * \brief Encodes \p input into \p output
     * \param input     View to a byte string to encode
     * \param output    Output iterator accepting the converted characters
     * \return \b true on succees (cannot fail)
     */
    template<class OutputIterator>
        bool encode(byte_view input, OutputIterator output)
    {
        uint32_t group = 0;
        int count = 0;

        // Convert groups of 3 octects into 4 sextets
        for ( auto bin : input )
        {
            group = (group << 8) | bin;
            count++;
            if ( count == 3 )
            {
                encode_bits(group, output);
                group = 0;
                count = 0;
            }
        }

        // Handle stray octects
        if ( count )
        {
            // Encode available bits
            encode_bits(group, output, count * 8);

            // Append padding characters
            for ( int i = count; i < 3; i++ )
            {
                *output = padding;
                ++output;
            }
        }

        return true;
    }

private:
    /**
     * \brief Converts a 6-bit integer into a Base64-encoded 8-bit character
     * \param data 6-bit integer [0, 64)
     * \pre data & 0xC0 == 0
     */
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
        // Since is being called masking the lowest 6 bits, it can never be
        // larger than 63
        return '/';
    }

    /**
     * \brief Encodes an integer into Base64
     * \param data      Integer containing the bits to be converted
     * \param output    Output iterator accepting the converted characters
     * \param bits      Number of bits in \p data to be considered
     * \pre \p bits <= 32 (Usually should be 24 aka 3 octets)
     */
    template<class OutputIterator>
        void encode_bits(uint32_t data, OutputIterator output, int bits = 4*6)
    {
        // Align the most significan bit to a sextet
        if ( bits % 6 )
        {
            int diff = 6 - (bits % 6);
            data <<= diff;
            bits += diff;
        }

        // Eats groups of 6 bits from the most significant to the least
        for ( ;  bits > 0; bits -= 6 )
        {
            int shift = bits - 6;
            byte bout = encode_6bits((data >> shift) & 0x3F);
            *output = bout;
            ++output;
        }
    }

    static const byte padding = '=';
};

} // namespace httpony
#endif // HTTPONY_BASE_ENCODING_HPP
