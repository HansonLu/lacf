#ifndef LACF_TIMER_TASK_H 
#define LACF_TIMER_TASK_H

#include "ace/Timer_Queue_Adapters.h"
#include "ace/Timer_Heap.h"

class TimerTask 
{
public : 
    typedef ACE_Thread_Timer_Queue_Adapter<ACE_Timer_Heap> ActiveTimer;
  
    static TimerTask * instance() 
    {
        static TimerTask instance_; 
        return &instance_;
    }

    int open () 
    {
        return timer_.activate();
    }

    long schedule (ACE_Event_Handler * handler, const void *act,
        const ACE_Time_Value &future_time,
        const ACE_Time_Value &interval = ACE_Time_Value::zero)
    {
        return timer_.schedule(handler, act, future_time, interval);
    }

    void cancel ( long timerid , const void  ** act) 
    {
        timer_.cancel(timerid, act);
    }

private: 

    ActiveTimer  timer_;
};


#endif
