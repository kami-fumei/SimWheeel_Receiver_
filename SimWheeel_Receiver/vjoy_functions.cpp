#include "Lib_Simwheel.h"

void SetVJoyButton(UINT btnNumber, bool pressed, UINT vJoyId)
{
    // btnNumber starts at 1
    SetBtn(pressed, vJoyId, static_cast<UCHAR>(btnNumber));
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


bool vjoyeorrs(UINT vJoyId) {

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
        return true;
    }
    VjdStat status = GetVJDStatus(vJoyId);
    if (status == VJD_STAT_OWN || status == VJD_STAT_FREE) {
        if (!AcquireVJD(vJoyId)) {
            std::cerr << "Failed to acquire vJoy device #" << vJoyId << "\n";
            checkVJoyOwnership(vJoyId);
            WSACleanup();
            pressEnterToExit();
            return true;
        }
    }
    else {
        std::cerr << "vJoy device #" << vJoyId << " not available (status=" << status << ")\n";
        checkVJoyOwnership(vJoyId);
        WSACleanup();
        pressEnterToExit();
        return true;
    }
    return false;
}
