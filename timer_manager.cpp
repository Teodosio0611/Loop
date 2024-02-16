#include "timer_manager.h"

namespace Loop {
int TimerManager::AddEvent(BaseTimerEvent* event)
{
    return !timers_.insert(event).second;
}

int TimerManager::DeleteEvent(BaseTimerEvent* event)
{
    return timers_.erase(event) != 1;
}

int TimerManager::UpdateEvent(BaseTimerEvent* event)
{
    timers_.erase(event);
    return !timers_.insert(event).second;
}
}