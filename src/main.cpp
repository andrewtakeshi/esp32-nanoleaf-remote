#include <Wire.h>
#include <Bounce2.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <RotaryEncoder.h>
#include <U8x8lib.h>
// State enum
enum State
{
  IDLE,
  ADJUST_BRIGHTNESS,
  MENU,
  SELECT_COLOR
};

// Function prototypes
void IRAM_ATTR readEncoderISR();
void setup();
void loop();
void handleIdle();
void handleAdjustBrightness();
void handleMenu();
void handleColorSelection();
void enterDeepSleep();
void toggleLights();
void changeBrightness(int change);

#define wrap(amt, lb, ub) ((amt) < (lb) ? (ub) : ((amt) > (ub) ? (lb) : (amt)))

// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 32
// #define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define SDA_PIN 4
#define SCL_PIN 16
U8X8_SSD1306_128X32_UNIVISION_HW_I2C display(U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

#define BUTTON_PIN 32
#define ENCODER_CLK 26
#define ENCODER_DT 27

#define MENU_TRANSITION_INTERVAL 2000
#define MENU_TIMEOUT 10000
#define SLEEP_TIMEOUT 60000
#define FRAME_INTERVAL 34
#define BRIGHTNESS_MIN 1
#define BRIGHTNESS_MAX 100

RotaryEncoder *encoder = nullptr;
Bounce button = Bounce();

State interfaceState = IDLE;

char *menuItems[] = {(char *)"Red", (char *)"Green", (char *)"Blue", (char *)"White"};
int menuItemsLength = sizeof(menuItems) / sizeof(menuItems[0]);
bool menuModified = true;

unsigned long lastInteractionTime = 0;
int brightness = 0;
int selectedMenuItem = 0;
bool lightOn = false;

void IRAM_ATTR readEncoderISR()
{
  encoder->tick();
}

void setup()
{
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.begin(115200);
  while (!Serial)
    ;
  if (!display.begin())
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (1)
      ;
  }
  display.setFont(u8x8_font_chroma48medium8_r);

  display.clear();

  encoder = new RotaryEncoder(ENCODER_DT, ENCODER_CLK, RotaryEncoder::LatchMode::FOUR3);
  encoder->setPosition(brightness);

  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), readEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), readEncoderISR, CHANGE);

  button.attach(BUTTON_PIN, INPUT_PULLUP);
  button.interval(25);

  lastInteractionTime = millis();

  // TODO: Connect to Wifi
  // TODO: Pull current light status on startup
}

void loop()
{
  long loopStartTime = millis();
  button.update();
  encoder->tick();

  if (millis() - lastInteractionTime > SLEEP_TIMEOUT)
  {
    enterDeepSleep();
  }

  if (millis() - lastInteractionTime > MENU_TIMEOUT && interfaceState == MENU)
  {
    display.clear();
    interfaceState = IDLE;
  }

  switch (interfaceState)
  {
  case IDLE:
    handleIdle();
    break;
  case ADJUST_BRIGHTNESS:
    handleAdjustBrightness();
    break;
  case MENU:
    handleMenu();
    break;
  case SELECT_COLOR:
    handleColorSelection();
    break;
  }

  while (millis() - loopStartTime < FRAME_INTERVAL)
    ;
}

void handleIdle()
{
  long encoderPos = encoder->getPosition();
  int newBrightness = constrain(encoderPos, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  if (brightness != newBrightness)
  {
    brightness = newBrightness;
    encoder->setPosition(brightness);
    interfaceState = ADJUST_BRIGHTNESS;
    lastInteractionTime = millis();
  }

  if (button.fell() || button.read() == LOW)
  {
    unsigned long pressTime = millis();
    while (button.read() == LOW)
    {
      button.update();
    }
    if (millis() - pressTime > MENU_TRANSITION_INTERVAL)
    {
      Serial.println("Encoder button held");
      interfaceState = MENU;
      menuModified = true;
      lastInteractionTime = millis();
    }
    else
    {
      Serial.println("Encoder button pressed");
      toggleLights();
      lastInteractionTime = millis();
    }
  }
}

void handleAdjustBrightness()
{
  Serial.print("Setting brighntess to ");
  Serial.println(brightness);
  interfaceState = IDLE;
}

void handleMenu()
{
  RotaryEncoder::Direction dir = encoder->getDirection();

  if (dir != RotaryEncoder::Direction::NOROTATION)
  {
    selectedMenuItem = wrap(selectedMenuItem + (dir == RotaryEncoder::Direction::CLOCKWISE) ? 1 : -1, 0, menuItemsLength - 1);

    menuModified = true;
    // Encoder movement in menu should not affect brightness
    encoder->setPosition(brightness);
    lastInteractionTime = millis();
  }

  if (button.fell() || button.read() == LOW)
  {
    unsigned long pressTime = millis();
    while (button.read() == LOW)
    {
      button.update();
    }
    // Go back to IDLE state
    if (millis() - pressTime > MENU_TRANSITION_INTERVAL)
    {
      Serial.println("Switch to IDLE due to > 2000 ms press");
      interfaceState = IDLE;
    }
    // Set color to current selectedMenuItem
    else
    {
      interfaceState = SELECT_COLOR;
    }
    display.clear();
    lastInteractionTime = millis();
  }

  // Only draw menu if it needs to be drawn
  if (menuModified)
  {
    display.clear();
    // u8x8 treats each row and column as 8 wide - hence, 128x32 screen is 32x4
    for (int i = 0; i < 3; ++i)
    {
      switch (i)
      {
      case 0:
        display.drawString(1, 0, menuItems[wrap(selectedMenuItem - 1, 0, menuItemsLength - 1)]);
        break;
      case 1:
        display.setInverseFont(1);
        display.drawString(1, 1, menuItems[wrap(selectedMenuItem, 0, menuItemsLength - 1)]);
        display.setInverseFont(0);
        break;
      case 2:
        display.drawString(1, 2, menuItems[wrap(selectedMenuItem + 1, 0, menuItemsLength - 1)]);
        break;
      }
    }
    menuModified = false;
  }
}

void handleColorSelection()
{
  String color = menuItems[selectedMenuItem];
  Serial.print("Selected color: ");
  Serial.println(color);
  interfaceState = IDLE;
}

void enterDeepSleep()
{
  // Configure wakeup sources
  gpio_wakeup_enable((gpio_num_t)ENCODER_CLK, GPIO_INTR_HIGH_LEVEL);
  gpio_wakeup_enable((gpio_num_t)ENCODER_DT, GPIO_INTR_HIGH_LEVEL);
  gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_HIGH_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  display.clear();
  esp_deep_sleep_start();
}

void toggleLights()
{
  lightOn = !lightOn;
  Serial.print("Turning light ");
  Serial.println(lightOn);
  interfaceState = IDLE;
}
