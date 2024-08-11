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

#include "GameSocket.h"
#include "ConsoleDiscoveryMessage.h"

class IChannelsHolder;

class DiscoveryChannel
{
public:
	DiscoveryChannel( IChannelsHolder* in_pHolder );
	~DiscoveryChannel();

	enum ResponseState
	{
		StateAvailable,
		StateBusy
	};

	bool Init();
	void Term();

	ResponseState GetResponseState() const;
	void SetResponseState( ResponseState in_eState );

	void SetControllerName( const char* in_pszControllerName );

	void Process();

private:
	DiscoveryResponse::ConsoleState ConvertStateToResponseType() const;

	char m_szComputerName[16];
	char m_szControllerName[128];

	ResponseState m_eState;

	GameSocket m_socket;

	IChannelsHolder* m_pHolder;
};
