#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "FS.h"
#include "SD_MMC.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <EEPROM.h>

#define EEPROM_SIZE 2
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
//#include "camera_pins.h"

int pictureNumber = 0;
camera_fb_t * fb = NULL;
bool pressed = false;

bool triggered() {
  if (digitalRead(1)==0) {
    if (pressed) return false;
    return pressed = true;
  } else {
    return pressed = false;
  }
}

bool testImage() {
  digitalWrite(4, HIGH);
  delay(500);
  digitalWrite(4, LOW);
}

void saveImage() {
  fb = esp_camera_fb_get();  
  if(!fb) return;
  // initialize EEPROM with predefined size SDMMC_INTMASK_SBE
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;
  // path where new picture will be saved in SD Card
  String path = "/picture" + String(pictureNumber) +".jpg";

  fs::FS &fs = SD_MMC; 
  File file = fs.open(path.c_str(), FILE_WRITE);
  if (file) {
    file.write(fb->buf, fb->len); // payload (image, payload length)
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;
  
  #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
  #endif
  
  delay(500);
  
  // set modelbit=true to free up pin4 for flash off
  SD_MMC.begin("/sdcard", true);
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE)return;
  
  // turn off flash light
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  // rtc_gpio_hold_en(GPIO_NUM_4);

  // trigger setup
  pinMode(1, INPUT_PULLUP);

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) return;

   sensor_t * s = esp_camera_sensor_get();
   if (s->id.PID == OV3660_PID) {
     s->set_vflip(s, 1); // flip it back
     s->set_brightness(s, 1); // up the brightness just a bit
     
     s->set_saturation(s, -2); // lower the saturation
   }
}

void loop() {
  if (triggered()) {
    saveImage();
    delay(1000);
  }
}
