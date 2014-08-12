#include "msg_task.h" 
#include <assert.h> 
#include "timer_task.h" 
#include "log.h"

MsgTask::MsgTask()
    : stop_(false)
{

}

int MsgTask::open (void*) 
{
    int ret = activate();

    if (ret == -1) 
    {
        return -1;
    }

    return 0;
}


int MsgTask::put_msg(const Message & msg, ACE_Time_Value *tv) 
{
    Message * m = new Message; 
    * m = msg;
    if ( this->msg_queue_->enqueue_tail (m, tv) == -1) 
    {
        delete m;
        return -1;
    }

    return 0;
}

int MsgTask::get_msg (Message * msg)
{
    static ACE_Time_Value tv(1) ;

    int rc =  this->msg_queue_->dequeue_head( msg, &tv);

    if (rc < 0 ) 
    {
        if (errno != EWOULDBLOCK)
        {
            LOG_ERROR_OS("MsgTask " << this << " get_msg dequeue msg failed. errno = " << errno );
            //stop_ = true;
        }

        return -1;
    }
    else
    {
        assert ( msg);
    }

    return 0;
}


int MsgTask::svc (void)
{
    Message  *  m  =0 ; 
    while (!stop_) 
    {
        if ( get_msg(m) == 0)
        {
            assert(m);
            (void) handle_msg(*m);
            delete m;
        }
    }

    return 0;
}

long MsgTask::register_timer(const Message & msg, const ACE_Time_Value &future_time,
    const ACE_Time_Value &interval)

{
    MsgTimerAct * act = new MsgTimerAct; 
    assert(act);
    act->msg_ = msg;
    act->repeat = (interval !=  ACE_Time_Value::zero);

    long timerid = TimerTask::instance()->schedule(this, act, future_time, interval) ;

    if (timerid < 0) 
    {
        delete act; 
        act = 0;
    }

    return timerid;
}

void MsgTask::cancel_timer(long timerid)
{
    const void * act = 0;

    TimerTask::instance()->cancel(timerid, & act);

    if (act) 
    {
        delete act;
        act = 0;
    }

}


int MsgTask::handle_timeout (const ACE_Time_Value &tv,  const void *act)
{
    assert(act); 

    const MsgTimerAct * mact = static_cast<const MsgTimerAct *>( act) ;
    if (!this->stop_)
    {
        //it could  block if msg queue is full
        (void) this->put_msg(mact->msg_);
    }

    if ( !mact->repeat )
    {
        delete mact;
        mact = 0;
        act = 0;
    }


    return 0;
}
