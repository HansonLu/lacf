#include "TcpClient.h"
#include "ace/OS.h"
#include "ace/ACE.h"
#include "ace/Time_Value.h"
#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Stream.h"
#include "ace/OS_NS_unistd.h"
#include "ace/Handle_Set.h"

#include <assert.h>
#include <iostream> 

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

int TcpClient::open ( const char * remote_addr, unsigned short port ) 
{
    assert(remote_addr);
    remote_addr_ = remote_addr; 
    port_ = port;
    int ret = activate();

    if (ret == -1) 
    {
        cerr << " start TcpClient addr:" << remote_addr << " port:" << port << " failed!" << endl;
        return -1;
    }
    return 0;
}

 int TcpClient::handle_connect()
 {
     cerr << " TcpClient  addr:" << remote_addr_ << " port:" << port_<< " connected! " << endl;

     return 0;
 }

 int TcpClient::handle_close()
 {
    cerr << " TcpClient  addr:" << remote_addr_ << " port:" << port_<< " closed! " << endl;
    return 0;
 }

int TcpClient::svc() 
{ 
    while (!stop_ )
    {
        if (! cli_stream_) 
        {
            if (connect_server() != 0) 
            {
                cerr << " connect to server failed" << endl;
                ACE_OS::sleep(10);
                continue;
            }
        }

        if (wait_for_multiple_events() == -1) 
        {
            cerr << " waing event failed" << endl;
            return -1;
        }

    }

    if (cli_stream_)
    {
        close_i();
    }

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
        cerr << " connect to server failed " << endl; 
        delete cli_stream_;
        cli_stream_ = 0;
        return -1;
    }

    cerr << " connect to server ok " << cli_stream_->get_handle() << endl;
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
        cerr <<  " select error " << errno << endl;
        return -1;
    }

    if (handle_pending_read(active_read_handles) == -1)
    {
        cerr <<  " handle pending read failed" << endl;
        return -1;
    }

    if (handle_pending_write(active_write_handles_) == -1)
    {
        cerr <<  " handle pending write failed" << endl;
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

    bool close_handle = false;
    int rc = cli_stream_->recv(recv_buff_, RECV_BUF_SIZE);
    if (rc < 0) 
    {
        if ( errno != EWOULDBLOCK)
        {
            cerr << "recv failed. errno = " << errno<< endl;
            close_handle = true;
        }
    }
    else if (rc == 0 ) 
    {
        cerr<< " recv rc == 0" << endl;
        close_handle = true;
    }
    else if (rc > RECV_BUF_SIZE) 
    {
        cerr << " invalid len. len = %d" << rc << endl; 
        close_handle = true;
    }
    else
    {
        int r = handle_input(recv_buff_, rc);
        if (r != 0) 
        {
            close_handle =true;
        }
    }

    if (close_handle) 
    {
        close_i();
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
        cerr << "client buffered stream is overlfow! handle !" << endl;
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
        cerr << " handle write while stream is closed or  empty. handle "  <<  endl;
        return 0;
    }

    int rc = cli_stream_->send(write_stream_->data(), write_stream_->size());
    if (rc < 0 ) 
    {
        if ( errno != EWOULDBLOCK)
        {
            cerr << "recv failed. errno = " << errno<< endl;
            close_handle = true;
        }
    }
    else if (rc == 0 ) 
    {
        cerr<< " recv rc == 0" << endl;
        close_handle = true;
    }
    else  
    {
        write_stream_->skip(rc);
    }

    if (close_handle) 
    {
        close_i();
    }

    return 0;
}

int TcpClient::send(const char * data, size_t len) 
{
    LockGuard guard(mutex_); 
    if (write_stream_.get() == 0)
    {
        cerr << " connection not exist!" << endl;
        return -1;
    }

    int rc =  write_stream_->write(data, len);
    if (rc != 0)
    {
        cerr  << " buffered stream is overflow ! handle = "  << endl;
        return -1;
    }

    return 0;
}

int TcpClient::shutdown()
{
    stop_ = true;
    wait();
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