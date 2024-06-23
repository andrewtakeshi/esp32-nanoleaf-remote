// #ifndef SMARTLIGHTCONTROLLER_H
// #define SMARTLIGHTCONTROLLER_H

// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
// #include <Bounce2.h>
// #include <esp_sleep.h>
// #include <driver/gpio.h>
// #include <RotaryEncoder.h>

// // State enum
// enum State
// {
//     IDLE,
//     ADJUST_BRIGHTNESS,
//     MENU,
//     SELECT_COLOR
// };

// // Function prototypes
// void IRAM_ATTR readEncoderISR();
// void setup();
// void loop();
// void handleIdle();
// void handleAdjustBrightness();
// void handleMenu();
// void handleColorSelection();
// void enterDeepSleep();
// void toggleLights();
// void changeBrightness(int change);
// void setColor(String color);

// // Global variables
// // extern Adafruit_SSD1306 display;
// // extern RotaryEncoder encoder;
// // extern Bounce button;
// // extern unsigned long lastInteractionTime;
// // extern State state;
// // extern String menuItems[];
// // extern int selectedItem;

// #endif // SMARTLIGHTCONTROLLER_H