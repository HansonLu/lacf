#ifndef LACF_LOCK_GUARD_H
#define LACF_LOCK_GUARD_H 
#include "ace/Synch.h"

#include "ace/Guard_T.h"


typedef ACE_Guard<ACE_Recursive_Thread_Mutex> LockGuard; 

#endif 