
#include "TcpServer.h"
#include "TcpClient.h"
#include <iostream> 

using namespace std; 

const int PACKET_SIZE = 100;

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
        while (stream->size() >= PACKET_SIZE)
        {
            char buf [PACKET_SIZE];
            int rc = stream->read(buf, sizeof(buf));
            assert (rc == sizeof(buf));
            stream->skip(sizeof(buf));

            recv_++;

            if (recv_ %1000 == 0) 
            {
                cout << "client recv " << recv_ << endl;
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
        while (stream->size() >= PACKET_SIZE)
        {
            char buf [PACKET_SIZE];
            int rc = stream->read(buf, sizeof(buf));
            assert (rc == PACKET_SIZE);
            stream->skip(PACKET_SIZE);
            recv_++;
            rc = send(handle, buf, sizeof(buf));

            assert(rc == 0);

            if (recv_ %100 == 0) 
            {
                cout << "server recv " << recv_ << endl;
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
    //char buf[100] = {'a'};
    //char * rbuf = new char[50000];
    //CBufferredStream st; 
    //time_t start = time(0);
    //int totalSend = 0;
    //for (int i =0; i < 100000; i++) 
    //{
    //   int rc =  st.write(buf, sizeof(buf)); 
    //   totalSend += totalSend;
    //   if ( i%20 == 0 && totalSend > 0) 
    //   {
    //       st.read(rbuf, totalSend);
    //       st.skip(totalSend);
    //       totalSend = 0;
    //   }
    //}

    //time_t end = time(0);
    //delete []rbuf;
    //cout << " time = " << end - start << endl;

    while (!client.connect_) 
    {

        ACE_OS::sleep(1);
    }

    char buf[PACKET_SIZE] = {'a'};

    const int SEND_TIMES = 100000;
    for (int i= 0; i< SEND_TIMES; i++)
    {
        rc = client.send(buf ,sizeof(buf));
        assert(rc ==0);

        if (i%1000 ==0)
        {
            cout << "client send " << i << endl;
            ACE_OS::sleep(ACE_Time_Value(0, 100000));
        }
    }

    ACE_OS::sleep(5);

    client.shutdown();
    assert(client.recv_ == SEND_TIMES);
    assert(server.recv_  == SEND_TIMES);

    server.shutdown();

    ACE_Thread_Manager::instance()->wait();

    return 0;

}
