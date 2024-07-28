#include <jni.h>
#include <string>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>

/**
 * Checks if the given IPv6 address is a global unicast address.
 *
 * @param addr Pointer to an in6_addr structure representing the IPv6 address.
 * @return True if the address is a global unicast address, false otherwise.
 */
bool isGlobalUnicastIPv6(const struct in6_addr *addr) {
    return (addr->s6_addr[0] & 0xE0) == 0x20;
}

/**
 * Checks if the given IPv4 address is a public address.
 *
 * @param addr Pointer to an in_addr structure representing the IPv4 address.
 * @return True if the address is a public IPv4 address, false otherwise.
 */
bool isPublicIPv4(const struct in_addr *addr) {
    uint32_t ip = ntohl(addr->s_addr);
    return !(ip >= 0x0A000000 && ip <= 0x0AFFFFFF) &&  // 10.0.0.0 - 10.255.255.255
           !(ip >= 0xAC100000 && ip <= 0xAC1FFFFF) &&  // 172.16.0.0 - 172.31.255.255
           !(ip >= 0xC0A80000 && ip <= 0xC0A8FFFF) &&  // 192.168.0.0 - 192.168.255.255
           !(ip >= 0xA9FE0000 && ip <= 0xA9FEFFFF) &&  // 169.254.0.0 - 169.254.255.255
           ip != 0x7F000001;                           // 127.0.0.1 (loopback)
}

/**
 * Retrieves the IP address of the device.
 *
 * This function retrieves the device's IP address by iterating over the
 * network interfaces and checking their addresses. It prefers IPv6 global
 * unicast addresses but falls back to public IPv4 addresses if no suitable
 * IPv6 address is found.
 */
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_cloudonix_MainActivity_getIPAddress(JNIEnv *env, jobject /* this */) {
    struct ifaddrs *ifaddr, *ifa;
    char addr[INET6_ADDRSTRLEN];
    std::string ipv4Address;
    bool foundIPv4 = false;

    if (getifaddrs(&ifaddr) == -1) {
        return env->NewStringUTF("Error");
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;

        int family = ifa->ifa_addr->sa_family;

        if (family == AF_INET6) { // IPv6
            auto *sa = (struct sockaddr_in6 *) ifa->ifa_addr;
            if (isGlobalUnicastIPv6(&sa->sin6_addr)) {
                inet_ntop(AF_INET6, &sa->sin6_addr, addr, sizeof(addr));
                freeifaddrs(ifaddr);
                return env->NewStringUTF(addr);
            }
        }
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;

        int family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) { // IPv4
            auto *sa = (struct sockaddr_in *) ifa->ifa_addr;
            if (isPublicIPv4(&sa->sin_addr)) {
                inet_ntop(AF_INET, &sa->sin_addr, addr, sizeof(addr));
                freeifaddrs(ifaddr);
                return env->NewStringUTF(addr);
            }
            if (!foundIPv4 && sa->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
                inet_ntop(AF_INET, &sa->sin_addr, addr, sizeof(addr));
                ipv4Address = addr;
                foundIPv4 = true;
            }
        }
    }

    if (foundIPv4) {
        freeifaddrs(ifaddr);
        return env->NewStringUTF(ipv4Address.c_str());
    }

    freeifaddrs(ifaddr);
    return env->NewStringUTF("Error");
}