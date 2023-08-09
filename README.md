# mnamp
(LV2) Amplifier plugin based on gathered principles and rules of thumb more than any kind of direct emulation of anything

Compilation:
Just run make.

It will give a help text explaining the possible options, when run with no arguments.

If you ran make for a plugin target then the corresponding plugin is located in the build/plugin-name.lv2/ directory, ready to copied into your preferred LV2 plugin install directory (as a sub directory that is).
Except on the Windows platform (MSYS2 / MinGW) the manifest.ttl file has to be edited by replacing the reference to plugin-name.so within it with a reference to plugin-name.dll

The Makefile has now been tested on Windows (MSYS2 / MinGW) also.

One may need to provide the following flags in addition to other flags:

    make PLUGIN_TARGET_TYPE=.dll EXECUTABLE_TARGET_TYPE=.exe CXXFLAGS="-I/path/to/lv2/includes ..."


This is provided as is, and available here mostly for my own archiving purposes, so far.

There is currently only available the generic GUI controls possibly provided by the host application used. As far as hosts are considered I recommend a DAW which can plot an analysis of the plugin response, like Ardour does for example, to dial in the parameters more easily.


Control parameters (this section needs a bit of updating)
-----------------------------------------------------------

There is a gain control and factor for oversampling and two drive controls to blend polynomial functions.
This has been much simplified at least for the moment.

Now added the possibility of running multiple stages in one plugin instance, with slightly varying parameters.

