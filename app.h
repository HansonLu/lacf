#ifndef LACF_APP_H
#define LACF_APP_H

#include <string> 

class App
{
public :
    App();
    virtual ~App();

    virtual int init() = 0;

    virtual int fini() = 0;

    int main_i(int argc, char ** argv);

    void shutdown() ;

    const std::string config() const ;

private:
    std::string config_;

    unsigned short port_;

    bool daemon_;

    bool shutdown_;
};

#endif