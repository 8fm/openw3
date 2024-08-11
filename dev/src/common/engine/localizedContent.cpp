#include "build.h"
#include "localizedContent.h"
#include "localizationManager.h"
#include "../core/scriptStackFrame.h"
#include "../core/feedback.h"
#include "game.h"

IMPLEMENT_ENGINE_CLASS( CLocalizedContent )
IMPLEMENT_SIMPLE_RTTI_TYPE( LocalizedString );


LocalizedString::LocalizedString()
	: m_index( 0 )
{
}

LocalizedString::~LocalizedString()
{
	SLocalizationManager::GetInstance().ReleaseLanguagePack( m_index );
}

String LocalizedString::GetString() const
{
	return SLocalizationManager::GetInstance().GetString( m_index, true );
}

String LocalizedString::GetString( const String& locale ) const
{
	// If this function is called for current locale then we have to delegate
	// our task to GetString() so that modified strings are also considered.
	if( locale == SLocalizationManager::GetInstance().GetCurrentLocale() )
	{
		return GetString();
	}

	return SLocalizationManager::GetInstance().GetLocalizedText( m_index, locale );
}

void LocalizedString::SetString( const String& text, Bool setModified /*= true */ )
{
	if( IsFallback() == true && ( text == TXT("") ) )
	{
		// this really doesn't modify anything because when fallback actual string in database doesn't exist yet
		// we do not want lots of empty inserts to db
		return;
	}

	String oldString  = SLocalizationManager::GetInstance().GetString( m_index, false );
	if( text != oldString )
	{
		if( setModified )
		{
			if ( CanModify() == false )
			{
				return;
			}
		}

		AssignIndex();
		
		SLocalizationManager::GetInstance().ModifyString( m_index, text );
		
	}
	else
	{
		SLocalizationManager::GetInstance().DiscardStringModification( m_index );
	}
}

Bool LocalizedString::CanModify()
{
	if ( GGame && GGame->IsActive() )
	{
		ASSERT( TXT( "Nothing should modify strings during game - fix your code!" ) );
		GFeedback->ShowError( TXT( "Nothing should modify strings during game - fix your code!" ) );
		return false;
	}

	if( ( m_index != 0 ) && !SLocalizationManager::GetInstance().IsConnected() )
	{
		if( !SLocalizationManager::GetInstance().OpenSomeDataAccess( true ) )
		{
			GFeedback->ShowError( TXT("Unable to modify string without connecting to sql db") );
			ERR_ENGINE( TXT("Unable to modify string without connecting to sql db") );
			return false;
		}
	}
	return true;
}

Uint32 LocalizedString::GetLang()
{
	return SLocalizationManager::GetInstance().GetCurrentLocaleId();
}

Bool LocalizedString::MakeUniqueCopy()
{
	if ( m_index != 0 )
	{
		m_index = SLocalizationManager::GetInstance().CopyString( m_index );
		return true;
	}
	else
	{
		return false;
	}
}

Bool LocalizedString::IsFallback() const
{
	return SLocalizationManager::GetInstance().IsFallbackString( m_index );
}

Bool LocalizedString::IsModified() const
{
	return SLocalizationManager::GetInstance().IsModifiedString( m_index );
}

void LocalizedString::SetModified( Bool modified )
{
	if ( modified == false )
	{
		SLocalizationManager::GetInstance().DiscardStringModification( m_index );
	}
	//SLocalizationManager::GetInstance().MarkModifiedString( m_index, modified );
}

Bool LocalizedString::Load() const
{
	// return true both for missing or not defined localized strings to prevent constant loading attempts
	// and how about renaming this function to LoadAsync to emphasize its behavior?
	if ( m_index == 0 )
	{
		return true;
	}
	if ( SLocalizationManager::GetInstance().IsMissingString( m_index ) )
	{
		return true;
	}
	return SLocalizationManager::GetInstance().GetLanguagePackAsync( m_index ) != NULL;
}

void LocalizedString::Unload() const
{
	SLocalizationManager::GetInstance().ReleaseLanguagePack( m_index );
}

void LocalizedString::AssignIndex()
{
	if ( m_index == 0 )
	{
		m_index = SLocalizationManager::GetInstance().RegisterNewString();
	}
}

//////////////////////////////////////////////////////////////////////////

CLocalizedContent::CLocalizedContent()
{
}

String CLocalizedContent::GetString() const
{
	return SLocalizationManager::GetInstance().GetString( m_index, true );
}

String CLocalizedContent::GetString( const String& locale ) const
{
	return SLocalizationManager::GetInstance().GetLocalizedText( m_index, locale );
}

void CLocalizedContent::SetString( const String& text, Bool setModified /*= true */ )
{
	if( IsFallback() == true && ( text == TXT("") ) )
	{
		// this really doesn't modify anything because when fallback actual string in database doesn't exist yet
		// we do not want lots of empty inserts to db
		return;
	}

	String oldString  = SLocalizationManager::GetInstance().GetString( m_index, false );
	if( text != oldString )
	{
		if( setModified )
		{
			if ( CanModify() == false )
			{
				return;
			}
			else
			{
				LOG_ENGINE( TXT( "String %u was set as modified, old text '%ls', new text '%ls'" ), m_index, oldString.AsChar(), text.AsChar() );
			}
		}

		if ( m_index == 0 )
		{
			m_index = SLocalizationManager::GetInstance().RegisterNewString();
		}

		SLocalizationManager::GetInstance().ModifyString( m_index, text );

	}
	else
	{
		SLocalizationManager::GetInstance().DiscardStringModification( m_index );
	}
}

Bool CLocalizedContent::CanModify()
{
	if ( GGame && GGame->IsActive() )
	{
		ASSERT( TXT( "Nothing should modify strings during game - fix your code!" ) );
		GFeedback->ShowError( TXT( "Nothing should modify strings during game - fix your code!" ) );
		return false;
	}

	if( ( m_index != 0 ) && !SLocalizationManager::GetInstance().IsConnected() )
	{
		if( !SLocalizationManager::GetInstance().OpenSomeDataAccess( true ) )
		{
			GFeedback->ShowError( TXT("Unable to modify string without connecting to sql db") );
			ERR_ENGINE( TXT("Unable to modify string without connecting to sql db") );
			return false;
		}
	}
	return true;
}

void CLocalizedContent::OnPostLoad()
{
	//SLocalizationManager::GetInstance().ReadFromStringDatabase( this );
}

Uint32 CLocalizedContent::GetLang()
{
	return SLocalizationManager::GetInstance().GetCurrentLocaleId();
}

Bool CLocalizedContent::IsFallback() const
{
	return SLocalizationManager::GetInstance().IsFallbackString( m_index );
}

Bool CLocalizedContent::IsModified() const
{
	return SLocalizationManager::GetInstance().IsModifiedString( m_index );
}

void CLocalizedContent::SetModified( Bool modified )
{
	if ( modified == false )
	{
		SLocalizationManager::GetInstance().DiscardStringModification( m_index );
	}
	//SLocalizationManager::GetInstance().MarkModifiedString( m_index, modified );
}

void funcGetLocStringById( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, stringID, 0 );
	FINISH_PARAMETERS;

	String stringValue = SLocalizationManager::GetInstance().GetString( stringID );
	RETURN_STRING( stringValue );
}

void funcGetLocStringByKey( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, stringKey, String::EMPTY );
	FINISH_PARAMETERS;

	String stringValue = SLocalizationManager::GetInstance().GetStringByStringKeyCached( stringKey );
	RETURN_STRING( stringValue );
}

void funcGetLocStringByKeyExt( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, stringKey, String::EMPTY );
	FINISH_PARAMETERS;

	String stringValue = SLocalizationManager::GetInstance().GetStringByStringKeyCachedWithFallback( stringKey );
	RETURN_STRING( stringValue );
}

void funcFixStringForFont( IScriptable* content, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, originalString, String::EMPTY);
	FINISH_PARAMETERS;

	String currentLocale = SLocalizationManager::GetInstance().GetTextLocale();
	String newString = originalString;

#ifndef RED_PLATFORM_ORBIS
	// Some special characters in some fonts don't render properly. Since this is only a problem on user
	// Generated strings and only a problem on console, we can use this function to fix those rare cases (only one on xbox known at the moment)
	// Swapping those characters to one thats at least somewhat supported
	// Test xbox string: G:ÀÆËÐß×Øãçêîðñ÷þÿ!”#$%_*-
	if (currentLocale == TXT("KR"))
	{
		// Korean font will properly put a [] graphic for some characters and not others. 
		// 208 is Ð
		newString.ReplaceAll(Char(255), Char(208));  // ÿ is being rendered as ...
		newString.ReplaceAll(Char(192), Char(208)); // À is being rendered as different square (TTP#115494)
	}
	else if (currentLocale == TXT("JP"))
	{
		newString.ReplaceAll(TXT("”"), TXT("\""));
	}
	else if (currentLocale == TXT("ZH"))
	{
		newString.ReplaceAll(TXT("”"), TXT("\""));
	}
#endif

	RETURN_STRING(newString);
}

void RegisterLocalizedContentFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "GetLocStringById", funcGetLocStringById );
	NATIVE_GLOBAL_FUNCTION( "GetLocStringByKey", funcGetLocStringByKey );
	NATIVE_GLOBAL_FUNCTION( "GetLocStringByKeyExt", funcGetLocStringByKeyExt );
	NATIVE_GLOBAL_FUNCTION( "FixStringForFont", funcFixStringForFont );
}
