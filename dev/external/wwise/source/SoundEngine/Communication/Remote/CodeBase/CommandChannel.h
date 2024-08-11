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

#include "IncomingChannel.h"

namespace AK
{
	namespace Comm
	{
		class ICommandChannelHandler;
	}
}

class IChannelsHolder;

struct CommandMessage;

class CommandChannel : public IncomingChannel
{
public:
	CommandChannel( IChannelsHolder* in_pHolder, AkMemPoolId in_pool, const AkThreadProperties& in_threadProperties );
	virtual ~CommandChannel();

	void SetCommandChannelHandler( AK::Comm::ICommandChannelHandler* in_pCmdChannelHandler );

protected:
	virtual bool ProcessCommand( AkUInt8* in_pData, AkUInt32 in_uDataLength );
	virtual void PeerConnected( const GameSocketAddr& in_rControllerAddr );
	virtual void PeerDisconnected();
	virtual const char* GetRequestedPortName();
	virtual bool RequiresAccumulator(){ return true; }

private:
	AK::Comm::ICommandChannelHandler* m_pCmdChannelHandler;
	IChannelsHolder* m_pHolder;

	DECLARE_BASECLASS( IncomingChannel );
};
