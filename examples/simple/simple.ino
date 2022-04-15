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
  // This is example 3 from Wikipedia (https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing)
  // Create a buffer. Note, that there is an additional byte in front and behind of the example data.
  // This is used to show, that we are not writing into that area
  uint8_t buffer[] = {0xAA, 0x11, 0x22, 0x00, 0x33, 0xAA};
  size_t bufferSize = sizeof(buffer)-1;// the data size is size-1, because the final byte is not to be touched
  size_t encodedLength = cobs::encode(buffer, bufferSize);
  Serial.print("Encoded data: ");
  printBuffer(buffer, encodedLength);
  Serial.println("00"); // Add the final delimiter to complete the message

  // Now decode the data again
  size_t decodedLength = cobs::decode(buffer, encodedLength);

  Serial.print("Decoded data: ");
  printBuffer(buffer+1, decodedLength);
  Serial.print("\n");
  Serial.print("Final buffer content: ");
  printBuffer(buffer, sizeof(buffer));
}

void loop() {

}
