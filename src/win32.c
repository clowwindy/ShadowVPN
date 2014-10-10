/**
  win32.c

  Copyright (c) 2014 clowwindy

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/
/* 
 * Windows TUN/TAP support from iodine
 * http://code.kryo.se/iodine/
 *
 * Copyright (c) 2006-2014 Erik Ekman <yarrick@kryo.se>,
 * 2006-2009 Bjorn Andersson <flex@kryo.se>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "win32.h"
#include "log.h"
#include <winioctl.h>

struct tun_data {
  HANDLE tun;
  int sock;
  struct sockaddr_storage addr;
  int addrlen;
};

#define TUN_READER_BUF_SIZE (64 * 1024)
#define TUN_NAME_BUF_SIZE 256
#define TUN_DELEGATE_ADDR "127.0.0.1"
#define TUN_DELEGATE_PORT 55151

#define TAP_CONTROL_CODE(request,method) CTL_CODE(FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)
#define TAP_IOCTL_CONFIG_TUN       TAP_CONTROL_CODE(10, METHOD_BUFFERED)
#define TAP_IOCTL_SET_MEDIA_STATUS TAP_CONTROL_CODE(6, METHOD_BUFFERED)

#define TAP_ADAPTER_KEY "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define NETWORK_KEY "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define TAP_DEVICE_SPACE "\\\\.\\Global\\"
#define TAP_VERSION_ID_0801 "tap0801"
#define TAP_VERSION_ID_0901 "tap0901"
#define KEY_COMPONENT_ID "ComponentId"
#define NET_CFG_INST_ID "NetCfgInstanceId"

#define IP_OPT_DONT_FRAG IP_DONTFRAGMENT
#define DONT_FRAG_VALUE 1

HANDLE dev_handle;
char *args_tun_ip = NULL;
int args_tun_mask = 24;

static struct tun_data data;
static char if_name[TUN_NAME_BUF_SIZE];

static void get_name(char *ifname, int namelen, char *dev_name);

static int get_addr(char *host, int port, int addr_family, int flags, struct sockaddr_storage *out) {
  struct addrinfo hints, *addr;
  int res;
  char portnum[8];

  memset(portnum, 0, sizeof(portnum));
  snprintf(portnum, sizeof(portnum) - 1, "%d", port);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = addr_family;
  hints.ai_flags = flags;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  res = getaddrinfo(host, portnum, &hints, &addr);
  if (res == 0) {
    int addrlen = addr->ai_addrlen;
    /* Grab first result */
    memcpy(out, addr->ai_addr, addr->ai_addrlen);
    freeaddrinfo(addr);
    return addrlen;
  }
  return res;
}

static int open_udp(struct sockaddr_storage *sockaddr, size_t sockaddr_len) {
  int flag = 1;
  int fd;

  if ((fd = socket(sockaddr->ss_family, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    err("socket");
  }

  flag = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*) &flag, sizeof(flag));

#ifdef IP_OPT_DONT_FRAG
  /* Set dont-fragment ip header flag */
  flag = DONT_FRAG_VALUE;
  setsockopt(fd, IPPROTO_IP, IP_OPT_DONT_FRAG, (const void*) &flag, sizeof(flag));
#endif

  if(bind(fd, (struct sockaddr*) sockaddr, sockaddr_len) < 0)
    err("bind");

  logf("opened IPv%d UDP socket\n", sockaddr->ss_family == AF_INET6 ? 6 : 4);

  return fd;
}

static int open_udp_from_host(char *host, int port, int addr_family, int flags) {
  struct sockaddr_storage addr;
  int addrlen;

  addrlen = get_addr(host, port, addr_family, flags, &addr);
  if (addrlen < 0)
    return addrlen;

  return open_udp(&addr, addrlen);
}

static void get_device(char *device, int device_len, const char *wanted_dev) {
  LONG status;
  HKEY adapter_key;
  int index;

  index = 0;
  status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TAP_ADAPTER_KEY, 0, KEY_READ, &adapter_key);

  if (status != ERROR_SUCCESS) {
    errf("Error opening registry key " TAP_ADAPTER_KEY );
    return;
  }

  while (TRUE) {
    char name[TUN_NAME_BUF_SIZE];
    char unit[TUN_NAME_BUF_SIZE];
    char component[TUN_NAME_BUF_SIZE];

    char cid_string[TUN_NAME_BUF_SIZE] = KEY_COMPONENT_ID;
    HKEY device_key;
    DWORD datatype;
    DWORD len;

    /* Iterate through all adapter of this kind */
    len = sizeof(name);
    status = RegEnumKeyEx(adapter_key, index, name, &len, NULL, NULL, NULL, NULL);
    if (status == ERROR_NO_MORE_ITEMS) {
      break;
    } else if (status != ERROR_SUCCESS) {
      errf("Error enumerating subkeys of registry key " TAP_ADAPTER_KEY );
      break;
    }

    snprintf(unit, sizeof(unit), TAP_ADAPTER_KEY "\\%s", name);
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, unit, 0, KEY_READ, &device_key);
    if (status != ERROR_SUCCESS) {
      errf("Error opening registry key %s", unit);
      goto next;
    }

    /* Check component id */
    len = sizeof(component);
    status = RegQueryValueEx(device_key, cid_string, NULL, &datatype, (LPBYTE)component, &len);
    if (status != ERROR_SUCCESS || datatype != REG_SZ) {
      goto next;
    }
    if (strncmp(TAP_VERSION_ID_0801, component, strlen(TAP_VERSION_ID_0801)) == 0 ||
      strncmp(TAP_VERSION_ID_0901, component, strlen(TAP_VERSION_ID_0901)) == 0) {
      /* We found a TAP32 device, get its NetCfgInstanceId */
      char iid_string[TUN_NAME_BUF_SIZE] = NET_CFG_INST_ID;

      status = RegQueryValueEx(device_key, iid_string, NULL, &datatype, (LPBYTE) device, (DWORD *) &device_len);
      if (status != ERROR_SUCCESS || datatype != REG_SZ) {
        errf("Error reading registry key %s\\%s on TAP device", unit, iid_string);
      } else {
        /* Done getting GUID of TAP device,
         * now check if the name is the requested one */
        if (wanted_dev) {
          char name[TUN_NAME_BUF_SIZE];
          get_name(name, sizeof(name), device);
          if (strncmp(name, wanted_dev, strlen(wanted_dev))) {
            /* Skip if name mismatch */
            goto next;
          }
        }
        /* Get the if name */
        get_name(if_name, sizeof(if_name), device);
        RegCloseKey(device_key);
        return;
      }
    }
next:
    RegCloseKey(device_key);
    index++;
  }
  RegCloseKey(adapter_key);
}

static void get_name(char *ifname, int namelen, char *dev_name) {
  char path[TUN_NAME_BUF_SIZE];
  char name_str[TUN_NAME_BUF_SIZE] = "Name";
  LONG status;
  HKEY conn_key;
  DWORD len;
  DWORD datatype;

  memset(ifname, 0, namelen);

  snprintf(path, sizeof(path), NETWORK_KEY "\\%s\\Connection", dev_name);
  status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &conn_key);
  if (status != ERROR_SUCCESS) {
    errf("could not look up name of interface %s: error opening key\n", dev_name);
    RegCloseKey(conn_key);
    return;
  }
  len = namelen;
  status = RegQueryValueEx(conn_key, name_str, NULL, &datatype, (LPBYTE)ifname, &len);
  if (status != ERROR_SUCCESS || datatype != REG_SZ) {
    errf("could not look up name of interface %s: error reading value\n", dev_name);
    RegCloseKey(conn_key);
    return;
  }
  RegCloseKey(conn_key);
}

static int inet_aton(const char *cp, struct in_addr *inp) {
  inp->s_addr = inet_addr(cp);
  return inp->s_addr != INADDR_ANY;
}

static int tun_setip(const char *ip, int netbits) {
  int netmask;
  struct in_addr net;
  int i;
  int r;
  DWORD status;
  DWORD ipdata[3];
  struct in_addr addr;
  DWORD len;

  if (ip == NULL) {
    errf("missing tunip: win32 needs to specify tun ip");
    return -1;
  }

  netmask = 0;
  for (i = 0; i < netbits; i++) {
    netmask = (netmask << 1) | 1;
  }
  netmask <<= (32 - netbits);
  net.s_addr = htonl(netmask);

  if (inet_addr(ip) == INADDR_NONE) {
    errf("invalid tun ip: %s", ip);
    return -1;
  }

  /* Set device as connected */
  logf("enabling interface '%s'", if_name);
  status = 1;
  r = DeviceIoControl(dev_handle, TAP_IOCTL_SET_MEDIA_STATUS, &status,
      sizeof(status), &status, sizeof(status), &len, NULL);
  if (!r) {
    errf("failed to enable interface");
    return -1;
  }

  if (inet_aton(ip, &addr)) {
    ipdata[0] = (DWORD) addr.s_addr;   /* local ip addr */
    ipdata[1] = net.s_addr & ipdata[0]; /* network addr */
    ipdata[2] = (DWORD) net.s_addr;    /* netmask */
  } else {
    return -1;
  }

  /* Tell ip/networkaddr/netmask to device for arp use */
  r = DeviceIoControl(dev_handle, TAP_IOCTL_CONFIG_TUN, &ipdata,
      sizeof(ipdata), &ipdata, sizeof(ipdata), &len, NULL);
  if (!r) {
    errf("failed to set interface in tun mode");
    return -1;
  }

  return 0;
}

DWORD WINAPI tun_reader(LPVOID arg) {
  struct tun_data *tun = arg;
  char buf[TUN_READER_BUF_SIZE];
  int len;
  int res;
  OVERLAPPED olpd;
  int sock;

  sock = open_udp_from_host(TUN_DELEGATE_ADDR, 0, AF_INET, 0);

  olpd.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  while(TRUE) {
    olpd.Offset = 0;
    olpd.OffsetHigh = 0;
    res = ReadFile(tun->tun, buf, sizeof(buf), (LPDWORD) &len, &olpd);
    if (!res) {
      WaitForSingleObject(olpd.hEvent, INFINITE);
      res = GetOverlappedResult(dev_handle, &olpd, (LPDWORD) &len, FALSE);
      res = sendto(sock, buf, len, 0, (struct sockaddr*) &(tun->addr),
        tun->addrlen);
    }
  }

  return 0;
}

int tun_open(const char *tun_device) {
  char adapter[TUN_NAME_BUF_SIZE];
  char tapfile[TUN_NAME_BUF_SIZE * 2];
  int tunfd;
  struct sockaddr_storage localsock;
  int localsock_len;

  memset(adapter, 0, sizeof(adapter));
  memset(if_name, 0, sizeof(if_name));
  get_device(adapter, sizeof(adapter), tun_device);

  if (strlen(adapter) == 0 || strlen(if_name) == 0) {
    if (tun_device) {
      errf("no TAP adapters found");
    } else {
      errf("no TAP adapters found: version 0801 and 0901 are supported");
    }
    return -1;
  }

  logf("opening device %s\n", if_name);
  snprintf(tapfile, sizeof(tapfile), "%s%s.tap", TAP_DEVICE_SPACE, adapter);
  dev_handle = CreateFile(tapfile, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, NULL);
  if (dev_handle == INVALID_HANDLE_VALUE) {
    errf("can not open device");
    return -1;
  }
  if (0 != tun_setip(args_tun_ip, args_tun_mask)) {
    errf("can not connect device");
    return -1;
  }

  /* Use a UDP connection to forward packets from tun,
   * so we can still use select() in main code.
   * A thread does blocking reads on tun device and
   * sends data as udp to this socket */

  localsock_len = get_addr(TUN_DELEGATE_ADDR, TUN_DELEGATE_PORT, AF_INET, 0, &localsock);
  tunfd = open_udp(&localsock, localsock_len);

  data.tun = dev_handle;
  memcpy(&(data.addr), &localsock, localsock_len);
  data.addrlen = localsock_len;
  CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) tun_reader, &data, 0, NULL);

  return tunfd;
}

int setenv(const char *name, const char *value, int overwrite) {
  char envbuf[TUN_NAME_BUF_SIZE];
  snprintf(envbuf, sizeof(envbuf), "%s=%s", name, value);
  return _putenv(envbuf);
}
