/**
  win32.h

  Copyright (C) 2015 clowwindy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef WIN32_H
#define WIN32_H

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0501

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define bzero(...) ZeroMemory(__VA_ARGS__)
#define TUN_DELEGATE_ADDR "127.0.0.1"
#define TUN_DELEGATE_PORT 55151

extern HANDLE dev_handle;

int tun_open(const char *tun_device, const char *tun_ip, int tun_mask, int tun_port);
int setenv(const char *name, const char *value, int overwrite);
int disable_reset_report(int fd);

#endif
