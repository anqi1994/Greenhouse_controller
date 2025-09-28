#include "screen_selection.h"
#include "icons.h"
#include "cstring"
#include <cstdio>

using namespace std;

void currentScreen::welcome() {
    lcd->fill(0);
    lcd->text("GROUP 07",30,20);
    lcd->text("Carol, Qi, Linh, Julija",0,40);
    lcd->show();
}

void currentScreen::screenSelection(){
    color0 = 1;
    color1 = 1;
    color2 = 1;
    lcd->fill(0);
    switch (selection_screen_option) {
        case 0:
            color0 = 0;
            lcd->fill_rect(0, 0, 128, 11, 1);
            break;
        case 1:
            color1 = 0;
            lcd->fill_rect(0, 16, 128, 11, 1);
            break;
        case 2:
            color2 = 0;
            lcd->fill_rect(0, 30, 128, 11, 1);
            break;
        default:
            break;
    }
    lcd->text("SHOW INFO", 28, 2, color0);
    lcd->text("SET CO2 LEVEL", 8, 18, color1);
    lcd->text("CONFIG WIFI", 20, 32, color2);
    lcd->fill_rect(0, 53, 33, 10, 1);
    lcd->text("OK", 1, 54, 0);
    lcd->show();
}

void currentScreen::setCo2(const int val){
    lcd->fill(0);
    lcd->text("CO2 LEVEL", 50, 20);
    text = to_string(val) + " ppm";
    lcd->text(text, 55, 38);
    lcd->fill_rect(0, 0, 33, 10, 1);
    lcd->text("BACK", 1, 1, 0);
    lcd->fill_rect(0, 53, 33, 10, 1);
    lcd->text("OK", 1, 54, 0);
    lcd->show();
}

void currentScreen::info(const int c, const int t, const int h, const int s, const int sp) {
    lcd->fill(0);
    mono_vlsb icon(info_icon, 20, 20);
    lcd->blit(icon, 0, 40);
    text = "CO2:" + to_string(c) + " ppm";
    lcd->text(text, 38, 2);
    text = "RH:" + to_string(h) + " %";
    lcd->text(text, 38, 2+13);
    text = "T:" + to_string(t) + " C";
    lcd->text(text, 38, 2+13+13);
    text = "S:" + to_string(s) + " %";
    lcd->text(text, 38,2+13+13+13);
    text = "SP:" + to_string(sp) + " ppm";
    lcd->text(text, 38, 2+13+13+13+13);
    lcd->fill_rect(0, 0, 33, 10, 1);
    lcd->text("BACK", 1, 1, 0);
    lcd->show();
}

void currentScreen::wifiConfig(const char* ssid, const char* pw) {
    lcd->fill(0);
    switch (wifi_screen_option) {
        case 0:
            lcd->rect(0,17,128,13,1);
            break;
        case 1:
            lcd->rect(0,32,128,13,1);
            break;
        default:
            break;
    }
    lcd->text("SSID:", 2,20);
    lcd->text(ssid, 44, 20);
    lcd->text("PASS:",2,35);
    lcd->text(pw, 44, 35);
    lcd->fill_rect(0, 0, 33, 10, 1);
    lcd->text("BACK", 1, 1, 0);
    lcd->fill_rect(0, 53, 33, 10, 1);
    lcd->text("OK", 1, 54, 0);
    lcd->show();
}

void currentScreen::asciiCharSelection(const int posX, const int asciiChar){
    int posY = 0;
    char c[2];
    sprintf(c, "%c", asciiChar);
    switch (wifi_screen_option) {
        case 0:
            posY = 20;
            break;
        case 1:
            posY = 35;
            break;
        default:
            break;
    }
    if (asciiChar < START_ASCII){
        lcd->fill_rect(posX,posY,8,8,1);
    } else {
        lcd->fill_rect(posX,posY,8,8,0);
        lcd->text(c, posX, posY);
    }
    lcd->show();
}