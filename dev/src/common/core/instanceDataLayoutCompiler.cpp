/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "instanceDataLayoutCompiler.h"
#include "instanceVar.h"
#include "instanceDataLayout.h"
#include "rttiType.h"

InstanceDataLayoutCompiler::InstanceDataLayoutCompiler( InstanceDataLayout& dataLayout )
	: m_currentSize( 0 )
	, m_dataLayout( &dataLayout )
{
}

static Uint32 CalculateAlignment( Uint32 variableSize )
{
	if ( variableSize == 1 )
	{
		return 1;
	}
	else if ( variableSize == 2 )
	{
		return 2;
	}
	else if ( variableSize <= 4 )
	{
		return 4;
	}
	else if ( variableSize <= 8 )
	{
		return 8;
	}
	else if ( variableSize <= 12 )
	{
		return 12;
	}

	return 16;
}

InstanceDataLayoutCompiler& InstanceDataLayoutCompiler::operator<<( InstanceVar& var )
{
	ASSERT( var.GetType() );

	// Make sure variable is not already registered
#ifdef _DEBUG
	for ( Uint32 i=0; i<m_variables.Size(); i++ )
	{
		if ( m_variables[i].m_first == &var )
		{
			HALT( "Multiple registration of the same instance variable. Please DEBUG." );
			return *this;
		}
	}
#endif

	// Get the size of the variable to add
	const Uint32 varSize = var.GetType()->GetSize();
	const Uint32 alignment = CalculateAlignment( varSize );

	// Calculate offset at which the variable will be added
	Uint32 addOffset = static_cast< Uint32 >( AlignOffset( m_currentSize, alignment ) );
	m_currentSize = addOffset + varSize;

	#if defined(NO_EDITOR)

		var.m_offset = addOffset;
#ifdef RED_ASSERTS_ENABLED
		var.m_index = m_dataLayout->m_vars.Size();
#endif
		m_dataLayout->m_vars.PushBack( &var );

		if ( var.GetType()->NeedsCleaning() || var.GetType()->GetType() == RT_Class )
		{
			m_dataLayout->m_destructVars.PushBack( &var );
		}

	#else

		// Insert the variable into the compiled layout
		TPair< InstanceVar*, Uint32 > info( &var, addOffset );
		m_variables.PushBack( info );

	#endif

	// Needed by the stream operator
	return *this;
}

