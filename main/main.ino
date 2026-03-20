#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_TSC2007.h>

//--------  DISPLAY  --------/

#define TFT_CS 10
#define TFT_DC 9

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_TSC2007 ts;

//-------- TOUCH --------/
// Sets touch parameters taken from examples TFT examples
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000
#define TS_MIN_PRESSURE 100

//-------- SCREEN --------/
// Just defines number screen currently screen is just a initial screen not really used
// TODO

#define SCREEN_MAIN   0 // Some fix here
#define SCREEN_FUEL   1
#define SCREEN_DAMAGE 2
#define SCREEN_JOB    3

#define MIN_SCREEN    0
#define MAX_SCREEN    3

uint8_t currentScreen = MIN_SCREEN;

//-------- SERIAL PARSER --------/
// Consider changes in scope? - should this be global variable...

#define KEY_MAX 20
#define VALUE_MAX 12

char keyBuffer[KEY_MAX];
char valueBuffer[VALUE_MAX];

uint8_t keyPos = 0;
uint8_t valPos = 0;

enum ParseState
{
  WAIT_START,
  READ_KEY,
  READ_VALUE
};

ParseState parserState = WAIT_START;

//-------- DATA --------/
// struct for data

struct Field {
  const char* key;
  char value[9];
  char lastValue[9];
  const char* label;
  uint8_t screen;
};

Field fields[] = {
  {"fuel", "000", "", "Fuel: ", SCREEN_FUEL},
  {"max_fuel", "000", "", "Max: ", SCREEN_FUEL},
  {"fuel_percent", "000", "", "%: ", SCREEN_FUEL},
  {"fuel_range", "000", "", "Range: ", SCREEN_FUEL},
  {"dam_cabin", "000", "", "Cabin: ", SCREEN_DAMAGE},
  {"dam_chassis", "000", "", "Chassis: ", SCREEN_DAMAGE},
  {"dam_engine", "000", "", "Engine: ", SCREEN_DAMAGE},
  {"dam_trans", "000", "", "Trans: ", SCREEN_DAMAGE},
  {"dam_wheels", "000", "", "Wheel: ", SCREEN_DAMAGE},
  {"job_dest", "000", "", "Dest: ", SCREEN_JOB},
  {"job_dist", "000", "", "Dist: ", SCREEN_JOB},
  {"job_inc", "000", "", "Inc: ", SCREEN_JOB},
  {"job_time", "000", "", "Time: ", SCREEN_JOB}
};

const uint8_t FIELD_COUNT = sizeof(fields) / sizeof(Field);


//-------- SAFE COPY --------/
// Just ensure NULL terminator
void safeCopy(char *dest, const char *src, size_t size)
{
  size_t i = 0;

  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';  
}

//-------- SCREEN CHANGE --------/

void changeScreen(int dir)
{
  int next = currentScreen + dir;

  if (next > MAX_SCREEN) next = MIN_SCREEN;
  if (next < MIN_SCREEN) next = MAX_SCREEN;

  currentScreen = next;

  tft.fillScreen(ILI9341_BLACK);
  
  // force redraw
  for (uint8_t i = 0; i < FIELD_COUNT; i++)
  {
    fields[i].lastValue[0] = '\0';
  }
}

//-------- SERIAL COMMIT --------/
// TODO used if statements for ease of initial set up
// to review can we reference directly?
// lso requires 3 changes to add new value - Telemetry, commit and screen

void commitValue()
{
  for (uint8_t i = 0; i < FIELD_COUNT; i++)
  {
    if (strcmp(keyBuffer, fields[i].key) == 0)
    {
      safeCopy(fields[i].value, valueBuffer, sizeof(fields[i].value));
      return;
    }
  }
}

//-------- SERIAL PARSER --------/

void readSerial()
{
  while (Serial.available())
  {
    char c = Serial.read();

    switch (parserState)
    {
      case WAIT_START:
        if (c == '!')
        {
          keyPos = 0;
          valPos = 0;
          parserState = READ_KEY;
        }
      break;

      case READ_KEY:

        if (c == '*')
        {
          keyBuffer[keyPos] = 0;
          parserState = READ_VALUE;
        }
        else if (keyPos < KEY_MAX - 1)
        {
          keyBuffer[keyPos++] = c;
        }
      break;

      case READ_VALUE:

        if (c == '\n')
        {
          valueBuffer[valPos] = 0;
          commitValue();
          parserState = WAIT_START;
        }
        else if (valPos < VALUE_MAX - 1)
        {
          valueBuffer[valPos++] = c;
        }
      break;
    }
  }
}

//-------- TOUCH --------/
// handles touch screen for changing screen

void readTouch()
{
  uint16_t x, y, z1, z2;

  if (ts.read_touch(&x, &y, &z1, &z2) && z1 > TS_MIN_PRESSURE)
  {
    if (y < 1200) changeScreen(-1);
    if (y > 2300) changeScreen(1);
  }
}

//-------- PARTIAL DRAW HELPERS --------/

void drawField(Field &f, int y)
{
  if (strcmp(f.value, f.lastValue) == 0) return;

  safeCopy(f.lastValue, f.value, sizeof(f.lastValue));

  // --- EMPTY SPACE ---/ If value is less characters ensures old character dont remain on screen   
  char emptySpace[9-strlen(f.value)];
  memset(emptySpace, ' ', sizeof(emptySpace));
  emptySpace[sizeof(emptySpace) - 1] = '\0';
  
  tft.setCursor(0, y);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(3);

  tft.print(f.label);
  tft.print("     ");
  tft.setCursor(150, y);
  //tft.print("        ");
  //tft.setCursor(x + 150, y);
  tft.print(f.value);
  tft.print(emptySpace);

}

//-------- DRAW --------/

void drawScreen()
{
  if (currentScreen == SCREEN_MAIN) // Initial screen
  {
    tft.setCursor(0,0);
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Screen 0");
  }

  uint16_t space = 40;
  uint16_t y = 40;

  for (uint8_t i = 0; i < FIELD_COUNT; i++)

  {
    if (fields[i].screen == currentScreen)
    {
      drawField(fields[i], y);
      y += space;
    }
  }
  
}

//-------- SETUP --------/

void setup()
{
  Serial.begin(9600);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  if (!ts.begin())
  {
    Serial.println("Touch failed");
    while(1);
  }

  Serial.println("Ready");
}

//-------- LOOP --------/

void loop()
{
  readSerial();
  readTouch();
  drawScreen();
}