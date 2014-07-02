#include "app.h"
#include "ace/OS.h"
#include "ace/ACE.h"
#include "ace/Get_Opt.h"
#include "ace/Thread_Manager.h"
#include "log.h"
#include "cmd_task.h"
#include <iostream>

App* get_app();

using namespace std; 

static void term_handler(int sig) 
{
    LOG_INFO_OS("app term ...");
    get_app()->shutdown();
}

static void do_init_sig()
{
    ACE_OS::signal (SIGTERM, term_handler);
    ACE_OS::signal (SIGQUIT, term_handler);
    ACE_OS::signal (SIGINT, term_handler);
}

static int daemonize ()
{
    ACE_TRACE ("ACE::daemonize");
#if !defined (ACE_LACKS_FORK)
    pid_t pid = ACE_OS::fork ();

    if (pid == -1)
        return -1;
    else if (pid != 0)
        ACE_OS::exit (0); // Parent exits.

    // 1st child continues.
    ACE_OS::setsid (); // Become session leader.

    ACE_OS::signal (SIGHUP, SIG_IGN);

    pid = ACE_OS::fork ();

    if (pid != 0)
        ACE_OS::exit (0); // First child terminates.

    // Second child continues.

    ACE_OS::umask (0); // clear our file mode creation mask.

    // Close down the I/O handles.
    /* if (close_all_handles)
    {
    for (int i = ACE::max_handles () - 1; i >= 0; i--)
    ACE_OS::close (i);

    int fd = ACE_OS::open ("/dev/null", O_RDWR, 0);
    if (fd != -1)
    {
    ACE_OS::dup2 (fd, ACE_STDIN);
    ACE_OS::dup2 (fd, ACE_STDOUT);
    ACE_OS::dup2 (fd, ACE_STDERR);

    if (fd > ACE_STDERR)
    ACE_OS::close (fd);
    }
    }*/

    return 0;
#else
    //ACE_UNUSED_ARG (pathname);
    // ACE_UNUSED_ARG (close_all_handles);
    //ACE_UNUSED_ARG (program_name);

    ACE_NOTSUP_RETURN (-1);
#endif /* ACE_LACKS_FORK */
}


App::App()
    : daemon_(false),
    shutdown_ (false)
{
}


App::~App()
{
}

const std::string App::config() const 
{
    return config_;
}

int App::main_i(int argc, char ** argv)
{
    LOG_INFO_OS("app start ...");

    int c;
    
    ACE_Get_Opt getopt (argc, argv, ACE_TEXT("dp:c:"));
    while ((c = getopt ()) != -1)
        switch (c)
    {
        case 'p':
            this->port_ = getopt.opt_arg ();
            break;
        case 'c':
            config_ = getopt.opt_arg ();

            break;
        case 'd':
            daemon_ = true;
            break;
    }


    if (port_.empty()) 
    {
        cerr << " App's port is invalid" << endl;
        return -1;
    }

    if (daemon_)
    {
        daemonize();
    }

    do_init_sig();

    ACE::init();

    cmd_task_ = new CmdTask(); 
    if ( cmd_task_->open(port_.c_str() )!= 0) 
    {
        cerr << " open cmd port " << port_ << " failed!" << endl;
        return -1;
    }


    init();

    while (!shutdown_) 
    {
        ACE_OS::sleep(1);
    }

    fini();

    cmd_task_->shutdown();

    ACE_Thread_Manager::instance()->wait();
    ACE::fini();
    LOG_INFO_OS("app exit ...");

    delete cmd_task_;
    cmd_task_ =0;
    return 0;
}

void App::shutdown()
{
    shutdown_ = true;
}

