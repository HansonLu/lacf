#ifndef LACF_CMD_TASK_H 
#define LACE_CMD_TASK_H

#include <map> 
#include <string>

#include  "tcp_server.h"

struct CmdConn
{
    CmdConn(); 
    ACE_HANDLE handle; 
    std::string pending_cmd;
    bool need_auth;
    bool auth;
};


class CmdTask : public TcpServer
{
public:
    CmdTask(void);

    ~CmdTask(void);

    int handle_cmd( CmdConn &conn, const std::string & cmd); 

    virtual int handle_connection( ACE_SOCK_Stream & conn);

    virtual int handle_close(ACE_HANDLE handle);

    virtual int handle_input(ACE_HANDLE handle, BufferedStreamRefPtr stream);

private: 
    typedef std::map<ACE_HANDLE, CmdConn> CmdConnMapT; 
    CmdConnMapT   cmd_conn_map_;
};


#endif 
