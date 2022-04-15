Consistent Overhead Byte Stuffing Library
===================

This repository contains a fast C library of the [COBS](http://www.stuartcheshire.org/papers/COBSforToN.pdf) (Consistent Overhead Byte Stuffing) algorithm.
It is designed for small embedded devices and the Internet of Things. When using any serial connection
to transfer data, packet boundaries need to be unambiguous within the data stream.

The COBS encoder works by reencoding a dataframe to remove a a certain byte (typically `0x00`), so that
this byte can then be used as a special marker to denote the end of a frame. The encoding is especially
efficient and requires little computational resources.

The implementation is both efficient and rquires little memory. It works in-place on the input data
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
  cobs::decode(buffer, sizeof(buffer)-1);

  Serial.print("Decoded data: ");
  printBuffer(buffer+1, sizeof(buffer)-1);
}

void loop() {

}
```

Installation
-----
Currently the library does not support the Arduino library manager, so it is highly recommended to copy the full library to a subfolder called
```
src/
```
within your Arduino project.
