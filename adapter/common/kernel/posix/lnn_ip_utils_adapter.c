/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lnn_ip_utils_adapter.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#include "comm_log.h"

static int32_t GetNetworkIfIp(int32_t fd, struct ifreq *req, char *ip, char *netmask, uint32_t len)
{
    if (ioctl(fd, SIOCGIFFLAGS, (char *)req) < 0) {
        return SOFTBUS_NETWORK_IOCTL_FAIL;
    }
    if (!((uint16_t)req->ifr_flags & IFF_UP)) {
        return SOFTBUS_NETWORK_IFF_NOT_UP;
    }

    /* get IP of this interface */
    if (ioctl(fd, SIOCGIFADDR, (char *)req) < 0) {
        return SOFTBUS_NETWORK_IOCTL_FAIL;
    }
    struct sockaddr_in *sockAddr = (struct sockaddr_in *)&(req->ifr_addr);
    if (inet_ntop(sockAddr->sin_family, &sockAddr->sin_addr, ip, len) == NULL) {
        COMM_LOGE(COMM_ADAPTER, "convert ip addr to string failed");
        return SOFTBUS_NETWORK_INET_NTOP_FAIL;
    }

    /* get netmask of this interface */
    if (netmask != NULL) {
        if (ioctl(fd, SIOCGIFNETMASK, (char *)req) < 0) {
            COMM_LOGE(COMM_ADAPTER, "ioctl SIOCGIFNETMASK fail, errno=%{public}d", errno);
            return SOFTBUS_NETWORK_IOCTL_FAIL;
        }
        sockAddr = (struct sockaddr_in *)&(req->ifr_netmask);
        if (inet_ntop(sockAddr->sin_family, &sockAddr->sin_addr, netmask, len) == NULL) {
            COMM_LOGE(COMM_ADAPTER, "convert netmask addr to string failed");
            return SOFTBUS_NETWORK_INET_NTOP_FAIL;
        }
    }
    return SOFTBUS_OK;
}

int32_t GetNetworkIpByIfName(const char *ifName, char *ip, char *netmask, uint32_t len)
{
    if (ifName == NULL || ip == NULL) {
        COMM_LOGE(COMM_ADAPTER, "ifName or ip buffer is NULL!");
        return SOFTBUS_INVALID_PARAM;
    }
    int32_t fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        COMM_LOGE(COMM_ADAPTER, "open socket failed");
        return SOFTBUS_NETWORK_OPEN_SOCKET_FAIL;
    }
    struct ifreq ifr;
    if (strncpy_s(ifr.ifr_name, sizeof(ifr.ifr_name), ifName, strlen(ifName)) != EOK) {
        COMM_LOGE(COMM_ADAPTER, "copy netIfName fail. netIfName=%{public}s", ifName);
        close(fd);
        return SOFTBUS_STRCPY_ERR;
    }
    int32_t ret = GetNetworkIfIp(fd, &ifr, ip, netmask, len);
    if (ret != SOFTBUS_OK) {
        close(fd);
        return ret;
    }
    close(fd);
    return SOFTBUS_OK;
}

int32_t GetNetworkIpv6ByIfName(const char *ifName, char *ip, uint32_t len)
{
    if (ifName == NULL || ip == NULL) {
        COMM_LOGE(COMM_ADAPTER, "ifName or ip buffer is NULL!");
        return SOFTBUS_INVALID_PARAM;
    }
    struct ifaddrs *allAddr = NULL;
    if (getifaddrs(&allAddr) == -1) {
        COMM_LOGE(COMM_ADAPTER, "getifaddrs fail!");
        return SOFTBUS_NETWORK_GET_IP_ADDR_FAILED;
    }
    for (struct ifaddrs *ifa = allAddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET6 || ifa->ifa_netmask == NULL ||
            strcmp(ifa->ifa_name, ifName) != 0) {
            continue;
        }
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *)(ifa->ifa_addr);
        if (inet_ntop(AF_INET6, &addr->sin6_addr.s6_addr, ip, len) == NULL) {
            COMM_LOGE(COMM_ADAPTER, "convert ip addr to string failed");
            freeifaddrs(allAddr);
            return SOFTBUS_NETWORK_GET_IP_ADDR_FAILED;
        }
        freeifaddrs(allAddr);
        return SOFTBUS_OK;
    }
    freeifaddrs(allAddr);
    COMM_LOGE(COMM_ADAPTER, "not found ifname %{public}s ip", ifName);
    return SOFTBUS_NETWORK_GET_IP_ADDR_FAILED;
}

bool GetLinkUpStateByIfName(const char *ifName)
{
    if (ifName == NULL) {
        COMM_LOGE(COMM_ADAPTER, "ifName buffer is NULL!");
        return false;
    }
    int32_t fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        COMM_LOGE(COMM_ADAPTER, "open socket failed");
        return false;
    }
    struct ifreq ifr;
    if (strncpy_s(ifr.ifr_name, sizeof(ifr.ifr_name), ifName, strlen(ifName)) != EOK) {
        COMM_LOGE(COMM_ADAPTER, "copy netIfName fail. netIfName=%{public}s", ifName);
        close(fd);
        return false;
    }
    if (ioctl(fd, SIOCGIFFLAGS, (char *)&ifr) < 0) {
        COMM_LOGE(COMM_ADAPTER, "open socket failed");
        close(fd);
        return false;
    }
    if (!((uint16_t)ifr.ifr_flags & IFF_UP)) {
        COMM_LOGE(COMM_ADAPTER, "ifname flag is not up");
        close(fd);
        return false;
    }
    close(fd);
    return true;
}
