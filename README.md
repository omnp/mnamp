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

Some notes about some of the current control parameters
-----------------------------------------------------------

First of all *blend* is the dry/wet mix slider (linear).
To hear any difference from the input increase this control.

Then *level* is output level multiplier (dB). Careful with this one of course.

The pair of *drive 1* and *drive 2* control a post-applied-shaping-function asymmetrical multiplier.

Then the pair of *drive 3* and *drive 4* control a pre-applied-shaping function asymmetrical multiplier.

Those two pairs control the positive and negative part within each pair respectively.

Then there are the filter controls for *high pass* and *low pass* filters used. *Cutoff* and *Q* for each filter.

*Power* and *even power* control how many terms to construct for polynomials during the processing.

*Mode* selects between the different implemented soft-shaping functions. You'll need to look at the code for a better explanation at the moment, sorry. But in short the functions (those currently enabled) compute the inverse of what they are called, so for example *Exponential* is the sign-of-x-times-log(absolute-value-of-x plus one), to put it mildly.

*Factor* is the internal oversampling factor.

*Offset* is not really an offset but input + input*offset. May remove this one in the future.

*Even blend* and *even divider* are parameters for added harmonics processing the mode of which is set by the parameter named *post harmonics*.

Finally,

The input multiplier *gain* goes to 100 and is also in dB.

How was it tested or how did I use it so far?
---------------------------------------------

Ardour is the DAW that has been used in testing (on Windows and Linux). This plugin was run in several instances with some Ardour-default delays, eqs, and reverbs (and possibly compressor).

For example one instance of this plugin followed by a delay followed by another instance of this plugin follow by eq followed by reverb.

This plugin may be a bit heavy on the cpu to run in a project above 48kHz. Most testing was done in 48k float.
