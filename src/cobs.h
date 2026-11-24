/**
# ##### BEGIN GPL LICENSE BLOCK #####
#
# Copyright (C) 2022  Patrick Baus
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
@version 1.2.0 04/15/2022
*/
#ifndef COBS_CPP_H
#define COBS_CPP_H

#include <stdint.h>  // uint8_t, etc.
#include <stddef.h>  // size_t

namespace cobs {
    /**
     * In place COBS encoder. The maximum block size is 254, because for larger
     * packets, inplace encoding becomes inefficient. This arises from the fact
     * that for 254 non-zero bytes an overhead byte has to be inserted. This means
     * copying around the remainder of the input making space for the overhead
     * byte. On the Arduino we have limited memory anyway, so it is unlikely, that
     * we will ever set the message size that high.
     */

    /**
     * Encode an input array of bytes with the COBS algorithm.
     *
     * @param buffer The i/o buffer. The data must start at index 1. Index 0 will be overwritten with the overhead byte
     * @param size The size of the buffer to be encoded, i.e. the number of data bytes plus one for the
     * overhead byte. Must be at least one and not greater than 255. The size of the data block must be at
     * most 254 bytes, because one byte is the overhead byte.
     * @return The encoded size of the data, which is equal to size, or 0 if size is out of range. The
     * buffer is not touched in case of an error.
     */
    inline size_t encode(uint8_t* buffer, const size_t size) {
      // Error out if the message is larger than the maximum block size of
      // the COBS algorithm (254). This code does not handle multiple blocks.
      // Also abort if there is not even room for the overhead byte.
      if (size > 255 || size < 1)
          return 0;

      // Points one past the last byte of the block that is currently encoded
      uint8_t* endOfBlock = buffer + size;
      // Write a 0 before the data block. This is the COBS overhead byte.
      // This 0x00 byte will be overwritten later, but serves as a terminator for the parser for now.
      buffer[0] = 0x00;

      uint8_t* cursor;
      do {
          // Search for a 0x00 byte starting from the back of the stream.
          // This is the reason why we needed to prepend the 0x00 byte to the
          // data block. It serves as a terminator and saves us a bounds check while
          // iterating the loop.
          for (cursor = endOfBlock - 1; *cursor != 0x00; cursor--) {};
          // 0x00 0xXX 0xXX 0xXX 0xYY 0xXX 0xXX 0xXX ....
          //   ^                   ^
          //   |                   |
          // cursor            endOfBlock (was 0x00)
          // Calculate the number of bytes until the next 0x00 byte. This is at least 1 and at most 255,
          // so the overhead byte at buffer[0] is always overwritten with a valid, non-zero value.
          *cursor = static_cast<uint8_t>(endOfBlock - cursor);
          // Go to the next block and repeat
          endOfBlock = cursor;
      } while (cursor > buffer);

      return size;
    }

    /**
     * Decodes a COBS (Consistent Overhead Byte Stuffing) buffer in place. The encoded data
     * will be *overwritten*!
     *
     * @param *buffer A pointer to the buffer containing the encoded data, without the delimiter/framing byte.
     * After decoding the buffer will contain the data without the overhead byte.
     * The overhead byte is the first byte, so the data stream will have an offset of +1.
     * @param size The size of the buffer. Must be at least one and not greater than 255.
     * @return The block size, which is size - 1, or 0 in case of an error. Note, that 0 is also returned
     * for a valid, but empty packet (size == 1, buffer[0] == 0x01). The content of the buffer is undefined
     * if the data was invalid. An error is detected if the size is out of range, if an offset byte is
     * 0x00 or if an offset points beyond the end of the buffer (truncated packet). The caller must split
     * the stream at the 0x00 delimiter, so 0x00 bytes between the offset bytes are not checked.
     */
    inline size_t decode(uint8_t* buffer, const size_t size) {
        // All COBS encoded blocks contain at least the header, even encoded empty
        // packets (size == 1). The maximum size of a message is limited to
        // 254 bytes (unencoded). Therefore the maximum input size is 255 encoded bytes.
        if (size < 1 || size > 255)
            return 0;

        const uint8_t* const endOfBuffer = buffer + size;

        do {
            const uint8_t offset = *buffer;  // Store the offset of the next encoded character
            // An offset of 0 is invalid and so is an offset, that points further than one past the last
            // byte, because the packet was truncated (or corrupted).
            if (offset == 0 || offset > static_cast<size_t>(endOfBuffer - buffer))
                return 0;
            *buffer = 0x00;
            buffer += offset;  // The last offset points exactly to the end of the buffer
        } while (buffer < endOfBuffer);

        return size - 1;
    }
}   // Namespace cobs
#endif  // COBS_CPP_H

