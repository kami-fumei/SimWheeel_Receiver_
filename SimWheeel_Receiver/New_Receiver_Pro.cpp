// pc_udp_receiver_with_vjoy.cpp
// UDP receiver with vJoy integration: listens on port 4567, parses controls,
// and feeds them into a vJoy virtual joystick.

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
//#include <regex>
#include "vJoyInterface.h"
#include <nlohmann/json.hpp>
#include <iphlpapi.h>
#include <fstream>
#include <vector>
#include <utility>
//#include <algorithm>
//#include <limits>

// vJoy axis usage codes (if undefined)
//#ifndef HID_USAGE_X
//#define HID_USAGE_X 0x30
//#define HID_USAGE_Y 0x31
//#define HID_USAGE_Z 0x32
//#endif

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "vJoyInterface.lib")

using json = nlohmann::json;

template <typename T>
T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}



// No longer strictly needed but good practice

// This is the single header file from the nlohmann/json library.
// Download it from here: https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp

// For convenience, use the nlohmann::json namespace.
using json = nlohmann::json;

// --- A struct to hold the driver settings ---
// This provides clear, descriptive names for the returned values.
struct Settings {
    double steering_range;
    bool is_log;
};


// --- THIS FUNCTION NOW HANDLES EVERYTHING ---
// It checks if the settings file exists, creates it if not, then reads it
// and returns the settings inside the new Settings struct.
Settings getSettings() {
    const std::string filename = "settings.json";

    // --- Step 1: Check if the settings file exists ---
    std::ifstream check_file(filename);
    if (!check_file.is_open()) {
        // The file does not exist, so create it with default values.
        std::cout << "File '" << filename << "' not found. Creating a new one with default settings." << std::endl;
        std::ofstream create_file(filename);
        create_file << R"({
  // IF YOU MESSED UP THE SETTINGS FILE, DELETE IT AND RUN THE PROGRAM AGAIN

  // IMPORTANT: If you change the steering range, you must also change it
  // in both the GAME and APP settings to match the new range,
  // otherwise the steering will not work properly.

  // Default steering range (minimum: 90, maximum: 2520)
  "steering_range_default": 0,

  // Enable or disable logging of control data
  "is_log": true
})";
        create_file.close();
    }
    // The check_file object goes out of scope and closes here.


    // --- Step 2: Read and parse the settings.json file ---
    // At this point, the file is guaranteed to exist.
    try {
        // Open the file for reading.
        std::ifstream settings_file(filename);
        if (!settings_file.is_open()) {
            // This should ideally not happen since we just checked/created the file.
            std::cerr << "Error: Could not open settings.json for reading." << std::endl;
            // Return default values on error
            return { 0, true };
        }

        // Parse the file stream into a json object.
       // allows comments
        json settings_json = json::parse(settings_file, nullptr, true, true);


        // --- Step 3: Extract the values 
        // From that object, we get the values. If the keys don't exist, use the default 'true'.
        double steering = settings_json.value("steering_range_default", 0);
        bool log = settings_json.value("is_log", true);

        // Return the two values packaged in our new struct.
        return { steering, log };

    }
    catch (json::parse_error& e) {
        // Catch parsing errors (e.g., malformed JSON).
        std::cerr << "JSON Parse Error in " << filename << ": " << e.what() << '\n'
            << "exception id: " << e.id << '\n'
            << "byte position of error: " << e.byte << std::endl;
    }
    catch (json::exception& e) {
        // Catch other library exceptions.
        std::cerr << "JSON general exception: " << e.what() << std::endl;
    }

    // In case of any exception, return the default values.
    return { 900.0, true };
}


//void ShowLocalIP() {
//    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
//    ULONG family = AF_INET; // Only IPv4. Use AF_UNSPEC for both IPv4 & IPv6
//
//    ULONG bufLen = 15000;
//    PIP_ADAPTER_ADDRESSES adapters = (IP_ADAPTER_ADDRESSES*)malloc(bufLen);
//
//    DWORD ret = GetAdaptersAddresses(family, flags, NULL, adapters, &bufLen);
//    if (ret != NO_ERROR) {
//        printf("GetAdaptersAddresses failed with error: %lu\n", ret);
//        free(adapters);
//        return;
//    }
//    printf("Use this/those ip in app to connect\n");
//
//    for (PIP_ADAPTER_ADDRESSES aa = adapters; aa != NULL; aa = aa->Next) {
//        // Skip down/loopback interfaces
//        if (aa->OperStatus != IfOperStatusUp || aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
//            continue;
//
//        for (PIP_ADAPTER_UNICAST_ADDRESS ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
//            SOCKADDR* addr = ua->Address.lpSockaddr;
//            if (addr->sa_family == AF_INET) {
//                char ip[INET_ADDRSTRLEN];
//                sockaddr_in* ipv4 = (sockaddr_in*)addr;
//                inet_ntop(AF_INET, &ipv4->sin_addr, ip, sizeof(ip));
//                printf("Local IPv4: %s \n", ip);
//            }
//        }
//    }
//
//    free(adapters);
//}

// Map a normalized [-1,1] value to vJoy axis range [0,0x8000]
LONG MapToVJoyAxis(double norm) {
    if (norm < -1.0) norm = -1.0;
    if (norm > 1.0) norm = 1.0;
    return static_cast<LONG>(16384 + norm * 16384);
}

double userSteering() {
    double userRange = 900.0; // Default
    std::string input;

    std::cout << "Enter steering range (min 90, max 2520), or press Enter for default (900) :";
    std::getline(std::cin, input);

    if (!input.empty()) {
        try {
            double tempRange = std::stod(input);
            if (tempRange >= 90.0 && tempRange <= 2520.0) {
                userRange = tempRange;
            }
            else {
                std::cerr << "Range too low or high. Using default (900).\n";
            }
        }
        catch (...) {
            std::cerr << "Invalid input. Using default (900).\n";
        }
    }

    std::cout << "Using steering range: " << userRange << "\n";

    return userRange;
}

UINT vJoyId = 1;

void SetVJoyButton(UINT btnNumber, bool pressed)
{
    // btnNumber starts at 1
    SetBtn(pressed, vJoyId, static_cast<UCHAR>(btnNumber));
}


void pressEnterToExit() {
    std::cout << "Press Enter to exit...";
    std::cin.ignore(10000, '\n');
    std::cin.get();
}

void checkVJoyOwnership(UINT id) {
    VjdStat status = GetVJDStatus(id);

    switch (status) {
    case VJD_STAT_FREE:
        std::cout << "Device " << id << " is FREE.\n";
        break;

    case VJD_STAT_OWN:
        std::cout << "Device " << id << " is OWNED by this process.\n";
        break;

    case VJD_STAT_BUSY:
        std::cout << "Device " << id << " is BUSY (owned by another process).\n";
        break;

    case VJD_STAT_MISS:
        std::cout << "Device " << id << " is not configured in vJoyConf.\n";
        break;

    default:
        std::cout << "Unknown device status.\n";
        break;
    }
}

//void moveMouseRelative(int dx) {
//    INPUT input = { 0 };
//    input.type = INPUT_MOUSE;
//    input.mi.dx = dx;
//    input.mi.dy = 0;
//    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_MOVE_NOCOALESCE;
//    SendInput(1, &input, sizeof(INPUT)); 
//}

int main() {

    ShowLocalIP(); // Print local IP address for debugging

    Settings usersetting = getSettings();

    std::cout << "\n **Settings** \n1.Default Steering range enabled: " << (usersetting.steering_range==0?"No":"Yes") << "\n"
        << "2.Logging enabled: " << (usersetting.is_log ? "Yes" : "No") << "\n" << std::endl;


    // 1. Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        pressEnterToExit();
        return 1;
    }

    // 2. Initialize vJoy
    //UINT vJoyId = 1;
    if (!vJoyEnabled()) {
        std::cout << "\n!!  vJoy Device " << vJoyId << " is not available.\n";
        std::cout << " To fix this:\n";
        //std::cout << "watch this on yt:\n";
        //std::cout << "OR:\n";
        std::cout << "1. Press the Windows key and search for \"Configure vJoy or vJoyConf\".\n";
        std::cout << "2. Open the vJoy Configuration Tool.\n";
        std::cout << "3. Check 'Enable vJoy' in bottom corner.\n";
        std::cout << "4. Select Device " << vJoyId << ".\n";
        std::cout << "5. Enable all axes (like X, Y) and set number of buttons or just set 32.\n";
        std::cout << "6. Turn off force feedback for better experience.\n";
        std::cout << "7. Click 'Apply', then close the tool and restart this app.\n\n";
        WSACleanup();
        pressEnterToExit();
        return 1;
    }
    VjdStat status = GetVJDStatus(vJoyId);
    if (status == VJD_STAT_OWN || status == VJD_STAT_FREE) {
        if (!AcquireVJD(vJoyId)) {
            std::cerr << "Failed to acquire vJoy device #" << vJoyId << "\n";
            checkVJoyOwnership(vJoyId);
            WSACleanup();
            pressEnterToExit();
            return 1;
        }
    }
    else {
        std::cerr << "vJoy device #" << vJoyId << " not available (status=" << status << ")\n";
        checkVJoyOwnership(vJoyId);
        WSACleanup();
        pressEnterToExit();
        return 1;
    }
    std::cout << "vJoy device #" << vJoyId << " acquired\n";

    // 3. Create UDP socket
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // 4. Bind socket to port 4567
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(4567);
    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    std::cout << "Listening for UDP on port 4567...\n";


    //user steering range

    double userRange;

    if (usersetting.steering_range == 0) {
        userRange = userSteering(); // Get user input for steering range
    }
    else {
        userRange = usersetting.steering_range; // Use default or predefined value
        if (userRange < 90.0 || userRange > 2520.0) {
            std::cerr << "Steering range out of bounds (90-2520). Using default (900) Please change in setting.json .\n";
            userRange = 900.0;
        }
        std::cout << "Using steering range: " << userRange << "\n";
    }


    // 5. Receive loop
    const int bufSize = 512;
    char buffer[bufSize];
    sockaddr_in client;
    int clientLen = sizeof(client);



    while (true) {
        int bytes = recvfrom(sock, buffer, bufSize - 1, 0,
            reinterpret_cast<sockaddr*>(&client), &clientLen);
        if (bytes == SOCKET_ERROR) {
            std::cerr << "recvfrom() error: " << WSAGetLastError() << "\n";
            break;
        }
        buffer[bytes] = '\0';
        std::string msg(buffer);

        wchar_t ipStr[INET_ADDRSTRLEN]; // Wide-character buffer for IP address
        InetNtopW(AF_INET, &client.sin_addr, ipStr, INET_ADDRSTRLEN);

        /* std::cout << "re "
             << ":" << ntohs(client.sin_port)
             << " -> " << msg.c_str() << std::endl;*/

             // This clears the remainder of the line before overwriting it
        if (usersetting.is_log) {
            std::cout << "[log]Received" << " -> " << msg << std::endl;
        }
        // Parse values

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
            SetVJoyButton(1, j.value("horn", false));

            //mouse relative movement
            //moveMouseRelative(dx);

            j.erase("horn");

            for (json::iterator it = j.begin(); it != j.end(); ++it) {
                std::string key = it.key();
                int buttonId = std::stoi(key);
                bool value = it.value();
                SetVJoyButton(buttonId, j.value(key, false));
                j.erase(key); // Remove processed button
            }


        }
        catch (json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << "\n";
        }

    }

    // 7. Cleanup
    RelinquishVJD(vJoyId);
    closesocket(sock);
    WSACleanup();
    return 0;
}
