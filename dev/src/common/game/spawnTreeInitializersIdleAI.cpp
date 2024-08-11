#include "build.h"
#include "spawnTreeInitializersIdleAI.h"
#include "behTreeNodeIdleRoot.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerIdleAI );

IMPLEMENT_ENGINE_CLASS( ISpawnTreeInitializerIdleSmartAI );
IMPLEMENT_ENGINE_CLASS( ISpawnTreeInitializerCommunityAI );

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerIdleAI
////////////////////////////////////////////////////////////////////
CName CSpawnTreeInitializerIdleAI::GetDynamicNodeEventName() const
{
	return CNAME( AI_Load_IdleRoot );
}

String CSpawnTreeInitializerIdleAI::GetEditorFriendlyName() const
{
	static String STR( TXT("Idle AI") );
	return STR;
}


////////////////////////////////////////////////////////////////////
// ISpawnTreeInitializerIdleSmartAI
////////////////////////////////////////////////////////////////////
Bool ISpawnTreeInitializerIdleSmartAI::HasSubInitializer() const
{
	return m_subInitializer.Get() != nullptr;
}
ISpawnTreeInitializer* ISpawnTreeInitializerIdleSmartAI::GetSubInitializer() const
{
	return m_subInitializer.Get();
}

void ISpawnTreeInitializerIdleSmartAI::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	ISpawnTreeInitializer* initializer = m_subInitializer.Get();
	if ( initializer )
	{
		initializer->OnBuildDataLayout( compiler );
	}
}
void ISpawnTreeInitializerIdleSmartAI::OnInitData( CSpawnTreeInstance& instance )
{
	TBaseClass::OnInitData( instance );
	ISpawnTreeInitializer* initializer = m_subInitializer.Get();
	if ( initializer )
	{
		initializer->OnInitData( instance );
	}
}
void ISpawnTreeInitializerIdleSmartAI::OnDeinitData( CSpawnTreeInstance& instance )
{
	TBaseClass::OnDeinitData( instance );
	ISpawnTreeInitializer* initializer = m_subInitializer.Get();
	if ( initializer )
	{
		initializer->OnDeinitData( instance );
	}
}

Bool ISpawnTreeInitializerIdleSmartAI::GenerateIdsRecursively()
{
	Bool result = TBaseClass::GenerateIdsRecursively();
	
	if ( ISpawnTreeInitializer* const initializer = m_subInitializer.Get() )
	{
		result |= initializer->GenerateIdsRecursively();
	}
	return result;
}

IScriptable* ISpawnTreeInitializerIdleSmartAI::GetObjectForPropertiesEdition()
{
	IScriptable* context = this;
	THandle< IScriptable > propObject;
	CallFunctionRet< THandle< IScriptable > >( context, CNAME( GetObjectForPropertiesEdition ), propObject );
	return propObject.Get();
}
String ISpawnTreeInitializerIdleSmartAI::GetEditorFriendlyName() const
{
	IScriptable* context = const_cast< ISpawnTreeInitializerIdleSmartAI* >( this );
	String friendlyName;
	CallFunctionRet< String >( context, CNAME( GetEditorFriendlyName ), friendlyName );
	return friendlyName;
}
Bool ISpawnTreeInitializerIdleSmartAI::IsSpawnable() const
{
	return GetClass() != ISpawnTreeInitializerIdleSmartAI::GetStaticClass();
}
CName ISpawnTreeInitializerIdleSmartAI::GetSubinitializerClass() const
{
	CObject* context = const_cast< ISpawnTreeInitializerIdleSmartAI* >( this );
	CName className;
	CallFunctionRet< CName >( context, CNAME( GetSubInitializerClassName ), className );
	return className;
}
Bool ISpawnTreeInitializerIdleSmartAI::CanAddChild() const
{
	if ( m_subInitializer.Get() )
	{
		return false;
	}
	CName className = GetSubinitializerClass();
	return !className.Empty();
}
void ISpawnTreeInitializerIdleSmartAI::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	CName className = GetSubinitializerClass();
	if ( className.Empty() )
	{
		return;
	}
	CClass* supportedClass = SRTTI::GetInstance().FindClass( className );
	if ( supportedClass )
	{
		rootClasses.PushBack( supportedClass );
	}
}
Bool ISpawnTreeInitializerIdleSmartAI::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
{
	return true;
}
void ISpawnTreeInitializerIdleSmartAI::AddChild( IEdSpawnTreeNode* node )
{
	m_subInitializer = static_cast< ISpawnTreeInitializer* >( node );
}

void ISpawnTreeInitializerIdleSmartAI::RemoveChild( IEdSpawnTreeNode* node )
{
	if ( node == m_subInitializer.Get() )
	{
		m_subInitializer = NULL;
	}
}
Int32 ISpawnTreeInitializerIdleSmartAI::GetNumChildren() const
{
	return m_subInitializer.Get() ? 1 : 0;
}
IEdSpawnTreeNode* ISpawnTreeInitializerIdleSmartAI::GetChild( Int32 index ) const
{
	return m_subInitializer.Get();
}

////////////////////////////////////////////////////////////////////
// ISpawnTreeInitializerCommunityAI
////////////////////////////////////////////////////////////////////

Bool ISpawnTreeInitializerCommunityAI::IsSpawnable() const
{
	return false;
}
CName ISpawnTreeInitializerCommunityAI::GetDynamicNodeEventName() const
{
	return CName::NONE;
}
