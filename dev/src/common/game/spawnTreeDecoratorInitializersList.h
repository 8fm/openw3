/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "spawnTreeNode.h"

#include "spawnTreeInitializersIterator.h"

class ISpawnTreeInitializer;

class CSpawnTreeDecoratorInitializersList : public ISpawnTreeDecorator
{
	DECLARE_ENGINE_CLASS( CSpawnTreeDecoratorInitializersList, ISpawnTreeDecorator, 0 );
protected:
	struct InitializersIterator : public ISpawnTreeInitializersIterator
	{
		InitializersIterator( const CSpawnTreeDecoratorInitializersList& o, CSpawnTreeInstance& i )
			: m_initializers( o.m_topInitializers )
			, m_instance( i )
			, m_currIdx( 0 )																		{}
		const TDynArray< ISpawnTreeInitializer* >&	m_initializers;
		CSpawnTreeInstance&							m_instance;
		Uint32										m_currIdx;

		Bool				Next( const ISpawnTreeInitializer*& outInitializer, CSpawnTreeInstance*& instanceBuffer ) override;
		void				Reset() override;
	};

	TDynArray< ISpawnTreeInitializer* >		m_topInitializers;

public:
	CSpawnTreeDecoratorInitializersList()															{}
	~CSpawnTreeDecoratorInitializersList()															{}

	void						Deactivate( CSpawnTreeInstance& instance ) override;

	void						CollectSpawnTags( TagList& tagList ) override;
	void						OnFullRespawn( CSpawnTreeInstance& instance ) const override;

	// Initialization interface
	void						OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void						OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) override;

	virtual Bool				GenerateIdsRecursively() override;

	// IEdSpawnTreeNode interface
	Bool						CanAddChild() const override;
	void						AddChild( IEdSpawnTreeNode* node ) override;
	void						RemoveChild( IEdSpawnTreeNode* node ) override;
	Int32						GetNumChildren() const override;
	IEdSpawnTreeNode*			GetChild( Int32 index ) const override;
	void						GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool						CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const override;
	Bool						IsHiddenByDefault() const override;
	Bool						CanBeHidden() const override;
	Color						GetBlockColor() const override;
	String						GetEditorFriendlyName() const override;

	// Saving state
	Bool						IsNodeStateSaving( CSpawnTreeInstance& instance ) const override;
	void						SaveNodeState( CSpawnTreeInstance& instance, IGameSaver* writer ) const override;
	Bool						LoadNodeState( CSpawnTreeInstance& instance, IGameLoader* reader ) const override;
};


BEGIN_CLASS_RTTI( CSpawnTreeDecoratorInitializersList )
	PARENT_CLASS( ISpawnTreeDecorator )
	PROPERTY_INLINED_RO( m_topInitializers, TXT("Top-level initializers") )
END_CLASS_RTTI()