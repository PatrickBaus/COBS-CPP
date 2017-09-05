/* Copyright 2011, Jacques Fortier. All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted, with or without modification.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../cobs4.h"

#define ASSERT_EQUAL_LUINT(value, expected) \
	do {\
		if( (value) != (expected) ) { \
			printf( "%30s: Failed, %s != %s. Expected %lu, got %lu\n", __func__, #value, #expected, (unsigned long)(expected), (unsigned long)(value) ); \
			return false; \
		} \
	} while(0)


#define ASSERT_EQUAL_MEM(value, expected, length) \
	do {\
		for( size_t i = 0; i < (length); i++ ) { \
			if( (value)[i] != (expected)[i] ) { \
				printf( "%30s: Failed, %s != %s. Expected %s[%lu] = 0x%02X, got 0x%02X\n", __func__, #value, #expected, #value, i, (expected)[i], (value)[i] ); \
				return false; \
			} \
		} \
	} while(0)

bool test_encode_empty(void)
{
	// We need at least one byte more than the payload, because we might add
	// one byte overhead.
	uint8_t input[] = {0xAA, 0xAA, 0xAA};
	uint8_t expected_output[sizeof(input)] = {0x01, 0xAA, 0xAA};

	size_t encoded_length = COBS::encode(input, 0);

	ASSERT_EQUAL_LUINT(encoded_length, 1);
	ASSERT_EQUAL_MEM(input, expected_output, sizeof(input));

	return true;
}

bool test_encode_single_zero(void)
{
	// We need at least one byte more than the payload, because we might add
	// one byte overhead.
	uint8_t input[] = {0xAA, 0x00, 0xAA};
	uint8_t expected_output[sizeof(input)] = {0x01, 0x01, 0xAA};

	size_t encoded_length = COBS::encode(input, 1);

	ASSERT_EQUAL_LUINT(encoded_length, 2);
	ASSERT_EQUAL_MEM(input, expected_output, sizeof(input));

	return true;
}

bool test_encode_single_BB(void)
{
	uint8_t input[3] = {0xAA, 0xBB, 0xAA};
	uint8_t expected_output[sizeof(input)] = {0x02, 0xBB, 0xAA};

	size_t encoded_length = COBS::encode(input, 1);

	ASSERT_EQUAL_LUINT(encoded_length, 2);
	ASSERT_EQUAL_MEM(input, expected_output, sizeof(input));

	return true;
}

bool test_encode_one_zeros(void)
{
	uint8_t input[] = {0xAA, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0xAA};
	uint8_t expected_output[sizeof(input)] = {0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x01, 0xAA};

	size_t encoded_length = COBS::encode(input, 8);

	ASSERT_EQUAL_LUINT(encoded_length, 9);
	ASSERT_EQUAL_MEM(input, expected_output, sizeof(input));

	return true;
}

bool test_encode_254_bytes_non_zero(void)
{
	// Make the array 1 byte larger to test for bytes written out of bounds
	uint8_t input[256];
	for(unsigned int i = 1; i < sizeof(input) -1; i++ )
	{
		input[i] = i;
	}
	input[0] = 0xAA;
	// Set the last byte to an arbitrary flag to test if the encoder touches it.
	input[255] = 0xAA;

	uint8_t expected_output[sizeof(input)];

	expected_output[0] = 0xFF;
	for(unsigned int i = 1; i < sizeof(input); i++ )
	{
		expected_output[i] = i;
	}
	// The last byte should remain untouched
	expected_output[255] = 0xAA;

	size_t encoded_length = COBS::encode(input, 254);

	ASSERT_EQUAL_LUINT(encoded_length, 255);
	ASSERT_EQUAL_MEM(input, expected_output, 255);

	return true;
}

bool test_encode_255_bytes(void)
{
	uint8_t input[256];
	for(unsigned int i = 1; i < sizeof(input) - 1; i++ )
	{
		input[i] = i;
	}

	uint8_t expected_output[sizeof(input)];

	size_t encoded_length = COBS::encode(input, 255);

	// We are expecting an error, therefore size 0
	ASSERT_EQUAL_LUINT(encoded_length, 0);

	return true;
}

bool test_encode_decode_single_zero(void) {
	printf("Encoding and decoding single 0:\n");
	uint8_t input[] = {0xAA, 0x00, 0xAA};
	// The number of bytes to be encoded
	uint8_t nBytes = 1;
	uint8_t expected_output[sizeof(input) - 1];
	memcpy(expected_output, &input[1], sizeof(expected_output));

	printf("\tOriginal:\n");
	printf("\t");
	for (int i = 0; i < nBytes; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	// Now encode the packet
	size_t encoded_length = COBS::encode(input, nBytes);
	ASSERT_EQUAL_LUINT(encoded_length, 2);

	printf("\tEncoded (%lu byte(s)):\n", encoded_length);
	printf("\t");
	for (int i = 0; i < encoded_length; i++) {
		printf(" %02X", input[i]);
	}
	printf("\n");

	size_t decoded_length = COBS::decode(input, encoded_length);
	printf("\tDecoded (%lu byte(s)):\n", decoded_length);
	printf("\t");
	for (int i = 0; i < decoded_length; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");
	ASSERT_EQUAL_LUINT(decoded_length, nBytes);
	ASSERT_EQUAL_MEM(&input[1], expected_output, decoded_length);

	return true;
}

bool test_encode_decode_single_one(void) {
	printf("Encoding and Decoding single 1:\n");
	uint8_t input[] = {0xAA, 0x01, 0xAA};
	// The number of bytes to be encoded
	uint8_t nBytes = 1;
	uint8_t expected_output[sizeof(input) - 1];
	memcpy(expected_output, &input[1], sizeof(expected_output));

	printf("\tOriginal:\n");
	printf("\t");
	for (int i = 0; i < nBytes; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	// Now encode the packet
	size_t encoded_length = COBS::encode(input, nBytes);
	ASSERT_EQUAL_LUINT(encoded_length, 2);

	printf("\tEncoded (%lu byte(s)):\n", encoded_length);
	printf("\t");
	for (int i = 0; i < encoded_length; i++) {
		printf(" %02X", input[i]);
	}
	printf("\n");

	size_t decoded_length = COBS::decode(input, encoded_length);

	printf("\tDecoded (%lu byte(s)):\n", decoded_length);
	printf("\t");
	for (int i = 0; i < decoded_length; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	ASSERT_EQUAL_LUINT(decoded_length, nBytes);
	ASSERT_EQUAL_MEM(&input[1], expected_output, decoded_length);

	return true;
}

bool test_encode_decode_one_zeros(void)
{
	printf("Encoding and decoding four 0x01-0x00 pairs:\n");
	uint8_t input[] = {0xAA, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0xAA};
	uint8_t nBytes = 8;
	uint8_t expected_output[sizeof(input) -1 ];
	memcpy(expected_output, &input[1], sizeof(expected_output));

	printf("\tOriginal:\n");
	printf("\t");
	for (int i = 0; i < nBytes; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	// Now encode the packet
	size_t encoded_length = COBS::encode(input, nBytes);
	ASSERT_EQUAL_LUINT(encoded_length, 9);

	printf("\tEncoded (%lu byte(s)):\n", encoded_length);
	printf("\t");
	for (int i = 0; i < encoded_length; i++) {
		printf(" %02X", input[i]);
	}
	printf("\n");

	size_t decoded_length = COBS::decode(input, encoded_length);
	printf("\tDecoded (%lu byte(s)):\n", decoded_length);
	printf("\t");
	for (int i = 0; i < decoded_length; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	ASSERT_EQUAL_LUINT(decoded_length, 8);
	ASSERT_EQUAL_MEM(&input[1], expected_output, decoded_length);
	return true;
}

bool test_encode_decode_254_bytes_non_zero(void) {
	printf("Encoding and decoding 254 non-zero bytes:\n");
	// Make the array 1 byte larger to test for bytes written out of bounds
	uint8_t input[256];
	for(unsigned int i = 1; i < sizeof(input) -1; i++ )
	{
		input[i] = i;
	}
	input[0] = 0xAA;
	// Set the last byte to an arbitrary flag to test if the encoder touches it.
	input[255] = 0xAA;
	uint8_t nBytes = 254;

	uint8_t expected_output[sizeof(input) -1 ];
	memcpy(expected_output, &input[1], sizeof(expected_output));

	printf("\tOriginal:\n");
	printf("\t");
	for (int i = 0; i < nBytes; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	// Now encode the packet
	size_t encoded_length = COBS::encode(input, nBytes);
	ASSERT_EQUAL_LUINT(encoded_length, 255);

	printf("\tEncoded (%lu byte(s)):\n", encoded_length);
	printf("\t");
	for (int i = 0; i < encoded_length; i++) {
		printf(" %02X", input[i]);
	}
	printf("\n");

	size_t decoded_length = COBS::decode(input, encoded_length);
	printf("\tDecoded (%lu byte(s)):\n", decoded_length);
	printf("\t");
	for (int i = 0; i < decoded_length; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	ASSERT_EQUAL_LUINT(decoded_length, 254);
	ASSERT_EQUAL_MEM(&input[1], expected_output, decoded_length);
	return true;
}

bool test_encode_decode_byte_code(void) {
	printf("Encoding Wikipedia example 3:\n");
	uint8_t input[] = {0x00, 0x11, 0x22, 0x00, 0x33, 0xAA};
	// The number of bytes to be encoded
	uint8_t nBytes = 4;
	uint8_t expected_output[sizeof(input) - 1];
	memcpy(expected_output, &input[1], sizeof(expected_output));

	printf("\tOriginal:\n");
	printf("\t");
	for (int i = 0; i < nBytes; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	// Now encode the packet
	size_t encoded_length = COBS::encode(input, nBytes);
	ASSERT_EQUAL_LUINT(encoded_length, 5);

	printf("\tEncoded (%lu byte(s)):\n", encoded_length);
	printf("\t");
	for (int i = 0; i < encoded_length; i++) {
		printf(" %02X", input[i]);
	}
	printf("\n");

	size_t decoded_length = COBS::decode(input, encoded_length);

	printf("\tDecoded (%lu byte(s)):\n", decoded_length);
	printf("\t");
	for (int i = 0; i < decoded_length; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	ASSERT_EQUAL_LUINT(decoded_length, nBytes);
	ASSERT_EQUAL_MEM(&input[1], expected_output, decoded_length);

	return true;
}

bool test_encode_decode_two_byte(void) {
	printf("Encoding and decoding two non-zero bytes:\n");
	uint8_t input[] = {0x00, 0x21, 0xAA, 0xAA};
	// The number of bytes to be encoded
	uint8_t nBytes = 2;
	uint8_t expected_output[sizeof(input) - 1];
	memcpy(expected_output, &input[1], sizeof(expected_output));

	printf("\tOriginal:\n");
	printf("\t");
	for (int i = 0; i < nBytes; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	// Now encode the packet
	size_t encoded_length = COBS::encode(input, nBytes);
	ASSERT_EQUAL_LUINT(encoded_length, 3);

	printf("\tEncoded (%lu byte(s)):\n", encoded_length);
	printf("\t");
	for (int i = 0; i < encoded_length; i++) {
		printf(" %02X", input[i]);
	}
	printf("\n");

	size_t decoded_length = COBS::decode(input, encoded_length);

	printf("\tDecoded (%lu byte(s)):\n", decoded_length);
	printf("\t");
	for (int i = 0; i < decoded_length; i++) {
		printf(" %02X", input[i + 1]);
	}
	printf("\n");

	ASSERT_EQUAL_LUINT(decoded_length, nBytes);
	ASSERT_EQUAL_MEM(&input[1], expected_output, decoded_length);

	return true;
}

int main(int argc, char*argv[])
{
	test_encode_empty();
	test_encode_single_zero();
	test_encode_single_BB();
	test_encode_one_zeros();
	test_encode_254_bytes_non_zero();
	test_encode_255_bytes();

	// Test the decoder
	test_encode_decode_single_zero();
	test_encode_decode_single_one();
	test_encode_decode_one_zeros();
	test_encode_decode_two_byte();
	test_encode_decode_254_bytes_non_zero();
	test_encode_decode_byte_code();
	return 0;
}
