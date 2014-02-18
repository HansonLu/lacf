
#include "TcpServer.h"

class TelnetServer : public TcpServer 
{

public: 

	virtual int handle_input(ACE_HANDLE handle, BufferedStreamRefPtr stream) 
	{
		send (handle, "hello", 6);
		return 0;
	}

};


int
	ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
	TelnetServer server; 

	server.open(9999); 
	server.run();

	return 0;

}

