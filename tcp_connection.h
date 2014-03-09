#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <sstream> 

#include "ace/Handle_Set.h"
#include "ace/SOCK_Stream.h"

#include "buffered_stream.h"
#include "log.h"

class TcpConnection
{ 
public:
   /// enum { RECV_BUF_SIZE = 1024*1024 }; 

    explicit TcpConnection ( ACE_HANDLE handle) 
        :handle_(handle)
    {
        ///recv_buff_ = new char [RECV_BUF_SIZE];
    }

    ~TcpConnection()
    {
       // if (recv_buff_)
       // {
       //     delete recv_buff_;
       //     recv_buff_ = 0;
       // }
    }

    const std::string name() 
    {
        std::ostringstream ss ; 
        ss << "[Tcp Connection] ["<< handle_ << "]"; 
        return ss.str();
    }

    int send (const char * send_buff, size_t buff_size)
    {
        ACE_SOCK_Stream stream (handle_);

        int rc = stream.send(send_buff , buff_size);
        if (rc < 0 ) 
        {
            if ( errno != EWOULDBLOCK)
            {
                LOG_ERROR_OS (name() << "send failed. errno = " << errno );
                return -1;
            }
            else
            {
                return 0;
            }
        }
        else if (rc == 0 ) 
        {
            LOG_DEBUG_OS (name() <<  " send failed. rc == 0" );
            return -1;
        }
        else  
        {
            return rc;
        }

    }

    int recv (char * recv_buff, size_t buff_size)
    {
        assert(recv_buff);
        assert(buff_size);

        ACE_SOCK_Stream stream (handle_);

        int rc = stream.recv (recv_buff, buff_size);
        if (rc < 0) 
        {
            if ( errno != EWOULDBLOCK)
            {
                LOG_ERROR_OS( name() <<  "recv failed. errno = " << errno);
                return -1;
            }
            else
            {
                return 0;
            }
        }
        else if (rc == 0 ) 
        {
            LOG_ERROR_OS( name() << "recv rc == 0" );
            return -1;
        } 
        else if (rc > buff_size)
        {
            LOG_ERROR_OS( name() <<  " recv return invalid len. len = " << rc ); 
            return -1;
        }

        return rc;
    }

private: 
  ///  char  * recv_buff_;

  ///  BufferedStreamRefPtr read_stream_; 

  ///  BufferedStreamRefPtr write_stream_;

    ACE_HANDLE handle_;
};


#endif