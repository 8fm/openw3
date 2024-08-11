
(C) 2007-2013 Umbra Software Ltd.
All Rights Reserved.

This package consists of unpublished, proprietary source code of Umbra Software
Ltd., and is considered Confidential Information for purposes of non-disclosure
agreement. Disclosure outside the terms outlined in signed agreement may result
in irrepairable harm to Umbra Software Ltd. and legal action against the party
in breach.

Umbra 3 - rendering optimization middleware
===========================================

This package contains Umbra 3, the new and improved rendering optimization
middleware from Umbra Software Ltd.


1. Overview
-----------

Umbra 3 is an occlusion culling middleware. Its purpose is to
increase frame rate and ease game development. Umbra 3 does this
by optimizing critical parts of a game such as rendering and
by providing tools to help with content streaming and game logic.

Umbra 3 builds an internal representation of a game scene and uses this data
at runtime to perform efficient queries that can be used to e.g. determine the
set of visible objects for the player or determine the set of objects that are
within a given distance from a point.


2. Package contents
-------------------

The contents of the package are in the following subdirectories:

bin/
- Contains the executables and dynamic libraries

build/
- Contains the tutorial project file

doc/
- The Umbra 3 Quick Start Guide is available here in:
  English, Japanese, Standard Chinese, Simplified Chinese and Korean
- Features the Umbra 3 Users Guide in both HTML and CHM formats

lib/
- Contains static and import libraries

interface/
- Contains the Umbra 3 interface header files

samples/
- Contains the tutorial sample application source code

README.TXT
- This file

doc/ChangeLog
- Change log lists the changes in each new version of Umbra 3


3. Getting Started
------------------

To get started, please read through the "Umbra 3 Evaluation Guide".

This seven-step document gives you a quick overview on how to
best get started with Umbra 3 and how to use the Viewer program.

After you have gone through the Evaluation Guide and the Viewer,
please refer to the "tutorial" example application. It demonstrates the
simplest use case of the Umbra API, including the generation of a simple
scene, visibility computation for it and the runtime API usage.

For more information, either refer to the Umbra 3 API reference found in
the doc/ directory or go online to http://support.umbrasoftware.com to
read the online documentation.


4. License key
-----------------

Umbra 3 optimizer needs a valid license key to work. Please contact
sales@umbrasoftware.com to obtain a valid license key or to extend your
evaluation period.

The license key is sent separately from the SDK code drop. Copy the license
file umbra_license.txt to application's working directory. For running the
tutorial, copy umbra_license.txt to build\vs2008. For running viewer and
command line application from the  evaluation package, copy
umbra_license.txt to bin\win32 and bin\win64.

You can also set the license key using Umbra::Task::setLicenseKey() if you
are using the Task computation API. If you are using the Builder computation
API you can set the license key in the PlatformServices constructor and
pass the PlatformServices instance to the Builder constructor. When passing
the license key through the API functions, use the entire text string
contained in the umbra_license.txt file.

If umbra_license.txt is expired, invalid or not found, umbra computation task
will generate error ERROR_EVALUATION_EXPIRED.


5. Toolchain versions
---------------------

Win32 and Win64 binaries in the package have been built with Microsoft Visual
Studio 2008 Version 9.30729.1 SP. The packages have been confirmed to be working
with both Visual Studio 2008 and 2010.

Both release and debug static libraries contain the debugging information
embedded within the library using the C7 Compatible format.

The binaries have been linked against static C runtime (compiler option /MT).

Provided Windows runtime binaries have been confirmed to work with Windows XP,
Windows Vista and Windows 7.

For the static libraries to work with Visual Studio 2005, install the following
hotfix:

http://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=18623


6. Contact Information
----------------------

For technical issues and inquiries, please contact:
support@umbrasoftware.com

For licensing and evaluation inquiries, please contact:
sales@umbrasoftware.com
