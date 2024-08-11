
#pragma once

class DefaultCharactersIterator
{
	TDynArray< String > m_paths;
	TDynArray< String > m_names;
	TDynArray< String > m_cats;
	Uint32				m_index;

public:
	DefaultCharactersIterator()
		: m_index( 0 )
	{
		LoadPaths();
	}

	RED_INLINE operator Bool () const
	{
		return m_index < m_paths.Size();
	}

	RED_INLINE void operator++ ()
	{
		m_index++;
	}

	RED_INLINE void GoTo( Uint32 index )
	{
		m_index = index;
	}

	RED_INLINE CEntityTemplate* GetTemplate() const
	{
		return NULL;
	}

	RED_INLINE String GetPath() const
	{
		return m_paths[ m_index ];
	}

	RED_INLINE String GetName() const
	{
		return m_names[ m_index ];
	}

private:
	void LoadPaths();
};
