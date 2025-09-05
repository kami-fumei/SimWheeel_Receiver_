#include "Lib_Simwheel.h"
using json = nlohmann::json;

template <typename T>
T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}


void processInput(const std::string& buffer, int vJoyId, double userRange)
{
    try {
        auto j = json::parse(buffer);

        double steering = j.at("steering").get<double>();  // e.g. 450.0
        double throttle = j.at("throttle").get<double>();  // 0.75
        double brake = j.at("brake").get<double>();     // 0.25
        double zaxis = j.at("zaxis").get<double>();
        //double dx = j.at("dx").get<double>(); // Mouse relative movement

        j.erase("steering");
        j.erase("throttle");
        j.erase("brake");
        j.erase("zaxis");
        //j.erase("dx");

        // 6. Feed to vJoy axes
        double normSteer = steering / userRange;

        LONG vJoyValue = clamp(MapToVJoyAxis(normSteer), static_cast <LONG> (0), static_cast <LONG> (32768));


        //SetAxis(MapToVJoyAxis(normSteer), vJoyId, HID_USAGE_X);

        SetAxis(vJoyValue, vJoyId, HID_USAGE_X);
        SetAxis(MapToVJoyAxis(throttle * 2 - 1), vJoyId, HID_USAGE_Y);
        SetAxis(MapToVJoyAxis(brake * 2 - 1), vJoyId, HID_USAGE_Z);
        SetAxis(MapToVJoyAxis(zaxis * 2 - 1), vJoyId, HID_USAGE_RZ);
        SetVJoyButton(1, j.value("horn", false), vJoyId);

        //mouse relative movement
        //moveMouseRelative(dx);

        j.erase("horn");

        for (json::iterator it = j.begin(); it != j.end(); ++it) {
            std::string key = it.key();
            int buttonId = std::stoi(key);
            bool value = it.value();
            SetVJoyButton(buttonId, j.value(key, false), vJoyId);
            j.erase(key); // Remove processed button
        }

    }
    catch (json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
    }
}
