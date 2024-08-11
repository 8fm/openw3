/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeVoidNode.h"


IMPLEMENT_ENGINE_CLASS( CSpawnTreeVoidDecorator )


void CSpawnTreeVoidDecorator::UpdateLogic( CSpawnTreeInstance& instance )
{

}
void CSpawnTreeVoidDecorator::Activate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = true;
}
void CSpawnTreeVoidDecorator::Deactivate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = false;
}
ISpawnTreeBaseNode* CSpawnTreeVoidDecorator::GetChildMember( Uint32 i ) const
{
	return nullptr;
}
Uint32 CSpawnTreeVoidDecorator::GetChildMembersCount() const
{
	 return 0;
}

Int32 CSpawnTreeVoidDecorator::GetNumChildren() const
{
	return m_childNode ? 1 : 0;
}
IEdSpawnTreeNode* CSpawnTreeVoidDecorator::GetChild( Int32 index ) const
{
	return m_childNode;
}


void CSpawnTreeVoidDecorator::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	ISpawnTreeBaseNode::OnBuildDataLayout( compiler );
}
void CSpawnTreeVoidDecorator::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context )
{
	ISpawnTreeBaseNode::OnInitData( instance, context );
}
void CSpawnTreeVoidDecorator::OnDeinitData( CSpawnTreeInstance& instance )
{
	ISpawnTreeBaseNode::OnDeinitData( instance );
}


#ifndef NO_RESOURCE_COOKING
void CSpawnTreeVoidDecorator::OnCook( class ICookerFramework& cooker )
{
	m_childNode = nullptr;

	TBaseClass::OnCook( cooker );
}
#endif

Bool CSpawnTreeVoidDecorator::IsUtilityNode() const
{
	return true;
}

// Editor interface
void CSpawnTreeVoidDecorator::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	rootClasses.PushBack( ISpawnTreeBaseNode::GetStaticClass() );
}
Bool CSpawnTreeVoidDecorator::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
{
	if ( classId->IsAbstract() )
	{
		return false;
	}
	return true;
}
Color CSpawnTreeVoidDecorator::GetBlockColor() const
{
	return Color::RED;
}
String CSpawnTreeVoidDecorator::GetEditorFriendlyName() const
{
	return TXT("Void");
}
Bool CSpawnTreeVoidDecorator::HoldsInstanceBuffer() const
{
	return true;
}
CSpawnTreeInstance* CSpawnTreeVoidDecorator::GetInstanceBuffer( const CSpawnTreeInstance* parentBuffer )
{
	return nullptr;
}
CSpawnTreeVoidDecorator::EDebugState CSpawnTreeVoidDecorator::GetDebugState( const CSpawnTreeInstance* instanceBuffer ) const
{
	return EDEBUG_NOT_RELEVANT;
}
