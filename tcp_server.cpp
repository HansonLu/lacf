#include "tcp_server.h"

#include <assert.h>
#include <iostream> 
#include <string> 

#include "ace/OS.h"
#include "ace/ACE.h"
#include "ace/SOCK_Stream.h"
#include "ace/Time_Value.h"

#include "lock_guard.h"
#include "log.h"

using namespace std; 

TcpServer::TcpServer() 
 : stop_(false)
{
	recv_buff_ = new char [RECV_BUF_SIZE];
}

TcpServer::~TcpServer()
{
	if (recv_buff_) 
	{
		delete [] recv_buff_;
		recv_buff_ = 0;
	}
}

const std::string TcpServer::name()
{
	ostringstream ss;
	ss << "[Tcp server task]" << "[" <<  this << "]";
	return ss.str();
}

int TcpServer::open (const char * addr)
{
	int result;

	assert(addr);

	result = server_addr_.set(addr);
	if (result == -1)
	{
		LOG_ERROR_OS( name() << " invalid addr  " << addr  );
		return -1;
	}
	
	result =  acceptor_.open (server_addr_, 1);

	if(result == -1) 
	{
	   LOG_ERROR_OS( name() << " open  addr " << addr <<  " failed!" );
	   return -1;
	}

	master_handle_set_.set_bit (acceptor_.get_handle ());
	acceptor_.enable (ACE_NONBLOCK);

	int ret = activate();

	if (ret == -1) 
	{
		LOG_ERROR_OS ( name() << " start stask failed! addr = " << addr );
		return -1;
	}


	server_addr_s_ = addr; 
	return 0;
};

int TcpServer::handle_connection (ACE_SOCK_Stream & conn) 
{
	 LOG_DEBUG_OS ( name() <<  " get a connection. [" <<  conn.get_handle() << "]" );
	return 0;
}; 

int TcpServer::handle_close (ACE_HANDLE handle) 
{
	 LOG_DEBUG_OS ( name() << " connectition [ " << handle << "] is disconnected!"); 
	return 0;
}


int TcpServer::close (ACE_HANDLE handle) 
{
	LockGuard guard(close_mutex_); 

	req_close_handles_.push_back(handle);
	
	return 0;
}

int TcpServer::svc () 
{
	LOG_TRACE_OS ( name() << "[" << server_addr_s_  <<  "] start ...!");

	while (!stop_) 
	{
		if (wait_for_multiple_events() == -1) 
		{
			LOG_ERROR_OS( name() << " waing event failed!");
			break;
		}

		if ( handle_connections() == -1) 
		{
			LOG_ERROR_OS( name() <<  " handle connections failed!");
			break;
		}

		if (handle_pending_read() == -1)
		{
			LOG_ERROR_OS( name() << " handle pending read failed!");
			break;
		}


		if (handle_pending_write() == -1)
		{
			LOG_ERROR_OS( name() <<  " handle pending write failed!");
			break;
		}

		close_handles_i();
	}

	LOG_TRACE_OS ( name() << "[" << server_addr_s_ << "] exit ...!");

	return 0;
}


int TcpServer::send(ACE_HANDLE handle ,const char * data, size_t len) 
{
	LockGuard guard(mutex_); 

	BufferedStreamRefPtr stream = write_streams_[handle];

	if (stream.get() == 0)
	{
		LOG_ERROR_OS( name() <<  " handle " << handle << " not exist!" );
		return -1;
	}

	int rc =  stream->write(data, len);
	if (rc != 0)
	{
		LOG_ERROR_OS( name() << "write stream is overflow ! handle = " << handle);
		return -1;
	}
	return 0;
}

int TcpServer::wait_for_multiple_events () 
{
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
		LOG_ERROR_OS( name() << " select failed .error =  " << errno  );
		return -1;
	}

	//TODO
	//   active_read_handles_.sync
	//    ((ACE_HANDLE) ((intptr_t) active_read_handles_.max_set () + 1));
	return 0;

}

void TcpServer::reset_handle_streams(ACE_HANDLE handle ) 
{
	LockGuard guard(mutex_); 
	read_streams_[handle].reset(new CBufferredStream);
	write_streams_[handle].reset(new CBufferredStream);
}

int TcpServer::handle_connections ()
{
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
			reset_handle_streams(peer.get_handle());
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
			LOG_ERROR_OS( name() << " write stream is empty. handle = " << handle);
			continue;
		}

		int rc = sockstream.send(bufstream->data(), bufstream->size());
		if (rc < 0 ) 
		{
			if ( errno != EWOULDBLOCK)
			{
				LOG_ERROR_OS( name() << "recv failed. errno = " << errno);
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
			close_handle_i(handle);
		}
	}

	return 0;

}

int TcpServer::handle_pending_read () 
{
	//no need to accuire lock for read
	//LockGuard guard(mutex_); 

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
					LOG_ERROR_OS( name() <<  "recv failed. errno = " << errno);
					close_handle = true;
				}
			}
			else if (rc == 0 ) 
			{
				LOG_ERROR_OS( name() << "recv rc == 0" );
				close_handle = true;
			}
			else if (rc > RECV_BUF_SIZE) 
			{
				LOG_ERROR_OS( name() <<  " recv return invalid len. len = " << rc ); 
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
		LOG_ERROR_OS( name() << " read stream is overlfow! handle = "  << handle );
		return -1;
	}

	return handle_input(handle, read_streams_[handle]);
}

void TcpServer::set_write_handles()
{
	LockGuard guard(mutex_); 
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

void TcpServer::close_handles_i()
{
	LockGuard guard(close_mutex_); 
	std::list<ACE_HANDLE>::iterator it = req_close_handles_.begin() ;
	while (it != req_close_handles_.end()) 
	{
		close_handle_i(*it);
		it++;
	}
}

void TcpServer::close_handle_i( ACE_HANDLE  handle) 
{
	ACE_SOCK_Stream stream(handle);
	LOG_DEBUG_OS (" close handle [" << handle << "].");
	//active_read_handles_.clr_bit(handle);
	read_streams_.erase(handle);
	write_streams_.erase(handle);
	stream.close();

	handle_close(handle);
	
}

int TcpServer::shutdown()
{
	stop_ = true;
	//wait();
	return 0;
}

