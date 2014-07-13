#include "cmd_task.h"
#include <assert.h>
#include <iostream>
#include "app.h"

using namespace std; 


const char  CMD_SPLIT [] = "\r\n";
const char  PROMPT []    = "\r\n-->";

CmdConn::CmdConn() 
{
    need_auth =  false;
    auth      =  false;
}

class ShutdownCmd : public CmdHandler 
{
    const std::string name () const
    {
        return "shutdown";
    }

    virtual const std::string exec (std::vector<std::string> &argv) 
    {
        get_app()->shutdown();
        return "Ok";
    }

    virtual const std::string help ()
    {
        return name();
    }
};

class HelpCmd : public CmdHandler 
{
public: 

    const std::string name () const
    {
        return "help";
    }

    virtual const std::string exec (std::vector<std::string> &argv) 
    {
        std::ostringstream ss; 

        if (argv.size() == 2)
        {  
            //list specific cmd's help
            CmdHandlerRefPtr handler =  CmdTarget::instance().get_handler(argv[1]); 

            if (!handler.get())
            {
                return "can not find command";
            }

            return handler->help();
        }

        //list all cmd 
        CmdTarget::CMD_MAP_T cmd_map = CmdTarget::instance().get_all_handlers();
        CmdTarget::CMD_MAP_T::const_iterator it = cmd_map.begin(); 
        while (it != cmd_map.end() )
        {
            CmdHandlerRefPtr handler = it->second;
            ss << handler->name() << "\r\n";
            it++;
        }

        return ss.str();
    }

    virtual const std::string help ()
    {
        return name();
    }
};

CmdTask::CmdTask(void)
{
    init_cmd();
}

CmdTask::~CmdTask(void)
{

}

int CmdTask::init_cmd()
{
    CmdTarget::instance().reg_handler(CmdHandlerRefPtr( new ShutdownCmd()));
    CmdTarget::instance().reg_handler(CmdHandlerRefPtr( new HelpCmd()));
    return 0;
}

int CmdTask::handle_connection( ACE_SOCK_Stream & sock)
{
    //if need auth , set auth flag here 
    CmdConn & conn = cmd_conn_map_[sock.get_handle()];
    conn.handle = sock.get_handle();
    send(conn.handle, PROMPT, sizeof(PROMPT));

    return 0;
}

int CmdTask::handle_close(ACE_HANDLE handle)
{
    cmd_conn_map_.erase(handle);

    return 0;
}

int CmdTask::handle_input(ACE_HANDLE handle, BufferedStreamRefPtr stream)
{
    CmdConn & conn = cmd_conn_map_[handle];
    conn.handle = handle;
    assert(!stream->empty()); 

    conn.pending_cmd.append(stream->data() , stream->size());
    stream->skip(stream->size());

    size_t start = 0;
    size_t last_pos = string::npos;
    size_t  pos =  conn.pending_cmd.find_first_of(CMD_SPLIT);

    while (pos != string::npos ) 
    {
        last_pos  = pos;

        if (pos != start)
        {
            string cmd = conn.pending_cmd.substr(start, pos - start);
            handle_cmd(conn, cmd);
        }

        start = pos +1;
        pos = conn.pending_cmd.find_first_of(CMD_SPLIT, start);

    }

    if (last_pos != string::npos)
    {
        conn.pending_cmd.erase(0, last_pos);
    }

    return 0;
}

static void split_argvs(const std::string & cmd, std::vector<std::string> & v) 
{
    size_t start = 0;
    size_t last_pos = string::npos;
    size_t pos =  cmd.find_first_of(" ");

    while (pos != string::npos ) 
    {
        if (pos != start) 
        {
            v.push_back(cmd.substr(start, pos -start));
        }

        start = pos +1;
        pos =  cmd.find_first_of(" ", start);
    }

    if (start < cmd.length()) 
    {
        v.push_back ( cmd.substr(start,  cmd.length() - start));
    }

}

int CmdTask::reply(CmdConn& conn, const std::string & r) 
{
     ostringstream ss; 

     ss <<  r << "\r\n" << PROMPT; 

     send( conn.handle, ss.str().c_str(), ss.str().length());
     return 0;
}

int CmdTask::handle_cmd(CmdConn & conn, const std::string & cmd)
{
    cout << " handle cmd [" << conn.handle << "]" << cmd << endl;

    std::vector<std::string> args; 

    split_argvs(cmd, args);

    if (args.empty()) 
    {
        reply(conn, "Invalid Command");
    }

    CmdHandlerRefPtr handler =  CmdTarget::instance().get_handler(args[0]);

    if (!handler.get()) 
    {
        reply(conn, "Unknown Command");
        return 0;
    }

    std::string res =  handler->exec(args);

    reply(conn, res);

    return 0;
}
