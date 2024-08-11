/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifdef USE_HAVOK_ANIMATION
class CHavokDataBufferParser
{
	hkVariant m_root;

public:
	CHavokDataBufferParser();
	CHavokDataBufferParser( const HavokDataBuffer& data );

public:
	void Clear();

	const hkVariant& SetRoot( const HavokDataBuffer& data );
	const hkVariant& SetRoot( const HavokDataBuffer* data );
	const hkVariant& GetRoot() const;

	void GetItemData( const hkVariant& var, hkString& name, hkArray< hkVariant >& children ) const;
};
#endif