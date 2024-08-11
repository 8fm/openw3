
#pragma once

struct ACNode
{
	Uint64				m_hash;

	TDynArray< String > m_behaviors;
	TDynArray< String > m_animsets;

	Uint32				m_usedAnims;

	TDynArray< String > m_owners;

	void Serialize( IFile& file )
	{
		file << m_hash;
		file << m_behaviors;
		file << m_animsets;
		file << m_usedAnims;
		file << m_owners;
	}
};

struct APNode
{
	Uint64				m_hash;

	String				m_jobTreePath;

	TDynArray< String > m_owners;

	void Serialize( IFile& file )
	{
		file << m_hash;
		file << m_jobTreePath;
		file << m_owners;
	}
};
