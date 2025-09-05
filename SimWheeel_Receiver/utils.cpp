#include "Lib_Simwheel.h"

// Map a normalized [-1,1] value to vJoy axis range [0,0x8000]
LONG MapToVJoyAxis(double norm) {
    if (norm < -1.0) norm = -1.0;
    if (norm > 1.0) norm = 1.0;
    return static_cast<LONG>(16384 + norm * 16384);
}

void pressEnterToExit() {
    std::cout << "Press Enter to exit...";
    std::cin.ignore(10000, '\n');
    std::cin.get();
}

//void moveMouseRelative(int dx) {
//    INPUT input = { 0 };
//    input.type = INPUT_MOUSE;
//    input.mi.dx = dx;
//    input.mi.dy = 0;
//    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_MOVE_NOCOALESCE;
//    SendInput(1, &input, sizeof(INPUT)); 
//}