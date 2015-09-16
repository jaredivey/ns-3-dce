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

#include "linux-epoll-fd.h"
#include "utils.h"
#include "process.h"
#include "dce-manager.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "task-manager.h"
#include <errno.h>
#include <sys/mman.h>
#include <poll.h>

NS_LOG_COMPONENT_DEFINE ("LinuxEpollFd");

namespace ns3 {

LinuxEpollFd::LinuxEpollFd (int size)
  : m_waiter (0)
{
  //  std::map <int, struct epoll_event *> epollEvs;
}

int
LinuxEpollFd::Close (void)
{
  if (m_waiter != 0)
    {
      m_waiter->process->manager->Wakeup (m_waiter);
    }
  return 0;
}

ssize_t
LinuxEpollFd::Write (const void *buf, size_t count)
{
  NS_LOG_FUNCTION (this << buf << count);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}
ssize_t
LinuxEpollFd::Read (void *buf, size_t count)
{
  Thread *current = Current ();
  if (count < 8)
    {
      current->err = EINVAL;
      return -1;
    }
  m_waiter = current;
  current->process->manager->Wait ();
  m_waiter = 0;

  return 0;
}
ssize_t
LinuxEpollFd::Recvmsg (struct msghdr *msg, int flags)
{
  NS_LOG_FUNCTION (this << msg << flags);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
ssize_t
LinuxEpollFd::Sendmsg (const struct msghdr *msg, int flags)
{
  NS_LOG_FUNCTION (this << msg << flags);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
bool
LinuxEpollFd::Isatty (void) const
{
  return false;
}
int
LinuxEpollFd::Setsockopt (int level, int optname,
                         const void *optval, socklen_t optlen)
{
  NS_LOG_FUNCTION (this << level << optname << optval << optlen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
LinuxEpollFd::Getsockopt (int level, int optname,
                         void *optval, socklen_t *optlen)
{
  NS_LOG_FUNCTION (this << level << optname << optval << optlen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
LinuxEpollFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  NS_LOG_FUNCTION (this << name << namelen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
LinuxEpollFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  NS_LOG_FUNCTION (this << name << namelen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
LinuxEpollFd::Ioctl (unsigned long request, char *argp)
{
  //XXX
  return -1;
}
int
LinuxEpollFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  NS_LOG_FUNCTION (this << my_addr << addrlen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
LinuxEpollFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  NS_LOG_FUNCTION (this << my_addr << addrlen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
LinuxEpollFd::Listen (int backlog)
{
  NS_LOG_FUNCTION (this << backlog);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
LinuxEpollFd::Shutdown (int how)
{
  NS_LOG_FUNCTION (this << how);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
LinuxEpollFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  NS_LOG_FUNCTION (this << my_addr << addrlen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
void *
LinuxEpollFd::Mmap (void *start, size_t length, int prot, int flags, off64_t offset)
{
  NS_LOG_FUNCTION (this << start << length << prot << flags << offset);
  Thread *current = Current ();
  current->err = EINVAL;
  return MAP_FAILED;
}
off64_t
LinuxEpollFd::Lseek (off64_t offset, int whence)
{
  NS_LOG_FUNCTION (this << offset << whence);
  Thread *current = Current ();
  current->err = ESPIPE;
  return -1;
}
int
LinuxEpollFd::Fxstat (int ver, struct ::stat *buf)
{
  NS_LOG_FUNCTION (this << buf);
  //XXX: I do not know what I should write here.
  // but this call is expected to succeed by the kernel.
  return 0;
}
int
LinuxEpollFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  NS_LOG_FUNCTION (this << buf);
  //XXX: I do not know what I should write here.
  // but this call is expected to succeed by the kernel.
  return 0;
}
int
LinuxEpollFd::Fcntl (int cmd, unsigned long arg)
{
  // XXX: this really needs to be fixed
  return 0;
}
int
LinuxEpollFd::Settime (int flags,
                      const struct itimerspec *new_value,
                      struct itimerspec *old_value)
{
  NS_LOG_FUNCTION (this << Current () << flags << new_value << old_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}
int
LinuxEpollFd::Gettime (struct itimerspec *cur_value) const
{
  NS_LOG_FUNCTION (this << Current () << cur_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}

bool
LinuxEpollFd::CanRecv (void) const
{
  // XXX ?
  return false;
}
bool
LinuxEpollFd::CanSend (void) const
{
  return false;
}
bool
LinuxEpollFd::HangupReceived (void) const
{
  return false;
}
int
LinuxEpollFd::Poll (PollTable* ptable)
{
  int ret = 0;

  if (CanRecv ())
    {
      ret |= POLLIN;
    }
  if (CanSend ())
    {
      ret |= POLLOUT;
    }
  if (HangupReceived ())
    {
      ret |= POLLHUP;
    }

  if (ptable)
    {
      ptable->PollWait (this);
    }

  return ret;
}

int
LinuxEpollFd::Ftruncate (off_t length)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (this << current << length);

  current->err = EINVAL;
  return -1;
}

int
LinuxEpollFd::Fsync (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EBADF;
  return -1;
}

} // namespace ns3
