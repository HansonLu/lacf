#ifndef TCP_CLIENT_H 
#define TCP_CLIENT_H 

#include "buffered_stream.h"
#include "ace/Task.h"


class ACE_SOCK_Stream;
class ACE_Handle_Set;

class  TcpClient :  public ACE_Task_Base
{
public : 
    enum { RECV_BUF_SIZE = 1024*1024 }; 

    TcpClient() ;
    ~TcpClient() ;

    int open ( const char * server_ip , unsigned short port) ;

    //override 
    int svc (void) ;

    int  shutdown(void);

    virtual int handle_input( const char * data, size_t len) ;

    virtual int handle_input(BufferedStreamRefPtr stream) = 0;

    virtual int handle_connect();

    virtual int handle_close() ;

    int send(const char * data, size_t len) ;


protected: 

    int check_connect();

    int connect_server();

    int wait_for_multiple_events ();

    int handle_pending_read (ACE_Handle_Set  & active_read_handles);

    int handle_pending_write (ACE_Handle_Set & active_write_handles);

    void close_i( );

private :
   
    ACE_SOCK_Stream * cli_stream_;
    const char * remote_addr_;
    unsigned short port_;

    BufferedStreamRefPtr read_stream_; 
    BufferedStreamRefPtr write_stream_;
    bool stop_;

    char  * recv_buff_;

    ACE_Recursive_Thread_Mutex mutex_;
};


#endif 
