#ifndef TCP_CHANNEL_H
#define TCP_CHANNEL_H

#include <sstream> 

#include "ace/Handle_Set.h"
#include "ace/SOCK_Stream.h"

#include "buffered_stream.h"
#include "log.h"

class TcpChannel
{ 
public:
    explicit TcpChannel ( ACE_HANDLE handle) 
    {
        sock_stream_ = new ACE_SOCK_Stream(handle);
        read_stream_.reset(new CBufferredStream());
        write_stream_.reset(new CBufferredStream());
    }
   
    ~TcpChannel()
    {
        close();
    }

    const std::string name() 
    {
        std::ostringstream ss ;

        assert(sock_stream_);
        ss << "[Tcp channel] [" << sock_stream_->get_handle()  << "]"; 
        return ss.str();
    }

    BufferedStreamRefPtr write_stream() 
    {
        return write_stream_;
    }

    BufferedStreamRefPtr read_stream() 
    {
        return read_stream_;
    }

    ACE_HANDLE get_handle() 
    {
        ACE_HANDLE  handle = sock_stream_? sock_stream_->get_handle() : 0;
        return handle;
    }

    void close() 
    {
        if (sock_stream_)
        {
            sock_stream_->close();
            delete sock_stream_;
            sock_stream_ = 0;
        }
    }

    int handle_input( const char * data, size_t len) 
    {
        assert(data);
        assert(len > 0);
        assert(read_stream_.get());

        int rc =  read_stream_->write(data ,len ) ;
        if (rc != 0) 
        {
            LOG_ERROR_OS( name() <<  " input stream is overlfow!  !");
            return -1;
        }
        return 0;
    }

    int  handle_write()
    {
        assert(write_stream_.get());
        if (write_stream_->empty())
        {
            LOG_ERROR_OS(name() <<  " write stream is empty!");
            return 0;
        }

        int rc = send(write_stream_->data(), write_stream_->size());
        if (rc < 0) 
        {
            return -1;
        }
        else if (rc >0 ) 
        {
            write_stream_->skip(rc);
        }

        // rc == 0 mean would block

        return 0;
    }

    ///return: bytes that have sent,  negative (-1) if failed
    int send (const char * send_buff, size_t buff_size)
    {
        assert(sock_stream_);
        int rc = sock_stream_->send(send_buff , buff_size);

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

    ///return: bytes that have received,  negative (-1) if failed
    int recv (char * recv_buff, size_t buff_size)
    {
        assert(recv_buff);
        assert(buff_size);
        assert(sock_stream_);

        int rc = sock_stream_->recv (recv_buff, buff_size);
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
        else if (rc > (int) buff_size)
        {
            LOG_ERROR_OS( name() <<  " recv return invalid len. len = " << rc ); 
            return -1;
        }

        return rc;
    }


    int send_write(const char * data, size_t len) 
    {
        int rc = write_stream_->write(data, len);
        if (rc != 0)
        {
            LOG_ERROR_OS (name() <<   "write stream is overflow !");
            return -1;
        }

        return 0;
    }


private : 
     TcpChannel ();
     TcpChannel (const TcpChannel &);
     TcpChannel & operator=(const TcpChannel&);

     
private: 
    ACE_SOCK_Stream * sock_stream_;

    BufferedStreamRefPtr read_stream_; 

    BufferedStreamRefPtr write_stream_;
 };

typedef ACE_Refcounted_Auto_Ptr<TcpChannel, ACE_Null_Mutex> TcpChannelRefPtr; 


#endif
