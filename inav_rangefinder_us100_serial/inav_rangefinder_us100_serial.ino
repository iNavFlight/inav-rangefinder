//#define DEBUG
#include <SoftwareSerial.h>;
 
#define US100_TX 5
#define US100_RX 4
#define LED_PIN 13

SoftwareSerial US100(US100_RX, US100_TX);

#define I2C_SLAVE_ADDRESS 0x14

#define STATUS_OK 0
#define STATUS_OUT_OF_RANGE 1

#include <Wire.h>

#ifndef TWI_RX_BUFFER_SIZE
  #define TWI_RX_BUFFER_SIZE ( 16 )
#endif

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

    if (howMany < 1 || howMany > TWI_RX_BUFFER_SIZE) {
        // Sanity-check
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

#ifdef DEBUG
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
#endif

  pinMode(US100_RX, INPUT);
  pinMode(US100_TX, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  US100.begin(9600);
  
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

#define MIN_RANGE 2   //Range of 2 cm 
#define MAX_RANGE 400 //Range of 400 cm

int buffer[2];

void loop() {
  US100.listen();
  US100.flush();
  US100.write(0x55);
  
  delay(50);
  if(US100.available() >= 2) {
    for (int i=0; i<2; i++) {
      buffer[i] = US100.read();
    }
    
    int cm = (((buffer[0] * 256) + buffer[1])/10 );
    if ((cm < MAX_RANGE) && (cm > MIN_RANGE)) {
      
      i2c_regs[0] = STATUS_OK;
      i2c_regs[1] = cm >> 8;
      i2c_regs[2] = cm & 0xFF;
    }
    else {
      i2c_regs[0] = STATUS_OUT_OF_RANGE;
      if (cm > MAX_RANGE) {
        cm = MAX_RANGE;
        i2c_regs[1] = cm >> 8;
        i2c_regs[2] = cm & 0xFF;
      }
      if (cm < MIN_RANGE) {
        cm = 0;
        i2c_regs[1] = cm >> 8;
        i2c_regs[2] = cm & 0xFF;
      }
    }
#ifdef DEBUG
  Serial.print(" Regs: ");
  Serial.print(i2c_regs[0]);
  Serial.print(" ");
  Serial.print(i2c_regs[1]<<8);
  Serial.print(" ");
  Serial.print(i2c_regs[2]);
  Serial.print(" ");
  Serial.println(cm);
#endif
  }

  delay(50);
}
