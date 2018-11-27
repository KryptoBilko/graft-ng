#include "walletnode/server.h"
#include "lib/graft/backtrace.h"
#include "lib/graft/graft_exception.h"

namespace graft
{

namespace walletnode
{

std::terminate_handler prev_terminate = nullptr;

// Unhandled exception in a handler with noexcept specifier causes
// termination of the program, stack backtrace is created upon the termination.
// The exception in a handler with no noexcept specifier doesn't effect
// the workflow, the error propagates back to the client.
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

} //namespace walletnode
} //namespace graft

int main(int argc, const char** argv)
{
    graft::walletnode::prev_terminate = std::set_terminate( graft::walletnode::terminate );

    try
    {
        //TODO: return result from server
        graft::walletnode::WalletServer server;
        bool res = server.run(argc, argv);
        if(!res) return -2;
    } catch (const std::exception & e) {
        std::cerr << "Exception thrown: " << e.what() << std::endl;
        return -1;
    } catch(...) {
        std::cerr << "Exception of unknown type!\n";
        return -1;
    }

    return 0;
}
