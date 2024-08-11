/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once


///////////////////////////////////////////////////////////////////////////////

class ICharacterCustomizationOperation;

///////////////////////////////////////////////////////////////////////////////

class CVirtualContainerEntity : public CGameplayEntity
{
	DECLARE_ENGINE_CLASS( CVirtualContainerEntity, CGameplayEntity, 0 )

private:
	TDynArray< Uint8 >	m_characterState;

public:
	//! Initializes the virtual container with the entity inventory
	void Initialize( const CGameplayEntity* gameplayEntity );

	//! Restores entity state from the virtual container
	void Restore( CGameplayEntity* gameplayEntity );

	//! Load data into given inventory component
	void RestoreIntoInventory( CInventoryComponent* inventory );

	//! Get/Set state data
	const TDynArray< Uint8 >& GetStateData() const { return m_characterState; }
	void SetStateData( const TDynArray< Uint8 >& sourceStateData ) { m_characterState = sourceStateData; }

protected:
	//! Called when we need to store gameplay state of this entity
	virtual void OnSaveGameplayState( IGameSaver* saver );

	//! Called when we need to restore gameplay state of this entity
	virtual void OnLoadGameplayState( IGameLoader* loader );
};
BEGIN_CLASS_RTTI( CVirtualContainerEntity );
	PARENT_CLASS( CGameplayEntity );
	PROPERTY_SAVED( m_characterState );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CQuestCharacterCustomizerBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestCharacterCustomizerBlock, CQuestGraphBlock, 0 )

private:
	CName												m_actorTag;
	TDynArray< ICharacterCustomizationOperation* >		m_operations;

public:
	CQuestCharacterCustomizerBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Rounded; }
	virtual Color GetClientColor() const { return Color( 155, 187, 89 ); }
	virtual String GetBlockCategory() const { return TXT( "Gameplay" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; } // OBSOLETE

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

private:
	CActor* FindActor( const CName& tag ) const;
};

BEGIN_CLASS_RTTI( CQuestCharacterCustomizerBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_actorTag, TXT( "Tag of an actor we want to customize" ) )
	PROPERTY_INLINED( m_operations, TXT( "Customization operations to perform" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class ICharacterCustomizationOperation : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ICharacterCustomizationOperation, CQuestGraphBlock )

public:
	virtual ~ICharacterCustomizationOperation() {}

	virtual void Execute( CGameplayEntity* gameplayEntity ) const {}
};
BEGIN_ABSTRACT_CLASS_RTTI( ICharacterCustomizationOperation )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CCCOpScript : public ICharacterCustomizationOperation
{
	DECLARE_ENGINE_CLASS( CCCOpScript, ICharacterCustomizationOperation, 0 )

public:
	virtual void Execute( CGameplayEntity* gameplayEntity ) const;
};
BEGIN_CLASS_RTTI( CCCOpScript )
	PARENT_CLASS( ICharacterCustomizationOperation )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CCCOpClearInventory : public ICharacterCustomizationOperation
{
	DECLARE_ENGINE_CLASS( CCCOpClearInventory, ICharacterCustomizationOperation, 0 )

public:
	virtual void Execute( CGameplayEntity* gameplayEntity ) const;
};
BEGIN_CLASS_RTTI( CCCOpClearInventory )
	PARENT_CLASS( ICharacterCustomizationOperation )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CCCOpCustomizeInventory : public ICharacterCustomizationOperation
{
	DECLARE_ENGINE_CLASS( CCCOpCustomizeInventory, ICharacterCustomizationOperation, 0 )

private:
	THandle< CEntityTemplate >	m_template;
	Bool						m_applyMounts;

public:
	CCCOpCustomizeInventory();

	virtual void Execute( CGameplayEntity* gameplayEntity ) const;
};
BEGIN_CLASS_RTTI( CCCOpCustomizeInventory )
	PARENT_CLASS( ICharacterCustomizationOperation )
	PROPERTY_EDIT( m_template, TXT( "Template to take inventory from" ) )
	PROPERTY_EDIT( m_applyMounts, TXT( "Should mount flags be reflected after customizing" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CCCOpVirtualContainerOp : public ICharacterCustomizationOperation
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CCCOpVirtualContainerOp, ICharacterCustomizationOperation );

private:
	CName												m_virtualContainerTag;

public:
	static	THandle< CVirtualContainerEntity >			s_lastVirtualContainer;


public:
	CCCOpVirtualContainerOp();
	virtual ~CCCOpVirtualContainerOp() {}

	virtual void Execute( CGameplayEntity* gameplayEntity ) const;

protected:
	virtual void Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const {}

	// A helper function for accessing an existing virtual container
	CVirtualContainerEntity* GetVirtualContainer( CName tag ) const;
};
BEGIN_CLASS_RTTI( CCCOpVirtualContainerOp )
	PARENT_CLASS( ICharacterCustomizationOperation )
	PROPERTY_EDIT( m_virtualContainerTag, TXT( "Tag of a virtual container we want to use" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CCCOpItemsToVirtualContainer : public CCCOpVirtualContainerOp
{
	DECLARE_ENGINE_CLASS( CCCOpItemsToVirtualContainer, CCCOpVirtualContainerOp, 0 );

private:
	Bool					m_canOverride;

public:
	CCCOpItemsToVirtualContainer();

protected:
	virtual void Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const;
};
BEGIN_CLASS_RTTI( CCCOpItemsToVirtualContainer )
	PARENT_CLASS( CCCOpVirtualContainerOp )
	PROPERTY_EDIT( m_canOverride, TXT( "If a container already exists, can its contents be overriden?" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CCCOpItemsFromVirtualContainer : public CCCOpVirtualContainerOp
{
	DECLARE_ENGINE_CLASS( CCCOpItemsFromVirtualContainer, CCCOpVirtualContainerOp, 0 )

private:
	Bool					m_applyMounts;

public:
	CCCOpItemsFromVirtualContainer();

protected:
	virtual void Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const;
};
BEGIN_CLASS_RTTI( CCCOpItemsFromVirtualContainer )
	PARENT_CLASS( CCCOpVirtualContainerOp )
	PROPERTY_EDIT( m_applyMounts, TXT( "Should mount flags be reflected after customizing" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CCCOpItemsRemoveMatchingVirtualContainer : public CCCOpVirtualContainerOp
{
	DECLARE_ENGINE_CLASS( CCCOpItemsRemoveMatchingVirtualContainer, CCCOpVirtualContainerOp, 0 )

protected:
	virtual void Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const;
};
BEGIN_CLASS_RTTI( CCCOpItemsRemoveMatchingVirtualContainer )
	PARENT_CLASS( CCCOpVirtualContainerOp )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CCCOpItemsRemoveMatchingTemplate : public ICharacterCustomizationOperation
{
	DECLARE_ENGINE_CLASS( CCCOpItemsRemoveMatchingTemplate, ICharacterCustomizationOperation, 0 )

private:
	THandle< CEntityTemplate >	m_template;

public:
	CCCOpItemsRemoveMatchingTemplate();

	virtual void Execute( CGameplayEntity* gameplayEntity ) const;
};
BEGIN_CLASS_RTTI( CCCOpItemsRemoveMatchingTemplate )
	PARENT_CLASS( ICharacterCustomizationOperation )
	PROPERTY_EDIT( m_template, TXT( "Template to compare the inventory to" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

TDynArray< Uint8 >			GVirtualContainerStatePersistentBuffer;

class CCCOpPreserveVirtualContainerContents : public CCCOpVirtualContainerOp
{
	DECLARE_ENGINE_CLASS( CCCOpPreserveVirtualContainerContents, CCCOpVirtualContainerOp, 0 )

public:
	CCCOpPreserveVirtualContainerContents();

protected:
	virtual void Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const;
};
BEGIN_CLASS_RTTI( CCCOpPreserveVirtualContainerContents )
	PARENT_CLASS( CCCOpVirtualContainerOp )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CCCOpRestoreVirtualContainerContents : public CCCOpVirtualContainerOp
{
	DECLARE_ENGINE_CLASS( CCCOpRestoreVirtualContainerContents, CCCOpVirtualContainerOp, 0 )

public:
	CCCOpRestoreVirtualContainerContents();

protected:
	virtual void Execute( CGameplayEntity* gameplayEntity, CName virtualContainerTag ) const;
};
BEGIN_CLASS_RTTI( CCCOpRestoreVirtualContainerContents )
	PARENT_CLASS( CCCOpVirtualContainerOp )
END_CLASS_RTTI()