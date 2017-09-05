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
#ifndef COBS4_H
#define COBS4_H

#include <stdint.h>	 // uint8_t, etc.
#include <stddef.h>	 // size_t

/* In place COBS encoder. The maximum block size is 254, because for larger
 * packets, inplace encoding becomes inefficient. This arises from the fact
 * that for 254 non-zero bytes an overhead byte has to be inserted. This means
 * copying around the remainder of the input making space for the overhead
 * byte. On the Arduino we have limited memory anway, so it is unlikely, that
 * we will ever set the message size that high.
 */

/* Encode an input array of byte with the COBS algorithm.
 * @param input Must be at least of length (size + 1)!
 * @param size of the data to be encoded. Must be at least
 	zero and no greater than 254. The memory allocated for the input
 	array must be one additional byte.
 */
class COBS {
	public:
		static size_t encode(uint8_t* input, const size_t size, const size_t offset);
		static size_t decode(uint8_t* start, const size_t size);
		static size_t overhead(size_t bufferSize);
};
#endif

