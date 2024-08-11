#include "build.h"
#include "behTreeVars.h"

#include "aiParameters.h"
#include "behTreeNode.h"

////////////////////////////////////////////////////////////////////////
// TBehTreeValue
////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( IBehTreeValueEnum )
IMPLEMENT_ENGINE_CLASS( CBehTreeValList )
IMPLEMENT_ENGINE_CLASS( CBehTreeValFloat )
IMPLEMENT_ENGINE_CLASS( CBehTreeValBool )
IMPLEMENT_ENGINE_CLASS( CBehTreeValInt )
IMPLEMENT_ENGINE_CLASS( CBehTreeValVector )
IMPLEMENT_ENGINE_CLASS( CBehTreeValTemplate )
IMPLEMENT_ENGINE_CLASS( CBehTreeValFile )
IMPLEMENT_ENGINE_CLASS( CBehTreeValString )
IMPLEMENT_ENGINE_CLASS( CBehTreeValCName )
IMPLEMENT_ENGINE_CLASS( CBehTreeValRes )
IMPLEMENT_ENGINE_CLASS( CBehTreeValEntityHandle )
IMPLEMENT_ENGINE_CLASS( CBehTreeValSteeringGraph )
IMPLEMENT_ENGINE_CLASS( CBehTreeValFormation )
IMPLEMENT_ENGINE_CLASS( CBehTreeSpawnContext )

Bool CBehTreeValList::GetValRef( const CBehTreeSpawnContext& context, TBehTreeValList& val ) const
{
	if( !TGetValRef( context, val ) )
	{
		val = m_value;
	}
	return true;
}
Float CBehTreeValFloat::GetVal( const CBehTreeSpawnContext& context ) const
{
	return TGetVal( context );
}
Bool CBehTreeValBool::GetVal( const CBehTreeSpawnContext& context ) const
{
	return TGetVal( context );
}
Int32 CBehTreeValInt::GetVal( const CBehTreeSpawnContext& context ) const
{
	return TGetVal( context );
}
Bool CBehTreeValVector::GetValRef( const CBehTreeSpawnContext& context, Vector& outVal ) const
{
	if( !TGetValRef( context, outVal ) )
	{
		outVal = m_value;
	}
	return true;
}
IBehTreeNodeDefinition* CBehTreeValTemplate::GetVal( const CBehTreeSpawnContext& context ) const
{
	return TGetVal( context );
}
CBehTree* CBehTreeValFile::GetVal( const CBehTreeSpawnContext& context ) const
{
	return TGetVal( context ).Get();
}
Bool CBehTreeValString::GetValRef( const CBehTreeSpawnContext& context, String& str ) const
{
	if ( !TGetValRef( context, str ) )
	{
		str = m_value;
	}
	return true;
}
CName CBehTreeValCName::GetVal( const CBehTreeSpawnContext& context ) const
{
	return TGetVal( context );
}
CAITree* CBehTreeValRes::GetVal( const CBehTreeSpawnContext& context ) const
{
	return TGetVal( context );
}
Bool CBehTreeValEntityHandle::GetValRef( const CBehTreeSpawnContext& context, EntityHandle& val ) const
{
	if( !TGetValRef( context, val ) )
	{
		val = m_value;
	}
	return true;	
}

CMoveSteeringBehavior* CBehTreeValSteeringGraph::GetVal( const CBehTreeSpawnContext& context ) const
{
	return TGetVal( context ).Get();
}

CFormation* CBehTreeValFormation::GetVal( const CBehTreeSpawnContext& context ) const
{
	return TGetVal( context ).Get();
}


void IBehTreeValueEnum::SetVal( Int32 val )
{
	CClass* classObj = GetClass();
	const auto& properties = classObj->GetCachedProperties();
	for ( auto it = properties.Begin(), end = properties.End(); it != end; ++it )
	{
		CProperty* prop = *it;
		if ( prop->GetName() == CNAME( value ) )
		{
			prop->Set(this, &val);
			break;
		}
	}
}

Int32 IBehTreeValueEnum::GetValue() const
{
	Int32 val = 0;
	CClass* classObj = GetClass();
	const auto& properties = classObj->GetCachedProperties();
	for ( auto it = properties.Begin(), end = properties.End(); it != end; ++it )
	{
		CProperty* prop = *it;
		if ( prop->GetName() == CNAME( value ) )
		{
			prop->Get( const_cast<IBehTreeValueEnum*>(this), &val );
			break;
		}
	}
	return val;
}

const IRTTIType* IBehTreeValueEnum::GetEnumType() const
{
	CClass* classObj = GetClass();
	const auto& properties = classObj->GetCachedProperties();
	for ( auto it = properties.Begin(), end = properties.End(); it != end; ++it )
	{
		CProperty* prop = *it;
		if ( prop->GetName() == CNAME( value ) )
		{
			return prop->GetType();
		}
	}
	return nullptr;
}

Int32 IBehTreeValueEnum::GetVal( const CBehTreeSpawnContext& context ) const
{
	Int32 val = 0;

	if ( !m_varName.Empty() )
	{
		return context.GetVal< Int32 >( m_varName, val );
	}

	return GetValue();
}


////////////////////////////////////////////////////////////////////////
// CBehTreeSpawnContext
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_EVENT_LISTENERS_BINDING
CBehTreeSpawnContext::~CBehTreeSpawnContext()
{
	RED_FATAL_ASSERT( m_eventListeners.Empty(), "Nie uzyte iventy" );
}
#endif

Bool CBehTreeSpawnContext::Push( const IAIParameters* aiParams )
{
	ASSERT( aiParams );
	if ( m_context.Size() < m_context.Capacity() )
	{
		m_context.PushBack( aiParams );
		return true;
	}
	ERR_GAME( TXT("AI have run out of spawning parameters context! Either your ai tree hierarchy is too deep, or you have made it looped foreva!") );
	return false;

}
void CBehTreeSpawnContext::Pop( const IAIParameters* aiParams )
{
	ASSERT( m_context.Back() == aiParams );
	m_context.PopBack();
}
void CBehTreeSpawnContext::Pop( Uint32 i )
{
	m_context.Resize( m_context.Size() - i );
}

void CBehTreeSpawnContext::AddEventListener( const SBehTreeEvenListeningData& e, IBehTreeNodeInstance* l )
{
	m_eventListeners.PushBack( SEventHandlerInfo() );
	m_eventListeners.Back().m_event.m_eventName = e.m_eventName;
	m_eventListeners.Back().m_event.m_eventType = e.m_eventType;
	m_eventListeners.Back().m_instance = l;
}
