#include "daScript/daScript.h"

using namespace das;

#define TUTORIAL_NAME   "/scripts/dasStackWalk/stackwalk.das"

void tutorial () {
    TextPrinter tout;                               // output stream for all compiler messages (stdout. for stringstream use TextWriter)
    ModuleGroup dummyLibGroup;                      // module group for compiled program
    CodeOfPolicies policies;                        // compiler setup
    auto fAccess = make_smart<FsFileAccess>();      // default file access
#ifdef AOT
    policies.aot = true;
#endif
    // compile program
    auto program = compileDaScript(getDasRoot() + TUTORIAL_NAME, fAccess, tout, dummyLibGroup, false, policies);
    if ( program->failed() ) {
        // if compilation failed, report errors
        tout << "failed to compile\n";
        for ( auto & err : program->errors ) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
        }
        return;
    }
    // create daScript context
    Context ctx(program->getContextStackSize());
    if ( !program->simulate(ctx, tout) ) {
        // if interpretation failed, report errors
        tout << "failed to simulate\n";
        for ( auto & err : program->errors ) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
        }
        return;
    }
    // find function 'main' in the context
    auto fnTest = ctx.findFunction("main");
    if ( !fnTest ) {
        tout << "function 'main' not found\n";
        return;
    }
    // verify if 'main' is a function, with the correct signature
    // note, this operation is slow, so don't do it every time for every call
    if ( !verifyCall<void>(fnTest->debugInfo, dummyLibGroup) ) {
        tout << "function 'main', call arguments do not match. expecting def main : void\n";
        return;
    }
    // evaluate 'main' function in the context
    ctx.evalWithCatch(fnTest, nullptr);
    if ( auto ex = ctx.getException() ) {       // if function cased panic, report it
        tout << "exception: " << ex << "\n";
        return;
    }
}

int main(int argc, char * argv[])
{
    NEED_ALL_DEFAULT_MODULES;
    Module::Initialize();
    // run the tutorial
    tutorial();
    // shut-down daScript, free all memory
    Module::Shutdown();
    return 0;
}