#ifndef TCP_CLIENT_H 
#define TCP_CLIENT_H 

#include "ace/Task.h"

#include "tcp_channel.h"

class ACE_SOCK_Stream;
class ACE_Handle_Set;

class  TcpClient :  public ACE_Task_Base
{
public : 
    enum { RECV_BUF_SIZE = 1024*1024 }; 

    TcpClient() ;

    virtual ~TcpClient() ;

    int open ( const char * server_ip , unsigned short port) ;

    //override 
    int svc (void) ;

    int  shutdown(void);

    virtual int handle_input( const char * data, size_t len) ;

    virtual int handle_input(BufferedStreamRefPtr stream) = 0;

    virtual int handle_connect();

    virtual int handle_close() ;

    int send(const char * data, size_t len) ;
  
    const std::string name();
   
protected: 

    int check_connect();

    int connect_server();

    int wait_for_multiple_events ();

    int handle_pending_read (ACE_Handle_Set  & active_read_handles);

    int handle_pending_write (ACE_Handle_Set & active_write_handles);

    void close_i( );

private :
    const char * remote_addr_;

    unsigned short port_;

    TcpChannelRefPtr  channel_; 

    bool stop_;

    char  * recv_buff_;

    ACE_Recursive_Thread_Mutex mutex_;
};


#endif 
