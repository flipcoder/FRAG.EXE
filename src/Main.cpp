#include <iostream>
#include <memory>
#include <thread>
#include "kit/kit.h"
#include "Qor/Qor.h"
#include "Info.h"
#include "Qor/Texture.h"

#include "kit/log/log.h"
#include "kit/async/async.h"

#ifdef DEBUG
    #include <backward/backward.cpp>
#endif

#include "Game.h"
#include "MenuScreen.h"
#include "Pregame.h"

using namespace std;
using namespace kit;

int main(int argc, char* argv[])
{
    Args args(argc, (const char**)argv);
    unsigned args_len = args.size();
    args.set("mod","FRAG.EXE");
    args.set("title", "FRAG.EXE");
    args.set("no_loading_fade","true");
    
    Texture::DEFAULT_FLAGS = Texture::TRANS | Texture::MIPMAP | Texture::FILTER;
    
#ifndef DEBUG
    try{
#endif
        auto engine = kit::make_unique<Qor>(args, Info::Program);
        engine->states().register_class<Game>("game");
        engine->states().register_class<Pregame>("pregame");
        engine->states().register_class<MenuScreen>("menu");

        if(args_len > 1)
            engine->run("pregame");
        else
            engine->run("menu");
#ifndef DEBUG
    }catch(const Error&){
        // already logged
    }catch(const std::exception& e){
        LOGf("Uncaught exception: %s", e.what());
    }
#endif
    return 0;
}

