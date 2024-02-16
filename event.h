#ifndef EVENT_H
#define EVENT_H

#include <iostream>

namespace Loop {
class EventLoop;

class BaseEvent {
public:
    BaseEvent(uint32_t events = 0) : events_(events) {}
    virtual ~BaseEvent() = default;
    
    virtual void OnEvents(uint32_t events) = 0;
    
    void SetEvents(uint32_t events)
    {
        events_ = events;
    }

    uint32_t GetEvents()
    {
        return events_;
    }

    static const uint32_t NONE = 0;
    static const uint32_t ONESHOT = 1 << 30;
    static const uint32_t TIME_OUT = 1 << 31;

protected:
    uint32_t events_;
};

class BaseFileEvent : public BaseEvent {
public:
    explicit BaseFileEvent(uint32_t events = BaseEvent::NONE) : BaseEvent(events) {}
    virtual ~BaseFileEvent() = default;

    virtual void OnEvents(uint32_t events) = 0;

    void SetFile(int fd)
    {
        file_ = fd;
    }

    int GetFile()
    {
        return file_;
    }

    static const uint32_t READ = 1 << 0;
    static const uint32_t WRITE = 1 << 1;
    static const uint32_t ERROR = 1 << 2;

    friend class EventLoop;
protected:
    int file_;
};

class BufferFileEvent : public BaseFileEvent {
public:
    explicit BufferFileEvent() : BaseFileEvent(BaseFileEvent::READ | BaseFileEvent::ERROR) {}
    virtual ~BufferFileEvent() = default;

    void Receive(char* buffer, uint32_t len);
    void Send(char* buffer, uint32_t len);

    virtual void OnReceived(char* buffer, uint32_t len) {}
    virtual void OnSent(const char* buffer, uint32_t len) {}
    virtual void OnError() {}

    void OnEvents(uint32_t events) override;

    friend class EventLoop;
private:
    char* recvBuf_;
    uint32_t totalRecv_;
    uint32_t recvd_;

    char* sendBuf_;
    uint32_t totalSend_;
    uint32_t sent_;

    EventLoop* loop_;
};

class BaseSignalEvent : public BaseEvent {
public:
    explicit BaseSignalEvent(uint32_t events = BaseEvent::NONE) : BaseEvent(events) {}
    virtual ~BaseSignalEvent() = default;

    static const uint32_t INT = 1 << 0;
    static const uint32_t PIPE = 1 << 1;
    static const uint32_t TERM = 1 << 2;
};

class BaseTimerEvent : public BaseEvent {
public:
    explicit BaseTimerEvent(uint32_t events = BaseEvent::NONE) : BaseEvent(events) {}
    virtual ~BaseTimerEvent() = default;

    void SetTimeout(timeval tv)
    {
        timeout_ = tv;
    }

    timeval GetTimeout() const
    {
        return timeout_;
    }

    static const uint32_t TIMER = 1 << 0;

protected:
    timeval timeout_;
};

class PeriodicTimerEvent : public BaseTimerEvent {
public:
    PeriodicTimerEvent() : BaseTimerEvent(), loop_(nullptr) {}
    explicit PeriodicTimerEvent(timeval interval) : PeriodicTimerEvent()
    {
        interval_ = interval;
    }

    virtual ~PeriodicTimerEvent() = default;

    void SetInterval(timeval tv)
    {
        interval_ = tv;
    }

    void Start();
    void Stop();

    bool IsRunning()
    {
        return running_;
    }

    virtual void OnTimer() = 0;
    void OnEvents(uint32_t events) override;

    friend class EventLoop;

private:
    timeval interval_;
    bool running_;

    EventLoop* loop_;
};
}
#endif