/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//	Iterates through template and its includes (breadth-first search)
//
//	Usage example:
//
//	for( EntityTemplateIterator iter( template ); !iter.Finished(); ++iter )
//	{
//		CEntityTemplate* templ = iter.Get();
//		if( templ )
//		{
//			...
//		}
//	}
//

class EntityTemplateIterator
{
private:
	TDynArray< THandle< CEntityTemplate > >		m_templates;
	Uint32										m_startIdx;
	Int32											m_currentIdx;

	static const Int32							FINISHED = -1;
	
public:
	EntityTemplateIterator( CEntityTemplate* entityTemplate )
		: m_startIdx( 0 )
		, m_currentIdx( 0 )
	{
		ASSERT( entityTemplate );
		m_templates.PushBack( entityTemplate );
	}

	CEntityTemplate* operator->() const
	{
		ASSERT( m_currentIdx != FINISHED );
		return m_templates[ m_currentIdx ].Get();
	}

	CEntityTemplate& operator*() const
	{
		ASSERT( m_currentIdx != FINISHED );
		return *( m_templates[ m_currentIdx ].Get() );
	}

	CEntityTemplate* Get() const
	{
		ASSERT( m_currentIdx != FINISHED );
		return m_templates[ m_currentIdx ].Get();
	}

	void operator++()
	{
		ASSERT( m_currentIdx != FINISHED );

		m_currentIdx++;

		// If template list ended, try adding included templates
		const Uint32 s = m_templates.Size();
		if ( m_currentIdx >= Int32(s) )
		{
			for( Uint32 i=m_startIdx; i<s; i++ )
			{
				CEntityTemplate* templ = m_templates[i].Get();
				if( templ )
				{
					m_templates.PushBack( templ->GetIncludes() );
				}
			}

			// If no new templates, break;
			if( m_templates.Size() == s )
			{
				m_currentIdx = FINISHED;				
			}

			m_startIdx = s;
		}
	}

	Bool Finished() const
	{
		return ( m_currentIdx == FINISHED );
	}
};