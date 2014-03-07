#ifndef LACF_LOG_H 
#define LACF_LOG_H 

#include <string> 
#include <fstream> 
#include <sstream>

#include "ace/Synch.h"

enum LogPriority 
{
    LL_TRACE = 1, 
    LL_DEBUG = 2,
    LL_INFO =  4,
    LL_WARN =  010,
    LL_ERROR = 020,
    LL_EXCEPTION = 040
};

class Logger 
{ 
public: 
    enum { STD = 1, FILESTREAM = 2} ;

    enum {MAX_ROLL_FILE = 10};

    
    Logger () ;
    ~ Logger ()  ;

    static Logger * instance() 
    {
        static Logger instance_; 
        return & instance_;
    }

    int init (int flag = STD, const char * log_file = 0, size_t max_filesize = 5*1024*1024); 

    void set_priority( LogPriority pri);
    
    bool is_pri_enabled(LogPriority pri) const ;

    int log(LogPriority pri, const std::string & content);

private:

    void backup_file();
  
    int  open_log_file() ;

    static const std::string get_log_rec(LogPriority pri,  const std::string & content);
    
    static const char * log_level(LogPriority pri);

    long flag_;
   
    long long priority_;

    const char *  log_file_;

    size_t  log_file_max_size_;

    std::ofstream * file_stream_; 

    ACE_Recursive_Thread_Mutex mutex_;
};

#define LOG_OS(L, X) \
    do{ \
      if (Logger::instance()->is_pri_enabled(L)) \
      {  \
           std::ostringstream ss;  \
           ss << X << endl;\
           Logger::instance()->log(L, ss.str()); \
       }\
  }while(0);

#endif 

