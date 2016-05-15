#!/bin/bash
ccache x86_64-w64-mingw32-g++ --pipe --std=c++11 -m32 \
    -include cfloat -include cmath -D QOR_NO_PYTHON \
    -include SDL2/SDL.h \
    -include SDL2/SDL_main.h \
    $(find -L ../src | grep .cpp$ | \
    grep -v Qor/Main | \
    grep -v Qor/Info | \
    grep -v Qor/tests | \
    grep -v Qor/Demo | \
    grep -v Qor/addons) \
    -I/usr/x86_64-w64-mingw32/include/ \
    -I/usr/x86_64-w64-mingw32/include/raknet/DependentExtensions \
    -I/usr/x86_64-w64-mingw32/include/jsoncpp/ \
    `pkg-config --cflags glib-2.0` \
    `pkg-config --cflags cairomm-1.0 pangomm-1.4` \
    -I/usr/x86_64-w64-mingw32/include/python27 \
    -mwindows \
    -L/usr/x86_64-w64-mingw32/lib/ \
    -L`pkg-config --libs cairomm-1.0 pangomm-1.4` \
    -lmingw32 -lfreeimage -lopengl32 -lSDL2main -lws2_32 -lpthread -lSDL2 \
    -lglew32 -logg -lvorbis -lvorbisfile -lboost_system-mt \
    -lboost_thread-mt -lboost_filesystem-mt -lboost_coroutine-mt \
    -lboost_regex-mt -ljsoncpp -lBulletSoftBody -lBulletDynamics -lBulletCollision \
    -lLinearMath -lz -lassimp -lRakNetDLL -lOpenAL32 -lalut
    
    #-DWINVER=0x0400 -D__WIN95__ -D__GNUWIN32__ -DSTRICT \
    #-DHAVE_W32API_H -D__WXMSW__ -D__WINDOWS__ \
