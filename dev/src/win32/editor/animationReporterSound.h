
#pragma once

struct SAnimReportSound
{
	struct SParentAnimation
	{
		CName	m_animation;
		String	m_animset;
		Uint32	m_localNum;
		Uint32	m_externalNum;
	};

	String							m_soundEventName;
	TDynArray< SParentAnimation >	m_dependences;

	void Serialize( IFile& file )
	{
		file << m_soundEventName;

		Uint32 size = m_dependences.Size();
		file << size;

		if ( file.IsReader() )
		{
			m_dependences.Resize( size );
		}

		for ( Uint32 i=0; i<size; ++i )
		{
			SParentAnimation& a = m_dependences[ i ];

			file << a.m_animation;
			file << a.m_animset;
			file << a.m_localNum;
			file << a.m_externalNum;
		}
	}

	Bool IsEqual( const SAnimReportSound* s ) const
	{
		return m_soundEventName == s->m_soundEventName;
	}

	void CopyTo( SAnimReportSound* s )
	{
		s->m_dependences = m_dependences;
		s->m_soundEventName = m_soundEventName;
	}
};
