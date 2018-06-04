/*
 * Decide on rangefinder type here. Only one type should be uncommented
 */
//  #define USE_US100
#define USE_HCSR04
// #define USE_URM37

/*
 * Set I2C Slave address
 */
#define I2C_SLAVE_ADDRESS 0x14

// #define DEBUG;

#define TRIGGER_PIN A2
#define ECHO_PIN A1
#define LED_PIN 13

#define STATUS_OK 0
#define STATUS_OUT_OF_RANGE 1

#include <Wire.h>

#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif

/*
 * Configuration magic, do not touch
 */
#ifdef USE_US100
  #define PULSE_TO_CM 59 //Multiplier pulse length to distance in [cm]
  #define MAX_RANGE 300 //Range of 4 meters
#endif

#ifdef USE_HCSR04
  #define PULSE_TO_CM 58 //Multiplier pulse length to distance in [cm]
  #define MAX_RANGE 300 //Range of 4 meters
#endif

#define PULSE_TIMEOUT (MAX_RANGE * PULSE_TO_CM) //this is an equivalent of 4 meters range

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
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  /*
   * Setup I2C
   */
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  Serial.begin(115200);
}

long microsecondsToCentimeters(long microseconds){
  return (microseconds * 34 / 100 / 2) / 10;
}

void loop() {

    static uint32_t nextUpdate = 0;

  /*
   * Measurement is done every 6th wakeup, that gives more less 10Hz update rate (96ms)
   */
  if (millis() > nextUpdate) {
    
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);

    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, PULSE_TIMEOUT);

    if (duration > 0) {
      i2c_regs[0] = STATUS_OK;
    } else {
      i2c_regs[0] = STATUS_OUT_OF_RANGE;
    }
    
    uint16_t cm = (uint16_t) microsecondsToCentimeters(duration);
    
    i2c_regs[1] = cm >> 8;
    i2c_regs[2] = cm & 0xFF;

#ifdef DEBUG
    Serial.print(i2c_regs[0]);
    Serial.print(" ");
    Serial.print(i2c_regs[1]);
    Serial.print(" ");
    Serial.println(i2c_regs[2]);
 #endif    

    nextUpdate = millis() + 100;
  }
}
