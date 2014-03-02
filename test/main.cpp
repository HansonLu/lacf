
#include "TcpServer.h"
#include "TcpClient.h"
#include <iostream> 

using namespace std; 


class TelnetServer : public TcpServer 
{
public: 
	virtual int handle_input(ACE_HANDLE handle, BufferedStreamRefPtr stream) 
	{
		send (handle, "hello", 6);
		return 0;
	}
};

class TestTcpClient : public TcpClient 
{
public : 
    TestTcpClient() 
    {
        recv_ = 0;
        connect_ = false;
    }

    int handle_input(BufferedStreamRefPtr stream) 
	{
	    while (stream->size() > 100)
        {
            char buf [200];
            int rc = stream->read(buf, 100);
            assert (rc = 100);
            stream->skip(100);

            recv_++;

            if (recv_ %1000 == 0) 
            {
                cout << " client recv " << recv_ << endl;
            }
        }

        return 0;
    }

    int handle_connect() 
    {
        connect_ = true;
        return 0;
    }

    int recv_;

    bool connect_;

};


class TestTcpServer:  public TcpServer
{
public: 
    TestTcpServer() 
    {

        recv_ = 0;
    }
    virtual int handle_input(ACE_HANDLE handle, BufferedStreamRefPtr stream) 
	{
		//send (handle, "hello", 6);
        while (stream->size() > 100)
        {
            char buf [200];
            int rc = stream->read(buf, 100);
            assert (rc = 100);
            stream->skip(100);
            recv_++;
             rc = send(handle, buf, 100);

            assert(rc == 0);

            if (recv_ %100 == 0) 
            {
                cout << " server recv " << recv_ << endl;
            }
      
        }

		return 0;
	}

 
    int recv_;
   
};

int
	ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
	//TelnetServer server; 

	//server.open(9999); 
    TestTcpServer server;

    int rc= server.open(9999);
    assert (rc == 0);
    TestTcpClient client; 

    rc = client.open("127.0.0.1", 9999);
    assert(rc == 0);

    char buf[100] = {'a'};

    while (!client.connect_) 
    {

        ACE_OS::sleep(1);
    }

    for (int i= 0; i< 100000; i++)
    {
        rc = client.send(buf ,100);
        assert(rc ==0);

        if (i%100 ==0)
        {
            cout << "send  " << i <<  endl;
            ACE_OS::sleep(ACE_Time_Value(0, 20000));

        }

    }
	
	return 0;

}
