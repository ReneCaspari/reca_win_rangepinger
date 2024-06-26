#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <ws2tcpip.h>
#include <unordered_map>
#include <mutex>
#include <algorithm>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS

struct Range {
    std::string name;
    std::string startIP;
    std::string endIP;
};

struct PingResult {
    std::string ipAddress;
    std::string macAddress;
    std::string manufacturer;
    DWORD roundTripTime;
};

std::vector<Range> loadConfig(const std::string& filename, int& pingTimeout) {
    std::ifstream file(filename);
    std::vector<Range> ranges;
    if (!file.is_open()) {
        std::ofstream config(filename);
        config << "Range1\n192.168.0.1\n192.168.0.254\n";
        config << "Range2\n192.168.3.1\n192.168.3.254\n";
        config << "PingTimeout\n1000\n"; // Default timeout of 1000 ms
        config.close();
        std::cout << "Default config file created. Please edit the config file and rerun the program.\n";
        exit(0);
    }
    else {
        std::string line;
        Range range;
        while (std::getline(file, line)) {
            if (line.find("Range") != std::string::npos) {
                if (!range.name.empty()) {
                    ranges.push_back(range);
                    range = Range();
                }
                range.name = line;
            }
            else if (range.startIP.empty()) {
                range.startIP = line;
            }
            else if (range.endIP.empty()) {
                range.endIP = line;
            }
            else if (line.find("PingTimeout") != std::string::npos) {
                std::getline(file, line);
                pingTimeout = std::stoi(line);
            }
        }
        if (!range.name.empty()) {
            ranges.push_back(range);
        }
    }
    return ranges;
}

void loadOUIFile(const std::string& filename, std::unordered_map<std::string, std::string>& ouiMap) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "OUI file not found. Manufacturer information will be unavailable.\n";
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix, manufacturer;
        if (iss >> prefix >> manufacturer) {
            ouiMap[prefix] = manufacturer;
        }
    }
}

std::string getManufacturer(const std::string& macAddress, const std::unordered_map<std::string, std::string>& ouiMap) {
    std::string prefix = macAddress.substr(0, 8); // Extract the OUI part
    auto it = ouiMap.find(prefix);
    if (it != ouiMap.end()) {
        return it->second;
    }
    return "";
}

void getMACAddress(const std::string& ip, std::string& macAddress) {
    struct sockaddr_in sa;
    char buf[sizeof(struct in_addr)];
    int result = InetPton(AF_INET, ip.c_str(), buf);
    if (result != 1) {
        macAddress = "Invalid IP";
        return;
    }

    ULONG macAddr[2];
    ULONG macAddrLen = 6;  // length of MAC address

    memset(&macAddr, 0xff, sizeof(macAddr));

    DWORD ret = SendARP(*reinterpret_cast<IPAddr*>(buf), 0, macAddr, &macAddrLen);
    if (ret == NO_ERROR) {
        BYTE* bMacAddr = (BYTE*)&macAddr;
        std::ostringstream oss;
        for (int i = 0; i < (int)macAddrLen; ++i) {
            if (i != 0)
                oss << "-";
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)bMacAddr[i];
        }
        macAddress = oss.str();
    }
    else {
        macAddress = "N/A";
    }
}

void pingIP(const std::string& ipAddress, int timeout, const std::unordered_map<std::string, std::string>& ouiMap, std::vector<PingResult>& results, std::mutex& mtx) {
    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Unable to open handle.\n";
        return;
    }

    char SendData[32] = "Data Buffer";
    LPVOID ReplyBuffer = nullptr;
    DWORD ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    ReplyBuffer = (VOID*)malloc(ReplySize);
    if (ReplyBuffer == nullptr) {
        std::cerr << "Unable to allocate memory.\n";
        IcmpCloseHandle(hIcmpFile);
        return;
    }

    struct sockaddr_in sa;
    char buf[sizeof(struct in_addr)];
    InetPton(AF_INET, ipAddress.c_str(), buf);

    DWORD dwRetVal = IcmpSendEcho(hIcmpFile, *reinterpret_cast<IPAddr*>(buf), SendData, sizeof(SendData),
        NULL, ReplyBuffer, ReplySize, timeout);

    if (dwRetVal != 0) {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
        char str[INET_ADDRSTRLEN];
        InetNtop(AF_INET, &pEchoReply->Address, str, INET_ADDRSTRLEN);
        std::string macAddress;
        getMACAddress(ipAddress, macAddress);
        std::string manufacturer = getManufacturer(macAddress, ouiMap);

        PingResult result = { ipAddress, macAddress, manufacturer, pEchoReply->RoundTripTime };

        std::lock_guard<std::mutex> lock(mtx);
        results.push_back(result);
    }

    free(ReplyBuffer);
    IcmpCloseHandle(hIcmpFile);
}

bool compareIP(const std::string& ip1, const std::string& ip2) {
    std::istringstream iss1(ip1);
    std::istringstream iss2(ip2);
    int a, b, c, d;
    char dot;
    iss1 >> a >> dot >> b >> dot >> c >> dot >> d;
    int e, f, g, h;
    iss2 >> e >> dot >> f >> dot >> g >> dot >> h;
    return std::tie(a, b, c, d) < std::tie(e, f, g, h);
}

void pingRange(const std::string& startIP, const std::string& endIP, int timeout, const std::unordered_map<std::string, std::string>& ouiMap) {
    std::stringstream start(startIP), end(endIP);
    int a, b, c, d, e, f, g, h;
    char dot;
    start >> a >> dot >> b >> dot >> c >> dot >> d;
    end >> e >> dot >> f >> dot >> g >> dot >> h;

    std::vector<PingResult> results;
    std::vector<std::thread> threads;
    std::mutex mtx;

    for (int i = d; i <= h; ++i) {
        std::stringstream ip;
        ip << a << "." << b << "." << c << "." << i;
        std::string ipAddress = ip.str();
        threads.emplace_back(pingIP, ipAddress, timeout, std::ref(ouiMap), std::ref(results), std::ref(mtx));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Sort results by IP address
    std::sort(results.begin(), results.end(), [](const PingResult& lhs, const PingResult& rhs) {
        return compareIP(lhs.ipAddress, rhs.ipAddress);
        });

    // Display results
    std::cout << "+------------------+----------------------+-----------------------+-------------------------+\n";
    std::cout << "| IP Address       | Time (ms)            | MAC Address           | Manufacturer            |\n";
    std::cout << "+------------------+----------------------+-----------------------+-------------------------+\n";
    for (const auto& result : results) {
        std::cout << "| " << std::setw(16) << result.ipAddress
            << " | " << std::setw(20) << result.roundTripTime
            << " | " << std::setw(21) << result.macAddress
            << " | " << std::setw(25) << result.manufacturer << " |\n";
    }
    std::cout << "+------------------+----------------------+-----------------------+-------------------------+\n";
}

int main() {
    std::string configFileName = "config.txt";
    std::string ouiFileName = "oui.txt";
    int pingTimeout = 1000; // Default timeout in milliseconds
    auto ranges = loadConfig(configFileName, pingTimeout);

    std::unordered_map<std::string, std::string> ouiMap;
    loadOUIFile(ouiFileName, ouiMap);

    std::string rangeName;
    std::cout << "Enter the range name to ping: ";
    std::cin >> rangeName;

    for (const auto& range : ranges) {
        if (range.name == rangeName) {
            pingRange(range.startIP, range.endIP, pingTimeout, ouiMap);
            return 0;
        }
    }
    std::cout << "Range not found in config file.\n";
    return 1;
}
