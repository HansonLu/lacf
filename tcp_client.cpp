#include "tcp_client.h"

#include <assert.h>
#include <iostream> 

#include "ace/OS.h"
#include "ace/ACE.h"
#include "ace/Time_Value.h"
#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Stream.h"
#include "ace/OS_NS_unistd.h"
#include "ace/Handle_Set.h"

#include "lock_guard.h"
#include "log.h"
#include "tcp_connection.h"


using namespace std; 


TcpClient::TcpClient() 
    :cli_stream_(0),
     stop_(false)
{
    recv_buff_ = new char [RECV_BUF_SIZE];
}

TcpClient::~TcpClient()
{
    if (recv_buff_) 
    {
        delete [] recv_buff_;
        recv_buff_ = 0;
    }

}  

const std::string TcpClient::name() 
{
    ostringstream ss;
    ss << "[Tcp client task]" << "[" <<  this << "]";
    return ss.str();
}

int TcpClient::open ( const char * remote_addr, unsigned short port ) 
{
    assert(remote_addr);
    remote_addr_ = remote_addr; 
    port_ = port;
    int ret = activate();

    if (ret == -1) 
    {
        LOG_ERROR_OS( name()  <<  "start task failed. remote [" << remote_addr <<":" << port <<"] !");
        return -1;
    }
   
    return 0;
}

 int TcpClient::handle_connect()
 {
     LOG_DEBUG_OS (name() <<   "connected! ");

     return 0;
 }

 int TcpClient::handle_close()
 {
    LOG_DEBUG_OS( name()  <<  " disconnected! ");
    return 0;
 }

int TcpClient::svc() 
{ 
    LOG_TRACE_OS (name () << " start...! remote = [ " << remote_addr_ << ":" << port_ <<"] !");

    while (!stop_ )
    {
        if (! cli_stream_) 
        {
            if (connect_server() != 0) 
            {
                ACE_OS::sleep(10);
                continue;
            }
        }

        if (wait_for_multiple_events() == -1) 
        {
            LOG_ERROR_OS(name() <<  "waing event failed!");
            break;
        }
    }

    if (cli_stream_)
    {
        close_i();
    }

    LOG_TRACE_OS ( name() <<   " exit!");

    return 0;
}

int TcpClient::connect_server()
{
    assert(cli_stream_ == 0);

    cli_stream_ = new ACE_SOCK_Stream();
    ACE_INET_Addr server_addr(port_, remote_addr_);
    ACE_SOCK_Connector con;

    ACE_Time_Value timeout (30);

    if (con.connect (*cli_stream_,
        server_addr,
        &timeout) == -1)
    {
        LOG_DEBUG_OS( name() <<  "connect to server failed! "); 
        delete cli_stream_;
        cli_stream_ = 0;
        return -1;
    }

    LOG_DEBUG_OS (name() <<   " connect to server  ok! ");
    cli_stream_->enable(ACE_NONBLOCK);

    read_stream_.reset(new CBufferredStream);
    write_stream_.reset(new CBufferredStream);


    (void ) handle_connect();

    return 0;
}

int TcpClient::wait_for_multiple_events () 
{
    ACE_Handle_Set active_read_handles,active_write_handles_;
    active_read_handles.set_bit(cli_stream_->get_handle());

    int width = (int)active_read_handles.max_set () + 1;

    if (write_stream_.get() !=0 && !write_stream_->empty())
    {
        active_write_handles_.set_bit(cli_stream_->get_handle());
    }

    ACE_Time_Value timeout(1);

    int rc = ACE_OS::select (width,
        active_read_handles.fdset (),
        active_write_handles_.fdset (),
        0,        // no except_fds
        timeout); 

    if (rc  < 0 && errno != EWOULDBLOCK) // no timeout
    {
        LOG_ERROR_OS (name() <<  " select failed. error: " << errno) ;
        return -1;
    }

    if (handle_pending_read(active_read_handles) == -1)
    {
        LOG_ERROR_OS (name() <<   " handle pending read failed!");
        return -1;
    }

    if (handle_pending_write(active_write_handles_) == -1)
    {
       LOG_ERROR_OS(name() <<   " handle pending write failed!");
        return -1;
    }
    //TODO
    //   active_read_handles_.sync
    //    ((ACE_HANDLE) ((intptr_t) active_read_handles_.max_set () + 1));
    return 0;
}

int TcpClient::handle_pending_read (ACE_Handle_Set & active_read_handles) 
{
    if ( ! active_read_handles.is_set(cli_stream_->get_handle()) )
    {
        return 0;
    }

    TcpConnection conn(cli_stream_->get_handle());
    int rc = conn.recv(recv_buff_, RECV_BUF_SIZE);
    if (rc < 0) 
    {
        close_i();
    } 
    else if (rc >0 )
    {
        int r = handle_input(recv_buff_, rc);
        if (r != 0) 
        {
             close_i();
        }
    }
    
    return 0;
}

int TcpClient::handle_input( const char * data, size_t len) 
{
    assert(data);
    assert(len > 0);
    assert(read_stream_.get());

    int rc =  read_stream_->write(data ,len ) ;
    if (rc != 0) 
    {
        LOG_ERROR_OS( name() <<  " input buffered stream is overlfow!  !");
        return -1;
    }

    return handle_input(read_stream_);
}

int TcpClient::handle_pending_write (ACE_Handle_Set & active_write_handles)
{
    LockGuard guard(mutex_); 

    if (!active_write_handles.is_set(cli_stream_->get_handle()))
    {
        return 0;
    }

    bool close_handle = false;
    
    if (!write_stream_.get() || write_stream_->empty())
    {
         LOG_ERROR_OS(name() <<  " write stream is closed or  empty.");
        return 0;
    }

    TcpConnection conn(cli_stream_->get_handle());


    int rc = conn.send(write_stream_->data(), write_stream_->size());

    if (rc < 0) 
    {
       close_i();
    }
    else if (rc >0 ) 
    {
        write_stream_->skip(rc);
    }
    // rc ==0 mean send would block

    return 0;
}

int TcpClient::send(const char * data, size_t len) 
{
    LockGuard guard(mutex_); 
    if (write_stream_.get() == 0)
    {
        LOG_ERROR_OS (name() <<   "connection not exist!" );
        return -1;
    }

    int rc =  write_stream_->write(data, len);
    if (rc != 0)
    {
        LOG_ERROR_OS (name() <<   "write stream is overflow !");
        return -1;
    }

    return 0;
}

int TcpClient::shutdown()
{
    stop_ = true;
    //wait();
    return 0;
}

void TcpClient::close_i( ) 
{
    read_stream_.reset(0);
    write_stream_.reset(0);

    cli_stream_->close();

    delete cli_stream_; 
    cli_stream_ = 0;

    handle_close();
}
 
