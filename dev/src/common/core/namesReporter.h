/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/hashmap.h"
#include "../core/bitset.h"

#define NAMES_MAP_VESRION_INITIAL	1
#define NAMES_MAP_VERSION_MAP		2

#define NAMES_MAP_VERSION_CURRENT	NAMES_MAP_VERSION_MAP

class CNamesRemapper
{
public:
	CNamesRemapper() { Reset(); }

	static const Uint32 MAX_NUM_GLOBAL_NAMES = 256 * 1024; // meaning we take 1 MB for this 
	static const Uint32 MAX_NUM_LOCAL_NAMES = 32 * 1024; // meaning we take 128 kb for this 
	static const Uint32 NC_MAGIC_START = 'UNAM';
	static const Uint32 NC_MAGIC_END = 'DONE';

private:
	Red::Threads::CSpinLock			m_assignLock;
	Red::Threads::CAtomic< Uint32 > m_toLocal[ MAX_NUM_GLOBAL_NAMES ]; 
	Red::Threads::CAtomic< Uint32 > m_toGlobal[ MAX_NUM_LOCAL_NAMES ]; 
	Red::Threads::CAtomic< Uint32 >	m_counter;

public:
	RED_INLINE Uint32 Map( CName name ) { return name ? ToLocal( name.GetIndex() ) : 0; }
	RED_INLINE CName Map( Uint32 idx ) { return idx ? CName( ToGlobal( idx ) ) : CName::NONE; }

	void Reset();
	void Save( IFile* writer );
	void Load( IFile* reader );

private:
	Bool LoadData( IFile* reader, Uint32& loadedSkipOffset );
	Uint32 ToLocal( Uint32 global );
	Uint32 ToGlobal( Uint32 local );
	Uint32 NewMapping( Uint32 global );
};


