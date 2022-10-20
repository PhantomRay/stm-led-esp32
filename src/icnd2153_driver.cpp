#include "icnd2153_driver.h"

#define REG_NUM 5
uint16_t reg_val[REG_NUM][3];
uint8_t cmd_latchs[REG_NUM] = {4,6,8,10,2};

/*--- gpio function ---*/
#define LED_MATRIX_PIN_NUM  12
gpio_num_t pins[LED_MATRIX_PIN_NUM]  = {PIN_CLK, PIN_LE, PIN_OE, PIN_A, PIN_B, PIN_C, PIN_R1, PIN_G1, PIN_B1, PIN_R2, PIN_G2, PIN_B2};

void gpio_init(){
    for (uint8_t x = 0; x < LED_MATRIX_PIN_NUM; x++) {        
        // if(gpio_set_direction(pins[x], GPIO_MODE_OUTPUT) != ESP_OK){
        //     Serial.printf("Pins[%d] err\n", x);
        // }
        pinMode(pins[x], OUTPUT);
        digitalWrite(pins[x], LOW);
    }
}

void IRAM_ATTR digitalWriteFast(uint8_t pinNum, bool value) {
    // GPIO.out_w1ts sets a pin high, GPIO.out_w1tc sets a pin low (ESP-IDF commands)
    if(value) GPIO.out_w1ts = 1 << pinNum;
    else GPIO.out_w1tc = 1 << pinNum;
}

void IRAM_ATTR digitalWriteEvenFaster(uint32_t data, uint32_t mask) {
    // GPIO.out writes multiple pins at the same time (ESP-IDF command)
    GPIO.out = (GPIO.out & ~mask) | data;
}
/*--- command ---*/
void IRAM_ATTR sendClock() {
    digitalWriteFast(PIN_CLK, 1);
    digitalWriteFast(PIN_CLK, 0);
}

void IRAM_ATTR sendLatch(uint8_t clocks) {
    // uint8_t  additionalClock = 16 - clocks;
    // while(additionalClock--) {
    //     sendClock();
    // }
    digitalWriteFast(PIN_LE, 1);
    while(clocks--) {
        sendClock();
    }
    digitalWriteFast(PIN_LE, 0);
}

// static void IRAM_ATTR sendScanLine(uint8_t line) {
//     uint32_t scanLine = 0x00000000;

//     if(line & 0x1) scanLine += 1;
//     if(line >> 1 & 0x1) scanLine += 2;
//     if(line >> 2 & 0x1) scanLine += 4;
//     if(line >> 3 & 0x1) scanLine += 16;
//     if(line >> 4 & 0x1) scanLine += 32;

//     digitalWriteEvenFaster(scanLine << 21, 0x06E00000);
// }

static void sendConfiguration(uint16_t reg_dat[], uint8_t reg_id, uint8_t chip_num)
{  
  sendLatch(14);           // Pre-active command
  
  uint32_t config_dat[16];  
  for (int b = 0; b < 16; b++) {
    uint16_t data_mask = 0x8000 >> b;      
    uint32_t value = 0;
    if (reg_dat[0] & data_mask){
      value |= PIN_MASK_R1 | PIN_MASK_R2;
    }
    if (reg_dat[1] & data_mask){
      value |= PIN_MASK_G1 | PIN_MASK_G2;
    }
    if (reg_dat[2] & data_mask){
      value |= PIN_MASK_B1 | PIN_MASK_B2;
    }
    config_dat[b] = value;
  }

  const uint32_t mask = PIN_MASK_DATA | PIN_MASK_CLK;
  for (uint8_t i = 0; i < chip_num; i++) { 
    for (uint8_t b = 0; b < 16; b++) {
      if((i == (chip_num-1)) && (b == (16 - cmd_latchs[reg_id]))){
        SET_LATCH;
      }
      digitalWriteEvenFaster(config_dat[b], mask); // color + reset clock
      SET_CLOCK; // Rising edge: clock color in.
    }
    digitalWriteEvenFaster(0, mask); //io->ClearBits(h.clock); 
    CLR_LATCH;
  }
}

void InitICND2153(uint8_t chip_num)
{
  gpio_init();
  /*reg1*/
  reg_val[0][0] = (0x07 << 8) | (1 << 6) | (3 << 4)  | (0 << 3) | (0 << 0);
  reg_val[0][1] = reg_val[0][0];
  reg_val[0][2] = reg_val[0][0];
  /*reg2*/
  reg_val[1][0] = (31 << 10) | (0 << 9) | (255 << 1)  | (0 << 0);
  reg_val[1][1] = (reg_val[1][0] & 0x83FF) | (28 << 10);
  reg_val[1][2] = (reg_val[1][0] & 0x83FF) | (23 << 10);
  /*reg3*/
  reg_val[2][0] = (4 << 12) | (0 << 10) | (1 << 9)  | (0 << 8) | (0 << 4) | (0 << 3)  | (1 << 2) | (3 << 0);
  reg_val[2][1] = reg_val[2][0];
  reg_val[2][2] = reg_val[2][0];
  /*reg4*/
  reg_val[3][0] = 0x0040;
  reg_val[3][1] = reg_val[3][0];
  reg_val[3][2] = reg_val[3][0];
  /*reg5 or Debug*/
  reg_val[4][0] = 0x0008;         //Reg5:0x003C
  reg_val[4][1] = reg_val[4][0];
  reg_val[4][2] = reg_val[4][0];

  sendLatch(14);     // Pre-active command
  sendLatch(12);     // Enable all output channels
  
  for(int id = 0; id < REG_NUM; id++){
    sendConfiguration(reg_val[id], id, chip_num);
  }
}
