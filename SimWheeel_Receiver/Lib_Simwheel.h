#ifndef Lib_Simwheel_H
#define Lib_Simwheel_H
#include <iostream>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include "vJoyInterface.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <utility>
#include <iphlpapi.h>
#include <fstream>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "vJoyInterface.lib")


struct Settings {
    double steering_range;
    bool is_log;
};

void ShowLocalIP();
Settings getSettings();
double userSteering();
LONG MapToVJoyAxis(double norm);
void pressEnterToExit();
void SetVJoyButton(UINT btnNumber, bool pressed, UINT vJoyId);
void checkVJoyOwnership(UINT id);
bool vjoyeorrs(UINT vJoyId);
void processInput(const std::string& buffer, int vJoyId, double userRange);
#endif // Lib_Simwheel

