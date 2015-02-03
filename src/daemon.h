/**
  daemon.h

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

#ifndef DAEMON_H
#define DAEMON_H

#include "args.h"

/*
   return 0 if success and in child, will also redirect stdout and stderr
   not return if master
   return non-zero if error
*/
int daemon_start(const shadowvpn_args_t *args);

/*
   return 0 if success
   return non-zero if error
*/
int daemon_stop(const shadowvpn_args_t *args);

#endif
