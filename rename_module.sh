#!/bin/bash
install_name_tool -change /usr/local/Cellar/ffmpeg/3.2/lib/libswresample.2.dylib @executable_path/../Frameworks/libswresample.2.dylib ./Frameworks/libavcodec.57.dylib
install_name_tool -change /usr/local/Cellar/ffmpeg/3.2/lib/libavutil.55.dylib @executable_path/../Frameworks/libavutil.55.dylib ./Frameworks/libavcodec.57.dylib

install_name_tool -change /usr/local/Cellar/ffmpeg/3.2/lib/libswresample.2.dylib @executable_path/../Frameworks/libswresample.2.dylib ./Frameworks/libavformat.57.dylib
install_name_tool -change /usr/local/Cellar/ffmpeg/3.2/lib/libavutil.55.dylib @executable_path/../Frameworks/libavutil.55.dylib ./Frameworks/libavformat.57.dylib
install_name_tool -change /usr/local/Cellar/ffmpeg/3.2/lib/libavcodec.57.dylib @exeuctable_path/../Frameworks/libavcodec.57.dylib ./Frameworks/libavformat.57.dylib

install_name_tool -change /usr/local/Cellar/ffmpeg/3.2/lib/libavutil.55.dylib @executable_path/../Frameworks/libavutil.55.dylib ./Frameworks/libswresample.2.dylib

