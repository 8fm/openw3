/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/


#pragma once

#include <AK/Comm/AkCommunication.h>

class IPConnectorPorts
{
public:
	// Ports to use for communication (initialy set to the
	// defaults in the AkCommSettings::Ports constructor,
	// but can be changed in comm initialization. See
	// AK::Comm::Init() in the Wwise SDK doc.
	static AkCommSettings::Ports Current;
};

