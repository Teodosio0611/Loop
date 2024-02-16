#include "event.h"
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

namespace Loop {
static timeval TimeAdd(timeval tv1, timeval tv2) {
    timeval t = tv1;
    t.tv_sec += tv2.tv_sec;
    t.tv_usec += tv2.tv_usec;
    t.tv_sec += t.tv_usec / 1000000;
    t.tv_usec %= 1000000;
    return t;
}

void BufferFileEvent::OnEvents(uint32_t events)
{
    if (events & BaseFileEvent::READ) {
        int len = read(file_, recvBuf_ + recvd_, totalRecv_ - recvd_);
        if (len <= 0) {
            OnError();
            return;
        }

        recvd_ += len;
        if (recvd_ == totalRecv_) {
            OnReceived(recvBuf_, recvd_);
        }
    }

    if (events & BaseFileEvent::WRITE) {
        int len = write(file_, sendBuf_ + sent_, totalSend_ - sent_);
        if (len < 0) {
            OnError();
            return;
        }

        sent_ += len;
        if (sent_ == totalSend_) {
            SetEvents(events_ & (~BaseFileEvent::WRITE));
            loop_->UpdateEvent(this);
            OnSent(sendBuf_, sent_);
        }
    }

    if (events & BaseFileEvent::ERROR) {
        OnError();
        return;
    }
}

void BufferFileEvent::Receive(char* buf, uint32_t len)
{
    recvBuf_ = buf;
    totalRecv_ = len;
    recvd_ = 0;
}

void BufferFileEvent::Send(char* buf, uint32_t len)
{
    sendBuf_ = buf;
    totalSend_ = len;
    sent_ = 0;
    if (!(events_ & BaseFileEvent::WRITE)) {
        SetEvents(events_ | BaseFileEvent::WRITE);
        loop_->UpdateEvent(this);
    }
}

void PeriodicTimerEvent::OnEvents(uint32_t events)
{
    OnTimer();
    timeout_ = TimeAdd(loop_->Now(), interval_);
    loop_->UpdateEvent(this);
}

void PeriodicTimerEvent::Start()
{
    if (loop_ == nullptr) {
        return;
    }
    running_ = true;
    timeval tv;
    gettimeofday(&tv, nullptr);
    SetTimeout(tv);
    loop_->UpdateEvent(this);
}

void PeriodicTimerEvent::Stop()
{
    if (loop_ == nullptr) {
        return;
    }
    running_ = false;
    loop_->UpdateEvent(this);
}
}