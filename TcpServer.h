#ifndef TCP_SERVER_H 
#define TCP_SERVER_H

#include "BufferedStream.h"
#include <map>
#include "ace/SOCK_Acceptor.h"
#include "ace/Handle_Set.h"


class ACE_SOCK_Stream; 


class TcpServer 
{
public: 
	enum { RECV_BUF_SIZE = 1024*1024 }; 

	TcpServer() 
		: stop_(false)
	{
		recv_buff_ = new char [RECV_BUF_SIZE];
	}

	~TcpServer()
	{
		if (recv_buff_) 
		{
			delete [] recv_buff_;
			recv_buff_ = 0;
		}
	}

	int open(unsigned short port);
	
	virtual int handle_connection( ACE_SOCK_Stream & conn);


	virtual int handle_close(ACE_HANDLE handle);


	virtual int handle_input(ACE_HANDLE handle, BufferedStreamRefPtr stream) = 0;

	int close(ACE_HANDLE handle) ;

	
	int run ();
	
	int send(ACE_HANDLE handle ,const char * data, size_t len);
	

private : 
	virtual int wait_for_multiple_events () ;


	virtual int handle_connections ();

	virtual int handle_pending_write (); 
	virtual int handle_pending_read () ;

	virtual int handle_input(ACE_HANDLE handle , const char * data, size_t len);
	
	void set_write_handles();
	
	void close_handle_i( ACE_HANDLE  handle);
	
private:
	typedef std::map<ACE_HANDLE, BufferedStreamRefPtr> HandleStreamMap; 

	typedef ACE_Guard<ACE_Recursive_Thread_Mutex> LockGuard; 


	ACE_SOCK_Acceptor acceptor_; // Socket acceptor endpoint.

	ACE_Handle_Set master_handle_set_;

	ACE_Handle_Set active_read_handles_;

	ACE_Handle_Set active_write_handles_;

	HandleStreamMap read_streams_;

	HandleStreamMap  write_streams_;

	ACE_Recursive_Thread_Mutex mutex_;

	char  * recv_buff_;

	bool stop_;
};




#endif 