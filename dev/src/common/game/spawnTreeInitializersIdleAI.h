#pragma once

#include "spawnTreeInitializerAI.h"


class CSpawnTreeInitializerIdleAI : public ISpawnTreeInitializerAI
{
	DECLARE_ENGINE_CLASS( CSpawnTreeInitializerIdleAI, ISpawnTreeInitializerAI, 0 );

public:
	CName			GetDynamicNodeEventName() const override;

	String			GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeInitializerIdleAI );
	PARENT_CLASS( ISpawnTreeInitializerAI );
END_CLASS_RTTI();


class ISpawnTreeInitializerIdleSmartAI : public CSpawnTreeInitializerIdleAI
{
	DECLARE_ENGINE_CLASS( ISpawnTreeInitializerIdleSmartAI, CSpawnTreeInitializerIdleAI, 0 );
protected:
	THandle< ISpawnTreeInitializer >	m_subInitializer;

	CName			GetSubinitializerClass() const;
public:
	ISpawnTreeInitializerIdleSmartAI()												{}

	Bool					HasSubInitializer() const override;
	ISpawnTreeInitializer*	GetSubInitializer() const override;

	// Instance buffer interface
	void			OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void			OnInitData( CSpawnTreeInstance& instance ) override;
	void			OnDeinitData( CSpawnTreeInstance& instance ) override;

	virtual Bool	GenerateIdsRecursively() override;

	// edSpawnTreeNode interface
	IScriptable*	GetObjectForPropertiesEdition() override;
	String			GetEditorFriendlyName() const override;
	Bool			IsSpawnable() const override;
	// subtree initializer
	Bool			CanAddChild() const override;
	void			GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool			CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const override;
	void			AddChild( IEdSpawnTreeNode* node ) override;
	void			RemoveChild( IEdSpawnTreeNode* node ) override;
	Int32			GetNumChildren() const override;
	IEdSpawnTreeNode* GetChild( Int32 index ) const override;
};

BEGIN_CLASS_RTTI( ISpawnTreeInitializerIdleSmartAI );
	PARENT_CLASS( CSpawnTreeInitializerIdleAI );
	PROPERTY( m_subInitializer );
END_CLASS_RTTI();

class ISpawnTreeInitializerCommunityAI : public ISpawnTreeInitializerAI
{
	DECLARE_ENGINE_CLASS( ISpawnTreeInitializerCommunityAI, ISpawnTreeInitializerAI, 0 );

public:
	ISpawnTreeInitializerCommunityAI()												{}

	Bool			IsSpawnable() const override;
	CName			GetDynamicNodeEventName() const override;
};

BEGIN_CLASS_RTTI( ISpawnTreeInitializerCommunityAI );
PARENT_CLASS( ISpawnTreeInitializerAI );
END_CLASS_RTTI();
