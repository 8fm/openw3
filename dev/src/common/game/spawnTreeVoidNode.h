/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "spawnTreeNode.h"

class CSpawnTreeVoidDecorator : public ISpawnTreeDecorator
{
	DECLARE_ENGINE_CLASS( CSpawnTreeVoidDecorator, ISpawnTreeDecorator, 0 );

public:
	CSpawnTreeVoidDecorator()											{}

	void						UpdateLogic( CSpawnTreeInstance& instance ) override;
	void						Activate( CSpawnTreeInstance& instance ) override;
	void						Deactivate( CSpawnTreeInstance& instance ) override;

	// gets child spawn tree node - runtime interface
	ISpawnTreeBaseNode*			GetChildMember( Uint32 i ) const;
	Uint32						GetChildMembersCount() const;

	// gets child spawn tree node - editor interface
	Int32						GetNumChildren() const override;
	IEdSpawnTreeNode*			GetChild( Int32 index ) const override;


	// Instance buffer interface
	void						OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void						OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) override;
	void						OnDeinitData( CSpawnTreeInstance& instance ) override;

#ifndef NO_RESOURCE_COOKING
	void						OnCook( class ICookerFramework& cooker ) override;
#endif

	Bool						IsUtilityNode() const override;
	// Editor interface
	void						GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool						CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const override;
	Color						GetBlockColor() const override;
	String						GetEditorFriendlyName() const override;
	Bool						HoldsInstanceBuffer() const override;
	CSpawnTreeInstance*			GetInstanceBuffer( const CSpawnTreeInstance* parentBuffer = NULL ) override;
	EDebugState					GetDebugState( const CSpawnTreeInstance* instanceBuffer ) const override;
};


BEGIN_CLASS_RTTI( CSpawnTreeVoidDecorator );
	PARENT_CLASS( ISpawnTreeDecorator );
END_CLASS_RTTI();

