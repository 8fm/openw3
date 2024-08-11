#include "build.h"
#include "journalBase.h"
#include "journalResource.h"
#include "../core/depot.h"

IMPLEMENT_ENGINE_CLASS( CJournalBase )

RED_DEFINE_NAME( baseName );

CJournalBase::CJournalBase()
	: m_active( false )
{

}

CJournalBase::~CJournalBase()
{

}

void CJournalBase::Initialize( Uint32 order, const Char* baseName /*= NULL*/ )
{
	m_guid = CGUID::Create();

	m_order = order;

	if ( baseName )
	{
		m_baseName = baseName;
	}
	else
	{
		m_baseName = TXT( "New Item" );
	}
}

void CJournalBase::funcGetUniqueScriptTag( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_NAME( GetUniqueScriptIdentifier() );
}

void CJournalBase::funcGetOrder( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	// because for some reason we use rediculously huge values here sometimes and we actually somehow end up losing values when converting to int
	// and its needed for objective sorting, sooo........ ya this must be done
	RETURN_INT( Int32(GetOrder() - INT_MAX) );
}

void CJournalBase::funcIsActive( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsActive() );
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalChildBase )

CJournalChildBase::CJournalChildBase()
{

}

CJournalChildBase::~CJournalChildBase()
{

}

void CJournalChildBase::Initialize( const CGUID& parentGuid, Uint32 order, const Char* baseName /*= NULL*/ )
{
	CJournalBase::Initialize( order, baseName );

	m_parentGuid = parentGuid;

	DefaultValues();
}

void CJournalChildBase::funcGetLinkedParentGUID( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( CGUID, GetLinkedParentGUID() );
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalContainerEntry )

CJournalContainerEntry::CJournalContainerEntry()
{

}

CJournalContainerEntry::~CJournalContainerEntry()
{

}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalContainer )

CJournalContainer::CJournalContainer()
{

}

CJournalContainer::~CJournalContainer()
{

}

void CJournalContainer::AddChildEntry( CJournalContainerEntry* child )
{
	child->SetIndex( static_cast< Uint8 >( m_children.Size() ) );
	m_children.PushBack( child );

	// For RTTI and serialisation
	child->SetParent( this );
}

void CJournalContainer::RemoveChildEntry( CJournalContainerEntry* child )
{
	ASSERT( child != NULL );

	RemoveChildEntry( child->GetIndex() );
}

void CJournalContainer::RemoveChildEntry( Uint32 childIndex )
{
	m_children.RemoveAt( childIndex );

	// Update indexes for altered phases - this should be the only time the index changes for a phase
	for( Uint32 i = childIndex; i < m_children.Size(); ++i )
	{
		m_children[ i ]->SetIndex( static_cast< Uint8 >( i ) );
	}
}

void CJournalContainer::funcGetChild( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, -1 );
	FINISH_PARAMETERS;

	CJournalBase* child = NULL;

	if( static_cast< Uint32 >( index ) < GetNumChildren() )
	{
		child = GetChild( static_cast< Uint32 >( index ) );
	}

	RETURN_OBJECT( child );
}

void CJournalContainer::funcGetNumChildren( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( GetNumChildren() );
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CJournalLink )

CJournalLink::CJournalLink()
{

}

CJournalLink::~CJournalLink()
{

}

CJournalResource* CJournalLink::GetLinkedResource()
{
	CJournalResource* linkedResource = Cast< CJournalResource >( GDepot->LoadResource( m_linkedObjectPath ) );
	if ( linkedResource != nullptr )
	{
		return linkedResource;
	}
	return GetParentAs< CJournalResource >(); // for safety reasons, let's just return link resource if nothing linked was found
}

CJournalBase* CJournalLink::GetLinkedObject()
{
	CJournalResource* linkedResource = GetLinkedResource();
	if ( linkedResource != nullptr )
	{
		return linkedResource->Get();
	}
	return this; // for safety reasons, let's just return itself if nothing linked was found
}