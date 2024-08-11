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


#include "stdafx.h"

#include "ALBytesMem.h"

#include <AK/Comm/AkCommunication.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include "CommunicationCentral.h"

#include "ProxyFrameworkFactory.h"
#include "IProxyFrameworkConnected.h"
#include "IPConnectorPorts.h"

#include "AkPoolSizes.h"

extern CAkLock g_csMain;

class ICommunicationCentral;

#ifdef AK_WII
	#define COMM_POOL_ATTRIBUTE		(AkMallocMEM2)
#elif defined (AK_3DS) 
	#define COMM_POOL_ATTRIBUTE		(AkMallocDevice)
#else
	#define COMM_POOL_ATTRIBUTE		(AkMalloc)
#endif

#ifndef AK_OPTIMIZED
	extern AK::Comm::ICommunicationCentral * g_pCommCentral;
#endif


namespace AK
{
	namespace Comm
	{
#ifndef AK_OPTIMIZED
		static AkMemPoolId							s_pool = AK_INVALID_POOL_ID;
		static AK::Comm::IProxyFrameworkConnected *	s_pProxyFramework = NULL;
		static AkCommSettings s_settings;
#endif
		// Communication module public interface.

		AKRESULT Init( const AkCommSettings	& in_settings )
		{
#ifndef AK_OPTIMIZED
			if ( 0 == in_settings.uPoolSize )
			{
				// No memory was granted to communication.
				AKASSERT( !"in_settings.uPoolSize cannot be 0!" );
				return AK_InvalidParameter;
			}
#ifndef AK_3DS // On the 3DS, the port 0 is legit and we cannot prevent it.
			if ( 0 == in_settings.ports.uDiscoveryBroadcast )
			{
				// uDiscoveryBroadcast cannot be dynamic
				AKASSERT( !"in_settings.ports.uDiscoveryBroadcast cannot be 0 (cannot be dynamic/ephemeral)!" );
				return AK_InvalidParameter;
			}
#endif
#if defined AK_WII_FAMILY
			if ( 0 == in_settings.ports.uCommand
				|| 0 == in_settings.ports.uNotification )
			{
				AKASSERT( !"On the Wii, in_settings.ports.uCommand and in_settings.ports.uNotification cannot be 0 (cannot be dynamic/ephemeral)!" );
				return AK_InvalidParameter;
			}
#endif // AK_WII

			if ( 0 != in_settings.ports.uCommand
				&& ( in_settings.ports.uCommand == in_settings.ports.uDiscoveryBroadcast
					|| in_settings.ports.uCommand == in_settings.ports.uNotification ) )
			{
				AKASSERT( !"in_settings.ports.uCommand must either be 0 (dynamic/ephemeral) or be different from all other ports in in_settings.ports!" );
				return AK_InvalidParameter;
			}
			
			if ( 0 != in_settings.ports.uNotification
				&& ( in_settings.ports.uNotification == in_settings.ports.uDiscoveryBroadcast
					|| in_settings.ports.uNotification == in_settings.ports.uCommand ) )
			{
				AKASSERT( !"in_settings.ports.uNotification must either be 0 (dynamic/ephemeral) or be different from all other ports in in_settings.ports!" );
				return AK_InvalidParameter;
			}
			
			// Set communication ports
			IPConnectorPorts::Current = in_settings.ports;

#ifdef AK_3DS
			//On 3DS, ports must be a serie of 3 ports in a row, prevent user errors bu overriding them.
			IPConnectorPorts::Current.uCommand			= IPConnectorPorts::Current.uDiscoveryBroadcast+1;
			IPConnectorPorts::Current.uNotification		= IPConnectorPorts::Current.uDiscoveryBroadcast+2;
#endif

			// Create the communications memory pool
			s_pool = AK::MemoryMgr::CreatePool(
				NULL, 
				in_settings.uPoolSize, 
				COMM_POOL_BLOCK_SIZE, 
				COMM_POOL_ATTRIBUTE );
			if ( AK_INVALID_POOL_ID == s_pool )
			{
				AKASSERT( !"Failed creating pool for communication" );
				return AK_Fail;
			}
			AK_SETPOOLNAME( s_pool, AKTEXT("Communication") );

			// Create and initialize the Proxy Framework and Communication Central
			s_pProxyFramework = AkCreateProxyFramework( s_pool );
			AKASSERT( s_pProxyFramework || !"Failed creating proxy framework" );
			
			AK::ALWriteBytesMem::SetMemPool( s_pool );
#ifdef AK_3DS
			g_pCommCentral = AkNew( s_pool, CommunicationCentral( s_pool, in_settings.threadProperties ) );
#else
			g_pCommCentral = AkNew( s_pool, CommunicationCentral( s_pool, AkThreadProperties() /* unused */ ) );
#endif
			if ( !g_pCommCentral )
				return AK_InsufficientMemory;

			if ( ! g_pCommCentral->Init( s_pProxyFramework, s_pProxyFramework, in_settings.bInitSystemLib ) )
				return AK_Fail;

			s_pProxyFramework->Init();
			s_pProxyFramework->SetNotificationChannel( g_pCommCentral->GetNotificationChannel()	);

			s_settings = in_settings;
			
			return AK_Success;
#else
			return AK_Fail;
#endif
		}

		void GetDefaultInitSettings(
            AkCommSettings &	out_settings	///< Returned default platform-independent comm settings
		    )
		{
			out_settings.uPoolSize = COMM_POOL_SIZE;

			// The AkCommSettings::Ports structure has a constructor that
			// initializes it to the default values
			out_settings.ports = AkCommSettings::Ports();
#ifdef AK_3DS
			AKPLATFORM::AkGetDefaultThreadProperties(out_settings.threadProperties);
			out_settings.threadProperties.nPriority = AK_THREAD_PRIORITY_ABOVE_NORMAL;
#endif

			out_settings.bInitSystemLib = true;
		}

		void Term()
		{
#ifndef AK_OPTIMIZED
			AK::SoundEngine::StopProfilerCapture();
			
			g_csMain.Lock();
			if ( g_pCommCentral )
			{
				//This will terminate the thread that is using s_pProxyFramework first.
				g_pCommCentral->PreTerm();
			}
			// Destroy the Proxy Framework
			if ( s_pProxyFramework )
			{
				s_pProxyFramework->Term();
				s_pProxyFramework->Destroy();
				s_pProxyFramework = NULL;
			}

			// Destroy the Communication Central
			if ( g_pCommCentral )
			{
				g_pCommCentral->Term();
				g_pCommCentral->Destroy();
				g_pCommCentral = NULL;
			}

			g_csMain.Unlock();

			// Destroy Communication Memory Pools
			if ( s_pool != AK_INVALID_POOL_ID )
			{
                AKASSERT( AK::MemoryMgr::IsInitialized() );
				AK::MemoryMgr::DestroyPool( s_pool );
			}
#endif
		}
		
		AKRESULT Reset()
		{
#ifndef AK_OPTIMIZED
			Term();
			return Init(s_settings);
#else
			return AK_Fail;
#endif // #ifndef AK_OPTIMIZED
		}
	}
}
