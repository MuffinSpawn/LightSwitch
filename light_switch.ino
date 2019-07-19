// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_GFX_RK.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_HX8357_RK.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_HX8357.h>

// This #include statement was automatically added by the Particle IDE.
// #include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_HX8357.h>
#include <Adafruit_HX8357_RK.h>
#include <Adafruit_STMPE610.h>
#include <SPI.h>

#define LED      D7
#define RANGER   A0

#define SD_CS    D2
#define STMPE_CS D3
#define TFT_CS   D4
#define TFT_DC   D5
#define TFT_RST -1
#define TFT_LITE D8
#define ROTATION_PORTRAIT 0
#define ROTATION_PORTRAIT_INV 2
#define ROTATION_LANDSCAPE 1
#define ROTATION_LANDSCAPE_INV 3
#define TEXT_COLOR 0xFF
#define TEXT_SIZE 2

Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// This is calibration data for the raw touch data to the screen coordinates
// #define TS_MINX 3800
// #define TS_MAXX 100
// #define TS_MINY 100
// #define TS_MAXY 3750
#define TS_MINX 200
#define TS_MAXX 3668
#define TS_MINY 200
#define TS_MAXY 3852

#define HX8357_BLACK       0x0000  ///<   0,   0,   0
#define HX8357_NAVY        0x000F  ///<   0,   0, 123
#define HX8357_DARKGREEN   0x03E0  ///<   0, 125,   0
#define HX8357_DARKCYAN    0x03EF  ///<   0, 125, 123
#define HX8357_MAROON      0x7800  ///< 123,   0,   0
#define HX8357_PURPLE      0x780F  ///< 123,   0, 123
#define HX8357_OLIVE       0x7BE0  ///< 123, 125,   0
#define HX8357_LIGHTGREY   0xC618  ///< 198, 195, 198
#define HX8357_DARKGREY    0x7BEF  ///< 123, 125, 123
#define HX8357_BLUE        0x001F  ///<   0,   0, 255
#define HX8357_GREEN       0x07E0  ///<   0, 255,   0
#define HX8357_CYAN        0x07FF  ///<   0, 255, 255
#define HX8357_RED         0xF800  ///< 255,   0,   0
#define HX8357_MAGENTA     0xF81F  ///< 255,   0, 255
#define HX8357_YELLOW      0xFFE0  ///< 255, 255,   0
#define HX8357_WHITE       0xFFFF  ///< 255, 255, 255
#define HX8357_ORANGE      0xFD20  ///< 255, 165,   0
#define HX8357_GREENYELLOW 0xAFE5  ///< 173, 255,  41
#define HX8357_PINK        0xFC18  ///< 255, 130, 198

/* create 15 buttons, in classic candybar phone style */
char buttonlabels[15][5] = {"Sel", "Clr", "Home", "1", "2", "3", "4", "5", "6", "7", "8", "9", "<", "0", ">" };
uint16_t buttoncolors[15] = {HX8357_DARKGREEN, HX8357_DARKGREY, HX8357_MAGENTA, 
                             HX8357_BLUE, HX8357_BLUE, HX8357_BLUE, 
                             HX8357_BLUE, HX8357_BLUE, HX8357_BLUE, 
                             HX8357_BLUE, HX8357_BLUE, HX8357_BLUE, 
                             HX8357_ORANGE, HX8357_BLUE, HX8357_ORANGE};
Adafruit_GFX_Button buttons[15];
#define BUTTON_X 40
#define BUTTON_Y 100
#define BUTTON_W 60
#define BUTTON_H 30
#define BUTTON_SPACING_X 20
#define BUTTON_SPACING_Y 20
#define BUTTON_TEXTSIZE 2

// text box where numbers go
#define TEXT_X 10
#define TEXT_Y 10
#define TEXT_W 220
#define TEXT_H 50
#define TEXT_TSIZE 3
#define TEXT_TCOLOR HX8357_MAGENTA
// the data (phone #) we store in the textfield
#define TEXT_LEN 12
char textfield[TEXT_LEN+1] = "";
uint8_t textfield_i = 0;

#define DISTANCE_RAW_MAX 3000
#define DISTANCE_RAW_MIN 600
#define DISTANCE_RAW_RANGE 2400  // MAX - MIN
#define DISTANCE_MAX 800  // mm
#define DISTANCE_MIN 100  // mm
#define DISTANCE_RANGE 700  // mm
float distance;

#define CIRCLE_RADIUS 20
uint16_t saved_patch[2*CIRCLE_RADIUS+11][2*CIRCLE_RADIUS+11];
bool first_pass = true;

// #define AVG_COUNT 64
#define AVG_COUNT 16
bool active = false;
bool touched_last = false;
bool touched = false;
bool state = false;

TS_Point p;
int16_t last_x, last_y;
int16_t x, y;
bool read_position;

Timer backlight_timer(5000, stop_timer, true);
void stop_timer() { }

void readData(uint16_t *x, uint16_t *y, uint8_t *z) {
  uint8_t data[4];
  
  for (uint8_t i=0; i<4; i++) {
    data[i] = ts.readRegister8(0xD7); //SPI.transfer(0x00); 
   // Serial.print("0x"); Serial.print(data[i], HEX); Serial.print(" / ");
  }
  *x = data[0];
  *x <<= 4;
  *x |= (data[1] >> 4);
  *y = data[1] & 0x0F; 
  *y <<= 8;
  *y |= data[2]; 
  *z = data[3];
}


TS_Point getPoint(void) {
    uint16_t x, y;
    uint8_t z;
  
    /* Making sure that we are reading all data before leaving */
    while(!ts.bufferEmpty()){
        readData(&x,&y,&z);
    }

    if (ts.bufferEmpty()) 
        ts.writeRegister8(STMPE_INT_STA, 0xFF); // reset all ints
  
    return TS_Point(x, y, z);
}

uint16_t readPixel(int x, int y) {
  tft.setAddrWindow(x,y,x,y);

  digitalWrite(TFT_DC, LOW);
//   digitalWrite(TFT_CLK, LOW);
  digitalWrite(TFT_CS, LOW);
  tft.spiWrite(0x2E); // memory read command

  digitalWrite(TFT_DC, HIGH);

  uint16_t r = 0;
  r = tft.spiRead(); // discard dummy read
  r = tft.spiRead() >> 3; // red: use 5 highest bits (discard three LSB)
  r = (r << 6) | tft.spiRead() >> 2; // green: use 6 highest bits (discard two LSB)
  r = (r << 5) | tft.spiRead() >> 3; // blue: use 5 highest bits (discard three LSB)

  digitalWrite(TFT_CS, HIGH);

  return r;
}

void readArea(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t data[2*CIRCLE_RADIUS+11][2*CIRCLE_RADIUS+11]) {
    // Serial.print("In readArea(");
    // Serial.print(x0); Serial.print(","); Serial.print(y0); Serial.print(","); Serial.print(x1); Serial.print(","); Serial.print(y1); Serial.println(")");
    uint16_t x_offset, y_offset;
    if (x0 < 0) {
        x0 = 0;
        x_offset = -x0;
    } else if (x0 >= tft.width()) {
        x0 = tft.width()-1;
    }
    if (y0 < 0) {
        y0 = 0;
        y_offset = -y0;
    } else if (y0 >= tft.height()) {
        y0 = tft.height()-1;
    }
    if (x1 < 0) {
        x1 = 0;
    } else if (x1 >= tft.width()) {
        x1 = tft.width()-1;
    }
    if (y1 < 0) {
        y1 = 0;
    } else if (y1 >= tft.height()) {
        y1 = tft.height()-1;
    }
    // Serial.print(x0); Serial.print(","); Serial.print(y0); Serial.print(","); Serial.print(x1); Serial.print(","); Serial.print(y1); Serial.print(","); Serial.print(x_offset); Serial.print(","); Serial.println(y_offset);

    const uint16_t width = x1 - x0;
    const uint16_t height = y1 - y0;

    tft.startWrite();
    tft.setAddrWindow(x0, y0, width, height);
    digitalWrite(TFT_DC, LOW);     // Command mode
    
    // digitalWrite(TFT_DC, LOW);
    // // digitalWrite(TFT_CLK, LOW);
    // digitalWrite(TFT_CS, LOW);
    tft.spiWrite(0x2E); // memory read command
    
    digitalWrite(TFT_DC, HIGH);    // Data mode
    // digitalWrite(TFT_DC, HIGH);

    tft.spiRead(); // discard dummy read
    uint16_t r = 0;
    for (int row=0; row<height; ++row) {
        for (int column=0; column<width; ++column) {
            uint16_t pixel = tft.spiRead() >> 3; // red: use 5 highest bits (discard three LSB)
            pixel = (pixel << 6) | tft.spiRead() >> 2; // green: use 6 highest bits (discard two LSB)
            pixel = (pixel << 5) | tft.spiRead() >> 3; // blue: use 5 highest bits (discard three LSB)
            data[row][column] = pixel;
            // Serial.println(pixel);
        }
    }
    tft.endWrite();
    // digitalWrite(TFT_CS, HIGH);
}

void drawArea(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t data[2*CIRCLE_RADIUS+11][2*CIRCLE_RADIUS+11]) {
    // Serial.print("In drawArea(");
    // Serial.print(x0); Serial.print(","); Serial.print(y0); Serial.print(","); Serial.print(x1); Serial.print(","); Serial.print(y1); Serial.println(")");
    uint16_t x_offset, y_offset;
    if (x0 < 0) {
        x0 = 0;
        x_offset = -x0;
    } else if (x0 >= tft.width()) {
        x0 = tft.width()-1;
    }
    if (y0 < 0) {
        y0 = 0;
        y_offset = -y0;
    } else if (y0 >= tft.height()) {
        y0 = tft.height()-1;
    }
    if (x1 < 0) {
        x1 = 0;
    } else if (x1 >= tft.width()) {
        x1 = tft.width()-1;
    }
    if (y1 < 0) {
        y1 = 0;
    } else if (y1 >= tft.height()) {
        y1 = tft.height()-1;
    }
    // Serial.print(x0); Serial.print(","); Serial.print(y0); Serial.print(","); Serial.print(x1); Serial.print(","); Serial.print(y1); Serial.print(","); Serial.print(x_offset); Serial.print(","); Serial.println(y_offset);

    const uint16_t width = x1 - x0;
    const uint16_t height = y1 - y0;

    tft.startWrite();
    tft.setAddrWindow(x0, y0, width, height);
    for (int16_t row=0; row<height; ++row) {
        for (int16_t column=0; column<width; ++column) {
            // tft.spiWrite(HX8357_WHITE>>8); tft.spiWrite(HX8357_WHITE);
            tft.spiWrite(data[row+y_offset][column+x_offset]>>8); tft.spiWrite(data[row+y_offset][column+x_offset]);
        }
    }
    tft.endWrite();
}

void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  tft.startWrite();
// writePixel(x, y, color);
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  tft.writePixel(x0  , y0+r, color);
  tft.writePixel(x0  , y0-r, color);
  tft.writePixel(x0+r, y0  , color);
  tft.writePixel(x0-r, y0  , color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
    tft.writePixel(x0 + x, y0 + y, color);
    tft.writePixel(x0 - x, y0 + y, color);
    tft.writePixel(x0 + x, y0 - y, color);
    tft.writePixel(x0 - x, y0 - y, color);
    tft.writePixel(x0 + y, y0 + x, color);
    tft.writePixel(x0 - y, y0 + x, color);
    tft.writePixel(x0 + y, y0 - x, color);
    tft.writePixel(x0 - y, y0 - x, color);
  }
  tft.endWrite();
}

float get_distance() {
    float distance_raw_sum = 0;
    for (int index=0; index<AVG_COUNT; index++) {
        distance_raw_sum += analogRead(RANGER);
    }
    const float distance_raw = distance_raw_sum / AVG_COUNT;
    // 1.20042023e+02 -1.06177458e-01  2.38401660e-05
    return 120. - 1.06177458e-01*distance_raw + 2.38401660e-05*distance_raw*distance_raw;
}

void setup() {
    pinMode(LED, OUTPUT);
    pinMode(RANGER, INPUT);
    pinMode(TFT_LITE, OUTPUT);

    digitalWrite(TFT_LITE, LOW);
    digitalWrite(LED, LOW);

    // attachInterrupt(TFT_LITE, function, mode, 0);

    Serial.begin(115200);
    // while(!Serial.isConnected()) Particle.process();

    Serial.println("HX8357D Featherwing touch test!"); 
    
    if (!ts.begin()) {
        Serial.println("Couldn't start touchscreen controller");
        while (1);
    }
    Serial.println("Touchscreen started");

    tft.begin();
    // SPI.setClockSpeed(400000);  // Set SPI clock to 100kHz
    tft.fillScreen(HX8357_BLACK);

    tft.setTextColor(TEXT_COLOR);
    tft.setTextSize(TEXT_SIZE);
    tft.setTextWrap(true);
    tft.setRotation(ROTATION_LANDSCAPE);

    // create buttons
    for (uint8_t row=0; row<5; row++) {
        for (uint8_t col=0; col<3; col++) {
            buttons[col + row*3].initButton(&tft, BUTTON_X+col*(BUTTON_W+BUTTON_SPACING_X), 
                     BUTTON_Y+row*(BUTTON_H+BUTTON_SPACING_Y),    // x, y, w, h, outline, fill, text
                      BUTTON_W, BUTTON_H, HX8357_WHITE, buttoncolors[col+row*3], HX8357_WHITE,
                      buttonlabels[col + row*3], BUTTON_TEXTSIZE); 
            buttons[col + row*3].drawButton();
        }
    }
  
    // create 'text field'
    tft.drawRect(TEXT_X, TEXT_Y, TEXT_W, TEXT_H, HX8357_WHITE);

    // tft.fillCircle(tft.width()/2, tft.height()/2, 20, HX8357_BLUE);

    // for (uint16_t row=0; row<(2*CIRCLE_RADIUS+1); ++row) {
    //     for (uint16_t column=0; column<(2*CIRCLE_RADIUS+1); ++column) {
    //         saved_patch[row][column] = row*column;
    //     }
    // }

    last_x = 0;
    last_y = 0;
    x = 0;
    y = 0;
}

void loop() {
    // digitalWrite(LED, HIGH);
    // Mesh.publish("shed-lock", "lock");
    
    // delay(1000);
    
    // digitalWrite(LED, LOW);
    // Mesh.publish("shed-lock", "unlock");
    
    // delay(1000);

    // tft.fillRect(0, 0, 160, 50, 0);
    // tft.fillRect(0, 0, tft.width(),tft.height(), 0);
    // tft.fillRect(0, 0, tft.width(),tft.height(), HX8357_GREEN);

    // Range: 3.1V @ 100mm to 0.4V @ 800mm
    // distance_raw = analogRead(RANGER);
    if (!active) {
        distance = get_distance();
        if (distance < 20) {
            // turn on the backlight
            digitalWrite(TFT_LITE, HIGH);
            active = true;
            backlight_timer.start();
        }
    } else if (!backlight_timer.isActive()) {
        distance = get_distance();
        if (distance < 20) {
            backlight_timer.start();
        } else {
            // turn off the backlight
            digitalWrite(TFT_LITE, LOW);
            active = false;
        }
    }

    /*
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(0, 0);
    tft.print(distance_raw);
    tft.setCursor(0, 25);
    tft.print(distance);
    */

    touched_last = touched;
    touched = ts.touched();
    if (touched) {
        if (!touched_last) {
            state = !state;
            digitalWrite(LED, state?HIGH:LOW);
        }

        last_x = x;
        last_y = y;
        p = getPoint();
        x = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
        y = map(p.x, TS_MINX, TS_MAXX, tft.height(), 0);
        if (last_x != x || last_y != y) {
            if (first_pass) {
                first_pass = false;
            } else {
                drawArea(last_x-CIRCLE_RADIUS-5, last_y-CIRCLE_RADIUS-5, last_x+CIRCLE_RADIUS+5, last_y+CIRCLE_RADIUS+5, saved_patch);
            }
            // drawArea(x-CIRCLE_RADIUS, y-CIRCLE_RADIUS, x+CIRCLE_RADIUS, y+CIRCLE_RADIUS, saved_patch);
            // tft.fillCircle(last_x, last_y, CIRCLE_RADIUS, 0);
            readArea(x-CIRCLE_RADIUS-5, y-CIRCLE_RADIUS-5, x+CIRCLE_RADIUS+5, y+CIRCLE_RADIUS+5, saved_patch);
            // getPoint();
            tft.fillCircle(x, y, CIRCLE_RADIUS, HX8357_BLUE);
        }
    }


    // if (read_position) {
    // }
    // Retrieve a point  
    // p = ts.getPoint();
    // Serial.print("X = "); Serial.print(p.x); Serial.print("\tY = "); Serial.print(p.y);  Serial.print("\tPressure = "); Serial.println(p.z); 
    // tft.setCursor(0, 0);
    // tft.print(sprintf("(%d, %d) @ %d", p.x, p.y, p.z));
    
    // Serial.print("X = "); Serial.print(p.x); Serial.print("\tY = "); Serial.print(p.y);  Serial.print("\tPressure = "); Serial.println(p.z); 
    
    // // Scale from ~0->4000 to tft.width using the calibration #'s
    // p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    // p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
    // x = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
    // y = map(p.x, TS_MINX, TS_MAXX, tft.height(), 0);
    // Serial.print("X* = "); Serial.print(x); Serial.print("\tY* = "); Serial.print(y);  Serial.print("\tPressure = "); Serial.println(p.z); 
    // tft.fillCircle(x, y, 20, HX8357_BLUE);
    // tft.print(p.x);
    // tft.print(" ");
    // tft.print(p.y);
    // tft.print(" ");
    // tft.print(p.z);
}

