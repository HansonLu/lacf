#include "log.h" 
#include <iostream> 
#include <sstream>
#include <assert.h> 
#include "ace/OS.h"
#include "lock_guard.h"

using namespace std; 

Logger::Logger()
    : flag_(STD), 
    priority_(LL_TRACE),
    log_file_(0),
    log_file_max_size_(0),
    file_stream_(0)
{

}

Logger::~Logger() 
{
    if (file_stream_)
    {
        delete file_stream_; 
        file_stream_ = 0;
    }

}
void Logger::set_priority( LogPriority pri)
{
    priority_ = pri;
}

bool Logger::is_pri_enabled(LogPriority pri) const 
{
    return (pri <= priority_);
}

int Logger::init (int flag, const char * log_file , size_t max_filesize)
{
    flag_ = flag;

    if (flag_ & FILESTREAM) 
    {
        assert(log_file); 
        log_file_ = log_file; 
        log_file_max_size_ = max_filesize;
        assert(file_stream_ == 0);
        return open_log_file();
    }

    return 0;
}

int Logger::open_log_file() 
{
    if (file_stream_) 
    {
        delete file_stream_; 
        file_stream_ = 0;
    }

    file_stream_ = new std::ofstream (log_file_, ios::out|ios::app);
    assert(file_stream_); 

    if (!file_stream_->is_open()) 
    {
        cerr << " open log file " << log_file_ << " failed " << endl;
        return -1;
    }

    return 0;
}

static const std::string get_time_str() 
{
    ACE_Time_Value t =  ACE_OS::gettimeofday();
    time_t sec = t.sec();

    struct tm res; 
    ACE_OS::localtime_r(&sec, &res);
    int msec = (t.usec() / 1000); 

    char buf [50]; 
    ACE_OS::snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d.%03d",
        res.tm_year+1900,
        res.tm_mon +1 , 
        res.tm_mday, 
        res.tm_hour,
        res.tm_min,
        res.tm_sec,
        msec);
    return buf;
}


void Logger::backup_file()
{
    delete file_stream_; 
    file_stream_ = 0;

    for (int i = MAX_ROLL_FILE; i > 0; i --) 
    {
        char last_file[255]; 
        char prev_file[255]; 

        ACE_OS::snprintf(last_file, sizeof(last_file), "%s.%d", log_file_, i);
        if ( i -1  > 0)
        {
            ACE_OS::snprintf(prev_file, sizeof(prev_file), "%s.%d", log_file_, i -1);
        }
        else
        {
            ACE_OS::snprintf(prev_file, sizeof(prev_file), "%s", log_file_);
        }

        ACE_OS::unlink(last_file);
        ACE_OS::rename(prev_file ,last_file);
    }

    (void) open_log_file();
}

int Logger::log(LogPriority pri, const std::string & content)
{
    LockGuard guard(mutex_);

    if ( !is_pri_enabled(pri)) 
    {
        return 0;
    }

    string log_rec = get_log_rec(pri, content) ;

    if (flag_ | STD) 
    {
        cout << log_rec;
    }

    if ( flag_ & FILESTREAM) 
    {
        assert(file_stream_); 
        *file_stream_ << log_rec;
        if (file_stream_->tellp() > log_file_max_size_) 
        {
            backup_file();
        }

    }

    return 0;
}

const std::string Logger::get_log_rec(LogPriority pri,  const std::string & content) 
{
    ostringstream ss; 
    ss << get_time_str() << " [" <<  log_level(pri) << "] " << content;
    return ss.str();
}

const char * Logger::log_level(LogPriority pri)
{
    switch ( pri)
    {
    case LL_TRACE:
        return "TRACE";
    case LL_DEBUG:
        return "DEBUG";
    case LL_INFO:
        return "INFO";
    case LL_WARN:
        return "WARN";
    case LL_ERROR:
        return "ERROR";
    case LL_EXCEPTION:
        return "EXCEPTION";
    default:
        break;
    }

    return "unknown";
}
