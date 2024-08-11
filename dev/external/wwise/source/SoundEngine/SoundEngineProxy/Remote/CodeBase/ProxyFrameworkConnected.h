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

#ifndef AK_OPTIMIZED

#include "IProxyFrameworkConnected.h"
#include "ICommandChannelHandler.h"
#include "ICommunicationCentralNotifyHandler.h"
#include "RendererProxyConnected.h"
#include "StateMgrProxyConnected.h"
#include "ALMonitorProxyConnected.h"
#include "ObjectProxyConnected.h"
#include "CommandDataSerializer.h"

#include "AkHashList.h"
#include <AK/Tools/Common/AkObject.h>
#include "AkPrivateTypes.h"
#include "AkAction.h"

class CommandDataSerializer;

class ProxyFrameworkConnected
	: public AK::Comm::IProxyFrameworkConnected
{
public:
	ProxyFrameworkConnected( AkMemPoolId in_pool );
	virtual ~ProxyFrameworkConnected();

	// IProxyFramework members
	virtual void Destroy();

	virtual void Init();
	virtual void Term();

	virtual void SetNotificationChannel( AK::Comm::INotificationChannel* in_pNotificationChannel );
	
	virtual AK::Comm::IRendererProxy* GetRendererProxy();

	// ICommandChannelHandler members
	virtual const AkUInt8* HandleExecute( const AkUInt8* in_pData, AkUInt32 & out_uReturnDataSize );

	// ICommunicationCentralNotifyHandler members
	virtual void PeerDisconnected();


	typedef AkHashList< AkUInt32, ObjectProxyConnectedWrapper, 31 > ID2ProxyConnected;
private:
	void ProcessProxyStoreCommand( CommandDataSerializer& io_rSerializer );
	ID2ProxyConnected::Item * CreateAction( AkUniqueID in_actionID, AkActionType in_eActionType );

	ID2ProxyConnected m_id2ProxyConnected;

	RendererProxyConnected m_rendererProxy;
	StateMgrProxyConnected m_stateProxy;
	ALMonitorProxyConnected m_monitorProxy;

    CommandDataSerializer m_returnData;//Making it a member, avoiding millions of allocations.

	AkMemPoolId m_pool;
};

namespace ObjectProxyStoreCommandData
{
	struct Create;
}

typedef void (*AkExternalProxyHandlerCallback)( ObjectProxyStoreCommandData::Create& in_Create, ProxyFrameworkConnected::ID2ProxyConnected::Item *& out_pProxyItem, const long in_lProxyItemOffset, AkMemPoolId in_PoolID );
#endif // #ifndef AK_OPTIMIZED
