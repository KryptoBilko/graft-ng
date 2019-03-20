
#define __GRAFTLET__
#include "lib/graft/GraftletRegistry.h"
#include "lib/graft/IGraftlet.h"
#include "lib/graft/ConfigIni.h"

#include<cassert>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "graftlet.TestGraftlet"

class TestGraftlet: public IGraftlet
{
public:
    TestGraftlet(const char* name) : IGraftlet(name) { }

    void testUndefined();

    int testInt1(int a) { return a; }
    int testInt2(int&& a, int b, int& c)
    {
        c = a + b;
        return c + a + b;
    }

    std::string testString1(std::string& s)
    {
        s = "testString1";
        return "res " + s;
    }

    std::string testString2(std::string&& srv, std::string slv, std::string& sr)
    {
        sr = srv + slv;
        return "res " + slv + srv + sr;
    }

    graft::Status testHandler(const graft::Router::vars_t& vars, const graft::Input& input, graft::Context& ctx, graft::Output& output)
    {
        std::string id;
        {
            auto it = vars.find("id");
            if(it != vars.end()) id = it->second;
        }
        output.body = input.data() + id;
        return graft::Status::Ok;
    }

    graft::Status testHandler1(const graft::Router::vars_t& vars, const graft::Input& input, graft::Context& ctx, graft::Output& output)
    {
        std::string id;
        {
            auto it = vars.find("id");
            if(it != vars.end()) id = it->second;
        }
        output.body = input.data() + id;
        return graft::Status::Ok;
    }

    static std::string value;
    static int count;

    std::string resetPeriodic(const std::string& val)
    {
        std::string res;
        res.swap(value);
        count = 0;
        value = val;
        return res;
    }

    graft::Status testPeriodic(const graft::Router::vars_t& vars, const graft::Input& input, graft::Context& ctx, graft::Output& output)
    {
        bool stop = value.empty();
        value = "count " + std::to_string(++count);
        return (stop)? graft::Status::Stop : graft::Status::Ok;
    }

    virtual void initOnce(const graft::CommonOpts& opts, graft::Context& ctx) override
    {
        if(!opts.config_filename.empty())
        {
            graft::ConfigIniSubtree config = graft::ConfigIniSubtree::create(opts.config_filename);
            ctx.global["graftlets.dirs"] = config.get<std::string>("graftlets.dirs");
        }
//        REGISTER_ACTION(TestGraftlet, testUndefined);
        REGISTER_ACTION(TestGraftlet, testInt1);
        REGISTER_ACTION(TestGraftlet, testInt2);
        REGISTER_ACTION(TestGraftlet, testString1);
        REGISTER_ACTION(TestGraftlet, testString2);

        REGISTER_ENDPOINT("/URI/test/{id:[0-9]+}", METHOD_GET | METHOD_POST, TestGraftlet, testHandler);
        REGISTER_ENDPOINT("/URI/test1/{id:[0-9]+}", METHOD_GET | METHOD_POST, TestGraftlet, testHandler1);

        REGISTER_ACTION(TestGraftlet, resetPeriodic);
        //Type, method, int interval_ms, int initial_interval_ms, double random_factor
        REGISTER_PERIODIC(TestGraftlet, testPeriodic, 100, 100, 0);
    }
};

GRAFTLET_EXPORTS_BEGIN("myGraftlet", GRAFTLET_MKVER(1,1));
GRAFTLET_PLUGIN(TestGraftlet, IGraftlet, "testGL");
GRAFTLET_EXPORTS_END

GRAFTLET_PLUGIN_DEFAULT_CHECK_FW_VERSION(GRAFTLET_MKVER(0,3))

std::string TestGraftlet::value;
int TestGraftlet::count = 0;

namespace
{

struct Informer
{
    Informer()
    {
        LOG_PRINT_L2("graftlet " << getGraftletName() << " loading");
    }
    ~Informer()
    {
        LOG_PRINT_L2("graftlet " << getGraftletName() << " unloading");
    }
};

Informer informer;

} //namespace


