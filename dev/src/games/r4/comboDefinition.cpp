
#include "build.h"
#include "comboDefinition.h"

IMPLEMENT_ENGINE_CLASS( CComboDefinition );

Int32 CComboDefinition::FindAspectIndex( const CName& aspectName ) const
{
	const Uint32 size = m_aspects.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_aspects[ i ]->GetAspectName() == aspectName )
		{
			return i;
		}
	}
	return -1;
}

CComboAspect* CComboDefinition::FindComboAspect( const CName& aspectName ) const
{
	Int32 index = FindAspectIndex( aspectName );
	return index != -1 ? m_aspects[ index ] : NULL;
}

CComboAspect* CComboDefinition::CreateComboAspect( const CName& aspectName )
{
	CComboAspect* aspect = FindComboAspect( aspectName );
	if ( !aspect )
	{
		CComboAspect* aspect = CreateObject< CComboAspect >( this );
		aspect->SetAspectName( aspectName );

		m_aspects.PushBack( aspect );

		return aspect;
	}

	return NULL;
}

Bool CComboDefinition::DeleteComboAspect( const CName& attackStyle )
{
	const Int32 index = FindAspectIndex( attackStyle );
	if ( index != -1 )
	{
		m_aspects.RemoveAt( index );
		return true;
	}

	return false;
}

void CComboDefinition::DebugDumpToLog()
{
	const Uint32 size = m_aspects.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		//...
	}
}

//////////////////////////////////////////////////////////////////////////

void CComboDefinition::funcCreateComboAspect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, attackType, CName::NONE );
	FINISH_PARAMETERS;

	CComboAspect* aspect = CreateComboAspect( attackType );

	RETURN_OBJECT( aspect );
}

void CComboDefinition::funcDeleteComboAspect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, attackType, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( DeleteComboAspect( attackType ) );
}

void CComboDefinition::funcFindComboAspect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, attackType, CName::NONE );
	FINISH_PARAMETERS;

	CComboAspect* aspect = FindComboAspect( attackType );

	RETURN_OBJECT( aspect );
}

void CComboDefinition::funcDebugDumpToLog( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	DebugDumpToLog();
}

//////////////////////////////////////////////////////////////////////////
