/**
# ##### BEGIN GPL LICENSE BLOCK #####
#
# Copyright (C) 2015  Patrick Baus
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
@version 1.0.0 08/14/2015
*/
#include "cobs4.h"

size_t COBS::encode(uint8_t* buffer, const size_t size, const size_t offset) {
	// Error out if the message larger than the maximum block size of
	// the COBS algorithm (254). The reason for this is that, this code
	// does not handle multiple blocks.
	if (size > 254)
		return 0;

	uint8_t* startOfData = buffer + offset;
	uint8_t* endOfBlock = &startOfData[size-1];
	uint8_t* cursor;
	// Write a 0 before the data block. This is the COBS overhead byte.
	// This 0x00 byte will be overwritten later, but stops the parser.
	*(startOfData -1) = 0x00;

	do {
		// Loook for a 0x00 byte starting from the back of the stream
		// This loop is the reason why we need to put a 0x00 byte before the
		// data block. Checking for bounds on every iteration is costly.
		for (cursor = endOfBlock; *cursor != 0x00; cursor--) {};
		// 0x00 0xXX 0xXX 0xXX 0xYY 0xXX 0xXX 0xXX ....
		//   ^             ^     ^
		//   |             |     |
		// cursor   endOfBlock   was 0x00
		*cursor = endOfBlock - cursor + 1;		// The number of bytes til the next zero
		// The next block ends in front of the cursor (if there is another block)
		endOfBlock = cursor - 1;
	} while (cursor > startOfData);

	// If the first DATA byte was 0x00, then the loop will abort after encoding
	// this block, so we need to manually check our overhead byte. If it still
	// says 0x00, then the loop aborted.
	if (*(startOfData - 1) == 0x00) {
		*(startOfData - 1) = 0x01;
	}
	return size + 1;
}

size_t COBS::decode(uint8_t* start, const size_t size) {
	uint8_t* pEndOfBlock;
	uint8_t* pEndOfMessage;
	uint8_t* pFinger;

	// All COBS encoded blocks contain at least the header, even encoded empty
	// packets (size > 1). We have also limited the maximum size of a message
	// to 254 bytes (unencoded). Therefore a maximum of 255 encoded bytes has
	// been set.
	if (size < 1 || size > 255)
		return 0;

	uint8_t tmp = 0;
	uint8_t* end = start + size;
	
	do {
		tmp = *start;
		*start = 0x00;
		start += tmp;		// If we are out of bounds, this will be the last iteration
	} while(start < end);

	return size - 1;
}
/**
	Returns the maximum number of bytes, that should be reserved for overhead.

	@param bufferSize The size of the buffer we want to calculate the overhead
		for
*/
size_t COBS::overhead(size_t bufferSize) {
	// For unlimited packet size
	// return 1 + buffersize / 254;
	return 1;
}

