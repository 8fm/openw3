/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "attitudeManager.h"
#include "../../common/core/gameSave.h"
#include "../../common/core/hash.h"

RED_DEFINE_NAME( player );

namespace
{

const Float ATTITUDES_UPDATE_INTERVAL = 60;		// remove empty attitudes every minute

}

//////////////////////////////////////////////////////////////////////////

CAttitudeManager::SPersistentActorsAttitude::SPersistentActorsAttitude()
	: m_attitude( CAttitudes::GetDefaultAttitude() )
{}

CAttitudeManager::SPersistentActorsAttitude::SPersistentActorsAttitude( const IdTag& first, const IdTag& second, EAIAttitude attitude )
	: m_attitude( attitude )
{
	SetTags( first, second );
}

CAttitudeManager::SPersistentActorsAttitude::SPersistentActorsAttitude( const CActor* first, const CActor* second, EAIAttitude attitude )
	: m_attitude( attitude )
{
	RED_ASSERT( first != nullptr && second != nullptr );
	SetTags( first->GetIdTag(), second->GetIdTag() );
}

void CAttitudeManager::SPersistentActorsAttitude::SetTags( const IdTag& first, const IdTag& second )
{
	if ( first < second )
	{
		m_first = first;
		m_second = second;
	}
	else
	{
		m_first = second;
		m_second = first;
	}
}

Bool CAttitudeManager::SPersistentActorsAttitude::Equal( const SPersistentActorsAttitude& a, const SPersistentActorsAttitude& b )
{
	return ( a.m_first == b.m_first && a.m_second == b.m_second );
}

Uint32 CAttitudeManager::SPersistentActorsAttitude::GetHash( const SPersistentActorsAttitude& a )
{
	return ( a.m_first.CalcHash() ^ a.m_second.CalcHash() );
}

//////////////////////////////////////////////////////////////////////////

CAttitudeManager::SNonPersistentActorsAttitude::SNonPersistentActorsAttitude()
	: m_first( nullptr )
	, m_second( nullptr )
	, m_attitude( CAttitudes::GetDefaultAttitude() )
	, m_firstHandle( nullptr )
	, m_secondHandle( nullptr )
{}

CAttitudeManager::SNonPersistentActorsAttitude::SNonPersistentActorsAttitude( const CActor* first, const CActor* second, EAIAttitude attitude, Bool fullInit /* = true */ )
	: m_attitude( attitude )
{
	RED_ASSERT( first != nullptr && second != nullptr );
	SetActors( first, second, fullInit );
}

void CAttitudeManager::SNonPersistentActorsAttitude::SetActors( const CActor* first, const CActor* second, Bool fullInit /* = true */ )
{
	if ( first < second )
	{
		m_first = first;
		m_second = second;
	}
	else
	{
		m_first = second;
		m_second = first;
	}
	if ( fullInit )
	{
		m_firstHandle = m_first;
		m_secondHandle = m_second;
	}
}

Bool CAttitudeManager::SNonPersistentActorsAttitude::IsValid() const
{
	return m_firstHandle.IsValid() && m_secondHandle.IsValid();
}

Bool CAttitudeManager::SNonPersistentActorsAttitude::Equal( const SNonPersistentActorsAttitude& a, const SNonPersistentActorsAttitude& b )
{
	return ( a.m_first == b.m_first && a.m_second == b.m_second );
}

Uint32 CAttitudeManager::SNonPersistentActorsAttitude::GetHash( const SNonPersistentActorsAttitude& a )
{
	return ( ::GetPtrHash( a.m_first ) ^ ::GetPtrHash( a.m_second ) );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAttitudeManager );

CAttitudeManager::CAttitudeManager()
	: m_nextAttitudesUpdate( ATTITUDES_UPDATE_INTERVAL )
{
}

Bool CAttitudeManager::AddAdditionalAttitudesXML( const String& filePath )
{
	if( m_additionalAttitudesXMLs.FindPtr( filePath ) == nullptr )
	{
		m_additionalAttitudesXMLs.PushBack( filePath );
		return true;
	}
	return false;
}

Bool CAttitudeManager::RemAdditionalAttitudesXML( const String& filePath )
{
	String* foundFilePath = m_additionalAttitudesXMLs.FindPtr( filePath );
	if( foundFilePath )
	{
		m_additionalAttitudesXMLs.Erase( foundFilePath );
		return true;
	}
	return false;
}

void CAttitudeManager::Tick( Float deltaTime )
{
	UpdateActorAttitudes( deltaTime );
}

void CAttitudeManager::Initialize()
{
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CAttitudeManager::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

Bool CAttitudeManager::SetGlobalAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude attitude )
{
	return m_attitudes.SetAttitude( srcGroup, dstGroup, attitude, true );
}

Bool CAttitudeManager::GetGlobalAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude& attitude /* out */ ) const
{
	Bool isCustom = false;
	return m_attitudes.GetAttitude( srcGroup, dstGroup, attitude, isCustom );
}

Bool CAttitudeManager::SetAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude attitude, Bool updateTree )
{
	return m_attitudes.SetAttitude( srcGroup, dstGroup, attitude, updateTree );
}

Bool CAttitudeManager::GetAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude& attitude /* out */, Bool& isCustom /* out */ ) const
{
	return m_attitudes.GetAttitude( srcGroup, dstGroup, attitude, isCustom );
}

Bool CAttitudeManager::GetAttitudeWithParents( const CName& srcGroup, const CName& dstGroup, EAIAttitude &attitude , Bool& isCustom , CName& srcGroupParent , CName& dstGroupParent ) const
{
	return m_attitudes.GetAttitudeWithParents( srcGroup, dstGroup, attitude, isCustom, srcGroupParent, dstGroupParent );
}

Bool CAttitudeManager::RemoveAttitude( const CName& srcGroup, const CName& dstGroup, bool updateTree )
{
	return m_attitudes.RemoveAttitude( srcGroup, dstGroup, updateTree );
}

Bool CAttitudeManager::GetActorAttitude( const CActor* srcActor, const CActor* dstActor, EAIAttitude& attitude /* out */ ) const
{
	if ( IsPersistentActorsAttitude( srcActor, dstActor ) )
	{
		TPersistentActorsAttitudes::const_iterator it = FindPersistentActorsAttitude( srcActor, dstActor );
		if ( it != m_persistentActorsAttitudes.End() )
		{
			attitude = it->m_attitude;
			return true;
		}
	}
	else
	{
		TNonPersistentActorsAttitudes::const_iterator it = FindNonPersistentActorsAttitude( srcActor, dstActor );
		if ( it != m_nonPersistentActorsAttitudes.End() )
		{
			attitude = it->m_attitude;
			return true;
		}
	}

	attitude = m_attitudes.GetDefaultAttitude();
	return false;
}

Bool CAttitudeManager::SetActorAttitude( CActor* srcActor, CActor* dstActor, EAIAttitude attitude )
{
	if ( IsPersistentActorsAttitude( srcActor, dstActor ) )
	{
		TPersistentActorsAttitudes::iterator it = FindPersistentActorsAttitude( srcActor, dstActor );
		if ( it == m_persistentActorsAttitudes.End() )
		{
			m_persistentActorsAttitudes.Insert( SPersistentActorsAttitude( srcActor, dstActor, attitude ) );
		}
		else
		{
			it->m_attitude = attitude;
		}
	}
	else
	{
		TNonPersistentActorsAttitudes::iterator it = FindNonPersistentActorsAttitude( srcActor, dstActor );
		if ( it == m_nonPersistentActorsAttitudes.End() )
		{
			m_nonPersistentActorsAttitudes.Insert( SNonPersistentActorsAttitude( srcActor, dstActor, attitude ) );
		}
		else
		{
			it->m_attitude = attitude;
		}
	}
	return true;
}

Bool CAttitudeManager::ResetActorAttitude( const CActor* srcActor, const CActor* dstActor )
{
	if ( IsPersistentActorsAttitude( srcActor, dstActor ) )
	{
		TPersistentActorsAttitudes::iterator it = FindPersistentActorsAttitude( srcActor, dstActor );
		if ( it != m_persistentActorsAttitudes.End() )
		{
			m_persistentActorsAttitudes.Erase( it );
			return true;
		}
		return false;
	}
	else
	{
		TNonPersistentActorsAttitudes::iterator it = FindNonPersistentActorsAttitude( srcActor, dstActor );
		if ( it != m_nonPersistentActorsAttitudes.End() )
		{
			m_nonPersistentActorsAttitudes.Erase( it );
			return true;
		}
		return false;
	}
}

void CAttitudeManager::RemoveActorAttitudes( const CActor* actor, Bool hostile, Bool neutral, Bool friendly )
{
	RED_ASSERT( actor != nullptr );
	
	// can be persistent attitude used?
	if ( actor->GetIdTag().IsValid() )
	{
		IdTag myTag = actor->GetIdTag();
		TDynArray< SPersistentActorsAttitude > entriesToRemove;
		for ( TPersistentActorsAttitudes::iterator it = m_persistentActorsAttitudes.Begin(); it != m_persistentActorsAttitudes.End(); ++it )
		{
			if ( it->m_first == myTag || it->m_second == myTag )
			{
				switch ( it->m_attitude )
				{
				case AIA_Friendly:
					if ( friendly ) entriesToRemove.PushBack( *it );
					break;
				case AIA_Neutral:
					if ( neutral ) entriesToRemove.PushBack( *it );
					break;
				case AIA_Hostile:
					if ( hostile ) entriesToRemove.PushBack( *it );
					break;
				default:
					break;
				}
			}
		}
		for ( TDynArray< SPersistentActorsAttitude >::iterator it = entriesToRemove.Begin(); it != entriesToRemove.End(); ++it )
		{
			m_persistentActorsAttitudes.Erase( *it );
		}
	}

	// non-persistent attitudes
	{
		TDynArray< SNonPersistentActorsAttitude > entriesToRemove;
		for ( TNonPersistentActorsAttitudes::iterator it = m_nonPersistentActorsAttitudes.Begin(); it != m_nonPersistentActorsAttitudes.End(); ++it )
		{
			if ( it->m_first == actor || it->m_second == actor )
			{
				switch ( it->m_attitude )
				{
				case AIA_Friendly:
					if ( friendly ) entriesToRemove.PushBack( *it );
					break;
				case AIA_Neutral:
					if ( neutral ) entriesToRemove.PushBack( *it );
					break;
				case AIA_Hostile:
					if ( hostile ) entriesToRemove.PushBack( *it );
					break;
				default:
					break;
				}
			}
		}
		for ( TDynArray< SNonPersistentActorsAttitude >::iterator it = entriesToRemove.Begin(); it != entriesToRemove.End(); ++it )
		{
			m_nonPersistentActorsAttitudes.Erase( *it );
		}
	}
}

Bool CAttitudeManager::GetAttitudeMapForActor( const CActor* actor, THashMap< CActor*, EAIAttitude > & attitudeMap /* out */ ) const
{
	RED_ASSERT( actor != nullptr );

	// can be persistent attitude used?
	if ( actor->GetIdTag().IsValid() )
	{
		IdTag myTag = actor->GetIdTag();
		IdTag otherTag;
		for ( TPersistentActorsAttitudes::const_iterator it = m_persistentActorsAttitudes.Begin(); it != m_persistentActorsAttitudes.End(); ++it )
		{
			if ( it->m_first == myTag )
			{
				otherTag = it->m_second;
			}
			else if ( it->m_second == myTag )
			{
				otherTag = it->m_first;
			}
			else
			{
				continue;
			}
			CPeristentEntity* entity = CPeristentEntity::FindByIdTag( otherTag );
			if ( entity != nullptr && entity->IsA< CActor >() )
			{
				attitudeMap.Insert( static_cast< CActor* >( entity ), it->m_attitude );
			}
		}
	}

	// non-persistent attitudes
	{
		const CActor* otherActor = nullptr;
		for ( TNonPersistentActorsAttitudes::const_iterator it = m_nonPersistentActorsAttitudes.Begin(); it != m_nonPersistentActorsAttitudes.End(); ++it )
		{
			// if one of handles is not valid
			if ( !it->IsValid() )
			{
				continue;
			}
			if ( it->m_first == actor )
			{
				otherActor = it->m_second;
			}
			else if ( it->m_second == actor )
			{
				otherActor = it->m_first;
			}
			else
			{
				continue;
			}
			attitudeMap.Insert( const_cast< CActor* >( otherActor ), it->m_attitude );
		}
	}

	return attitudeMap.Size() > 0;
}

Bool CAttitudeManager::IsPersistentActorsAttitude( const CActor* first, const CActor* second ) const
{
	RED_ASSERT( first != nullptr && second != nullptr );
	return first->GetIdTag().IsValid() && second->GetIdTag().IsValid();
}

void CAttitudeManager::UpdateActorAttitudes( Float deltaTime )
{
	m_nextAttitudesUpdate -= deltaTime;
	if ( m_nextAttitudesUpdate > 0.0f )
	{
		return;
	}
	m_nextAttitudesUpdate = ATTITUDES_UPDATE_INTERVAL;

	typedef TPair< const CActor*, const CActor* > TEntryToRemove;
	TDynArray< TEntryToRemove > entriesToRemove;
	for ( TNonPersistentActorsAttitudes::iterator it = m_nonPersistentActorsAttitudes.Begin(); it != m_nonPersistentActorsAttitudes.End(); ++it )
	{
		if ( !it->IsValid() )
		{
			entriesToRemove.PushBack( MakePair( it->m_first, it->m_second ) );
		}
	}

	SNonPersistentActorsAttitude entry; // to not create structure for every entry to remove
	for ( TDynArray< TEntryToRemove >::iterator it = entriesToRemove.Begin(); it != entriesToRemove.End(); ++it )
	{
		entry.SetActors( it->m_first, it->m_second, false );
		m_nonPersistentActorsAttitudes.Erase( entry );
	}
}

Bool CAttitudeManager::GetParentForGroup( const CName& group, CName& parent ) const
{
	return m_attitudes.GetParentForGroup( group, parent );
}

Bool CAttitudeManager::SetParentForGroup( const CName& group, const CName& parent, Bool updateTree )
{
	return m_attitudes.SetParentForGroup( group, parent, updateTree );
}

Bool CAttitudeManager::RemoveParentForGroup( const CName& group, Bool updateTree )
{
	return m_attitudes.RemoveParentForGroup( group, updateTree );
}

Bool CAttitudeManager::IsParentForGroup( const CName& child, const CName& parent ) const
{
	return m_attitudes.IsParentForGroup( child, parent );
}

Bool CAttitudeManager::GetAllParents( const CName& group, TDynArray< CName > & parents ) const
{
	return m_attitudes.GetAllParents( group, parents );
}

Bool CAttitudeManager::GetAllChildren( const CName& group, TDynArray< CName > & children ) const
{
	return m_attitudes.GetAllChildren( group, children );
}

Bool CAttitudeManager::CanAttitudeCreateConflict( const CName& srcGroup, const CName& dstGroup, CName& srcConflictGroup /* out */, CName& dstConflictGroup )
{
	return m_attitudes.CanAttitudeCreateConflict( srcGroup, dstGroup, srcConflictGroup, dstConflictGroup );
}

Bool CAttitudeManager::CanParenthoodCreateConflict( const CName& child, const CName& parent, CName& childConflictGroup /* out */, CName& parentConflictGroup )
{
	return m_attitudes.CanParenthoodCreateConflict( child, parent, childConflictGroup, parentConflictGroup );
}

Bool CAttitudeManager::AttitudeGroupExists( const CName& group )
{
	if ( !m_attitudes.AttitudeGroupsLoaded() )
	{
		m_attitudes.LoadAttitudeGroups();
	}
	return m_attitudes.AttitudeGroupExists( group );
}

void CAttitudeManager::OnGameStart( const CGameInfo& info )
{
	// Load data from save game
	if ( info.m_gameLoadStream )
	{
		m_persistentActorsAttitudes.Clear();
		m_nonPersistentActorsAttitudes.Clear();
		m_attitudes.LoadAttitudeGroups();
		m_attitudes.LoadDataFromXmls( m_additionalAttitudesXMLs );//! additional XML files have to be loaded before saved data are restored, in the case when DLC is installed after save was made
		LoadGame( info.m_gameLoadStream );
	}
	// Load initial state from config file
	else if ( !info.IsChangingWorld() )
	{
		m_persistentActorsAttitudes.Clear();
		m_nonPersistentActorsAttitudes.Clear();
		m_attitudes.LoadAttitudeGroups();
		TDynArray<String> filesToLoad( m_additionalAttitudesXMLs );
		filesToLoad.PushBack( String(ATTITUDES_XML) );
		m_attitudes.LoadDataFromXmls( filesToLoad );
	}
}

void CAttitudeManager::OnGameEnd( const CGameInfo& gameInfo )
{
	if ( !gameInfo.IsChangingWorld() )
	{
		m_attitudes.ClearAllAttitudes();
		m_persistentActorsAttitudes.Clear();
		m_nonPersistentActorsAttitudes.Clear();
	}
}

Bool CAttitudeManager::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	TDynArray< CName > srcGroups;
	TDynArray< CName > dstGroups;

	//RED_LOG( Save, TXT("attitudes size: %ld"), attitudesSize );

	srcGroups.Reserve( 2048 );
	dstGroups.Reserve( 2048 ); 

	{
		CGameSaverBlock block( saver, CNAME( attitudes ) );

		TDynArray< EAIAttitude > attitudes;
		attitudes.Reserve( 2048 );

		Uint32 attitudesSize = m_attitudes.GetAttitudes( srcGroups, dstGroups, attitudes );
		saver->WriteValue( CNAME(count), attitudesSize );
		saver->WriteValue( CNAME(1), srcGroups );
		saver->WriteValue( CNAME(2), dstGroups );
		saver->WriteValue( CNAME(a), attitudes );
	}

	{
		CGameSaverBlock parentGroupsBlock( saver, CNAME(parentGroups) );

		srcGroups.ClearFast();
		dstGroups.ClearFast();

		Uint32 parentGroupsCount = m_attitudes.GetParentGroups( srcGroups, dstGroups );
		saver->WriteValue( CNAME(count), parentGroupsCount );
		saver->WriteValue( CNAME(1), srcGroups );
		saver->WriteValue( CNAME(2), dstGroups );
	}

	{
		CGameSaverBlock actorAttitudesBlock( saver, CNAME(actorAttitudes) );

		Uint32 actorAttitudesCount = m_persistentActorsAttitudes.Size();
		saver->WriteValue( CNAME(count), actorAttitudesCount);
		//RED_LOG( Save, TXT("actorAttitudesCount: %ld"), actorAttitudesCount );

		for ( TPersistentActorsAttitudes::iterator it = m_persistentActorsAttitudes.Begin(); it != m_persistentActorsAttitudes.End(); ++it )
		{
			saver->WriteValue( CNAME(1), it->m_first );
			saver->WriteValue( CNAME(2), it->m_second );
			saver->WriteValue( CNAME(a), it->m_attitude );
		}
	}

	END_TIMER_BLOCK( time )

	return true;
}

void CAttitudeManager::LoadGame( IGameLoader* loader )
{
	if ( loader->GetSaveVersion() < SAVE_VERSION_OPTIMIZED_ATTITUDE_MANAGER )
	{
		{
			CGameSaverBlock block( loader, CNAME(attitudes) );

			Uint32 attitudeEntriesSize = 0;
			loader->ReadValue( CNAME(count), attitudeEntriesSize );

			for ( Uint32 i = 0; i < attitudeEntriesSize; ++i )
			{
				CName srcGroupName;
				CName dstGroupName;
				String attitudeValueName;

				CGameSaverBlock block( loader, CNAME(a) );
				loader->ReadValue( CNAME(1), srcGroupName );
				loader->ReadValue( CNAME(2), dstGroupName );
				loader->ReadValue( CNAME(a), attitudeValueName );

				if ( m_attitudes.AttitudeGroupExists( srcGroupName ) && m_attitudes.AttitudeGroupExists( dstGroupName ) )
				{
					m_attitudes.SetAttitude( srcGroupName, dstGroupName, m_attitudes.AttitudeFromString( attitudeValueName ), false );
				}
			}
		}

		{
			CGameSaverBlock parentGroupsBlock( loader, CNAME(parentGroups) );

			Uint32 parentGroupsCount = 0;
			loader->ReadValue( CNAME(count), parentGroupsCount );

			for ( Uint32 i = 0; i < parentGroupsCount; ++i )
			{
				CName groupName;
				CName parentName;

				CGameSaverBlock block( loader, CNAME(p) );
				loader->ReadValue( CNAME(1), groupName );
				loader->ReadValue( CNAME(2), parentName );

				if ( m_attitudes.AttitudeGroupExists( groupName ) && m_attitudes.AttitudeGroupExists( parentName ) )
				{
					m_attitudes.SetParentForGroup( groupName, parentName, false );
				}
			}
		}

		m_attitudes.InitAttitudesTree();

		{
			CGameSaverBlock actorAttitudesBlock( loader, CNAME(actorAttitudes) );

			Uint32 actorAttitudesCount = 0;
			loader->ReadValue( CNAME(count), actorAttitudesCount );

			for ( Uint32 i = 0; i < actorAttitudesCount; ++i )
			{
				IdTag srcTag;
				IdTag dstTag;
				String attitudeName;

				CGameSaverBlock block( loader, CNAME(l) );
				loader->ReadValue( CNAME(1), srcTag );
				loader->ReadValue( CNAME(2), dstTag );
				loader->ReadValue( CNAME(a), attitudeName );

				if ( srcTag.IsValid() && dstTag.IsValid() )
				{
					m_persistentActorsAttitudes.Insert( SPersistentActorsAttitude( srcTag, dstTag, m_attitudes.AttitudeFromString( attitudeName ) ) );
				}
			}
		}
	}
	else
	{
		TDynArray< CName > srcGroups;
		TDynArray< CName > dstGroups;
		TDynArray< EAIAttitude > attitudes;

		{
			CGameSaverBlock block( loader, CNAME(attitudes) );

			Uint32 attitudeEntriesSize = 0;
			loader->ReadValue( CNAME(count), attitudeEntriesSize );

			srcGroups.Resize( attitudeEntriesSize );
			dstGroups.Resize( attitudeEntriesSize );
			attitudes.Resize( attitudeEntriesSize );

			loader->ReadValue( CNAME(1), srcGroups );
			loader->ReadValue( CNAME(2), dstGroups );
			loader->ReadValue( CNAME(a), attitudes );

			for ( Uint32 i = 0; i < attitudeEntriesSize; ++i )
			{
				CName srcGroupName = srcGroups[ i ];
				CName dstGroupName = dstGroups[ i ] ;
				EAIAttitude attitudeValue = attitudes[ i ];

				if ( m_attitudes.AttitudeGroupExists( srcGroupName ) && m_attitudes.AttitudeGroupExists( dstGroupName ) )
				{
					m_attitudes.SetAttitude( srcGroupName, dstGroupName, attitudeValue, false );
				}
			}
		}

		{
			CGameSaverBlock parentGroupsBlock( loader, CNAME(parentGroups) );

			Uint32 parentGroupsCount = 0;
			loader->ReadValue( CNAME(count), parentGroupsCount );

			srcGroups.Resize( parentGroupsCount );
			dstGroups.Resize( parentGroupsCount );

			loader->ReadValue( CNAME(1), srcGroups );
			loader->ReadValue( CNAME(2), dstGroups );

			for ( Uint32 i = 0; i < parentGroupsCount; ++i )
			{
				CName groupName = srcGroups[ i ];
				CName parentName = dstGroups[ i ];

				if ( m_attitudes.AttitudeGroupExists( groupName ) && m_attitudes.AttitudeGroupExists( parentName ) )
				{
					m_attitudes.SetParentForGroup( groupName, parentName, false );
				}
			}
		}

		m_attitudes.InitAttitudesTree();

		{
			CGameSaverBlock actorAttitudesBlock( loader, CNAME(actorAttitudes) );

			Uint32 actorAttitudesCount = 0;
			loader->ReadValue( CNAME(count), actorAttitudesCount );

			for ( Uint32 i = 0; i < actorAttitudesCount; ++i )
			{
				IdTag srcTag;
				IdTag dstTag;
				EAIAttitude attitudeValue;

				loader->ReadValue( CNAME(1), srcTag );
				loader->ReadValue( CNAME(2), dstTag );
				loader->ReadValue( CNAME(a), attitudeValue );

				if ( srcTag.IsValid() && dstTag.IsValid() )
				{
					m_persistentActorsAttitudes.Insert( SPersistentActorsAttitude( srcTag, dstTag, attitudeValue ) );
				}
			}
		}
	}
}

CAttitudeManager::TPersistentActorsAttitudes::iterator CAttitudeManager::FindPersistentActorsAttitude( const CActor* srcActor, const CActor* dstActor )
{
	// since SActorsAttitude only actor tags for comparison we can pass anything as the third argument
	return m_persistentActorsAttitudes.Find( SPersistentActorsAttitude( srcActor, dstActor, CAttitudes::GetDefaultAttitude() ) );
}

CAttitudeManager::TPersistentActorsAttitudes::const_iterator CAttitudeManager::FindPersistentActorsAttitude( const CActor* srcActor, const CActor* dstActor ) const
{
	// since SActorsAttitude only actor tags for comparison we can pass anything as the third argument
	return m_persistentActorsAttitudes.Find( SPersistentActorsAttitude( srcActor, dstActor, CAttitudes::GetDefaultAttitude() ) );
}

CAttitudeManager::TNonPersistentActorsAttitudes::iterator CAttitudeManager::FindNonPersistentActorsAttitude( const CActor* srcActor, const CActor* dstActor )
{
	// since SActorsAttitude only actor tags for comparison we can pass anything as the third argument
	return m_nonPersistentActorsAttitudes.Find( SNonPersistentActorsAttitude( srcActor, dstActor, CAttitudes::GetDefaultAttitude(), false ) );
}

CAttitudeManager::TNonPersistentActorsAttitudes::const_iterator CAttitudeManager::FindNonPersistentActorsAttitude( const CActor* srcActor, const CActor* dstActor ) const
{
	// since SActorsAttitude only actor tags for comparison we can pass anything as the third argument
	return m_nonPersistentActorsAttitudes.Find( SNonPersistentActorsAttitude( srcActor, dstActor, CAttitudes::GetDefaultAttitude(), false ) );
}