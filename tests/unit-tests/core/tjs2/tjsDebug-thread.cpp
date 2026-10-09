#include <catch2/catch_test_macros.hpp>
#include "tjsCommHead.h"
#include "tjsDebug.h"
#include "tjsGlobalStringMap.h"
#include "tjs.h"
#include "tjsObject.h"
#include <functional>
#include <thread>
#include <array>
#include <atomic>

namespace {
class Probe final : public tTJSDispatch {
public:
    std::function<void()> run;
    explicit Probe(std::function<void()> callback) : run(std::move(callback)) {}
    tjs_error FuncCall(tjs_uint32, const tjs_char *, tjs_uint32 *,
                      tTJSVariant *, tjs_int, tTJSVariant **,
                      iTJSDispatch2 *) override {
        run();
        return TJS_S_OK;
    }
};
}

TEST_CASE("TJS worker diagnostics do not read another thread's script stack") {
    const bool oldDebug = TJSEnableDebugMode;
    TJSEnableDebugMode = true;
    auto *engine = new tTJS;
    bool mainHasTrace = false, workerHasTrace = true;
    {
        auto *probe = new Probe([&] {
            mainHasTrace = !TJSGetStackTraceString().IsEmpty();
            std::thread worker([&] {
                TJSAddRefStackTracer();
                // An audio decoder has no active game-script frame. It must
                // not follow pointers into the main thread's execution stack.
                workerHasTrace = !TJSGetStackTraceString().IsEmpty();
                TJSReleaseStackTracer();
            });
            worker.join();
        });
        tTJSVariant value(probe);
        probe->Release();
        auto *global = engine->GetGlobalNoAddRef();
        global->PropSet(TJS_MEMBERENSURE, TJS_W("probe"), nullptr,
                        &value, global);
        engine->ExecScript(TJS_W("probe();"), nullptr, nullptr,
                           TJS_W("main-thread.tjs"));
    }
    engine->Release();
    TJSEnableDebugMode = oldDebug;
    REQUIRE(mainHasTrace);
    REQUIRE_FALSE(workerHasTrace);
}

TEST_CASE("TJS object diagnostics survive concurrent decoder bookkeeping") {
    TJSAddRefObjectHashMap();
    TJSAddRefGlobalStringMap();
    std::atomic<bool> correct{true};
    std::array<std::thread, 4> workers;
    for(auto &worker : workers) {
        worker = std::thread([&] {
            int marker;
            for(int i = 0; i < 500; ++i) {
                TJSAddObjectHashRecord(&marker);
                TJSObjectHashSetType(&marker, ttstr(TJS_W("decoder-object")));
                if(TJSGetObjectTypeInfo(&marker) != TJS_W("decoder-object"))
                    correct = false;
                TJSRemoveObjectHashRecord(&marker);
            }
        });
    }
    for(auto &worker : workers)
        worker.join();
    TJSReleaseGlobalStringMap();
    TJSReleaseObjectHashMap();
    REQUIRE(correct.load());
}
