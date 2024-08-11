#include "build.h"
#include "r4DLCWorldMounter.h"
#include "../../common/core/depot.h"
#include "commonMapManager.h"

IMPLEMENT_ENGINE_CLASS( CR4WorldDLCExtender );
IMPLEMENT_ENGINE_CLASS( CR4WorldDescriptionDLC );
IMPLEMENT_ENGINE_CLASS( CR4WorldDLCMounter );

RED_DEFINE_STATIC_NAME( EAreaName );

CR4WorldDLCExtender::CR4WorldDLCExtender()
{

}

void CR4WorldDLCExtender::funcAreaTypeToName( CScriptStackFrame& stack, void* result )
{
	String areaName;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	if( m_areaTypeToName.KeyExist( areaType ) )
	{
		areaName = m_areaTypeToName[areaType];
	}
	RETURN_STRING( areaName );
}

void CR4WorldDLCExtender::funcAreaNameToType( CScriptStackFrame& stack, void* result )
{
	Int32 areaType = 0;
	GET_PARAMETER( String, areaName, 0 );
	FINISH_PARAMETERS;
	if( m_areaNameToType.KeyExist( areaName ) )
	{
		areaType = m_areaNameToType[areaName];
	}
	RETURN_INT( areaType );
}

void CR4WorldDLCExtender::funcGetMiniMapSize( CScriptStackFrame& stack, void* result )
{
	Float miniMapSize = 0.0f;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapSize = worldDescriptionDLC->Get()->m_worldMiniMapSize;
	}
	RETURN_FLOAT( miniMapSize );
}

void CR4WorldDLCExtender::funcGetMiniMapTileCount( CScriptStackFrame& stack, void* result )
{
	Int32 miniMapTileCount = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapTileCount = worldDescriptionDLC->Get()->m_worldMiniMapTileCount;
	}
	RETURN_INT( miniMapTileCount );
}

void CR4WorldDLCExtender::funcGetMiniMapExteriorTextureSize( CScriptStackFrame& stack, void* result )
{
	Int32 miniMapExteriorTextureSize = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapExteriorTextureSize = worldDescriptionDLC->Get()->m_worldMiniMapExteriorTextureSize;
	}
	RETURN_INT( miniMapExteriorTextureSize );
}

void CR4WorldDLCExtender::funcGetMiniMapInteriorTextureSize( CScriptStackFrame& stack, void* result )
{
	Int32 miniMapInteriorTextureSize = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapInteriorTextureSize = worldDescriptionDLC->Get()->m_worldMiniMapInteriorTextureSize;
	}
	RETURN_INT( miniMapInteriorTextureSize );
}

void CR4WorldDLCExtender::funcGetMiniMapTextureSize( CScriptStackFrame& stack, void* result )
{
	Int32 miniMapTextureSize = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapTextureSize = worldDescriptionDLC->Get()->m_worldMiniMapTextureSize;
	}
	RETURN_INT( miniMapTextureSize );
}

void CR4WorldDLCExtender::funcGetMiniMapMinLod( CScriptStackFrame& stack, void* result )
{
	Int32 miniMapMinLod = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapMinLod = worldDescriptionDLC->Get()->m_worldMiniMapMinLod;
	}
	RETURN_INT( miniMapMinLod );
}

void CR4WorldDLCExtender::funcGetMiniMapMaxLod( CScriptStackFrame& stack, void* result )
{
	Int32 miniMapMaxLod = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapMaxLod = worldDescriptionDLC->Get()->m_worldMiniMapMaxLod;
	}
	RETURN_INT( miniMapMaxLod );
}

void CR4WorldDLCExtender::funcGetMiniMapExteriorTextureExtension( CScriptStackFrame& stack, void* result )
{
	String miniMapExteriorTextureExtension;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapExteriorTextureExtension = worldDescriptionDLC->Get()->m_worldMiniMapExteriorTextureExtension;
	}
	RETURN_STRING( miniMapExteriorTextureExtension );
}

void CR4WorldDLCExtender::funcGetMiniMapInteriorTextureExtension( CScriptStackFrame& stack, void* result )
{
	String miniMapInteriorTextureExtension;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapInteriorTextureExtension = worldDescriptionDLC->Get()->m_worldMiniMapInteriorTextureExtension;
	}
	RETURN_STRING( miniMapInteriorTextureExtension );
}

void CR4WorldDLCExtender::funcGetMiniMapVminX( CScriptStackFrame& stack, void* result )
{
	Int32 vminX = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		vminX = worldDescriptionDLC->Get()->m_worldMiniMapVminX;
	}
	RETURN_INT( vminX );
}

void CR4WorldDLCExtender::funcGetMiniMapVmaxX( CScriptStackFrame& stack, void* result )
{
	Int32 vmaxX = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		vmaxX = worldDescriptionDLC->Get()->m_worldMiniMapVmaxX;
	}
	RETURN_INT( vmaxX );
}

void CR4WorldDLCExtender::funcGetMiniMapVminY( CScriptStackFrame& stack, void* result )
{
	Int32 vminY = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		vminY = worldDescriptionDLC->Get()->m_worldMiniMapVminY;
	}
	RETURN_INT( vminY );
}

void CR4WorldDLCExtender::funcGetMiniMapVmaxY( CScriptStackFrame& stack, void* result )
{
	Int32 vmaxY = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		vmaxY = worldDescriptionDLC->Get()->m_worldMiniMapVmaxY;
	}
	RETURN_INT( vmaxY );
}

void CR4WorldDLCExtender::funcGetMiniMapSminX( CScriptStackFrame& stack, void* result )
{
	Int32 sminX = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		sminX = worldDescriptionDLC->Get()->m_worldMiniMapSminX;
	}
	RETURN_INT( sminX );
}

void CR4WorldDLCExtender::funcGetMiniMapSmaxX( CScriptStackFrame& stack, void* result )
{
	Int32 smaxX = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		smaxX = worldDescriptionDLC->Get()->m_worldMiniMapSmaxX;
	}
	RETURN_INT( smaxX );
}

void CR4WorldDLCExtender::funcGetMiniMapSminY( CScriptStackFrame& stack, void* result )
{
	Int32 sminY = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		sminY = worldDescriptionDLC->Get()->m_worldMiniMapSminY;
	}
	RETURN_INT( sminY );
}

void CR4WorldDLCExtender::funcGetMiniMapSmaxY( CScriptStackFrame& stack, void* result )
{
	Int32 smaxY = 0;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		smaxY = worldDescriptionDLC->Get()->m_worldMiniMapSmaxY;
	}
	RETURN_INT( smaxY );
}

void CR4WorldDLCExtender::funcGetMiniMapMinZoom( CScriptStackFrame& stack, void* result )
{
	Float miniMapMinZoom = 0.0f;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapMinZoom = worldDescriptionDLC->Get()->m_worldMiniMapMinZoom;
	}
	RETURN_FLOAT( miniMapMinZoom );
}

void CR4WorldDLCExtender::funcGetMiniMapMaxZoom( CScriptStackFrame& stack, void* result )
{
	Float miniMapMaxZoom = 0.0f;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapMaxZoom = worldDescriptionDLC->Get()->m_worldMiniMapMaxZoom;
	}
	RETURN_FLOAT( miniMapMaxZoom );
}

void CR4WorldDLCExtender:: funcGetMiniMapZoom12( CScriptStackFrame& stack, void* result )
{
	Float miniMapZoom12 = 0.0f;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapZoom12 = worldDescriptionDLC->Get()->m_worldMiniMapZoom12;
	}
	RETURN_FLOAT( miniMapZoom12 );
}

void CR4WorldDLCExtender::funcGetMiniMapZoom23( CScriptStackFrame& stack, void* result )
{
	Float miniMapZoom23 = 0.0f;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapZoom23 = worldDescriptionDLC->Get()->m_worldMiniMapZoom23;
	}
	RETURN_FLOAT( miniMapZoom23 );
}

void CR4WorldDLCExtender::funcGetMiniMapZoom34( CScriptStackFrame& stack, void* result )
{
	Float miniMapZoom34 = 0.0f;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		miniMapZoom34 = worldDescriptionDLC->Get()->m_worldMiniMapZoom34;
	}
	RETURN_FLOAT( miniMapZoom34 );
}

void CR4WorldDLCExtender::funcGetGradientScale( CScriptStackFrame& stack, void* result )
{
	Float gradientScale = 1.0f;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		gradientScale = worldDescriptionDLC->Get()->m_worldGradientScale;
	}
	RETURN_FLOAT( gradientScale );
}

void CR4WorldDLCExtender::funcGetPreviewHeight( CScriptStackFrame& stack, void* result )
{
	Float previewHeight = 150.0f;
	GET_PARAMETER( Int32, areaType, 0 );
	FINISH_PARAMETERS;
	THandle< CR4WorldDescriptionDLC >* worldDescriptionDLC = m_dlcWorlds.FindPtr( areaType );
	if( worldDescriptionDLC != nullptr )
	{
		previewHeight = worldDescriptionDLC->Get()->m_worldPreviewHeight;
	}
	RETURN_FLOAT( previewHeight );
}

void CR4WorldDLCExtender::RegisterDlcWorld( const Int32 areaType, const String& areaName, THandle< CR4WorldDescriptionDLC >& worldDescriptionHandle )
{
	m_areaTypeToName[areaType] = areaName;
	m_areaNameToType[areaName] = areaType;
	m_dlcWorlds[areaType] = worldDescriptionHandle;
}

void CR4WorldDLCExtender::UnregisterDlcWorld( const Int32 areaType )
{
	String* areaNamePtr = m_areaTypeToName.FindPtr( areaType );
	if( areaNamePtr != nullptr )
	{
		m_areaNameToType.Erase( *areaNamePtr );
		m_areaTypeToName.Erase( areaType );
		m_dlcWorlds.Erase( areaType );
	}
}

CR4WorldDLCMounter::CR4WorldDLCMounter()
{	
}

void CR4WorldDLCMounter::OnPostLoad()
{
	//! enum entries have to loaded on post load of dlc mounter otherwise any other resources which use this enum value will be corrupted if it will be loaded before
	//! OnGameStarting or OnFinalize
	AddAreaNameEnumEntries();
}

void CR4WorldDLCMounter::OnFinalize()
{
	RemAreaNameEnumEntries();
}

bool CR4WorldDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4WorldDLCMounter::OnGameStarting()
{
	Activate();
}

void CR4WorldDLCMounter::OnGameEnding()
{
	Deactivate();
}

void CR4WorldDLCMounter::AddAreaNameEnumEntries()
{
	CEnum* areaNameEnum = SRTTI::GetInstance().FindEnum( CNAME( EAreaName ) );
	if( areaNameEnum )
	{
		for ( auto& world : m_worlds )
		{
			if ( areaNameEnum->GetOptions().FindPtr( world->m_worldEnumAreaName ) == nullptr )
			{
				areaNameEnum->Add( world->m_worldEnumAreaName, areaNameEnum->GetOptions().Size() );
			}			
		}		
	}	
}

void CR4WorldDLCMounter::RemAreaNameEnumEntries()
{
	CEnum* areaNameEnum = SRTTI::GetInstance().FindEnum( CNAME( EAreaName ) );
	if( areaNameEnum )
	{
		for ( auto& world : m_worlds )
		{
			areaNameEnum->Remove( world->m_worldEnumAreaName );
		}		
	}
}

void CR4WorldDLCMounter::OnScriptReloaded()
{
	TBaseClass::OnScriptReloaded();
	AddAreaNameEnumEntries();
}

void CR4WorldDLCMounter::AddAreaMapPinsEntries()
{
	if( GCommonGame )
	{
		CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
		if( manager )
		{		
			for ( auto& world : m_worlds )
			{
				CEnum* areaNameEnum = SRTTI::GetInstance().FindEnum( CNAME( EAreaName ) );
				if( areaNameEnum )
				{
					Int32 areaType = 0;
					areaNameEnum->FindValue( world->m_worldEnumAreaName, areaType );
					manager->AddAreaMapPin( areaType, world->m_worldMapPinX, world->m_worldMapPinY, world->m_worldMapPath, world->m_requiredChunk, world->m_worldMapLoactionNameStringKey , world->m_worldMapLoactionDescriptionStringKey );
				}
			}
		}
	}
}

void CR4WorldDLCMounter::RemAreaMapPinsEntries()
{
	if( GCommonGame )
	{
		CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
		if( manager )
		{		
			for ( auto& world : m_worlds )
			{
				CEnum* areaNameEnum = SRTTI::GetInstance().FindEnum( CNAME( EAreaName ) );
				if( areaNameEnum )
				{
					Int32 areaType = 0;
					areaNameEnum->FindValue( world->m_worldEnumAreaName, areaType );
					manager->RemAreaMapPin( areaType );
				}
			}
		}
	}
}

void CR4WorldDLCMounter::RemAreaMapPinsEntry( const CR4WorldDescriptionDLC* worldDescriptionDLC )
{
	THandle<CR4WorldDescriptionDLC>* foundEntry = m_worlds.FindPtr( worldDescriptionDLC );
	if( foundEntry )
	{
		m_worlds.Erase( foundEntry );
	}
}

//! CObject
void CR4WorldDescriptionDLC::OnFinalize()
{
	if ( GetParent() && GetParent()->IsA<CR4WorldDLCMounter>())
	{
		CR4WorldDLCMounter* worldDLCMounter  = Cast<CR4WorldDLCMounter>( GetParent() );
		worldDLCMounter->RemAreaMapPinsEntry( this );
	}
}

CR4WorldDescriptionDLC::CR4WorldDescriptionDLC():
	m_worldMiniMapSize( 0.0f ),
	m_worldMiniMapTileCount( 0 ),
	m_worldMiniMapExteriorTextureSize( 0 ),
	m_worldMiniMapInteriorTextureSize( 0 ),
	m_worldMiniMapTextureSize( 0 ),	
	m_worldMiniMapMinLod( 0 ),
	m_worldMiniMapMaxLod( 0 ),
	m_worldGradientScale( 1 ),
	m_worldPreviewHeight( 150 )
{

}

#ifndef NO_EDITOR

void CR4WorldDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4WorldDLCMounter::OnEditorStopped()
{
	Deactivate();
}

Bool CR4WorldDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	return true;
}

#endif // !NO_EDITOR

void CR4WorldDLCMounter::Activate()
{
	if( GR4Game && GR4Game->GetWorldDLCExtender() )
	{
		CEnum* areaNameEnum = SRTTI::GetInstance().FindEnum( CNAME( EAreaName ) );
		if( areaNameEnum )
		{
			for ( auto& world : m_worlds )
			{
				Int32 areaType = (Int32)areaNameEnum->GetOptions().GetIndex( world->m_worldEnumAreaName );
				if( world->m_worldName.Empty() == false )
				{
					GR4Game->GetWorldDLCExtender()->RegisterDlcWorld( areaType, world->m_worldName, world );
				}
			}
		}
	}

	AddAreaMapPinsEntries();
}

void CR4WorldDLCMounter::Deactivate()
{
	RemAreaMapPinsEntries();

	if( GR4Game && GR4Game->GetWorldDLCExtender() )
	{
		CEnum* areaNameEnum = SRTTI::GetInstance().FindEnum( CNAME( EAreaName ) );
		if( areaNameEnum )
		{
			for ( auto& world : m_worlds )
			{
				Int32 areaType = (Int32)areaNameEnum->GetOptions().GetIndex( world->m_worldEnumAreaName );
				if( world->m_worldName.Empty() == false )
				{
					GR4Game->GetWorldDLCExtender()->UnregisterDlcWorld( areaType );
				}
			}
		}
	}
}
