# mnamp
(LV2) Amplifier plugin based on gathered principles and rules of thumb more than any kind of direct emulation of anything

Compile (Clang on Windows 11):

\path\to\clang++.exe --verbose -Wall -shared -I \path\to\lv2\ -std=c++20 .\mnamp.cc -o mnamp.dll -mtune=native -O3 -Wno-unused-function -Wpedantic

Additionally on Windows change the references to mnamp.so into mnamp.dll in the two .ttl files.

Compile (GNU on Linux):

g++ -Wall -shared -I /path/to/lv2 -std=c++20 mnamp.cc -o mnamp.so -mtune=native -O3 -Wno-unused-function -Wpedantic -fPIC

Install (All platforms):

Copy the plugin .so or .dll and its associated .ttl files manifest.ttl and mnamp.ttl into to an appropriately named subdirectory of wherever your local (user) lv2 plugin install directory is located.

This is provided as is, and available here mostly for my own archiving purposes, so far.

There is currently only available the generic GUI controls possibly provided by the host application used.

Control parameters
-----------------------------------------------------------

There is a gain control and factor for oversampling and two drive controls to blend polynomial functions.

