/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraph.h"
#include "../core/diskFile.h"

IMPLEMENT_ENGINE_CLASS( CBaseBehaviorVariable );

IMPLEMENT_ENGINE_CLASS( CBehaviorVariable );

#ifndef NO_EDITOR_GRAPH_SUPPORT
const Char* CBehaviorVariable::GetParentBehaviorGraphDepotPath() const
{
	if (CBehaviorGraph* graph = Cast<CBehaviorGraph>(GetParent()))
	{
		if (graph->GetFile())
		{
			return graph->GetFile()->GetFileName().AsChar();
		}
		else
		{
			return graph->GetDepotPath().AsChar();
		}
	}
	return TXT("<no file>");
}

void CBaseBehaviorVariable::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == CNAME( name ) )
	{
		if ( CBehaviorGraph* graph = Cast<CBehaviorGraph>(GetParent()) )
		{
			graph->OnVariableNameChanged();
		}
	}
}

#endif

void CBehaviorVariablesList::AddVariable( const CName name,
										  Float value /* = 0.0f */,
										  Float minValue /* = 0.0f */,
										  Float maxValue /* = 1.0f */ )
{
	if ( !GetVariable( name ) )
	{
		CBehaviorVariable* newVariable = CreateObject< CBehaviorVariable >( m_behaviorGraph );
		newVariable->m_name = name;
		newVariable->m_value = value;
		newVariable->m_minValue = minValue;
		newVariable->m_maxValue = maxValue;
		newVariable->m_varIndex = m_varIndexGenerator++;

		m_variables.Insert( name, newVariable );		
	}
}

void CBehaviorVariablesList::RemoveVariable( const CName name )
{
	m_variables.Erase( name );
}

CBehaviorVariable* CBehaviorVariablesList::GetVariable( const CName name ) const
{
	CBehaviorVariable*const* var = m_variables.FindPtr( name );
	return var ? *var : nullptr;
}

Uint32 CBehaviorVariablesList::GetNumVariables() const
{
	return m_variables.Size();
}

void CBehaviorVariablesList::ReserveAdditionalVariables( Uint32 num )
{
	m_variables.Reserve( m_variables.Size() + num );
}

void CBehaviorVariablesList::Reset()
{
	for ( auto it = m_variables.Begin(), end = m_variables.End(); it != end; ++it )
	{
		it->m_second->Reset();
	}
}

void CBehaviorVariablesList::GetVariables( TDynArray< CBehaviorVariable* >& variables ) const
{
	variables.ClearFast();
	variables.Reserve( m_variables.Size() );
	for ( auto it = m_variables.Begin(), end = m_variables.End(); it != end; ++it )
	{
		variables.PushBack( it->m_second );
	}
}

Uint32 CBehaviorVariablesList::GetSize() const
{
	return static_cast<Uint32>( sizeof( CBehaviorVariablesList ) + m_variables.DataSize() );
}

Bool CBehaviorVariablesList::DoesContain( CBehaviorVariable* variable ) const
{
	// TODO: Can we do by-name lookup instead like this?
	// return m_variables.ExistKey( variable->m_name );

	for ( auto it = m_variables.Begin(), end = m_variables.End(); it != end; ++it )
	{
		if ( variable == it->m_second )
		{
			return true;
		}
	}
	return false;
}

CBehaviorVariablesList& CBehaviorVariablesList::operator=( const CBehaviorVariablesList& rhs )
{
	ASSERT( m_behaviorGraph );
	if ( !m_behaviorGraph )
	{
		return *this;
	}

	m_variables.ClearFast();
	m_variables.Reserve( rhs.m_variables.Size() );
	for ( auto it = rhs.m_variables.Begin(), end = rhs.m_variables.End(); it != end; ++it )
	{
		CBehaviorVariable* newVariable = CreateObject< CBehaviorVariable >( m_behaviorGraph );
		newVariable->Set( it->m_second );
		m_variables.Insert( it->m_first, newVariable );
	}
	m_varIndexGenerator = rhs.m_varIndexGenerator;

	return *this;
}

IFile& operator<<( IFile &file, CBehaviorVariablesList &list )
{
	if ( file.IsReader() && file.GetVersion() < VER_BEHAVIOR_VARS_BY_NAME )
	{
		file << list.m_oldVariables;
	}
	else
	{
		file << list.m_varIndexGenerator;
		file << list.m_variables;
	}

	return file;
}

void CBehaviorVariablesList::OnPostLoad()
{
	if ( !m_oldVariables.Empty() )
	{
		// Hashmap needs to be built in OnPostLoad() (and not in operator <<) because behavior vars were not fully serialized before (so, e.g. their names were None before)

		m_variables.Reserve( m_oldVariables.Size() );

		m_varIndexGenerator = 0;
		for ( auto it = m_oldVariables.Begin(), end = m_oldVariables.End(); it != end; ++it )
		{
			( *it )->m_varIndex = m_varIndexGenerator++;
			m_variables.Insert( ( *it )->m_name, *it );
		}

		m_oldVariables.Clear();
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorVariablesList::OnVariableNameChanged()
{
	// Move vars to temp container

	TDynArray< CBehaviorVariable* > temp;
	temp.Reserve( m_variables.Size() );
	for ( auto it = m_variables.Begin(), end = m_variables.End(); it != end; ++it )
	{
		temp.PushBack( it->m_second );
	}

	// Reinsert all elements

	m_variables.ClearFast();
	for ( auto it = temp.Begin(), end = temp.End(); it != end; ++it )
	{
		m_variables.Insert( ( *it )->GetName(), *it );
	}
}

#endif

/////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EVectorVariableType );
IMPLEMENT_RTTI_ENUM( EVariableSpace );
IMPLEMENT_ENGINE_CLASS( CBehaviorVectorVariable );

void CBehaviorVectorVariablesList::AddVariable(const CName name
											   , const Vector& defaultValue /* = Vector::ZEROS */
											   , const Vector& minValue /* = Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX) */
											   , const Vector& maxValue /* = Vector(FLT_MAX, FLT_MAX, FLT_MAX) */)
{
	if ( !GetVariable( name ) )
	{
		CBehaviorVectorVariable* newVariable = CreateObject<CBehaviorVectorVariable>( m_behaviorGraph );
		newVariable->m_name = name;
		newVariable->m_value = defaultValue;
		newVariable->m_minValue = minValue;
		newVariable->m_maxValue = maxValue;
		newVariable->m_varIndex = m_varIndexGenerator++;

		m_variables.Insert( name, newVariable );		
	}
}

void CBehaviorVectorVariablesList::RemoveVariable( const CName name )
{
	m_variables.Erase( name );
}

CBehaviorVectorVariable* CBehaviorVectorVariablesList::GetVariable( const CName name ) const
{
	CBehaviorVectorVariable*const* var = m_variables.FindPtr( name );
	return var ? *var : nullptr;
}

Uint32 CBehaviorVectorVariablesList::GetNumVariables() const
{
	return m_variables.Size();
}

void CBehaviorVectorVariablesList::ReserveAdditionalVariables( Uint32 num )
{
	m_variables.Reserve( m_variables.Size() + num );
}

void CBehaviorVectorVariablesList::GetVariables( TDynArray< CBehaviorVectorVariable* >& variables ) const
{
	variables.ClearFast();
	variables.Reserve( m_variables.Size() );
	for ( auto it = m_variables.Begin(), end = m_variables.End(); it != end; ++it )
	{
		variables.PushBack( it->m_second );
	}
}

void CBehaviorVectorVariablesList::Reset()
{
	for ( auto it = m_variables.Begin(), end = m_variables.End(); it != end; ++it )
	{
		it->m_second->Reset();
	}
}

Uint32 CBehaviorVectorVariablesList::GetSize() const
{
	return static_cast< Uint32 >( sizeof( CBehaviorVectorVariablesList ) + m_variables.DataSize() );
}

Bool CBehaviorVectorVariablesList::DoesContain( CBehaviorVectorVariable* variable ) const
{
	for ( auto it = m_variables.Begin(); it != m_variables.End(); ++it )
	{
		if ( variable == it->m_second )
		{
			return true;
		}
	}
	return false;
}

CBehaviorVectorVariablesList& CBehaviorVectorVariablesList::operator=( const CBehaviorVectorVariablesList& rhs )
{
	ASSERT( m_behaviorGraph );
	if ( !m_behaviorGraph )
	{
		return *this;
	}

	m_variables.ClearFast();
	m_variables.Reserve( rhs.m_variables.Size() );
	for ( auto it = rhs.m_variables.Begin(), end = rhs.m_variables.End(); it != end; ++it )
	{
		CBehaviorVectorVariable* newVariable = CreateObject< CBehaviorVectorVariable >( m_behaviorGraph );
		newVariable->Set( it->m_second );
		m_variables.Insert( it->m_first, newVariable );
	}
	m_varIndexGenerator = rhs.m_varIndexGenerator;

	return *this;
}

IFile& operator<<( IFile &file, CBehaviorVectorVariablesList &list )
{
	if ( file.IsReader() && file.GetVersion() < VER_BEHAVIOR_VARS_BY_NAME )
	{
		file << list.m_oldVariables;
	}
	else
	{
		file << list.m_varIndexGenerator;
		file << list.m_variables;
	}
	return file;
}

void CBehaviorVectorVariablesList::OnPostLoad()
{
	if ( !m_oldVariables.Empty() )
	{
		// Hashmap needs to be built in OnPostLoad() (and not in operator <<) because behavior vars were not fully serialized before (so, e.g. their names were None before)

		m_variables.Reserve( m_oldVariables.Size() );

		m_varIndexGenerator = 0;
		for ( auto it = m_oldVariables.Begin(), end = m_oldVariables.End(); it != end; ++it )
		{
			( *it )->m_varIndex = m_varIndexGenerator++;
			m_variables.Insert( ( *it )->m_name, *it );
		}

		m_oldVariables.Clear();
	}
}

Vector CBehaviorVectorVariable::GetValue() const 
{ 
	// 	if (m_space == VS_Global)
	// 	{
	// 		Vector value = m_value;
	// 		CBehaviorGraph* graph = SafeCast<CBehaviorGraph>( GetParent() );
	// 		CAnimatedComponent* animComp = graph->GetAnimatedComponent();
	// 		if (animComp)
	// 		{
	// 			if (m_type == VVT_Position)
	// 			{
	// 				value = animComp->GetLocalToWorld().FullInverted().TransformVectorAsPoint(value);
	// 			}
	// 			else if (m_type == VVT_Rotation)
	// 			{
	// 				// TODO
	// 				ASSERT(0 && !"TODO");
	// 			}
	// 		}
	// 
	// 		return value;
	// 	}
	// 	else
	return m_value; 
}

Vector CBehaviorVectorVariable::GetVectorValue() const
{ 
	// 	if (m_space == VS_Global)
	// 	{
	// 		Vector value = m_value;
	// 		CBehaviorGraph* graph = SafeCast<CBehaviorGraph>( GetParent() );
	// 		CAnimatedComponent* animComp = graph->GetAnimatedComponent();
	// 		if (animComp)
	// 		{
	// 			if (m_type == VVT_Position)
	// 			{
	// 				value = animComp->GetLocalToWorld().FullInverted().TransformVectorAsPoint(value);
	// 			}
	// 			else if (m_type == VVT_Rotation)
	// 			{
	// 				// TODO
	// 				ASSERT(!"TODO - press continue");
	// 			}
	// 		}
	// 
	// 		return value;
	// 	}
	// 	else
	return m_value; 
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorVectorVariablesList::OnVariableNameChanged()
{
	// Move vars to temp container

	TDynArray< CBehaviorVectorVariable* > temp;
	temp.Reserve( m_variables.Size() );
	for ( auto it = m_variables.Begin(), end = m_variables.End(); it != end; ++it )
	{
		temp.PushBack( it->m_second );
	}

	// Reinsert all elements

	m_variables.ClearFast();
	for ( auto it = temp.Begin(), end = temp.End(); it != end; ++it )
	{
		m_variables.Insert( ( *it )->GetName(), *it );
	}
}

#endif