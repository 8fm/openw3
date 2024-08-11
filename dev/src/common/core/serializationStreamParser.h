/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "serializationStreamData.h"

/// Parser (and executer) for serialization stream
class CSerializationStreamParser
{
public:
	CSerializationStreamParser();

	// parse serialization stream data
	void ParseStream( IFile& fallbackReader, const CFileDirectSerializationTables& tables, const void* streamData, const Uint32 streamSize, void* targetPtr, const CClass* targetClass );
};