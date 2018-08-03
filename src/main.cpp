#include "server.h"
#include "backtrace.h"

namespace graft
{

std::terminate_handler prev_terminate = nullptr;

void terminate()
{
    std::cerr << "\nTerminate called, dump stack:\n";
    graft_bt();

    //dump exception info
    std::exception_ptr eptr = std::current_exception();
    if(eptr)
    {
        try
        {
             std::rethrow_exception(eptr);
        }
        catch(std::exception& ex)
        {
            std::cerr << "\nTerminate caused by exception : '" << ex.what() << "'\n";
        }
        catch(...)
        {
            std::cerr << "\nTerminate caused by unknown exception.\n";
        }
    }

    prev_terminate();
}

} //namespace graft

int main(int argc, const char** argv)
{
    graft::prev_terminate = std::set_terminate( graft::terminate );

    try
    {
        graft::GraftServer gserver;
        bool res = gserver.run(argc, argv);
        if(!res) return -2;
    } catch (const std::exception & e) {
        std::cerr << "Exception thrown: " << e.what() << std::endl;
        return -1;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
        return -1;
    }

    return 0;
}
