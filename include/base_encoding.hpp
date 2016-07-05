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
#include <melanolib/string/ascii.hpp>

#include <stdexcept>

namespace httpony {

using byte = uint8_t;
using byte_view = melanolib::gsl::array_view<const byte>;

class EncodingError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/**
 * \brief Base 64 encoding
 * \see https://tools.ietf.org/html/rfc4648#section-4
 */
class Base64
{
public:
    /**
     * \param c62 The 62nd character of the base 64 alphabet
     * \param c63 The 63rd character of the base 64 alphabet
     * \param pad Whether to ensure data is properly padded
     */
    Base64(byte c62, byte c63, bool pad = true)
        : c62(c62), c63(c63), pad(pad)
    {}

    explicit Base64(bool pad) : Base64('+', '/', pad)
    {}

    Base64() : Base64(true)
    {}

    /**
     * \brief Encodes \p input
     * \returns The Base64-encoded string
     */
    std::string encode(const std::string& input) const
    {
        std::string output;
        encode(input, output);
        return output;
    }

    /**
     * \brief Encodes \p input into \p output
     * \param input  String to encode
     * \param output String to store the result into
     * \return \b true on succees (cannot fail)
     */
    bool encode(const std::string& input, std::string& output) const
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
        bool encode(byte_view input, OutputIterator output) const
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
            if ( pad )
            {
                for ( int i = count; i < 3; i++ )
                {
                    *output = padding;
                    ++output;
                }
            }
        }

        return true;
    }

    /**
     * \brief Decodes \p input
     * \param input A base64-encoded string
     * \returns The unencoded string
     * \throws EncodingError if the encoding failed
     */
    std::string decode(const std::string& input) const
    {
        std::string output;
        if ( !decode(input, output) )
            throw EncodingError("Invalid base 64 string");
        return output;
    }

    /**
     * \brief Decodes \p input into \p output
     * \param input  Base64-encoded string
     * \param output String to store the result into
     *
     * In case of failure \p output will result into an empty string
     * \return \b true on succees
     */
    bool decode(const std::string& input, std::string& output) const
    {
        output.clear();
        output.reserve(input.size() * 3 / 4);

        bool encoded = decode(
            byte_view(reinterpret_cast<const byte*>(input.data()), input.size()),
            std::back_inserter(output)
        );

        if ( !encoded )
            output.clear();

        return encoded;
    }

    /**
     * \brief Decodes \p input into \p output
     * \param input     View to a base64-encoded byte string
     * \param output    Output iterator accepting the converted characters
     * \return \b true on succees
     */
    template<class OutputIterator>
        bool decode(byte_view input, OutputIterator output) const
    {
        if ( pad && input.size() % 4 )
            return false;

        uint32_t group = 0;
        int count = 0;
        std::size_t i = 0;

        // Converts groups of 6 base64 sextets into 4 octets
        for ( ; i < input.size(); i++ )
        {
            if ( input[i] == padding )
            {
                if ( i < input.size() - 2 )
                    return false;
                else
                    break;
            }

            uint8_t bout;
            if ( !decode_6bits(input[i], bout) )
                return false;

            group = (group << 6) | bout;
            count++;

            if ( count == 4 )
            {
                decode_bits(group, output);
                group = 0;
                count = 0;
            }
        }

        // Handle padded string
        if ( count )
        {
            decode_bits(group, output, count * 6);
        }

        return true;
    }

private:
    /**
     * \brief Converts a 6-bit integer into a Base64-encoded 8-bit character
     * \param data 6-bit integer [0, 64)
     * \pre data & 0xC0 == 0
     */
    byte encode_6bits(byte data) const
    {
        if ( data < 26 )
            return 'A' + data;
        if ( data < 52 )
            return 'a' + (data - 26);
        if ( data < 62 )
            return '0' + (data - 52);
        if ( data == 62 )
            return c62;
        // Since is being called masking the lowest 6 bits, it can never be
        // larger than 63
        return c63;
    }

    /**
     * \brief Encodes an integer into Base64
     * \param data      Integer containing the bits to be converted
     * \param output    Output iterator accepting the converted characters
     * \param bits      Number of bits in \p data to be considered
     * \pre \p bits <= 32 (Usually should be 24 aka 3 octets)
     */
    template<class OutputIterator>
        void encode_bits(uint32_t data, OutputIterator output, int bits = 24) const
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

    /**
     * \brief Converts a base64 character into a 6-bit integer
     */
    bool decode_6bits(byte data, byte& output) const
    {
        if ( data == c63 )
            output = 63;
        else if ( data == c62 )
            output = 62;
        else if ( melanolib::string::ascii::is_digit(data) )
            output = data - '0' + 52;
        else if ( melanolib::string::ascii::is_lower(data) )
            output = data - 'a' + 26;
        else if ( melanolib::string::ascii::is_upper(data) )
            output = data - 'A';
        else
            return false;
        return true;
    }

    /**
     * \brief Decodes an integer from base 64
     * \param data      Integer containing the bits to be converted
     * \param output    Output iterator accepting the converted characters
     * \param bits      Number of bits in \p data to be considered
     * \pre \p bits <= 32 (Usually should be 24 aka 4 sextets)
     */
    template<class OutputIterator>
        void decode_bits(uint32_t data, OutputIterator output, int bits = 24) const
    {
        // Align the least significan bit to an octet
        data >>= bits % 8;
        bits -= bits % 8;

        // Eats groups of 8 bits from the most significant to the least
        for ( ;  bits > 0; bits -= 8 )
        {
            int shift = bits - 8;
            *output = (data >> shift) & 0xFF;
            ++output;
        }
    }


    static const byte padding = '=';

    byte c62;
    byte c63;
    bool pad;
};

} // namespace httpony
#endif // HTTPONY_BASE_ENCODING_HPP
