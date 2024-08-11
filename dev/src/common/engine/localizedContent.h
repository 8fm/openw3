#pragma once

#include "../core/object.h"
#include "../core/speechCollector.h"

RED_DECLARE_RTTI_NAME( LocalizedString );

class LocalizedString
{
	friend class IStringDBDataAccess;
	friend class CLocalizationManager;

private:
	// String id
	Uint32	m_index;

public:
	// Sets index of localized content
	void	SetIndex( Uint32 index ) { m_index = index; }

public:
	LocalizedString();
	~LocalizedString();

	Bool operator==( const LocalizedString& content ) const
	{
		return m_index == content.m_index;
	}

	//Assigns index in database if don't have one already
	void AssignIndex();

	// Gets index of localized content
	Uint32	GetIndex() const { return m_index; };

	// Gets string of localized content
	String	GetString() const;

	// Gets index in locale different from current
	String	GetString( const String& locale ) const;

	// Loads localized text asynchronously for later use
	Bool	Load() const;

	// Unloads localized text from cache
	void	Unload() const;

	// Sets string (remembers modification flag)
	void	SetString( const String& text, Bool setModified = true );

	Bool	CanModify();

	// Gets fallback flag
	Bool	IsFallback( ) const;

	// Gets modification flag
	Bool	IsModified( ) const;

	// Sets (or clears) modification flag
	void	SetModified( Bool modified );

	// Gets string flag
	Uint32	GetLang();

	Bool	MakeUniqueCopy();

	friend IFile& operator<<( IFile& file, LocalizedString &arg )
	{
		if( file.QuerySpeechCollector( ) != nullptr )
		{
			file.QuerySpeechCollector( )->ReportStringID( arg.m_index );
		}

		// Serialize string
		file.SerializeSimpleType( &arg.m_index, sizeof( Uint32 ) );
		return file;
	}
};

/*
Returns whether string ID is unstable.

See CLocalizationManager for more info on stable and unstable string IDs.
*/
RED_INLINE Bool IsUnstableStringId( Uint32 stringId )
{
	return stringId >= ( 1u << 31u ) || stringId == 0;
}

//////////////////////////////////////////////////////////////////////////
class CLocalizedContent : public CObject
{
	DECLARE_ENGINE_CLASS( CLocalizedContent, CObject, 0 )

	friend class IStringDBDataAccess;
	friend class CLocalizationManager;

private:
	// String id
	Uint32	m_index;

private:
	// Sets index of localized content
	void	SetIndex( Uint32 index ) { m_index = index; }

public:
	CLocalizedContent();

	Bool operator==( const CLocalizedContent& content ) const
	{
		return m_index == content.m_index;
	}

	// Gets index of localized content
	Uint32	GetIndex() const { return m_index; };

	// Gets string of localized content
	String	GetString() const;

	// Gets index in locale different from current
	String	GetString( const String& locale ) const;

	// Sets string (remembers modification flag)
	void	SetString( const String& text, Bool setModified = true );

	Bool	CanModify();

	// Gets fallback flag
	Bool	IsFallback( ) const;

	// Gets modification flag
	Bool	IsModified( ) const;

	// Sets (or clears) modification flag
	void	SetModified( Bool modified );

	// Gets string flag
	Uint32	GetLang();

	virtual void OnPostLoad();
};

BEGIN_CLASS_RTTI( CLocalizedContent );
	PARENT_CLASS( CObject );
#ifdef _DEBUG
	PROPERTY_EDIT( m_index, TXT( "String DB index" ) );
#else
	PROPERTY_RO( m_index, TXT( "String DB index" ) );
#endif
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////
// Localized String

class CSimpleRTTITypeLocalizedString : public TSimpleRTTIType< LocalizedString >
{
	virtual const CName& GetName() const { return CNAME( LocalizedString ); }

	virtual ERTTITypeType GetType() const { return RT_Simple; }

	virtual Bool Serialize( IFile& file, void* data ) const
	{
		LocalizedString *content = (LocalizedString *)data;
		/*Uint32 contentIndex = content->GetIndex();
		file << contentIndex;
		if ( file.IsReader() )
		{
			content->SetIndex( contentIndex );
		}
		return true;*/

		file << *content;
		return true;
	}

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		valueString = ::ToString( reinterpret_cast< const LocalizedString * >( data )->GetIndex() );
		return true;
	}

	virtual Bool FromString( void* data, const String& valueString ) const
	{
		Uint32 stringId = 0;
		::FromString( valueString, stringId );
		reinterpret_cast< LocalizedString * >( data )->SetIndex( stringId );
		return true;
	}
};

template<>
struct TTypeName< LocalizedString >
{													
	static const CName& GetTypeName() { return CNAME( LocalizedString ); }
};

void funcGetLocStringById( IScriptable* context, CScriptStackFrame& stack, void* result );
void funcGetLocStringByKey( IScriptable* context, CScriptStackFrame& stack, void* result );
void funcGetLocStringByKeyExt( IScriptable* context, CScriptStackFrame& stack, void* result );
void funcFixStringForFont( IScriptable* content, CScriptStackFrame& stack, void* result );

void RegisterLocalizedContentFunctions();