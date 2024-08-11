/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "attitude.h"
#include "actor.h"
#include "../../common/core/hashset.h"

RED_DECLARE_NAME( player );

class CAttitudeManager : public IGameSystem, public IGameSaveSection, public IAttitudesManager
{
	DECLARE_ENGINE_CLASS( CAttitudeManager, IGameSystem, 0 );

private:

	CAttitudes							m_attitudes;
	TDynArray<String>					m_additionalAttitudesXMLs;

	struct SPersistentActorsAttitude
	{
		IdTag		m_first;
		IdTag		m_second;
		EAIAttitude	m_attitude;

		SPersistentActorsAttitude();
		SPersistentActorsAttitude( const IdTag& first, const IdTag& second, EAIAttitude attitude );
		SPersistentActorsAttitude( const CActor* first, const CActor* second, EAIAttitude attitude );

		void SetTags( const IdTag& first, const IdTag& second );

		static Bool Equal( const SPersistentActorsAttitude& a, const SPersistentActorsAttitude& b );
		static Uint32 GetHash( const SPersistentActorsAttitude& a );
	};

	struct SNonPersistentActorsAttitude
	{
		const CActor*		m_first;
		const CActor*		m_second;
		EAIAttitude			m_attitude;

	private:

		THandle< CActor >	m_firstHandle;
		THandle< CActor >	m_secondHandle;

	public:

		SNonPersistentActorsAttitude();
		SNonPersistentActorsAttitude( const CActor* first, const CActor* second, EAIAttitude attitude, Bool fullInit = true );

		void SetActors( const CActor* first, const CActor* second, Bool fullInit = true );
		Bool IsValid() const;

		static Bool Equal( const SNonPersistentActorsAttitude& a, const SNonPersistentActorsAttitude& b );
		static Uint32 GetHash( const SNonPersistentActorsAttitude& a );
	};

	typedef THashSet< SPersistentActorsAttitude, SPersistentActorsAttitude, SPersistentActorsAttitude > TPersistentActorsAttitudes;
	typedef THashSet< SNonPersistentActorsAttitude, SNonPersistentActorsAttitude, SNonPersistentActorsAttitude > TNonPersistentActorsAttitudes;

	TPersistentActorsAttitudes		m_persistentActorsAttitudes;
	TNonPersistentActorsAttitudes	m_nonPersistentActorsAttitudes;
	Float							m_nextAttitudesUpdate;

public:
	CAttitudeManager();

	//! By default engine load always ATTITUDES_XML, to load additional XML call this func before CAttitudeManager::OnGameStart is called
	Bool AddAdditionalAttitudesXML( const String& filePath );
	Bool RemAdditionalAttitudesXML( const String& filePath );

	//! Set attitude between groups, return true on success
	Bool SetGlobalAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude attitude );

	//! Get attitude between groups, return true if attitude was defined
	Bool GetGlobalAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude& attitude ) const;

	//! Set attitude between groups and optionally update attitudes in the inheritance tree, return true on success
	Bool SetAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude attitude, bool updateTree ) override;

	//! Get attitude between groups and information if attitude is custom, return true if attitude was defined
	Bool GetAttitude( const CName& srcGroup, const CName& dstGroup, EAIAttitude& attitude, Bool& isCustom ) const override;

	//! Get attitude between groups and parents from which they've been inherited, return true if attitude was defined
	Bool GetAttitudeWithParents( const CName& srcGroup, const CName& dstGroup, EAIAttitude &attitude, Bool& isCustom,
							     CName& srcGroupParent, CName& dstGroupParent ) const override;
	
	//! Remove attitude definition between two groups (reset to default attitude) and optionally update attitudes in the inheritance tree, returns true on success
	Bool RemoveAttitude( const CName& srcGroup, const CName& dstGroup, bool updateTree ) override;

	//! Get attitude between two actors, return true on success
	Bool GetActorAttitude( const CActor* srcActor, const CActor* dstActor, EAIAttitude& attitude ) const;

	//! Set attitude between two actors, return true on success
	Bool SetActorAttitude( CActor* srcActor, CActor* dstActor, EAIAttitude attitude );

	//! Reset (remove) attitude between two actors, return true on success
	Bool ResetActorAttitude( const CActor* srcActor, const CActor* dstActor );

	//! Remove actor attitudes of given type
	void RemoveActorAttitudes( const CActor* actor, Bool hostile, Bool neutral, Bool friendly );

	//! Get actors attitude map for given actor, return true if map is nonempty
	//! This is quite expensive so use it only for debug purposes
	Bool GetAttitudeMapForActor( const CActor* actor, THashMap< CActor*, EAIAttitude > & attitudeMap ) const;

	//! Get parent for given attitude group, return true on success
	Bool GetParentForGroup( const CName& group, CName& parent ) const override;

	//! Set parent for given attitude group and optionally update inheritance tree, return true on success
	Bool SetParentForGroup( const CName& group, const CName& parent, bool updateTree ) override;

	//! Remove parent for given attitude group, and optionally update inheritance tree return true on success
	Bool RemoveParentForGroup( const CName& group, bool updateTree ) override;

	//! Return true if 'parent' group is really a parent for a 'child' group
	Bool IsParentForGroup( const CName& child, const CName& parent ) const override;

	//! Get all parent groups of given group, return true if there is any
	Bool GetAllParents( const CName& group, TDynArray< CName > & parents ) const override;

	//! Get all children groups of given group, return true if there is any
	Bool GetAllChildren( const CName& group, TDynArray< CName > & children ) const override;

	//! Return true if adding new attitude between two groups can create conflict
	Bool CanAttitudeCreateConflict( const CName& srcGroup, const CName& dstGroup, CName& srcConflictGroup, CName& dstConflictGroup ) override;

	//! Return true if setting parent to a group can create conflict
	Bool CanParenthoodCreateConflict( const CName& child, const CName& parent, CName& childConflictGroup, CName& parentConflictGroup ) override;

	//! Return true if given attitude group exists (automatically loads attitude groups list if needed)
	Bool AttitudeGroupExists( const CName& group );

public:
	virtual void Tick( Float deltaTime ) override;
	virtual void Initialize() override;
	virtual void Shutdown() override;

	//! Resets the facts at the game start
	virtual void OnGameStart( const CGameInfo& info );

	//! Cleanup ( to save memory )
	virtual void OnGameEnd( const CGameInfo& gameInfo );

	//! Save to game save
	virtual Bool OnSaveGame( IGameSaver* saver );

public:
	//! For debugger
	const CAttitudes& GetAttitudes() const { return m_attitudes; }

private:

	Bool IsPersistentActorsAttitude( const CActor* first, const CActor* second ) const;
	void UpdateActorAttitudes( Float deltaTime );

	void LoadGame( IGameLoader* loader );

	TPersistentActorsAttitudes::iterator FindPersistentActorsAttitude( const CActor* srcActor, const CActor* dstActor );
	TPersistentActorsAttitudes::const_iterator FindPersistentActorsAttitude( const CActor* srcActor, const CActor* dstActor ) const;
	TNonPersistentActorsAttitudes::iterator FindNonPersistentActorsAttitude( const CActor* srcActor, const CActor* dstActor );
	TNonPersistentActorsAttitudes::const_iterator FindNonPersistentActorsAttitude( const CActor* srcActor, const CActor* dstActor ) const;

	ASSIGN_GAME_SYSTEM_ID( GS_Attitudes );
};

BEGIN_CLASS_RTTI( CAttitudeManager );
	PARENT_CLASS( IGameSystem );
END_CLASS_RTTI();
