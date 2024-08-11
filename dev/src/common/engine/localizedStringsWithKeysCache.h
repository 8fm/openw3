/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "localizableObject.h"

class CLocalizedStringsWithKeysCache
{
public:
	void InitializeCache();
	void ClearCache();
	String GetCachedStringByKey( const String &stringKey );

protected:
	Bool AddLocStringEntryCache( Uint32 stringId, const String &stringKey, const String &stringCategory );	// Depracated

private:
	THashMap< String, String > m_cacheStrings;
};
