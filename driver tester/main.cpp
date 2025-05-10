#include "resources.h"

void bro();

int main()
{
    HANDLE hDevice = CreateFile(SYMBOLIC_LINK_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hDevice == NULL)
    {
        std::cout << "cannot create file " << std::endl;
    }

    FilterSettings filterSettings;
   // getLocalIp();
    memcpy(filterSettings.filterName, L"facebookTe", 10);
    filterSettings.filterName[FILTER_NAME_SIZE] = '\0';
    filterSettings.changeToSize = 0;
    filterSettings.dstIpAddr = 0;
    filterSettings.srcIpAddr = 0;
    filterSettings.isUDP = TRUE;
    filterSettings.dstPort = 53;
    filterSettings.payloadSize = strlen(DOMAIN);
    
    char buffer[sizeof(FilterSettings) + sizeof(DOMAIN) - 1] = { 0 };

    memcpy(buffer, &filterSettings, sizeof(FilterSettings));
    memcpy(buffer + sizeof(FilterSettings), DOMAIN, sizeof(DOMAIN) - 1);


    DWORD bytesReturned;
    BOOL result = DeviceIoControl(hDevice,
        IOCTL_ADD_FILTER,
        buffer,               // Input buffer
        sizeof(buffer),         // Size of input buffer
        NULL,                           // Output buffer (not used)
        0,                              // Size of output buffer
        &bytesReturned,                 // Bytes returned
        NULL);                          // Overlapped structure
   
    
    system("pause");
    CloseHandle(hDevice);
    return 0;
}

UINT32 getLocalIp()
{
    WSADATA wsaData;
    char hostname[256];
    struct addrinfo hints, * info;
    UINT32 ipAddress = 0;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 0;
    }

    // Get the hostname of the local machine
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "Error retrieving hostname.\n";
        WSACleanup();
        return 0;
    }

    std::cout << "Hostname: " << hostname << "\n";

    // Resolve hostname to IP address
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, NULL, &hints, &info) != 0) {
        std::cerr << "Error retrieving IP address.\n";
        WSACleanup();
        return 0;
    }

    // Get the first IP address from the list
    struct sockaddr_in* addr = (struct sockaddr_in*)info->ai_addr;
    ipAddress = ntohl(addr->sin_addr.s_addr); // IP address in network byte order

    freeaddrinfo(info);
    WSACleanup();

    return ipAddress;
}

void bro()
{
   
}
