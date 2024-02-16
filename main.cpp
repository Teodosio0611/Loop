#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fnctl.h>

#include "event_loop.h"

using namespace Loop;

EventLoop el;

static int SetNonblocking(int fd) {
    int opts;
    if ((opts = fcntl(fd, F_GETFL)) != -1) {
        opts = opts | O_NONBLOCK;
        if(fcntl(fd, F_SETFL, opts) != -1) {
            return 0;
        }
    }
    return -1;
}

int ConnectTo(const char *host, short port, bool async) {
  int fd;
  sockaddr_in addr;

  if (host == NULL) return -1;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  addr.sin_family = PF_INET;
  addr.sin_port = htons(port);
  if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
    inet_aton("127.0.0.1", &addr.sin_addr);
  } else if (!strcmp(host, "any")) {
    addr.sin_addr.s_addr = INADDR_ANY;
  } else {
    inet_aton(host, &addr.sin_addr);
  }

  if (async) SetNonblocking(fd);

  if (connect(fd, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1) {
    if (errno != EINPROGRESS) {
      close(fd);
      return -1;
    }
  }

  return fd;
}

int BindTo(const char *host, short port) {
  int fd, on = 1;
  sockaddr_in addr;

  if (host == NULL) return -1;

  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    return -1;
  }

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

  memset(&addr, 0, sizeof(sockaddr_in));
  addr.sin_family = PF_INET;
  addr.sin_port = htons(port);
  if (host[0] == '\0' || strcmp(host, "localhost") == 0) {
    inet_aton("127.0.0.1", &addr.sin_addr);
  } else if (strcmp(host, "any") == 0) {
    addr.sin_addr.s_addr = INADDR_ANY;
  } else {
    if (inet_aton(host, &addr.sin_addr) == 0) return -1;
  }

  if (bind(fd, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1 || listen(fd, 10) == -1) {
    return -1;
  }

  return fd;
}

class BufferEvent : public BufferFileEvent {
 public:
  BufferEvent() {
    state_ = 0;
    Receive(buf_, 1);
  }

  void OnRecived(char *buffer, uint32_t len) {
    switch (state_) {
      case 0:
        len_ = buf_[0] - '0';
        Receive(buf_, len_);
        state_ = 1;
        break;
      case 1:
        Send(buf_, len_);
        state_ = 2;
        break;
      default:
        break;
    }
  }

  void OnSent(char *buffer, uint32_t len) {
    state_ = 0;
    Receive(buf_, 1);
  }

  void OnError() {
  }

 private:
  char buf_[1024];
  int len_;
  int state_;
};

class AcceptEvent: public BaseFileEvent {
 public:
  void OnEvents(uint32_t events) {
    if (events & BaseFileEvent::READ) {
      uint32_t size = 0;
      struct sockaddr_in addr;

      int fd = accept(file_, (struct sockaddr*)&addr, &size);
      BufferEvent *e = new BufferEvent();
      e->SetFile(fd);
      el.AddEvent(e);
    }

    if (events & BaseFileEvent::ERROR) {
      close(file_);
    }
  }
};

class Periodic : public PeriodicTimerEvent {
public:
  void OnTimer() {
    printf("Periodic timer\n");
  }
};

class Timer : public BaseTimerEvent {
 public:
  void OnEvents(uint32_t events) {
    printf("timer:%u\n", static_cast<uint32_t>(time(0)));
    timeval tv = el.Now();
    tv.tv_sec += 1;
    SetTimeout(tv);
    el.UpdateEvent(this);
    if (!p) return;
    if (p->IsRunning()) p->Stop();
    else p->Start();
  }

 public:
  Periodic *p;
};

int main(int argc, char **argv) {
  int fd;
  AcceptEvent e;

  e.SetEvents(BaseFileEvent::READ | BaseFileEvent::ERROR);

  fd = BindTo("0.0.0.0", 11112);
  if (fd == -1) {
    printf("binding address %s", strerror(errno));
    return -1;
  }

  e.SetFile(fd);

  el.AddEvent(&e);

  Periodic p;
  Timer t;
  t.p = &p;

  timeval tv;
  gettimeofday(&tv, NULL);
  tv.tv_sec += 3;
  t.SetTimeout(tv);

  el.AddEvent(&t);

  timeval tv2;
  tv2.tv_sec = 0;
  tv2.tv_usec = 500 * 1000;
  p.SetInterval(tv2);

  el.AddEvent(&p);

  p.Start();
  el.StartLoop();

  return 0;
}
