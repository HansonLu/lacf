#include "TcpServer.h"
#include "ace/OS.h"
#include "ace/ACE.h"
#include "ace/SOCK_Stream.h"
#include "ace/Time_Value.h"
#include <iostream> 
#include <string> 
#include <assert.h>


using namespace std; 


int TcpServer::open(unsigned short port)
{
	ACE_INET_Addr server_addr;
	int result;

	if (port == 0)
	{
		cerr << " invalid port !" << endl;
		return -1;
	}

	result = server_addr.set (port, (ACE_UINT32) INADDR_ANY);
	if (result == -1) return -1;

	result =  acceptor_.open (server_addr, 1);

	if(result == -1) 
	{
		cerr << " open address failed! port[ " << port <<"]" <<  endl;
		return -1;
	}

	master_handle_set_.set_bit (acceptor_.get_handle ());
	acceptor_.enable (ACE_NONBLOCK);
	return 0;
};

int TcpServer::handle_connection( ACE_SOCK_Stream & conn) 
{
	cout << "get a connection. [" <<  conn.get_handle() << "]" <<  endl;
	return 0;
}; 

int TcpServer::handle_close(ACE_HANDLE handle) 
{
	cout << " connectition [ " << handle << "] is disconnected!" << endl; 
	return 0;
}


int TcpServer::close(ACE_HANDLE handle) 
{
	LockGuard guard(mutex_); 
	close_handle_i(handle);
	return 0;
}

int TcpServer::run () 
{
	while (!stop_) 
	{
		if (wait_for_multiple_events() == -1) 
		{
			cerr << " waing event failed" << endl;
			return -1;
		}

		if ( handle_connections() == -1) 
		{
			cerr << " handle connections failed" << endl;
			return -1;
		}

		if (handle_pending_read() == -1)
		{
			cerr <<  " handle pending read failed" << endl;
			return -1;
		}


		if (handle_pending_write() == -1)
		{
			cerr <<  " handle pending write failed" << endl;
			return -1;
		}
	}

	return 0;
}


int TcpServer::send(ACE_HANDLE handle ,const char * data, size_t len) 
{
	LockGuard guard(mutex_); 

	BufferedStreamRefPtr stream = write_streams_[handle];

	if (stream.get() == 0)
	{
		cerr << " handle " << handle << " not exist!" << endl;
		return -1;
	}

	int rc =  stream->write(data, len);
	if (rc != 0)
	{
		cerr  << " buffered stream is overflow ! handle = " << handle << endl;
		return -1;
	}
	return 0;
}

int TcpServer::wait_for_multiple_events () 
{
	LockGuard guard(mutex_); 

	active_read_handles_ = master_handle_set_;
	int width = (int)active_read_handles_.max_set () + 1;
	ACE_Time_Value timeout(1);
	set_write_handles();

	int rc = ACE_OS::select (width,
		active_read_handles_.fdset (),
		active_write_handles_.fdset (),
		0,        // no except_fds
		timeout); 

	if (rc  < 0 && errno != EWOULDBLOCK) // no timeout
	{
		cerr <<  " select error " << errno << endl;
		return -1;
	}

	//TODO
	//   active_read_handles_.sync
	//    ((ACE_HANDLE) ((intptr_t) active_read_handles_.max_set () + 1));
	return 0;

}


int TcpServer::handle_connections ()
{
	LockGuard guard(mutex_); 

	if (active_read_handles_.is_set (acceptor_.get_handle ())) 
	{
		ACE_SOCK_Stream peer;
		while (acceptor_ .accept (peer) == 0)
		{
			if ( handle_connection(peer) != 0 )
			{
				peer.close();
				continue;
			}

			peer.enable(ACE_NONBLOCK);
			master_handle_set_.set_bit(peer.get_handle ());
			read_streams_[peer.get_handle()].reset(new CBufferredStream);
			write_streams_[peer.get_handle()].reset(new CBufferredStream);
		}

		// Remove acceptor handle from further consideration.
		active_read_handles_.clr_bit (acceptor_.get_handle ());
	}

	return 0;
}


int TcpServer::handle_pending_write ()
{

	LockGuard guard(mutex_); 

	ACE_Handle_Set_Iterator peer_iterator (active_write_handles_);
	for (ACE_HANDLE handle;
		(handle = peer_iterator ()) != ACE_INVALID_HANDLE;
		) 
	{
		bool close_handle = false;
		ACE_SOCK_Stream sockstream (handle);
		BufferedStreamRefPtr bufstream = write_streams_[handle];

		if (bufstream->empty())
		{
			cerr << " handle write while strea is empty. handle " << handle <<  endl;
			continue;
		}

		int rc = sockstream.send(bufstream->data(), bufstream->size());
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
			bufstream->skip(rc);
		}


		if (close_handle) 
		{
			handle_close(handle);
			close_handle_i(handle);
		}
	}

	return 0;

}

int TcpServer::handle_pending_read () 
{

	LockGuard guard(mutex_); 

	ACE_Handle_Set_Iterator peer_iterator (active_read_handles_);

	for (ACE_HANDLE handle;
		(handle = peer_iterator ()) != ACE_INVALID_HANDLE;
		) {
			bool close_handle = false;
			ACE_SOCK_Stream stream (handle);

			int rc = stream.recv(recv_buff_, RECV_BUF_SIZE);
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
				int r = handle_input(handle, recv_buff_, rc);
				if (r != 0) 
				{
					close_handle =true;
				}
			}

			if (close_handle) 
			{
				handle_close(handle);
				close_handle_i(handle);
			}
	}

	return 0;
}



int TcpServer::handle_input(ACE_HANDLE handle , const char * data, size_t len) 
{
	assert(data);
	assert(len > 0);
	int rc =  read_streams_[handle]->write(data ,len ) ;
	if (rc != 0) 
	{
		cerr << " buffered stream is overlfow! handle = "  << handle << endl;
		return -1;
	}


	return handle_input(handle, read_streams_[handle]);
}

void TcpServer::set_write_handles()
{
	active_write_handles_.reset(); // TODO
	HandleStreamMap::const_iterator it = write_streams_.begin(); 
	while (it != write_streams_.end())
	{
		if (!it->second->empty())
		{
			active_write_handles_.set_bit(it->first);
		}

		it ++;
	}
}

void TcpServer::close_handle_i( ACE_HANDLE  handle) 
{

	ACE_SOCK_Stream stream(handle);
	cout << " close handle " << handle << endl;
	master_handle_set_.clr_bit(handle);
	//active_read_handles_.clr_bit(handle);
	read_streams_.erase(handle);
	write_streams_.erase(handle);
	stream.close();
}