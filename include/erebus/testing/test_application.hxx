#pragma once


#include <erebus/rtl/program.hxx>


namespace Erp::Testing
{

class TestApplication
    : public Er::Program
{
public:
    TestApplication(int options) noexcept;

protected:
    void addLoggers(Er::Log::ITee* main) override;
    int run(int argc, char** argv) override;
};


} // Erp::Testing{}