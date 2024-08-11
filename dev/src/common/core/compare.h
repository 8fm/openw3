/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

template < typename K >
struct DefaultCompareFunc
{
	static RED_INLINE Bool Less( const K& key1, const K& key2 ) { return key1 < key2; }	
};

template < typename K >
struct DefaultEqualFunc
{
	static RED_INLINE Bool Equal( const K& key1, const K& key2 ) { return key1 == key2; }
};
