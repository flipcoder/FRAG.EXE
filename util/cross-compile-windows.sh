#!/bin/bash
i686-w64-mingw32-g++ --pipe --std=c++11 \
    $(find -L ../src | grep .cpp$ | \
    grep -v Qor/Main | \
    grep -v Qor/Info | \
    grep -v Qor/tests | \
    grep -v Qor/Demo | \
    grep -v Qor/addons) \
    -I/usr/i686-w64-mingw32/include/raknet/DependentExtensions \
    -I/usr/i686-w64-mingw32/include/jsoncpp/ \
    `pkg-config --cflags cairomm-1.0 pangomm-1.4` \
    -I/usr/i686-w64-mingw32/include/python27 \
    -lws2_32i -mwindows \
    -L`pkg-config --libs cairomm-1.0 pangomm-1.4` \
    -L`python2-config --libs`
    
    #-DWINVER=0x0400 -D__WIN95__ -D__GNUWIN32__ -DSTRICT \
    #-DHAVE_W32API_H -D__WXMSW__ -D__WINDOWS__ \
