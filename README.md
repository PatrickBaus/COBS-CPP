Consistent Overhead Byte Stuffing Library
===================
[![CI](https://github.com/PatrickBaus/COBS-CPP/actions/workflows/ci.yml/badge.svg)](https://github.com/PatrickBaus/COBS-CPP/actions/workflows/ci.yml)

This repository contains a fast C library of the [COBS](http://www.stuartcheshire.org/papers/COBSforToN.pdf) (Consistent Overhead Byte Stuffing) algorithm.
It is designed for small embedded devices and the Internet of Things. When using any serial connection
to transfer data, packet boundaries need to be unambiguous within the data stream.

The COBS encoder works by reencoding a dataframe to remove a certain byte (typically `0x00`), so that
this byte can then be used as a special marker to denote the end of a frame. The encoding is especially
efficient and requires little computational resources.

The implementation is both efficient and requires little memory. It works in-place on the input data
to save memory, but it is limited to a single COBS frame of 254 bytes, which is typically more than
enough for small devices.

Usage
-----
The example data is taken from the Wikipedia article on [COBS](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing).
The code is written for the Arduino platform, but works on any platform.

```cpp
#include "cobs.h"

void printBuffer(uint8_t* buffer, size_t size) {
  for (uint8_t i = 0; i < size; i++) {
    if (buffer[i] < 0x10) {
      Serial.print(0);
    }
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
}

void setup() {
  Serial.begin(115200);
  // This is example 3 from Wikipedia
  uint8_t buffer[] = {0xFF, 0x11, 0x22, 0x00, 0x33};  //Note, that there is an additional byte in front of the example data
  cobs::encode(buffer, sizeof(buffer));
  Serial.print("Encoded data: ");
  printBuffer(buffer, sizeof(buffer));
  Serial.println("00"); // Add the final delimiter to complete the message

  // Now decode the data again
  cobs::decode(buffer, sizeof(buffer));

  Serial.print("Decoded data: ");
  printBuffer(buffer+1, sizeof(buffer)-1);
}

void loop() {

}
```

Both functions return `0` in case of an error. `cobs::encode()` fails, if the size of the buffer is not in the range of 1 to 255 bytes.
`cobs::decode()` additionally fails, if the data is not a valid COBS frame, because an offset byte is `0x00` or points beyond the end of
the buffer (truncated frame). The content of the buffer is undefined after a failed `cobs::decode()`. Note, that decoding a valid, but
empty frame (the single byte `0x01`) also returns a length of `0`.

Installation
-----
Currently the library does not support the Arduino library manager, so either copy the repository to the `libraries` folder of your
Arduino sketchbook and use `#include <cobs.h>` or copy [src/cobs.h](src/cobs.h) to your Arduino project and use `#include "cobs.h"`.

Tests
-----
The unit tests are found in [extras/tests](extras/tests) and are run for every commit by the CI. They require `make` and a C++ compiler.
[gcovr](https://gcovr.com/) is required for the coverage report. The build fails, if the line or branch coverage of the library drops
below 100 %.

```bash
make -C extras/tests test
```

```bash
make -C extras/tests coverage
```
