#include "server_app.hxx"

#include <iostream>


int main(int argc, char* argv[], char* env[])
{
#if ER_LINUX
    bool noroot = ServerApplication::argPresent(argc, argv, "--noroot", nullptr);
    if (!noroot && (::geteuid() != 0))
    {
        std::cerr << "Root privileges required" << std::endl;
        std::exit(EXIT_FAILURE);
    }
#endif

    try
    {
        ServerApplication app;

        return app.exec(argc, argv);
    }
    catch (std::exception& e)
    {
        std::cerr << "Unexpected exception: " << e.what() << std::endl;
    }

    return EXIT_FAILURE;
}