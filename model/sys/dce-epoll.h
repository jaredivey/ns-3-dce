/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Hajime Tazaki
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef DCE_EPOLL_H
#define DCE_EPOLL_H

#include <sys/epoll.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_epoll_create (int size);
int dce_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int dce_epoll_wait(int epfd, struct epoll_event *events,
                   int maxevents, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* DCE_EPOLL_H */
