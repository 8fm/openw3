#include "build.h"

#include "minimapManager.h"
#include "commonMapManager.h"
#include "r4HudModule.h"
#include "../../common/game/flashScriptSupport.h"

CMinimapManager::SFlashFuncImportDesc CMinimapManager::sm_flashFunctionImportTable[] =
{
	{ TXT("AddMapPin"),							&CMinimapManager::m_addMapPinHook },
	{ TXT("MoveMapPin"),						&CMinimapManager::m_moveMapPinHook },
	{ TXT("HighlightMapPin"),					&CMinimapManager::m_highlightMapPinHook },
	{ TXT("DeleteMapPin"),						&CMinimapManager::m_deleteMapPinHook },
	{ TXT("UpdateDistanceToHighlightedMapPin"),	&CMinimapManager::m_updateDistanceToHighlightedMapPinHook },
	{ TXT("AddWaypoint"),						&CMinimapManager::m_addWaypointHook },
	{ TXT("ClearWaypoints"),					&CMinimapManager::m_clearWaypointsHook },
};

CMinimapManager::CMinimapManager()
	: m_initialized( false )
	, m_currMinimapRadius( 1 )
{
}

CMinimapManager::~CMinimapManager()
{
}

void CMinimapManager::Initialize( const THandle< CR4HudModule >& minimapModuleHandle )
{
	CR4HudModule* minimapModule = minimapModuleHandle.Get();
	if ( !minimapModule )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( GUI ), TXT("CMinimapManager - cannot get minimap module") );
		return;
	}

	THandle< CScriptedFlashSprite > moduleFlashSpriteHandle = minimapModule->GetFlashSprite();
	CScriptedFlashSprite* moduleFlashSprite = moduleFlashSpriteHandle.Get();
	if ( !moduleFlashSprite )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( GUI ), TXT("CMinimapManager - cannot get minimap module sprite") );
		return;
	}

	size_t len = sizeof( sm_flashFunctionImportTable ) / sizeof( sm_flashFunctionImportTable[ 0 ] );
	for ( size_t i = 0; i < len; ++i )
	{
		if ( ! moduleFlashSprite->GetFlashObject( sm_flashFunctionImportTable[ i ].m_memberName, this->*sm_flashFunctionImportTable[ i ].m_flashFuncImport ) )
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( GUI ), TXT("CMinimapManager - cannot get function: %ls"), sm_flashFunctionImportTable[ i ].m_memberName );
		}
	}

	m_initialized = true;
}

void CMinimapManager::AddMapPin( const SCommonMapPinInstance& pin )
{
	if ( !m_initialized )
	{
		return;
	}

	m_addMapPinHook.InvokeSelf(			CFlashValue( pin.m_id ),
										GetStringFlashValue( pin.m_tag.AsString(), m_addMapPinHook ),
										GetStringFlashValue( pin.m_visibleType.AsString() + GetPinTypePostfix( pin ), m_addMapPinHook ),
										CFlashValue( pin.m_visibleRadius ),
										CFlashValue( pin.m_canBePointedByArrow ),
										CFlashValue( CCommonMapManager::GetPinRenderingPriority( pin.m_type ) ),
										CFlashValue( CCommonMapManager::IsQuestPinType( pin.m_type ) ),
										CFlashValue( CCommonMapManager::IsUserPinType( pin.m_type ) ),
										CFlashValue( pin.m_isHighlighted ) );
	m_moveMapPinHook.InvokeSelf(		CFlashValue( pin.m_id ),
										CFlashValue( pin.m_position.X ),
										CFlashValue( pin.m_position.Y ),
										CFlashValue( pin.m_position.Z ) );
}

void CMinimapManager::MoveMapPin( const SCommonMapPinInstance& pin )
{
	if ( !m_initialized )
	{
		return;
	}
	m_moveMapPinHook.InvokeSelf(	CFlashValue( pin.m_id ),
									CFlashValue( pin.m_position.X ),
									CFlashValue( pin.m_position.Y ),
									CFlashValue( pin.m_position.Z ) );
}

void CMinimapManager::HighlightMapPin( const SCommonMapPinInstance& pin )
{
	if ( !m_initialized )
	{
		return;
	}
	m_highlightMapPinHook.InvokeSelf(	CFlashValue( pin.m_id ),
										CFlashValue( pin.m_isHighlighted ) );
}

void CMinimapManager::UpdateMapPin( const SCommonMapPinInstance& pin )
{
	if ( pin.m_isAddedToMinimap )
	{
		DeleteMapPin( pin );
		AddMapPin( pin );
	}
}

void CMinimapManager::DeleteMapPin( const SCommonMapPinInstance& pin )
{
	if ( !m_initialized )
	{
		return;
	}
	m_deleteMapPinHook.InvokeSelf( CFlashValue( pin.m_id ) );
}

void CMinimapManager::UpdateDistanceToHighlightedMapPin( Float questDistance, Float userDistance )
{
	if ( !m_initialized )
	{
		return;
	}
	m_updateDistanceToHighlightedMapPinHook.InvokeSelf( CFlashValue( questDistance ),
														CFlashValue( userDistance ) );
}

void CMinimapManager::AddWaypoints( const TDynArray< Vector >& waypoints )
{
	PC_SCOPE_PIX( CMinimapManager_AddWaypoints );

	if ( !m_initialized )
	{
		return;
	}
	DeleteWaypoints( waypoints.Size() );
	for ( Uint32 i = 0, n = waypoints.Size(); i < n; ++i )
	{
		m_addWaypointHook.InvokeSelf(	CFlashValue( waypoints[ i ].X ),
										CFlashValue( waypoints[ i ].Y ) );
	}
}

void CMinimapManager::DeleteWaypoints( Uint32 startingFrom /*= 0*/ )
{
	if ( !m_initialized )
	{
		return;
	}
	m_clearWaypointsHook.InvokeSelf( CFlashValue( startingFrom ) );
}

void CMinimapManager::OnChangedMinimapRadius( Float radius )
{
	m_currMinimapRadius = radius;
}

Bool CMinimapManager::ShouldMapPinBeVisibleOnMinimap( const SCommonMapPinInstance& pin, const Vector& playerPos )
{
	if ( pin.m_canBePointedByArrow )
	{
		return true;
	}
	Float radius = m_currMinimapRadius * 1.25f;

	if ( pin.m_position.X > playerPos.X - radius &&
		 pin.m_position.X < playerPos.X + radius &&
		 pin.m_position.Y > playerPos.Y - radius &&
		 pin.m_position.Y < playerPos.Y + radius )
	{
		return true;
	}
	return false;
}

CFlashValue CMinimapManager::GetStringFlashValue( const String& stringArg, CFlashValue& functionFlashValue )
{
	CFlashValue flashValue;
	VERIFY( functionFlashValue.CreateString( stringArg, flashValue ) );
	return flashValue;
}

String CMinimapManager::GetPinTypePostfix( const SCommonMapPinInstance& pin )
{
	if ( pin.m_alternateVersion > 0 && !pin.m_isDisabled )
	{
		return String::Printf( TXT("_%d"), pin.m_alternateVersion );
	}
	return String::EMPTY;
}