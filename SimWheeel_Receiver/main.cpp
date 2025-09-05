#include "Lib_Simwheel.h"

UINT vJoyId = 1;

using json = nlohmann::json;



int main() {

    ShowLocalIP(); // Print local IP address for debugging

    Settings usersetting = getSettings();

    std::cout << "\n **Settings** \n1.Default Steering range enabled: " << (usersetting.steering_range==0?"No":"Yes") << "\n"
        << "2.Logging enabled: " << (usersetting.is_log ? "Yes" : "No") << "\n" << std::endl;


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


    // 1. Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        pressEnterToExit();
        return 1;
    }

    // 2. Initialize vJoy
    if (vjoyeorrs(vJoyId)) {
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
            std::cout << "Received" << " -> " << msg << std::endl;
        }

		//*core funtion to process input and send to vJoy*//
		processInput(buffer, vJoyId, userRange);
    }

    // 7. Cleanup
    RelinquishVJD(vJoyId);
    closesocket(sock);
    WSACleanup();
    return 0;
}
