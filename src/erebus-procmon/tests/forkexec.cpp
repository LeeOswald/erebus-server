#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


int main(int argc, char** argv, char** envp)
{
    bool is_last = false;
    int forks = 1;
    char* const child_args[] = 
    {
        argv[0],
        const_cast<char*>("--last"),
        nullptr
    }; 


    for (int i = 1; i < argc; )
    {
        if (!std::strcmp(argv[i], "--last"))
        {
            is_last = true;
        }
        else if ((i + 1 < argc) && !std::strcmp(argv[i], "--loop"))
        {
            forks = std::atoi(argv[i + 1]);
            if (forks < 1)
                forks = 1;

            ++i;
        }
        
        ++i;
    }

    if (is_last)
    {
        std::cout << ::getpid() << " I am the last\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << ::getpid() << " Good bye\n";
        return 0;
    }

    for (int i = 0; i < forks; ++i) 
    {
        std::cout << ::getpid() << " Forking...\n";
        auto pid = ::fork();
        if (pid < 0)
        {
            auto e = errno;
            std::cout << ::getpid() << " fork() failed: " << e << "\n";
            return e;
        }
        else if (pid == 0) 
        {
            // we're the child
        }
        else
        {
            // we're the parent
            std::cout << ::getpid() << " Waiting for " << pid << " ...\n";
            ::wait(nullptr);
            return 0;
        }
    }

    return ::execve(argv[0], child_args, envp);
}