#ifndef TCP_SERVER_H 
#define TCP_SERVER_H

#include <map>
#include <list>

#include "ace/SOCK_Acceptor.h"
#include "ace/Handle_Set.h"
#include "ace/Task.h"

#include "tcp_channel.h"

class ACE_SOCK_Stream; 

class TcpServer :  public ACE_Task_Base
{
public: 
    enum { RECV_BUF_SIZE = 1024*1024 }; 

    TcpServer();
    virtual ~TcpServer();

    int open (const char * addr);

    virtual int handle_connection( ACE_SOCK_Stream & conn);

    virtual int handle_close(ACE_HANDLE handle);

    virtual int handle_input(ACE_HANDLE handle, BufferedStreamRefPtr stream) = 0;

    int close(ACE_HANDLE handle) ;

    //override
    int svc (void);

    int send(ACE_HANDLE handle ,const char * data, size_t len);

    int shutdown();

    const std::string name(); 

private : 
    virtual int wait_for_multiple_events () ;

    virtual int handle_connections ();

    virtual int handle_pending_write (); 

    virtual int handle_pending_read () ;

    virtual int handle_input(ACE_HANDLE handle , const char * data, size_t len);

    void set_write_handles();

    void close_handles_i();

    void close_handle_i( ACE_HANDLE  handle);

    void reset_handle_streams(ACE_HANDLE handle );

private:
    // typedef std::map<ACE_HANDLE, BufferedStreamRefPtr> HandleStreamMap; 

    typedef std::map<ACE_HANDLE, TcpChannelRefPtr>TcpChannelMap; 


    ACE_SOCK_Acceptor acceptor_; // Socket acceptor endpoint.

    ACE_INET_Addr server_addr_;
    std::string   server_addr_s_;


    ACE_Handle_Set master_handle_set_;

    ACE_Handle_Set active_read_handles_;

    ACE_Handle_Set active_write_handles_;

    TcpChannelMap  channels_;

    //HandleStreamMap read_streams_;

    //HandleStreamMap  write_streams_;

    ACE_Recursive_Thread_Mutex mutex_;

    ACE_Recursive_Thread_Mutex close_mutex_;

    char  * recv_buff_;

    std::list<ACE_HANDLE> req_close_handles_;
    bool stop_;
};

#endif

