/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "sortedmap.h"

/// Basic URL parser
class CBasicURL
{
public:
	CBasicURL();
	CBasicURL( const CBasicURL& other );
	CBasicURL( CBasicURL&& other );

	// Parse URL from string
	static CBasicURL Parse( const StringAnsi& text, const Bool isRelative = false );

	// Parse parameters (POST payload)
	static CBasicURL ParseParams( const StringAnsi& text );

	// Decode URL string (% encoding)
	static StringAnsi Decode( const StringAnsi& text );

	// Set new protocol value
	void SetProtocol( const StringAnsi& protocol );

	// Set new address value
	void SetAddress( const StringAnsi& address );

	// Set new path
	void SetPath( const StringAnsi& path );

	// Set new value of key
	void SetKey( const StringAnsi& key, const StringAnsi& value );

	// Set new value of key (integer version)
	void SetKey( const StringAnsi& key, const Int32 value );

	// Convert URL to full string representation
	StringAnsi ToString() const;

	// Get protocol name
	RED_INLINE const StringAnsi& GetProtocol() const { return m_protocol; }

	// Get address
	RED_INLINE const StringAnsi& GetAddress() const { return m_address; }

	// Get server path
	RED_INLINE const StringAnsi& GetPath() const { return m_path; }

	// Get number of keys
	RED_INLINE const Uint32 GetNumKeys() const { return m_keys.Size(); }

	// Get key name
	RED_INLINE const TSortedMap< StringAnsi,StringAnsi >& GetKeys() const { return m_keys; }

	// Do we have a key ?
	RED_INLINE const Bool HasKey( const StringAnsi& key ) const { return m_keys.KeyExist(key); }

	// Get key value
	RED_INLINE const Bool GetKey( const StringAnsi& key, StringAnsi& outValue ) const { return m_keys.Find( key, outValue ); }

	// Get key value
	RED_INLINE const Bool GetKey( const StringAnsi& key, Int32& outValue ) const
	{
		StringAnsi txt;
		if ( !m_keys.Find( key, txt ) )
			return false;
		outValue = atoi(txt.AsChar());
		return true;
	}

private:
	typedef TSortedMap< StringAnsi,StringAnsi > TKeys;

	static const AnsiChar*		URL_PROTOCOL;	// "://"
	static const AnsiChar		URL_SEPARATOR;	// "/"
	static const AnsiChar		URL_FIRSTKEY;   // "?"
	static const AnsiChar		URL_KEY;		// "&"
	static const AnsiChar		URL_EQUALS;		// "="

	static Bool ParseURLPart( const AnsiChar*& pos, StringAnsi& out );
	static Bool ParseURLPath( const AnsiChar*& pos, StringAnsi& out );
	static Bool ParseURLKey( const AnsiChar*& pos, StringAnsi& out );
	static Bool MatchURLPart( const AnsiChar*& pos, const AnsiChar* txt );
	static Bool MatchURLPart( const AnsiChar*& pos, const AnsiChar ch );

	StringAnsi		m_protocol;		// protocol name (http)
	StringAnsi		m_address;		// address (localhost)
	StringAnsi		m_path;			// full path (test/dupa.html)
	TKeys			m_keys;			// matched keys (?a=100&b=50)
};