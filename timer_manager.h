#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <set>
#include "event.h"

namespace Loop {
class TimerManager {
public:
    int AddEvent(BaseTimerEvent* event);
    int DeleteEvent(BaseTimerEvent* event);
    int UpdateEvent(BaseTimerEvent* event);

    friend class EventLoop;
private:
    class Compare {
    public:
        bool operator()(const BaseTimerEvent* a, const BaseTimerEvent* b) const
        {
            timeval t1 = a->GetTimeout();
            timeval t2 = b->GetTimeout();
            return (t1.tv_sec < t2.tv_sec) || (t1.tv_sec == t2.tv_sec && t1.tv_usec < t2.tv_usec);
        }
    };

    using TimerSet = std::set<const BaseTimerEvent*, Compare>;
    TimerSet timers_;
};
}
#endif