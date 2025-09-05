#include "Lib_Simwheel.h"

void ShowLocalIP() {
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG family = AF_INET; // Only IPv4. Use AF_UNSPEC for both IPv4 & IPv6

    ULONG bufLen = 15000;
    PIP_ADAPTER_ADDRESSES adapters = (IP_ADAPTER_ADDRESSES*)malloc(bufLen);

    if (adapters == NULL) { // Check for memory allocation failure
        printf("Memory allocation failed\n");
        return;
    }

    DWORD ret = GetAdaptersAddresses(family, flags, NULL, adapters, &bufLen);
    if (ret != NO_ERROR) {
        printf("GetAdaptersAddresses failed with error: %lu\n", ret);
        free(adapters);
        return;
    }
    printf("Use this/those ip in app to connect\n");

    for (PIP_ADAPTER_ADDRESSES aa = adapters; aa != NULL; aa = aa->Next) {
        // Skip down/loopback interfaces
        if (aa->OperStatus != IfOperStatusUp || aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;

        for (PIP_ADAPTER_UNICAST_ADDRESS ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
            SOCKADDR* addr = ua->Address.lpSockaddr;
            if (addr->sa_family == AF_INET) {
                char ip[INET_ADDRSTRLEN];
                sockaddr_in* ipv4 = (sockaddr_in*)addr;
                inet_ntop(AF_INET, &ipv4->sin_addr, ip, sizeof(ip));
                printf("Local IPv4: %s \n", ip);
            }
        }
    }

    free(adapters);
}