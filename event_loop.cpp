#include "event_loop.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <set>
#include <map>

#include "timer_manager.h"

namespace Loop {

static int TimeDiff(timeval tv1, timeval tv2)
{
    return (tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000;
}

static timeval TimeAdd(timeval tv1, timeval tv2) {
    timeval t = tv1;
    t.tv_sec += tv2.tv_sec;
    t.tv_usec += tv2.tv_usec;
    t.tv_sec += t.tv_usec / 1000000;
    t.tv_usec %= 1000000;
    return t;
}

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

EventLoop::EventLoop()
{
    epFd_ = epoll_create(256);
    timerManager_ = new TimerManager();
}

EventLoop::~EventLoop()
{
    close(epFd_);
    delete timerManager_;
}

int EventLoop::CollectFileEvents(int timeout)
{
    return epoll_wait(epFd_, eventsList_, 256, timeout);
}

int EventLoop::DoTimeout()
{
    int n = 0;
    auto& timers = timerManager_->timers_;
    auto iter = timers.begin();
    while (iter != timers.end()) {
        auto tv = (*iter)->Time();
        if (TimeDiff(now_, tv) < 0) {
            break;
        }
        n++;
        BaseTimerEvent* event = *iter;
        timers.erase(iter);
        event->OnEvents(BaseTimerEvent::TIMER);
        iter = timers.begin();
    }
    return n;
}

int EventLoop::ProcessEvents(int timeout)
{
    int i, nt, n;
    n = CollectFileEvents(timeout);
    gettimeofday(&now_, NULL);
    nt = DoTimeout();
    for (int i = 0; i < n; i++) {
        BaseEvent* e = static_cast<BaseEvent*>(eventsList_[i].data.ptr);
        uint32_t events = 0;
        if (eventsList_[i].events & EPOLLIN) {
            events |= BaseFileEvent::READ;
        }
        if (eventsList_[i].events & EPOLLOUT) {
            events |= BaseFileEvent::WRITE;
        }
        if (eventsList_[i].events & (EPOLLHUP | EPOLLERR)) {
            events |= BaseFileEvent::ERROR;
        }
        e->OnEvents(events);
    }
}

void EventLoop::StopLoop()
{
    stop_ = true;
}

void EventLoop::StartLoop()
{
    stop_ = false;
    while (!stop_) {
        int timeout = 100;
        gettimeofday(&now_, NULL);
        if (!timerManager_->timers_.empty()) {
            auto iter = timerManager_->timers_.begin();
            auto tv = (*iter)->Time();
            int t = TimeDiff(now_, tv);
            if (t > 0 && timeout > t) {
                timeout = t;
            }

            ProcessEvents(timeout);
        }
    }
}

int EventLoop::AddEvent(BaseFileEvent* event)
{
    epoll_event ev;
    uint32_t events = event->events_;

    ev.events = 0;
    if (events & BaseFileEvent::READ) {
        ev.events |= EPOLLIN;
    }

    if (events & BaseFileEvent::WRITE) {
        ev.events |= EPOLLOUT;
    }

    if (events & BaseFileEvent::ERROR) {
        ev.events |= EPOLLHUP | EPOLLERR;
    }

    ev.data.fd = event->file_;
    ev.data.ptr = event;

    SetNonBlocking(event->file_);

    return epoll_ctl(epFd_, EPOLL_CTL_ADD, event->file_, &ev);
}

int EventLoop::UpdateEvent(BaseFileEvent* event)
{
    epoll_event ev;
    uint32_t events = event->events_;

    ev.events = 0;
    if (events & BaseFileEvent::READ) {
        ev.events |= EPOLLIN;
    }

    if (events & BaseFileEvent::WRITE) {
        ev.events |= EPOLLOUT;
    }

    if (events & BaseFileEvent::ERROR) {
        ev.events |= EPOLLHUP | EPOLLERR;
    }

    ev.data.fd = event->file_;
    ev.data.ptr = event;

    return epoll_ctl(epFd_, EPOLL_CTL_MOD, event->file_, &ev);
}

int EventLoop::DeleteEvent(BaseFileEvent* event)
{
    return epoll_ctl(epFd_, EPOLL_CTL_DEL, event->file_, nullptr);
}

int EventLoop::AddEvent(BaseTimerEvent* event)
{
    return timerManager_->AddEvent(event);
}

int EventLoop::UpdateEvent(BaseTimerEvent* event)
{
    return timerManager_->UpdateEvent(event);
}

int EventLoop::DeleteEvent(BaseTimerEvent* event)
{
    return timerManager_->DeleteEvent(event);
}

int EventLoop::AddEvent(BufferFileEvent* event)
{
    AddEvent(dynamic_cast<BaseFileEvent*>(event));
    event->loop_ = this;
    return 0;
}

int EventLoop::AddEvent(PeriodicTimerEvent* event)
{
    AddEvent(dynamic_cast<BaseTimerEvent*>(event));
    event->loop_ = this;
    return 0;
}
}