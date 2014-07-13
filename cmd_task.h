#ifndef LACF_CMD_TASK_H 
#define LACE_CMD_TASK_H

#include <map> 
#include <string>
#include <vector> 

#include  "tcp_server.h"

#include  "ace/Refcounted_Auto_Ptr.h"
#include "ace/Synch.h"

class CmdHandler
{
public: 
   virtual ~CmdHandler () {};

   virtual const std::string name () const = 0; 

   virtual const std::string exec (std::vector<std::string> &argv) = 0;
   
   virtual const std::string help () =0;
};


typedef ACE_Refcounted_Auto_Ptr<CmdHandler, ACE_Null_Mutex> CmdHandlerRefPtr; 


class CmdTarget
{
public: 
    typedef std::map<std::string, CmdHandlerRefPtr> CMD_MAP_T;
   
    static  CmdTarget & instance () 
    {
        static CmdTarget instance; 
        return instance;
    }

    void reg_handler(CmdHandlerRefPtr handler)
    {
         cmd_maps_[handler->name()] = handler;
    }

    CmdHandlerRefPtr get_handler (const std::string & tag)  
    {
        CmdHandlerRefPtr handler;

        CMD_MAP_T::const_iterator it = cmd_maps_.find(tag);
        if (it!= cmd_maps_.end())
        {
            handler = it->second;
        }

        return handler;
    }

    CMD_MAP_T get_all_handlers() const 
    {
        return cmd_maps_;
    }

private:
     CMD_MAP_T cmd_maps_;
};


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

    virtual int  init_cmd();

    int handle_cmd( CmdConn &conn, const std::string & cmd); 

    virtual int handle_connection( ACE_SOCK_Stream & conn);

    virtual int handle_close(ACE_HANDLE handle);

    virtual int handle_input(ACE_HANDLE handle, BufferedStreamRefPtr stream);

    int reply(CmdConn& conn, const std::string & r) ;

private: 
    typedef std::map<ACE_HANDLE, CmdConn> CmdConnMapT; 
    CmdConnMapT   cmd_conn_map_;
};


#endif 
