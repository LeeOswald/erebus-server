#include "client_app.hxx"

#include <iostream>


int main(int argc, char* argv[], char* env[])
{
    try
    {
        ClientApplication app;

        return app.exec(argc, argv);
    }
    catch (std::exception& e)
    {
        std::cerr << "Unexpected exception: " << e.what() << std::endl;
    }

    return EXIT_FAILURE;
}