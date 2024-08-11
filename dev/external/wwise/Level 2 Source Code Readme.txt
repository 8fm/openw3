1.0 - Audiokinetic Level 2 Source Code
======================================

This package, when combined with the Wwise SDK, contains the complete source
code of Audiokinetic's Wwise sound engine and default plug-ins. Additionally,
on Windows, sources for SoundFrame libraries are also included.

Using it, you will be able to trace through the SDK's code and rebuild all
of the sound engine and default plug-ins libraries.

Refer to the attached "SOURCE CODE ADDENDUM" for licensing information.

Note: Sources for add-ons are not included. Add-ons include, but may not be
limited to: Convolution Reverb, SoundSeed, GenAudio, Crankcase Audio and McDSP.

2.0 - Requirements
==================

Refer to "Reference Materials > Platform Requirements" in the Wwise SDK Help
for basic requirements such as compiler and SDK versions and setup for the various
platforms, including adding DirectX include/lib paths to Visual Studio.

3.0 - Instructions
==================

Extract the contents of the "SDK" folder into the "SDK" folder in your
Wwise installation.

3.1 - For Windows-based development
-----------------------------------
Visual Studio solutions or makefiles (depending on the target platform)
are available in the "SDK/source/SoundEngine" subfolder to build the
sound engine and default plug-ins.

Wii: Please refer to "Wii-Specific Information > Building Sample Code for the
Wii" in the Wii-specific Wwise SDK help file for information on using the
provided makefile.

3.2 - For Mac-based development
-------------------------------
Xcode projects are available in the "SDK/source/SoundEngine" subfolder
to build the sound engine and default plug-ins.
