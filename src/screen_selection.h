
#ifndef SCREEN_SELECTION_H
#define SCREEN_SELECTION_H

#include <memory>
#include "SystemContext.h"
#include "ssd1306os.h"

#define START_ASCII 32
#define END_ASCII 126
#define CHAR_WIDTH 8
#define SCREEN_OFFSET 35    //active screen area starting from x = 35
#define MAX_POS_X 120

using namespace std;

class currentScreen{
public:
    explicit currentScreen(const std::shared_ptr<ssd1306os> &lcd)
    : lcd(lcd), color0(1),color1(1), color2(1){};
    void welcome();
    void screenSelection();
    void setCo2(int val);
    void info(int c, int t, int h, int s, int sp);
    void wifiConfig(const char* ssid, const char* pw);
    void asciiCharSelection(int posX, int asciiChar);
private:
    std::shared_ptr<ssd1306os> lcd;
    int color0, color1, color2;
    string text;
};


#endif
