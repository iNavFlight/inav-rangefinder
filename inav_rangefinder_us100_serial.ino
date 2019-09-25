#include <PingSerial.h>
/*
 * Set I2C Slave address
 */
#define I2C_SLAVE_ADDRESS 0x14

#define STATUS_OK 0
#define STATUS_OUT_OF_RANGE 1

#include <Wire.h>

#ifndef TWI_RX_BUFFER_SIZE
  #define TWI_RX_BUFFER_SIZE ( 16 )
#endif

  #define MIN_RANGE 100  //Range of 10 cm meters
  #define MAX_RANGE 4000 //Range of 4 meters

PingSerial us100(Serial, MIN_RANGE, MAX_RANGE);  // Valid measurements are 650-1200mm

bool ping_enabled = FALSE;
unsigned int pingSpeed = 200; // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned long pingTimer = 0;     // Holds the next ping time.

uint8_t i2c_regs[] =
{
    0, //status
    0, //older 8 of distance
    0, //younger 8 of distance
};

const byte reg_size = sizeof(i2c_regs);
volatile byte reg_position = 0;

void requestEvent()
{  
    Wire.write(i2c_regs, 3);
}

void receiveEvent(uint8_t howMany) {

    if (howMany < 1) {
        // Sanity-check
        return;
    }

    if (howMany > TWI_RX_BUFFER_SIZE)
    {
        // Also insane number
        return;
    }

    reg_position = Wire.read();

    howMany--;

    if (!howMany) {
        // This write was only to set the buffer for next read
        return;
    }

    // Everything above 1 byte is something we do not care, so just get it from bus as send to /dev/null
    while(howMany--) {
      Wire.read();
    }
}

void setup() {
  us100.begin();
 /*
  * Setup I2C
  */
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

void loop() {

  byte data_available;
  unsigned int current_height = 0;

  data_available = us100.data_available();

  if (data_available & DISTANCE) {
      current_height = us100.get_distance();

      if (current_height > MAX_RANGE || current_height < MIN_RANGE) {
        i2c_regs[0] = STATUS_OUT_OF_RANGE;
      }
      else {
        uint16_t cm = (uint16_t) current_height/10;
        i2c_regs[0] = STATUS_OK;

        i2c_regs[1] = cm >> 8;
        i2c_regs[2] = cm & 0xFF;
      }
  }
  
  if (ping_enabled && (millis() >= pingTimer)) {   // pingSpeed milliseconds since last ping, do another ping.
      pingTimer = millis() + pingSpeed;      // Set the next ping time.
      us100.request_distance();
  }

}
