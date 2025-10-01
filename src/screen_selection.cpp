#include "screen_selection.h"
#include "icons.h"
#include "cstring"
#include <cstdio>
#include "SystemContext.h"

using namespace std;

void currentScreen::welcome() {
    ssd->fill(0);
    ssd->text("GROUP 07", 28, 5);
    ssd->text("Carol", 42, 22);
    ssd->text("Julija", 38, 34);
    ssd->text("Linh", 46, 46);
    ssd->text("Qi", 54, 58);
    ssd->show();
}

void currentScreen::screenSelection() {
    auto& ctx = SystemContext::getInstance();
    color0 = color1 = color2 = 1;

    ssd->fill(0);
    switch (ctx.selection_screen_option) {
        case 0:
            color0 = 0;
            ssd->fill_rect(0, 0, 128, 11, 1);
            break;
        case 1:
            color1 = 0;
            ssd->fill_rect(0, 16, 128, 11, 1);
            break;
        case 2:
            color2 = 0;
            ssd->fill_rect(0, 30, 128, 11, 1);
            break;
        default:
            break;
    }
    ssd->text("SHOW INFO", 28, 2, color0);
    ssd->text("SET CO2 LEVEL", 8, 18, color1);
    ssd->text("CONFIG WIFI", 20, 32, color2);
    ssd->fill_rect(0, 53, 33, 10, 1);
    ssd->text("OK", 1, 54, 0);
    ssd->show();
}

void currentScreen::setCo2(const int val) {
    ssd->fill(0);
    ssd->text("CO2 LEVEL", 50, 20);
    text = to_string(val) + " ppm";
    ssd->text(text, 55, 38);
    ssd->fill_rect(0, 0, 33, 10, 1);
    ssd->text("BACK", 1, 1, 0);
    ssd->fill_rect(0, 53, 33, 10, 1);
    ssd->text("OK", 1, 54, 0);
    ssd->show();
}

void currentScreen::info(const int c, const int t, const int h, const int s, const int sp) {
    ssd->fill(0);
    mono_vlsb icon(info_icon, 20, 20);
    ssd->blit(icon, 0, 40);
    text = "CO2:" + to_string(c) + " ppm";
    ssd->text(text, 38, 2);
    text = "RH:" + to_string(h) + " %";
    ssd->text(text, 38, 15);
    text = "T:" + to_string(t) + " C";
    ssd->text(text, 38, 28);
    text = "S:" + to_string(s) + " %";
    ssd->text(text, 38, 41);
    text = "SP:" + to_string(sp) + " ppm";
    ssd->text(text, 38, 54);
    ssd->fill_rect(0, 0, 33, 10, 1);
    ssd->text("BACK", 1, 1, 0);
    ssd->show();
}

void currentScreen::wifiConfig(const char* ssid, const char* pw) {
    auto& ctx = SystemContext::getInstance();
    ssd->fill(0);
    switch (ctx.wifi_screen_option) {
        case 0:
            ssd->rect(0,17,128,13,1);
            break;
        case 1:
            ssd->rect(0,32,128,13,1);
            break;
        default:
            break;
    }
    ssd->text("SSID:", 2,20);
    ssd->text(ssid, 44, 20);
    ssd->text("PASS:",2,35);
    ssd->text(pw, 44, 35);
    ssd->fill_rect(0, 0, 33, 10, 1);
    ssd->text("BACK", 1, 1, 0);
    ssd->fill_rect(0, 53, 33, 10, 1);
    ssd->text("OK", 1, 54, 0);
    ssd->show();
}

void currentScreen::asciiCharSelection(const int posX, const int asciiChar) {
    auto& ctx = SystemContext::getInstance();
    int posY = 0;
    char c[2];
    sprintf(c, "%c", asciiChar);
    switch (ctx.wifi_screen_option) {
        case 0: posY = 20; break;
        case 1: posY = 35; break;
        default: break;
    }
    if (asciiChar < START_ASCII) {
        ssd->fill_rect(posX,posY,8,8,1);
    } else {
        ssd->fill_rect(posX,posY,8,8,0);
        ssd->text(c, posX, posY);
    }
    ssd->show();
}
