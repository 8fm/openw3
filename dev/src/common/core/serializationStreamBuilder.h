/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "hashSet.h"
#include "serializationStreamData.h"

/// Helper class to build serialization stream
class CSerializationStreamBuilder
{
public:
	CSerializationStreamBuilder();

	// Write serialization stream
	void WriteStream( IFile& file, const CClass* objectClass, const void* data, const void* base );

private:
	// Property stuff
	void WritePropertyData( IFile& file, const CProperty* prop, const void* propData, const void* baseData );
	void WriteTypeData( IFile& file, const IRTTIType* type, const void* propData, const void* baseData  );

	// Array stuff
	void WriteArrayData( IFile& file, const IRTTIType* type, const void* propData, const void* baseData );

	// Stream writer
	void WriteUint8( IFile& file, const Uint8 value );
	void WriteUint16( IFile& file, const Uint16 value );
	void WriteUint32( IFile& file, const Uint32 value );
	void WriteUint64( IFile& file, const Uint64 value );
	void WriteProperty( IFile& file, const CProperty* prop );
	void WriteType( IFile& file, const IRTTIType* type );
	void WriteName( IFile& file, const CName name );
	void WriteBuf( IFile& file, const void* data, const Uint32 length );

	// cached types
	IRTTIType*	m_cachedTypeBool;
	IRTTIType*	m_cachedTypeName;
	IRTTIType*	m_cachedTypeString;
};