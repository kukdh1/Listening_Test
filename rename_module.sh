#!/bin/bash
install_name_tool -id @executable_path/../Frameworks/libxlsxwriter.dylib ./Frameworks/libxlsxwriter.dylib
install_name_tool -id @executable_path/../Frameworks/libfmod.dylib ./Frameworks/libfmod.dylib
install_name_tool -change /usr/lib/libxlsxwriter.dylib @executable_path/../Frameworks/libxlsxwriter.dylib ./MacOS/Listening_Test
install_name_tool -change @rpath/libfmod.dylib @executable_path/../Frameworks/libfmod.dylib ./MacOS/Listening_Test
otool -L ./MacOS/Listening_Test

