
#pragma once

#include "animationCache2.h"

enum ECookingPlatform : Int32;

class AnimationCacheCooker
{
private:
	THashMap< Uint64, Uint32 >			m_entriesMap;
	TDynArray< AnimCacheEntry >		m_entriesList;

	IFile*							m_file;
	Uint32							m_headerDataOffset;
	Uint32							m_animStartOffset;

public:
	AnimationCacheCooker( const String& absoluteFileName, ECookingPlatform platform );
	~AnimationCacheCooker();

	Bool GetAnimationData( Uint64 hash , Uint32& index, Uint32& animSize ) const;
	void AddAnimation( Uint32& index, Uint32& animSize, CSkeletalAnimation* animation, Uint64 hash );

	void Save();

	Bool CheckData() const;

private:
	void SaveAnimation( CSkeletalAnimation* animation, Uint32& offset, Uint32& size );
	Uint32 SaveEntries();
	Bool LoadFromFile( const String& absoluteFileName );
	void CreateFileHeader( ECookingPlatform platform );
	void CloseFile();
};

extern AnimationCacheCooker* GAnimationCacheCooker;
