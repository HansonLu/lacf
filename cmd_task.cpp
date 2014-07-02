#include "cmd_task.h"
#include <assert.h>
#include <iostream>

using namespace std; 


static const char  CMD_SPLIT [] = "\r\n";

CmdConn::CmdConn() 
{
    need_auth =  false;
    auth      =  false;
}

CmdTask::CmdTask(void)
{
}

CmdTask::~CmdTask(void)
{
}

int CmdTask::handle_connection( ACE_SOCK_Stream & sock)
{
    //if need auth , set auth flag here 
    CmdConn & conn = cmd_conn_map_[sock.get_handle()];
    conn.handle = sock.get_handle();
    return 0;
}

int CmdTask::handle_close(ACE_HANDLE handle)
{
    return 0;
}

int CmdTask::handle_input(ACE_HANDLE handle, BufferedStreamRefPtr stream)
{
    CmdConn & conn = cmd_conn_map_[handle];
    conn.handle = handle;
    assert(!stream->empty()); 

    conn.pending_cmd.append(stream->data() , stream->size());
    stream->skip(stream->size());

    string::size_type start = 0;
    string::size_type last_pos = string::npos;
    string::size_type pos =  conn.pending_cmd.find_first_of(CMD_SPLIT);

    while (pos != string::npos ) 
    {
        last_pos  = pos;
        string cmd = conn.pending_cmd.substr(start, pos);

        if (pos != start &&handle_cmd(conn, cmd) != 0)
        {
            cmd_conn_map_.erase(handle);
            return -1;//close the connection
        }

        if (pos  >= conn.pending_cmd.length() -1 )
            break;

        start = pos +1;
        pos = conn.pending_cmd.find_first_of(CMD_SPLIT, start);

    }

    if (last_pos != string::npos)
    {
        conn.pending_cmd.erase(0, last_pos);
    }

    return 0;
}

int CmdTask::handle_cmd(CmdConn & conn, const std::string & cmd)
{
    cout << " handle cmd [" << conn.handle << "]" << cmd << endl;
    return 0;
}
