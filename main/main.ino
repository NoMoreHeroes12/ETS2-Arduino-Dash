#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_TSC2007.h>

/--------  DISPLAY  --------/

#define TFT_CS 10
#define TFT_DC 9

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_TSC2007 ts;

/-------- TOUCH --------/
/ Sets touch parameters taken from examples TFT examples
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000
#define TS_MIN_PRESSURE 100

/-------- SCREEN --------/
/ Just defines number screen currently screen is just a initial screen not really used
/ TODO

#define MIN_SCREEN 0
#define MAX_SCREEN 3

uint8_t currentScreen = 0;

/-------- SERIAL PARSER --------/
/ Consider changes in scope? - should this be global variable...

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

/-------- DATA --------/
/ struct for data

struct Telemetry
{
  char fuel[8] = "000";
  char max_fuel[8] = "000";
  char fuel_percent[8] = "000";
  char fuel_range[8] = "000";

  char dam_cabin[8] = "000";
  char dam_chassis[8] = "000";
  char dam_engine[8] = "000";
  char dam_trans[8] = "000";
  char dam_wheels[8] = "000";

  char job_dest[8] = "000";
  char job_dist[8] = "000";
  char job_inc[8] = "000";
  char job_time[8] = "000";
  
};

Telemetry data;

/-------- LAST DRAWN -------/
/ Used to for comparison to only udpate screen on changes

Telemetry lastDrawn;

/-------- SAFE COPY --------/
/ Just ensure NULL terminator
void safeCopy(char *dest, const char *src, size_t size)
{
  size_t i = 0;

  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
  
}

/-------- SCREEN CHANGE --------/

void changeScreen(int dir)
{
  int next = currentScreen + dir;

  if (next > MAX_SCREEN) next = MIN_SCREEN;
  if (next < MIN_SCREEN) next = MAX_SCREEN;

  currentScreen = next;

  tft.fillScreen(ILI9341_BLACK);
  memset(&lastDrawn, 0, sizeof(lastDrawn)); // force redraw
}

/-------- SERIAL COMMIT --------/
/ TODO used if statements for ease of initial set up
/ to review can we reference directly?
/ also requires 3 changes to add new value - Telemetry, commit and screen


void commitValue()
{
  if (strcmp(keyBuffer, "fuel") == 0)
    safeCopy(data.fuel, valueBuffer, sizeof(data.fuel));

  else if (strcmp(keyBuffer, "max_fuel") == 0)
    safeCopy(data.max_fuel, valueBuffer, sizeof(data.max_fuel));

  else if (strcmp(keyBuffer, "fuel_percent") == 0)
    safeCopy(data.fuel_percent, valueBuffer, sizeof(data.fuel_percent));

  else if (strcmp(keyBuffer, "fuel_range") == 0)
    safeCopy(data.fuel_range, valueBuffer, sizeof(data.fuel_range));

  else if (strcmp(keyBuffer, "dam_cabin") == 0)
    safeCopy(data.dam_cabin, valueBuffer, sizeof(data.dam_cabin));

  else if (strcmp(keyBuffer, "dam_chassis") == 0)
    safeCopy(data.dam_chassis, valueBuffer, sizeof(data.dam_chassis));

  else if (strcmp(keyBuffer, "dam_engine") == 0)
    safeCopy(data.dam_engine, valueBuffer, sizeof(data.dam_engine));

  else if (strcmp(keyBuffer, "dam_trans") == 0)
    safeCopy(data.dam_trans, valueBuffer, sizeof(data.dam_trans));

  else if (strcmp(keyBuffer, "dam_wheels") == 0)
    safeCopy(data.dam_wheels, valueBuffer, sizeof(data.dam_wheels));

  else if (strcmp(keyBuffer, "job_dest") == 0)
    safeCopy(data.job_dest, valueBuffer, sizeof(data.job_dest));

  else if (strcmp(keyBuffer, "job_dist") == 0)
    safeCopy(data.job_dist, valueBuffer, sizeof(data.job_dist));

  else if (strcmp(keyBuffer, "job_inc") == 0)
    safeCopy(data.job_inc, valueBuffer, sizeof(data.job_inc));

  else if (strcmp(keyBuffer, "job_time") == 0)
    safeCopy(data.job_time, valueBuffer, sizeof(data.job_time));
}

/-------- SERIAL PARSER --------/

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

/-------- TOUCH --------/
/ handles touch screen for changing screen

void readTouch()
{
  uint16_t x, y, z1, z2;

  if (ts.read_touch(&x, &y, &z1, &z2) && z1 > TS_MIN_PRESSURE)
  {
    if (y < 1200) changeScreen(-1);
    if (y > 2300) changeScreen(1);
  }
}

/-------- PARTIAL DRAW HELPERS --------/

void drawField(int x, int y, const char *label, const char *value, char *last)
{
  if (strcmp(value, last) == 0) return;

  safeCopy(last, value, 8);

  / --- EMPTY SPACE ---/ If value is less characters ensures old character dont remain on screen   
  char emptySpace[8-strlen(value)];
  memset(emptySpace, ' ', sizeof(emptySpace));
  emptySpace[sizeof(emptySpace) - 1] = '\0';
  
  tft.setCursor(x, y);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(3);

  tft.print(label);
  tft.print("     ");
  tft.setCursor(x + 150, y);
  //tft.print("        ");
  //tft.setCursor(x + 150, y);
  tft.print(value);
  tft.print(emptySpace);

}

/-------- SCREENS --------/

void drawFuel()
{
  
  drawField(0, 40, "Fuel:", data.fuel, lastDrawn.fuel);
  drawField(0, 80, "Max:", data.max_fuel, lastDrawn.max_fuel);
  drawField(0, 120, "%:", data.fuel_percent, lastDrawn.fuel_percent);
  drawField(0, 160, "Range:", data.fuel_range, lastDrawn.fuel_range);
}

void drawDamage()
{
  drawField(0, 40, "Cabin:", data.dam_cabin, lastDrawn.dam_cabin);
  drawField(0, 80, "Chassis:", data.dam_chassis, lastDrawn.dam_chassis);
  drawField(0, 120, "Engine:", data.dam_engine, lastDrawn.dam_engine);
  drawField(0, 160, "Trans:", data.dam_trans, lastDrawn.dam_trans);
  drawField(0, 200, "Wheels:", data.dam_wheels, lastDrawn.dam_wheels);
}

void drawTrip()
{
  drawField(0, 40, "Destin:", data.job_dest, lastDrawn.job_dest);
  drawField(0, 80, "Dist:", data.job_dist, lastDrawn.job_dist);
  drawField(0, 120, "Income:", data.job_inc, lastDrawn.job_inc);
  drawField(0, 160, "Time:", data.job_time, lastDrawn.job_time);
  
}

/-------- DRAW --------/

void drawScreen()
{
  if (currentScreen == 0) / In itial screen
  {
    tft.setCursor(0,0);
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Screen 0");
  }

  if (currentScreen == 1)
    drawFuel();

  if (currentScreen == 2)
    drawDamage();

    if (currentScreen == 3)
    drawTrip();
}

/-------- SETUP --------/

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

/-------- LOOP --------/

void loop()
{
  readSerial();
  readTouch();
  drawScreen();
}