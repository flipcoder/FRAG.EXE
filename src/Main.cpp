#include <iostream>
#include <memory>
#include <thread>
#include "Qor/kit/kit.h"
#include "Qor/Qor.h"
#include "Info.h"
#include "Game.h"
#include "Pregame.h"
#include "Qor/Texture.h"

#include "Qor/kit/log/log.h"
#include "Qor/kit/async/async.h"

#ifdef DEBUG
    #include <backward/backward.cpp>
#endif

using namespace std;
using namespace kit;

int main(int argc, const char** argv)
{
    Args args(argc, argv);
    args.set("mod","FRAG.EXE");
    args.set("title", "FRAG.EXE");
    args.set("no_loading_fade","true");
    
    Texture::DEFAULT_FLAGS = Texture::TRANS | Texture::MIPMAP | Texture::FILTER;
    
#ifndef DEBUG
    try{
#endif
        auto engine = kit::make_unique<Qor>(args);
        engine->states().register_class<Game>("game");
        engine->states().register_class<Pregame>("pregame");
        engine->run("pregame");
#ifndef DEBUG
    }catch(const Error&){
        // already logged
    }catch(const std::exception& e){
        LOGf("Uncaught exception: %s", e.what());
    }
#endif
    return 0;
}

