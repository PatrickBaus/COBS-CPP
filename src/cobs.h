/**
# ##### BEGIN GPL LICENSE BLOCK #####
#
# Copyright (C) 2018  Patrick Baus
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# ##### END GPL LICENSE BLOCK #####

@author Patrick Baus
@version 1.1.0 09/17/2018
*/
#ifndef COBS_CPP_H
#define COBS_CPP_H

#include <stdint.h>	 // uint8_t, etc.
#include <stddef.h>	 // size_t

namespace cobs {
    /**
     * In place COBS encoder. The maximum block size is 254, because for larger
     * packets, inplace encoding becomes inefficient. This arises from the fact
     * that for 254 non-zero bytes an overhead byte has to be inserted. This means
     * copying around the remainder of the input making space for the overhead
     * byte. On the Arduino we have limited memory anway, so it is unlikely, that
     * we will ever set the message size that high.
     */

    /**
     * Encode an input array of byte with the COBS algorithm.
     *
     * @param buffer The i/o buffer. Must be at least of length $(size + 1) and the additional byte must be at the beginning of the data.
     * @param size of the data to be encoded. Must be at least
     * one and no greater than 254. The memory allocated for the input
     * buffer must be one additional byte.
     * @return The ecoded size of the data
     */
    static size_t encode(uint8_t* buffer, const size_t size) __attribute__((unused));
    static size_t encode(uint8_t* startOfData, const size_t size) {
      // Error out if the message larger than the maximum block size of
      // the COBS algorithm (254). The reason for this is, that this code
      // does not handle multiple blocks.
      if (size > 254 or size < 1)
  	    return 0;

      uint8_t* endOfBlock = &buffer[size-1];
      uint8_t* cursor;
      // Write a 0 before the data block. This is the COBS overhead byte.
      // This 0x00 byte will be overwritten later, but also serves as a
      // terminator for the parser.
      *(buffer -1) = 0x00;

      do {
          // Search for a 0x00 byte starting from the back of the stream.
          // This is the reason why we needed to prepend the 0x00 byte to the
          // data block. It serves as a terminator and saves us a bounds check while
          // iterating the loop.
          for (cursor = endOfBlock; *cursor != 0x00; cursor--) {};
          // 0x00 0xXX 0xXX 0xXX 0xYY 0xXX 0xXX 0xXX ....
          //   ^             ^     ^
          //   |             |     |
          // cursor   endOfBlock   was 0x00
          *cursor = endOfBlock - cursor + 1;		// Calculate the number of bytes until the next 0x00 byte
          // Go to the next block and repeat
          endOfBlock = cursor - 1;
      } while (cursor > buffer);

      // If the first data byte was 0x00, then the loop will abort after encoding
      // this block, so we need to manually check our overhead byte. If it still
      // says 0x00, then the loop aborted.
      if (*(buffer - 1) == 0x00) {
          *(buffer - 1) = 0x01;
      }
      return size + 1;
    }

    /**
     * Decodes a COBS (Consistent Overhead Byte Stuffing) buffer in place. The encoded data
     * will be *overwritten*!
     *
     * @param *buffer A pointer to the buffer containing the encoded data, without the delimiter/framing byte.
     * After decoding the buffer will contain the data without the overhead byte.
     * The overhead byte is the first byte, so the data stream will have an offset of +1.
     * @param size The size of the buffer
     *@return The block size, which is size - 1
     */
    static size_t decode(uint8_t* buffer, const size_t size) __attribute__((unused));
    static size_t decode(uint8_t* buffer, const size_t size) {
        // All COBS encoded blocks contain at least the header, even encoded empty
        // packets (size > 1). We have limited the maximum size of a message to 
        // size of of 254 bytes (unencoded), reasonable for microcontrollers.
        // Therefore a maximum of 255 encoded bytes has been set.
        if (size < 1 or size > 255)
            return 0;

        uint8_t tmp = 0;
        uint8_t* endOfBuffer = buffer + size;

        do {
            tmp = *buffer;   // Store the offset of the first encoded character
            *buffer = 0x00;
            buffer += tmp;		// If we are out of bounds, this will be the last iteration
        } while(buffer < endOfBuffer);

        return size - 1;
    }
}   // Namespace cobs
#endif    // End of header

