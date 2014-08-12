#ifndef LACF_MSG_TASK_H 
#define LACF_MSG_TASK_H 

#include "ace/Task_Ex_T.h"

struct Message 
{
    int id_;
    int lp_; 
    int rp_;
    void * data;
};

class MsgTask; 

struct MsgTimerAct
{
    Message msg_;

    bool repeat;
};


class MsgTask : public ACE_Task_Ex<ACE_MT_SYNCH, Message>
{
public: 
    MsgTask(); 

    virtual int open (void * = 0 );

    virtual int svc (void);

    int put_msg(const Message & msg, ACE_Time_Value *tv =0 ) ;

    void shutdown () { stop_ = true;};

    virtual int handle_msg(const Message & msg) = 0; 

    long register_timer(const Message & msg, const ACE_Time_Value &future_time,
        const ACE_Time_Value &interval = ACE_Time_Value::zero);


    void cancel_timer(long timerid);

    int handle_timeout (const ACE_Time_Value &tv, 
        const void *act); 

protected: 
    int get_msg (Message* m);

    bool stop_;
};




#endif 

