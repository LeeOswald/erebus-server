#include <erebus/testing/test_application.hxx>

#include "common.hpp"



class App
    : public Erp::Testing::TestApplication
{
public:
    using Base = Erp::Testing::TestApplication;

    App()
        : Base(Er::Program::Options::SyncLogger)
    {
    }
};


int main(int argc, char** argv)
{
    try
    {
        App app;
        
        auto resut = app.exec(argc, argv);

        return resut;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unexpected exception" << std::endl;
    }

    return -1;
}

