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
#include "sys/dce-epoll.h"
#include "utils.h"
#include "process.h"
#include "linux-epoll-fd.h"
#include "dce-poll.h"
#include "ns3/log.h"
#include <errno.h>
#include "file-usage.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceEpoll");

int dce_epoll_create (int size)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << size);
  NS_ASSERT (current != 0);

  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      return -1;
    }

  UnixFd *unixFd = new LinuxEpollFd (size);
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);
  return fd;
}

int
dce_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << op << epfd << fd << event);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  UnixFd *unixFd = current->process->openFiles[epfd]->GetFileInc ();
  LinuxEpollFd *epollFd = (LinuxEpollFd *)unixFd;
  struct epoll_event *ev;

  switch (op)
    {
    case EPOLL_CTL_ADD:
      ev = (struct epoll_event *)malloc (sizeof (struct epoll_event));
      memcpy (ev, event, sizeof (struct epoll_event));
      epollFd->evs[fd] = ev;
      break;
    case EPOLL_CTL_MOD:
      ev = epollFd->evs[fd];
      memcpy (ev, event, sizeof (struct epoll_event));
      break;
    case EPOLL_CTL_DEL:
      ev = epollFd->evs[fd];
      epollFd->evs.erase (fd);
      free (ev);
      break;
    default:
      break;
    }

  FdDecUsage (epfd);
  return 0;
}

int
dce_epoll_wait(int epfd, struct epoll_event *events,
               int maxevents, int timeout)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () <<  epfd << events 
                   << maxevents << timeout);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  UnixFd *unixFd = current->process->openFiles[epfd]->GetFileInc ();
  LinuxEpollFd *epollFd = (LinuxEpollFd *)unixFd;

  struct pollfd pollFd[maxevents];
  int j = 0;

  for (std::map<int, struct epoll_event *>::iterator i = epollFd->evs.begin (); 
       i != epollFd->evs.end (); ++i)
    {
      struct epoll_event *ev = (*i).second;
      int pevent = 0;
      if (ev->events & EPOLLIN)
        {
          pevent |= POLLIN;
        }
      if (ev->events & EPOLLOUT)
        {
          pevent |= POLLOUT;
        }

      NS_LOG_INFO ("set poll " << (*i).first << " event " <<pevent);
      pollFd[j].events = pevent;
      pollFd[j].fd = (*i).first;
      pollFd[j++].revents = 0;
    }

  int pollRet = dce_poll (pollFd, j, timeout);
  if (pollRet > 0)
    {
      pollRet = 0;
      for (uint32_t i = 0; i < maxevents; i++)
        {
          int fd = pollFd[i].fd;

          if ((POLLIN & pollFd[i].revents) || (POLLHUP & pollFd[i].revents)
              || (POLLERR & pollFd[i].revents))
            {
              memcpy (events, epollFd->evs[fd], sizeof (struct epoll_event));
              NS_LOG_INFO ("epoll woke up for read with " << fd);
              pollRet++;
              events++;
            }
          if (POLLOUT & pollFd[i].revents)
            {
              memcpy (events, epollFd->evs[fd], sizeof (struct epoll_event));
              // *events = *epollFd->evs[fd];
              // events->data.fd = fd;
              NS_LOG_INFO ("epoll woke up for write with " << fd);
              pollRet++;
              events++;
            }
          if (POLLPRI & pollFd[i].revents)
            {
              memcpy (events, epollFd->evs[fd], sizeof (struct epoll_event));
              NS_LOG_INFO ("epoll woke up for other with " << fd);
              // *events = *epollFd->evs[fd];
              // events->data.fd = fd;
              pollRet++;
              events++;
            }
        }
    }

  return pollRet;
}
       
