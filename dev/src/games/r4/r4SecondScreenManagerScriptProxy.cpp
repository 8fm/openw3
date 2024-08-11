/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "r4SecondScreenManagerScriptProxy.h"
#include "commonMapManager.h"

RED_DEFINE_STATIC_NAME( FastTravelGlobal );
RED_DEFINE_STATIC_NAME( FastTravelLocal );

IMPLEMENT_ENGINE_CLASS( CR4SecondScreenManagerScriptProxy );

void CR4SecondScreenManagerScriptProxy::funcSendGlobalMapPins( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< SCommonMapPinInstance >, mappins, TDynArray< SCommonMapPinInstance >() );
	FINISH_PARAMETERS;
#ifndef NO_SECOND_SCREEN 
	TDynArray< SSecondScreenMapPin > globalMapPins;
	SSecondScreenMapPin globalPin;
	TDynArray< SCommonMapPinInstance >::const_iterator iter = mappins.Begin();
	while( iter != mappins.End() )
	{
		globalPin.m_id				= iter->m_id;
		globalPin.m_tag				= iter->m_tag;
		globalPin.m_type			= iter->m_type;
		globalPin.m_position		= iter->m_position;
		globalPin.m_isDiscovered	= iter->m_isDiscovered;
		globalMapPins.PushBack( globalPin );
		++iter;
	}
	SCSecondScreenManager::GetInstance().SendGlobalStaticMapPins( globalMapPins );
#endif
}

void CR4SecondScreenManagerScriptProxy::funcSendAreaMapPins( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, areaType, 0 );
	GET_PARAMETER( TDynArray< SCommonMapPinInstance >, mappins, TDynArray< SCommonMapPinInstance >() );
	FINISH_PARAMETERS;
#ifndef NO_SECOND_SCREEN 
	TDynArray< SSecondScreenMapPin > areaMapPins;
	SSecondScreenMapPin areaPin;
	TDynArray< SCommonMapPinInstance >::const_iterator iter = mappins.Begin();
	while( iter != mappins.End() )
	{
		areaPin.m_id			= iter->m_id;
		areaPin.m_tag			= iter->m_tag;
		areaPin.m_type			= iter->m_type;
		areaPin.m_position		= iter->m_position;
		areaPin.m_isDiscovered	= iter->m_isDiscovered;
		areaMapPins.PushBack( areaPin );
		++iter;
	}
	SCSecondScreenManager::GetInstance().SendAreaStaticMapPins( areaType, areaMapPins );
#endif
}

void CR4SecondScreenManagerScriptProxy::funcSendGameMenuOpen( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	//! DEPRECATED
}

void CR4SecondScreenManagerScriptProxy::funcSendGameMenuClose( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
#ifndef NO_SECOND_SCREEN 
	SCSecondScreenManager::GetInstance().SendState( CSecondScreenManager::GS_GAME_SESSION );
#endif
}

void CR4SecondScreenManagerScriptProxy::funcSendFastTravelEnable( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
#ifndef NO_SECOND_SCREEN 
	SCSecondScreenManager::GetInstance().SendFastTravelEnable();
#endif
}

void CR4SecondScreenManagerScriptProxy::funcSendFastTravelDisable( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
#ifndef NO_SECOND_SCREEN 
	SCSecondScreenManager::GetInstance().SendFastTravelDisable();
#endif
}

void CR4SecondScreenManagerScriptProxy::funcPrintJsonObjectsMemoryUsage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	Int64 allocatedBytesPerMemoryMC_Json = Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_Default >( MC_Json );

	RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Memory used by Json objects: %lld." ), allocatedBytesPerMemoryMC_Json  );
#endif
}

#ifndef NO_SECOND_SCREEN 
void CR4SecondScreenManagerScriptProxy::OnHandleFastTravel( const CName mapPinTag, Uint32 areaType, Bool currentWorld )
{
	if(currentWorld)
	{
		CallFunction( this, CNAME( FastTravelLocal ), mapPinTag );
	}
	else
	{		
		CallFunction( this, CNAME( FastTravelGlobal ), areaType, mapPinTag );
	}

}

void CR4SecondScreenManagerScriptProxy::OnHandleTrackQuest( const CGUID questQuid )
{
	CWitcherJournalManager* manager = GCommonGame->GetSystem< CWitcherJournalManager >();

	manager->TrackQuest( questQuid, true );
}

Bool CR4SecondScreenManagerScriptProxy::IsGUIActive()
{
	const CCommonGame* commonGame = GCommonGame;
	const CGame* game = GGame;
	const CPlayer* player;
	
	if( game && commonGame )
	{
		player = commonGame->GetPlayer();

		if( player )
		{
			bool isActive = game->IsActive();
			if( isActive )
			{			
				if( commonGame->IsLoadingScreenShown() )
				{
					return false;
				}
				if( player->IsInNonGameplayScene() )
				{
					return false;
				}

				return true;
			}	
		}
	}
	return false;
}

#endif //! NO_SECOND_SCREEN
