#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "event.h"

namespace Loop {
class TimerManager;

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    int AddEvent(BaseFileEvent* event);
    int DeleteEvent(BaseFileEvent* event);
    int UpdateEvent(BaseFileEvent* event);

    int AddEvent(BaseTimerEvent* event);
    int DeleteEvent(BaseTimerEvent* event);
    int UpdateEvent(BaseTimerEvent* event);

    int AddEvent(BaseSignalEvent* event);
    int DeleteEvent(BaseSignalEvent* event);
    int UpdateEvent(BaseSignalEvent* event);
    
    int AddEvent(BufferFileEvent* event);
    int AddEvent(PeriodicTimerEvent* event);

    int ProcessEvents(int timeout);

    void StartLoop();
    void StopLoop();

    timeval Now() const
    {
        return now_;
    }

private:
    int CollectFileEvents(int timeout);
    int DoTimeout();

    int epFd_;
    epoll_event eventsList_[256];
    
    timeval now_;
    bool stop_;

    TimerManager* timerManager_;
};
}

#endif