#include "build.h"
#include "headManagerComponent.h"
#include "inventoryComponent.h"
#include "factsDB.h"
#include "../engine/mimicComponent.h"
#include "../engine/gameTimeManager.h"
#include "../core/gameSave.h"

IMPLEMENT_ENGINE_CLASS( CHeadManagerComponent );

namespace
{
const String HAS_TATTOO_FACT			= TXT( "import_geralt_has_tattoo" );
const String HAS_DEMON_MARK_FACT		= TXT( "q601_geralt_has_demon_mark" );
}

CHeadManagerComponent::CHeadManagerComponent()
	: m_hasTattoo( false )
	, m_hasDemonMark( false )
	, m_curIndex( 0 )
	, m_lastChangeGameTime()
	, m_curHeadId()
{

}

void CHeadManagerComponent::SetTattoo( Bool hasTattoo )
{
	if ( m_hasTattoo == hasTattoo )
	{
		return;
	}

	m_hasTattoo = hasTattoo;

	ChangeHeadsSet();
}

void CHeadManagerComponent::SetDemonMark( Bool hasDemonMark )
{
	if ( m_hasDemonMark == hasDemonMark )
	{
		return;
	}

	m_hasDemonMark = hasDemonMark;

	ChangeHeadsSet();
}

void CHeadManagerComponent::CustomTick( Float deltaTime )
{
	if ( m_blockGrowing || !GGame->IsBlackscreen() )
	{
		return;
	}
	// Check if actor isn't in any dialog
	if ( GetEntity()->IsA< CActor >() )
	{
		const CActor* actor = SafeCast< CActor >( GetEntity() );

		if ( actor && !actor->GetStoryScenes().Empty() )
		{
			return;
		}
	}
	TDynArray< CName >& curHeads = GetCurHeadArray();
	const Uint32 curHeadsSize = curHeads.Size();
	if ( curHeadsSize == 0 )
	{
		RED_LOG_WARNING( HeadManager, TXT( "There's no heads list defined for the following values: m_hasTattoo = %d, m_hasDemonMark = %d. Cannot update head." ), m_hasTattoo, m_hasDemonMark );
		return;
	}
	if ( m_curIndex >= curHeadsSize - 1 )
	{
		return;
	}
	const GameTime currGameTime = GGame->GetTimeManager()->GetTime();
	if ( currGameTime - m_lastChangeGameTime > m_timePeriod )
	{
		m_lastChangeGameTime = currGameTime;
		m_curIndex++;
		UpdateHead();
	}
}

void CHeadManagerComponent::UpdateHead()
{
	TDynArray< CName >& curHeads = GetCurHeadArray();	
	if ( curHeads.Size() == 0 )
	{
		RED_LOG_WARNING( HeadManager, TXT( "There's no heads list defined for the following values: m_hasTattoo = %d, m_hasDemonMark = %d. Cannot update head." ), m_hasTattoo, m_hasDemonMark );
		return;
	}

	ChangeHead( curHeads[ m_curIndex ] );
}

void CHeadManagerComponent::ChangeHead( CName head )
{
	CInventoryComponent* component = GetEntity()->FindComponent< CInventoryComponent >();
	RED_ASSERT( component, TXT("Can't change head on invalid inventory component") );
	if ( !component )
	{
		return;
	}

	TDynArray< CName > effects;
	if ( m_curHeadId != SItemUniqueId::INVALID )
	{
		CItemEntity* headEntity = component->GetItemEntityUnsafe( m_curHeadId );
		if ( headEntity )
		{
			headEntity->GetActiveEffects( effects );
		}
		component->UnMountItem( m_curHeadId );
		component->RemoveItem( m_curHeadId );

		m_curHeadId = SItemUniqueId::INVALID;
	}

	CInventoryComponent::SAddItemInfo addItemInfo;
	addItemInfo.m_quantity = 1;
	addItemInfo.m_informGui = false;
	addItemInfo.m_markAsNew = false;
	addItemInfo.m_isLootable = false;

	if ( !component->HasItem( head ) )
	{
		TDynArray< SItemUniqueId > ids = component->AddItem( head, addItemInfo );
		RED_ASSERT( ids.Size() == 1, TXT("Can't mount head") );
		m_curHeadId = ids[ 0 ];
	}
	else
	{
		m_curHeadId = component->GetItemId( head );
	}

	const SInventoryItem* existingItem = component->GetItem( m_curHeadId );
	RED_ASSERT( existingItem, TXT("Invalid pointer to existing item") );
	if( !existingItem )
	{
		return;
	}

	if ( !existingItem->IsMounted() )
	{
		CInventoryComponent::SMountItemInfo mountItemInfo;
		component->MountItem( m_curHeadId, mountItemInfo );
	}

	for ( CName effect : effects )
	{
		component->PlayItemEffect( m_curHeadId, effect );
	}

	if ( GetEntity()->IsA< CActor >() )
	{
		CActor* actor = SafeCast< CActor >( GetEntity() );

		actor->NotifyScenesAboutChangingApperance();
	}
}

void CHeadManagerComponent::ChangeHeadsSet()
{
	TDynArray< CName >& curHeads = GetCurHeadArray();
	
	const Uint32 curHeadsSize = curHeads.Size();
	if ( curHeadsSize == 0 )
	{
		RED_LOG_WARNING( HeadManager, TXT( "There's no heads list defined for the following values: m_hasTattoo = %d, m_hasDemonMark = %d. Cannot update head." ), m_hasTattoo, m_hasDemonMark );
		return;
	}

	if ( m_curIndex >= curHeadsSize )
	{
		m_curIndex = curHeadsSize - 1;
	}

	ChangeHead( curHeads[ m_curIndex ] );
}

TDynArray< CName >& CHeadManagerComponent::GetCurHeadArray()
{
	if ( m_hasDemonMark )
	{
		return m_hasTattoo ? m_headsDemonWithTattoo : m_headsDemon;		
	}
	else
	{
		return m_hasTattoo ? m_headsWithTattoo : m_heads;
	}
}

void CHeadManagerComponent::GetNamesList( TDynArray< CName >& names ) const
{
	CDefinitionsManager* dm = GCommonGame->GetDefinitionsManager();

	if ( !dm )
	{
		return;
	}

	names = dm->GetItemsOfCategory( CNAME(head) );	
}

void CHeadManagerComponent::InitHead()
{
	m_curIndex = m_initHeadIndex;
	const CInventoryComponent* component = GetEntity()->FindComponent< CInventoryComponent >();
	RED_ASSERT( component, TXT("Can't change head on invalid inventory component") );
	if ( !component )
	{
		return;
	}

	m_curHeadId = component->GetItemByCategory( CNAME(head) );

	// if there was a custom head applied before saving the game we are skipping regular head update
	if ( !m_blockGrowing )
	{
		UpdateHead();
	}
}

void CHeadManagerComponent::InitTattooAndDemonMark()
{
	Bool hadTattoo = m_hasTattoo;
	Bool hadDemonMark = m_hasDemonMark;

	CFactsDB *factsDB = GCommonGame? GCommonGame->GetSystem< CFactsDB >() : nullptr;
	if ( factsDB != nullptr )
	{
		m_hasTattoo = ( factsDB->QuerySum( HAS_TATTOO_FACT ) >= 1 );
		m_hasDemonMark = ( factsDB->QuerySum( HAS_DEMON_MARK_FACT) >= 1 );
	}

	if ( m_hasTattoo != hadTattoo || m_hasDemonMark != hadDemonMark )
	{
		ChangeHeadsSet();
	}
}

void CHeadManagerComponent::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );

	InitTattooAndDemonMark();

	InitHead();
}

void CHeadManagerComponent::OnAttachFinishedEditor( CWorld* world )
{
	TBaseClass::OnAttachFinishedEditor( world );

	InitHead();
}

void CHeadManagerComponent::SetBeardStage( Uint32 stage )
{
	m_curIndex = stage;
	m_lastChangeGameTime = GGame->GetTimeManager()->GetTime();

	TDynArray< CName >& curHeads = GetCurHeadArray();
	if ( curHeads.Size() == 0 )
	{
		RED_LOG_WARNING( HeadManager, TXT( "There's no heads list defined for the following values: m_hasTattoo = %d, m_hasDemonMark = %d. Cannot update head." ), m_hasTattoo, m_hasDemonMark );
		return;
	}

	ChangeHead( curHeads[ m_curIndex ] );
}

void CHeadManagerComponent::Shave()
{
	SetBeardStage( 0 );
}

void CHeadManagerComponent::SetCustomHead( CName head )
{
	m_blockGrowing = true;

	ChangeHead( head );
}

void CHeadManagerComponent::RemoveCustomHead()
{
	m_blockGrowing = false;

	TDynArray< CName >& curHeads = GetCurHeadArray();
	if ( curHeads.Size() == 0 )
	{
		RED_LOG_WARNING( HeadManager, TXT( "There's no heads list defined for the following values: m_hasTattoo = %d, m_hasDemonMark = %d. Cannot update head." ), m_hasTattoo, m_hasDemonMark );
		return;
	}

	ChangeHead( curHeads[ m_curIndex ] );
}

void CHeadManagerComponent::funcSetTattoo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, hasTattoo, false );
	FINISH_PARAMETERS;

	SetTattoo( hasTattoo );

	RETURN_VOID();
}

void CHeadManagerComponent::funcSetDemonMark( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, hasDemonMark, false );
	FINISH_PARAMETERS;

	SetDemonMark( hasDemonMark );

	RETURN_VOID();
}

void CHeadManagerComponent::funcShave( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Shave();

	RETURN_VOID();
}

void CHeadManagerComponent::funcSetBeardStage( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, maxBeard, false );
	GET_PARAMETER_OPT( Int32, stage, 0 );
	FINISH_PARAMETERS;

	const Int32 curHeadsSize = GetCurHeadArray().SizeInt();
	if ( curHeadsSize == 0 )
	{
		RED_LOG_WARNING( HeadManager, TXT( "There's no heads list defined for the following values: m_hasTattoo = %d, m_hasDemonMark = %d. Cannot update head." ), m_hasTattoo, m_hasDemonMark );
	}
	else
	{
		if ( maxBeard || stage >= curHeadsSize )
		{
			stage = curHeadsSize - 1;
		}

		SetBeardStage( static_cast< Uint32 >( stage ) );
	}

	RETURN_VOID();
}

void CHeadManagerComponent::funcSetCustomHead( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, head, CName::NONE );
	FINISH_PARAMETERS;

	SetCustomHead( head );

	RETURN_VOID();
}

void CHeadManagerComponent::funcRemoveCustomHead( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RemoveCustomHead();

	RETURN_VOID();
}

void CHeadManagerComponent::funcBlockGrowing( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, block, false );
	FINISH_PARAMETERS;

	m_blockGrowing = block;

	RETURN_VOID();
}

void CHeadManagerComponent::funcMimicTest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animName, CName::NONE );
	FINISH_PARAMETERS;

	IActorInterface* actor = GGame->GetPlayerEntity()->QueryActorInterface();
	if ( !actor )
	{
		return;
	}

	actor->MimicOn();

	CMimicComponent* component = actor->GetMimicComponent();
	if ( component )
	{
		component->PlayMimicAnimation( animName , CNAME( MIMIC_SLOT ) );
	}

	RETURN_VOID();
}

void CHeadManagerComponent::funcGetCurHeadName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	TDynArray< CName >& curHeads = GetCurHeadArray();
	CName headName = CName::NONE;
	if ( m_curIndex < curHeads.Size() )
	{
		headName = curHeads[ m_curIndex ];
	}

	RETURN_NAME( headName );
}

RED_DEFINE_STATIC_NAME ( HeadManager );
RED_DEFINE_STATIC_NAME ( HasTattoo );
RED_DEFINE_STATIC_NAME ( HeadId );
RED_DEFINE_STATIC_NAME ( BlockGrowing );
RED_DEFINE_STATIC_NAME ( HasDemonMark );

void CHeadManagerComponent::OnSaveGameplayState( IGameSaver* saver )
{
	TBaseClass::OnSaveGameplayState( saver );

	CGameSaverBlock block( saver, CNAME(HeadManager) );

	saver->WriteValue( CNAME(Time), m_lastChangeGameTime );
	saver->WriteValue( CNAME(HasTattoo), m_hasTattoo );
	saver->WriteValue( CNAME(Index), m_curIndex );
	saver->WriteValue( CNAME(HeadId), m_curHeadId );
	saver->WriteValue( CNAME(BlockGrowing), m_blockGrowing );
	saver->WriteValue( CNAME(HasDemonMark), m_hasDemonMark );
	
}

void CHeadManagerComponent::OnLoadGameplayState( IGameLoader* loader )
{
	TBaseClass::OnLoadGameplayState( loader );

	CGameSaverBlock block( loader, CNAME(HeadManager) );

	loader->ReadValue( CNAME(Time), m_lastChangeGameTime );
	loader->ReadValue( CNAME(HasTattoo), m_hasTattoo );
	loader->ReadValue( CNAME(Index), m_curIndex );
	loader->ReadValue( CNAME(HeadId), m_curHeadId );
	loader->ReadValue( CNAME(BlockGrowing), m_blockGrowing );
	loader->ReadValue( CNAME(HasDemonMark), m_hasDemonMark );

	m_initHeadIndex = m_curIndex;

	TDynArray< CName >& curHeads = GetCurHeadArray();
	const Uint32 curHeadsSize = curHeads.Size();
	if ( curHeadsSize > 0 && m_curIndex >= curHeadsSize )
	{
		m_curIndex = curHeadsSize - 1;
	}
	else
	{
		m_curIndex = 0;
	}
}