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
@version 1.1.0 09/17/2018
*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../src/cobs.h"

// The header must be includable from several translation units and more than once
#include "../../src/cobs.h"

static unsigned int failures = 0;
static unsigned int checks = 0;

#define CHECK(condition) \
  do { \
    checks++; \
    if (!(condition)) { \
      failures++; \
      printf("%s:%d: %s: check failed: %s\n", __FILE__, __LINE__, __func__, #condition); \
    } \
  } while (0)

#define CHECK_EQUAL(value, expected) \
  do { \
    const unsigned long value_ = static_cast<unsigned long>(value); \
    const unsigned long expected_ = static_cast<unsigned long>(expected); \
    checks++; \
    if (value_ != expected_) { \
      failures++; \
      printf("%s:%d: %s: %s != %s. Expected %lu, got %lu\n", __FILE__, __LINE__, __func__, #value, #expected, expected_, value_); \
    } \
  } while (0)

#define CHECK_EQUAL_MEM(value, expected, length) \
  do { \
    checks++; \
    for (size_t i_ = 0; i_ < static_cast<size_t>(length); i_++) { \
      if ((value)[i_] != (expected)[i_]) { \
        failures++; \
        printf("%s:%d: %s: %s != %s. Expected 0x%02X at index %lu, got 0x%02X\n", __FILE__, __LINE__, __func__, #value, #expected, \
               static_cast<unsigned int>((expected)[i_]), static_cast<unsigned long>(i_), static_cast<unsigned int>((value)[i_])); \
        break; \
      } \
    } \
  } while (0)

// The largest packet, that can be encoded
static const size_t MAX_PAYLOAD = 254;
// A marker written in front and behind the buffers to detect out of bounds writes
static const uint8_t CANARY = 0xA5;
static const size_t CANARY_SIZE = 8;
// The canary on both sides, the overhead byte and the payload. Two extra bytes are added for the tests using oversized buffers.
static const size_t ARENA_SIZE = CANARY_SIZE + 1 + MAX_PAYLOAD + 2 + CANARY_SIZE;

/**
 * A buffer surrounded by canaries. The library must never touch the canaries.
 */
struct Arena {
  uint8_t memory[ARENA_SIZE];
  uint8_t* buffer;

  Arena() : buffer(memory + CANARY_SIZE) { memset(memory, CANARY, sizeof(memory)); }

  bool isIntact(const size_t size) const {
    for (size_t i = 0; i < CANARY_SIZE; i++) {
      if (memory[i] != CANARY) return false;
    }
    for (size_t i = CANARY_SIZE + size; i < ARENA_SIZE; i++) {
      if (memory[i] != CANARY) return false;
    }
    return true;
  }
};

/**
 * A simple, independent (out-of-place, forward running) COBS reference encoder for packets of up to 254 bytes.
 *
 * @return The size of the encoded data, which is size + 1
 */
static size_t referenceEncode(const uint8_t* input, const size_t size, uint8_t* output) {
  size_t codeIndex = 0;
  uint8_t code = 1;
  for (size_t i = 0; i < size; i++) {
    if (input[i] == 0x00) {
      output[codeIndex] = code;
      codeIndex = i + 1;
      code = 1;
    } else {
      output[i + 1] = input[i];
      code++;
    }
  }
  output[codeIndex] = code;
  return size + 1;
}

// A small deterministic pseudo random number generator (xorshift32), so the tests are reproducible
static uint32_t prngState = 0x2545F491u;
static uint32_t prng(void) {
  prngState ^= prngState << 13;
  prngState ^= prngState >> 17;
  prngState ^= prngState << 5;
  return prngState;
}

/**
 * Encodes the payload, compares it to the expected encoding (if given) and to the reference encoder, then decodes it
 * again and makes sure nothing outside of the buffer was touched.
 */
static bool roundTrip(const uint8_t* payload, const size_t size, const uint8_t* expectedEncoding) {
  const unsigned int failuresBefore = failures;
  uint8_t reference[MAX_PAYLOAD + 1];
  const size_t referenceSize = referenceEncode(payload, size, reference);

  Arena arena;
  if (size > 0) {
    memcpy(arena.buffer + 1, payload, size);
  }

  const size_t encodedSize = cobs::encode(arena.buffer, size + 1);
  CHECK_EQUAL(encodedSize, size + 1);
  CHECK_EQUAL(encodedSize, referenceSize);
  CHECK_EQUAL_MEM(arena.buffer, reference, encodedSize);
  if (expectedEncoding != NULL) {
    CHECK_EQUAL_MEM(arena.buffer, expectedEncoding, encodedSize);
  }
  // The encoded data must not contain the delimiter
  CHECK(memchr(arena.buffer, 0x00, encodedSize) == NULL);
  CHECK(arena.isIntact(encodedSize));

  const size_t decodedSize = cobs::decode(arena.buffer, encodedSize);
  CHECK_EQUAL(decodedSize, size);
  CHECK_EQUAL(arena.buffer[0], 0x00);
  CHECK_EQUAL_MEM(arena.buffer + 1, payload, size);
  CHECK(arena.isIntact(encodedSize));

  return failures == failuresBefore;
}

/**
 * Makes sure, that the decoder rejects the input and does not touch anything outside of the buffer.
 */
static void expectDecodeError(const uint8_t* encoded, const size_t size) {
  Arena arena;
  memcpy(arena.buffer, encoded, size);
  CHECK_EQUAL(cobs::decode(arena.buffer, size), 0);
  CHECK(arena.isIntact(size));
}

static void test_encode_empty(void) {
  const uint8_t expected[] = {0x01};
  CHECK(roundTrip(NULL, 0, expected));
}

static void test_encode_single_zero(void) {
  const uint8_t payload[] = {0x00};
  const uint8_t expected[] = {0x01, 0x01};
  CHECK(roundTrip(payload, sizeof(payload), expected));
}

static void test_encode_single_non_zero(void) {
  const uint8_t payload[] = {0xBB};
  const uint8_t expected[] = {0x02, 0xBB};
  CHECK(roundTrip(payload, sizeof(payload), expected));
}

static void test_encode_two_zeros(void) {
  const uint8_t payload[] = {0x00, 0x00};
  const uint8_t expected[] = {0x01, 0x01, 0x01};
  CHECK(roundTrip(payload, sizeof(payload), expected));
}

static void test_encode_two_non_zero(void) {
  const uint8_t payload[] = {0x21, 0xAA};
  const uint8_t expected[] = {0x03, 0x21, 0xAA};
  CHECK(roundTrip(payload, sizeof(payload), expected));
}

static void test_encode_leading_zero(void) {
  const uint8_t payload[] = {0x00, 0x11, 0x22};
  const uint8_t expected[] = {0x01, 0x03, 0x11, 0x22};
  CHECK(roundTrip(payload, sizeof(payload), expected));
}

static void test_encode_trailing_zero(void) {
  const uint8_t payload[] = {0x11, 0x22, 0x00};
  const uint8_t expected[] = {0x03, 0x11, 0x22, 0x01};
  CHECK(roundTrip(payload, sizeof(payload), expected));
}

static void test_encode_one_zeros(void) {
  const uint8_t payload[] = {0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00};
  const uint8_t expected[] = {0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x01};
  CHECK(roundTrip(payload, sizeof(payload), expected));
}

// The examples from https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
static void test_encode_wikipedia_examples(void) {
  {
    const uint8_t payload[] = {0x00, 0x11, 0x00};
    const uint8_t expected[] = {0x01, 0x02, 0x11, 0x01};
    CHECK(roundTrip(payload, sizeof(payload), expected));
  }
  {
    const uint8_t payload[] = {0x11, 0x22, 0x00, 0x33};
    const uint8_t expected[] = {0x03, 0x11, 0x22, 0x02, 0x33};
    CHECK(roundTrip(payload, sizeof(payload), expected));
  }
  {
    const uint8_t payload[] = {0x11, 0x22, 0x33, 0x44};
    const uint8_t expected[] = {0x05, 0x11, 0x22, 0x33, 0x44};
    CHECK(roundTrip(payload, sizeof(payload), expected));
  }
  {
    const uint8_t payload[] = {0x11, 0x00, 0x00, 0x00};
    const uint8_t expected[] = {0x02, 0x11, 0x01, 0x01, 0x01};
    CHECK(roundTrip(payload, sizeof(payload), expected));
  }
}

static void test_encode_254_bytes_non_zero(void) {
  uint8_t payload[MAX_PAYLOAD];
  uint8_t expected[MAX_PAYLOAD + 1];
  expected[0] = 0xFF;
  for (size_t i = 0; i < sizeof(payload); i++) {
    payload[i] = static_cast<uint8_t>(i + 1);
    expected[i + 1] = payload[i];
  }
  CHECK(roundTrip(payload, sizeof(payload), expected));
}

static void test_encode_254_zeros(void) {
  uint8_t payload[MAX_PAYLOAD];
  uint8_t expected[MAX_PAYLOAD + 1];
  memset(payload, 0x00, sizeof(payload));
  memset(expected, 0x01, sizeof(expected));
  CHECK(roundTrip(payload, sizeof(payload), expected));
}

static void test_encode_254_bytes_trailing_zero(void) {
  uint8_t payload[MAX_PAYLOAD];
  uint8_t expected[MAX_PAYLOAD + 1];
  memset(payload, 0xCC, sizeof(payload));
  payload[MAX_PAYLOAD - 1] = 0x00;
  memset(expected, 0xCC, sizeof(expected));
  expected[0] = 0xFE;
  expected[MAX_PAYLOAD] = 0x01;
  CHECK(roundTrip(payload, sizeof(payload), expected));
}

static void test_encode_size_zero_fail(void) {
  Arena arena;
  // We are expecting an error, therefore size 0 and an untouched buffer
  CHECK_EQUAL(cobs::encode(arena.buffer, 0), 0);
  CHECK(arena.isIntact(0));
}

static void test_encode_255_bytes_fail(void) {
  Arena arena;
  // 255 bytes payload plus the overhead byte. This is one more than supported.
  CHECK_EQUAL(cobs::encode(arena.buffer, 256), 0);
  CHECK(arena.isIntact(0));
  CHECK_EQUAL(cobs::encode(arena.buffer, static_cast<size_t>(-1)), 0);
  CHECK(arena.isIntact(0));
}

// All packets with a length of up to two bytes and every possible content
static void test_round_trip_exhaustive_short(void) {
  uint8_t payload[2];
  bool success = true;
  for (unsigned int first = 0; first < 256; first++) {
    payload[0] = static_cast<uint8_t>(first);
    success = roundTrip(payload, 1, NULL) && success;
    for (unsigned int second = 0; second < 256; second++) {
      payload[1] = static_cast<uint8_t>(second);
      success = roundTrip(payload, 2, NULL) && success;
    }
  }
  CHECK(success);
}

// A single zero at every possible position for every possible length
static void test_round_trip_single_zero_all_positions(void) {
  uint8_t payload[MAX_PAYLOAD];
  bool success = true;
  for (size_t size = 1; size <= MAX_PAYLOAD; size++) {
    for (size_t position = 0; position < size; position++) {
      memset(payload, 0xEE, size);
      payload[position] = 0x00;
      success = roundTrip(payload, size, NULL) && success;
    }
  }
  CHECK(success);
}

// Random data of every possible length with a varying number of zeros
static void test_round_trip_random(void) {
  uint8_t payload[MAX_PAYLOAD];
  bool success = true;
  for (size_t size = 0; size <= MAX_PAYLOAD; size++) {
    for (unsigned int iteration = 0; iteration < 64; iteration++) {
      // The higher the modulus the less likely is a zero. The first iteration creates zeros only.
      const uint32_t modulus = 1u << (iteration % 9);
      for (size_t i = 0; i < size; i++) {
        payload[i] = static_cast<uint8_t>(prng() % modulus);
      }
      success = roundTrip(payload, size, NULL) && success;
    }
  }
  CHECK(success);
}

static void test_decode_size_zero_fail(void) {
  Arena arena;
  CHECK_EQUAL(cobs::decode(arena.buffer, 0), 0);
  CHECK(arena.isIntact(0));
}

static void test_decode_256_bytes_fail(void) {
  Arena arena;
  // A valid encoding, but too long
  memset(arena.buffer, 0x01, 256);
  CHECK_EQUAL(cobs::decode(arena.buffer, 256), 0);
  // The buffer must not be modified
  for (size_t i = 0; i < 256; i++) {
    CHECK_EQUAL(arena.buffer[i], 0x01);
  }
  CHECK(arena.isIntact(256));
  CHECK_EQUAL(cobs::decode(arena.buffer, static_cast<size_t>(-1)), 0);
}

static void test_decode_empty(void) {
  // An empty packet is valid and has a size of 0
  Arena arena;
  arena.buffer[0] = 0x01;
  CHECK_EQUAL(cobs::decode(arena.buffer, 1), 0);
  CHECK_EQUAL(arena.buffer[0], 0x00);
  CHECK(arena.isIntact(1));
}

static void test_decode_zero_offset_fail(void) {
  // A zero as the first offset. The decoder used to hang here.
  const uint8_t leading[] = {0x00};
  expectDecodeError(leading, sizeof(leading));
  const uint8_t leadingLong[] = {0x00, 0x11, 0x22};
  expectDecodeError(leadingLong, sizeof(leadingLong));
  // A zero as one of the following offsets
  const uint8_t middle[] = {0x02, 0x11, 0x00, 0x22};
  expectDecodeError(middle, sizeof(middle));
  const uint8_t last[] = {0x02, 0x11, 0x00};
  expectDecodeError(last, sizeof(last));
}

static void test_decode_truncated_fail(void) {
  // The first offset points beyond the end of the buffer
  const uint8_t single[] = {0x02};
  expectDecodeError(single, sizeof(single));
  const uint8_t first[] = {0x05, 0x11, 0x22};
  expectDecodeError(first, sizeof(first));
  const uint8_t maximum[] = {0xFF, 0x11, 0x22};
  expectDecodeError(maximum, sizeof(maximum));
  // One of the following offsets points beyond the end of the buffer
  const uint8_t last[] = {0x03, 0x11, 0x22, 0x02};
  expectDecodeError(last, sizeof(last));
  const uint8_t middle[] = {0x02, 0x11, 0x04, 0x22, 0x33};
  expectDecodeError(middle, sizeof(middle));

  // Every valid packet, that is cut short and does not end on an offset must be rejected
  uint8_t payload[MAX_PAYLOAD];
  memset(payload, 0xEE, sizeof(payload));
  Arena arena;
  memcpy(arena.buffer + 1, payload, sizeof(payload));
  CHECK_EQUAL(cobs::encode(arena.buffer, MAX_PAYLOAD + 1), MAX_PAYLOAD + 1);
  for (size_t size = 1; size <= MAX_PAYLOAD; size++) {
    expectDecodeError(arena.buffer, size);
  }
}

static void test_decode_maximum_offset(void) {
  // The offset 0xFF is only valid for a packet of 254 non-zero bytes
  Arena arena;
  memset(arena.buffer, 0xEE, 255);
  arena.buffer[0] = 0xFF;
  CHECK_EQUAL(cobs::decode(arena.buffer, 255), 254);
  CHECK_EQUAL(arena.buffer[0], 0x00);
  for (size_t i = 1; i < 255; i++) {
    CHECK_EQUAL(arena.buffer[i], 0xEE);
  }
  CHECK(arena.isIntact(255));
}

// Feed random data to the decoder. It must never touch anything outside of the buffer and if it accepts the
// data, encoding it again must reproduce the input.
static void test_decode_random(void) {
  unsigned int accepted = 0;
  for (unsigned int iteration = 0; iteration < 200000; iteration++) {
    const size_t size = 1 + prng() % 255;
    // Use small values to increase the probability of valid packets
    const uint32_t modulus = 2u << (iteration % 8);
    Arena arena;
    uint8_t input[MAX_PAYLOAD + 1];
    bool containsZero = false;
    for (size_t i = 0; i < size; i++) {
      input[i] = static_cast<uint8_t>(prng() % modulus);
      containsZero = containsZero || input[i] == 0x00;
    }
    memcpy(arena.buffer, input, size);

    const size_t decodedSize = cobs::decode(arena.buffer, size);
    CHECK(arena.isIntact(size));
    CHECK(decodedSize == 0 || decodedSize == size - 1);
    if (decodedSize == size - 1 && arena.buffer[0] == 0x00 && !containsZero) {
      accepted++;
      CHECK_EQUAL(cobs::encode(arena.buffer, size), size);
      CHECK_EQUAL_MEM(arena.buffer, input, size);
      CHECK(arena.isIntact(size));
    }
  }
  // Make sure the test does test something
  CHECK(accepted > 1000);
}

// The example from the README
static void test_readme_example(void) {
  uint8_t buffer[] = {0xAA, 0x11, 0x22, 0x00, 0x33, 0xAA};
  const size_t bufferSize = sizeof(buffer) - 1;
  const uint8_t expectedEncoding[] = {0x03, 0x11, 0x22, 0x02, 0x33, 0xAA};
  const uint8_t expectedDecoding[] = {0x00, 0x11, 0x22, 0x00, 0x33, 0xAA};

  const size_t encodedLength = cobs::encode(buffer, bufferSize);
  CHECK_EQUAL(encodedLength, 5);
  CHECK_EQUAL_MEM(buffer, expectedEncoding, sizeof(buffer));

  const size_t decodedLength = cobs::decode(buffer, encodedLength);
  CHECK_EQUAL(decodedLength, 4);
  CHECK_EQUAL_MEM(buffer, expectedDecoding, sizeof(buffer));
}

struct TestCase {
  const char* name;
  void (*function)(void);
};

#define TEST(function) {#function, function}

static const TestCase tests[] = {
  TEST(test_encode_empty),
  TEST(test_encode_single_zero),
  TEST(test_encode_single_non_zero),
  TEST(test_encode_two_zeros),
  TEST(test_encode_two_non_zero),
  TEST(test_encode_leading_zero),
  TEST(test_encode_trailing_zero),
  TEST(test_encode_one_zeros),
  TEST(test_encode_wikipedia_examples),
  TEST(test_encode_254_bytes_non_zero),
  TEST(test_encode_254_zeros),
  TEST(test_encode_254_bytes_trailing_zero),
  TEST(test_encode_size_zero_fail),
  TEST(test_encode_255_bytes_fail),
  TEST(test_round_trip_exhaustive_short),
  TEST(test_round_trip_single_zero_all_positions),
  TEST(test_round_trip_random),
  TEST(test_decode_size_zero_fail),
  TEST(test_decode_256_bytes_fail),
  TEST(test_decode_empty),
  TEST(test_decode_zero_offset_fail),
  TEST(test_decode_truncated_fail),
  TEST(test_decode_maximum_offset),
  TEST(test_decode_random),
  TEST(test_readme_example),
};

int main(void) {
  unsigned int failedTests = 0;
  const size_t numberOfTests = sizeof(tests) / sizeof(tests[0]);

  for (size_t i = 0; i < numberOfTests; i++) {
    const unsigned int failuresBefore = failures;
    tests[i].function();
    const bool passed = failures == failuresBefore;
    printf("[%s] %s\n", passed ? " OK " : "FAIL", tests[i].name);
    if (!passed) {
      failedTests++;
    }
  }

  printf("\n%lu tests, %u failed (%u checks, %u failed)\n", static_cast<unsigned long>(numberOfTests), failedTests, checks, failures);
  return failures == 0 ? 0 : 1;
}
