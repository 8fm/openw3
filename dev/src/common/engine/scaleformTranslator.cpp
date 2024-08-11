/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "guiGlobals.h"

#include "scaleformTranslator.h"

#ifdef USE_SCALEFORM

#include "localizationManager.h"

//////////////////////////////////////////////////////////////////////////
// CScaleformTranslator
//////////////////////////////////////////////////////////////////////////
CScaleformTranslator::CScaleformTranslator( SFUInt wwMode /*= 0*/, SFUInt extraCaps /*= 0 */ )
	: GFx::Translator( wwMode )
	, m_caps( GFx::Translator::Cap_StripTrailingNewLines | extraCaps )
{}

SFUInt CScaleformTranslator::GetCaps() const
{
	return m_caps;
}

void CScaleformTranslator::Translate( TranslateInfo* ptranslateInfo )
{
	const UniChar * key = ptranslateInfo->GetKey();

	if ( key[ 0 ] == TXT('[') && key[ 1 ] == TXT('[') )
	{
		//TBD: See if still need this check with multiline text fields with the above caps
		String keyStr = key;
		if (keyStr.EndsWith(TXT("\r")))
		{
			// A multiline textfield puts a carriage return at the end of the string, even if you
			// didn't press enter in Flash while entering the text. So it looks for "[[text.key]]\r" otherwise.
			keyStr.RemoveAt(keyStr.GetLength() - 1);
		}

		// Keys are of the format '[[key]]' in Flash, but the key lookup is just 'key'
		static const Uint32 delimiterLength = 2;
		const Uint32 keyLength = keyStr.GetLength();
		keyStr = keyLength > 2 * delimiterLength ? keyStr.MidString( delimiterLength, keyLength - 2 * delimiterLength ) : String::EMPTY;

		String translated = SLocalizationManager::GetInstance().GetStringByStringKeyCached( keyStr );

		if ( translated.Empty() )
		{
			GUI_WARN( TXT("Untranslated text key: %ls"), key );

			if( !SLocalizationManager::GetInstance().GetConfig_UsePlaceholdersForMissingStrings() )
			{
				// Setting result, even an empty string, prevents string key from being displayed.
				ptranslateInfo->SetResult( translated.AsChar(), 0 );
			}
		}
		else
		{
			const Char* translatedText = translated.AsChar();
			if ( translatedText[0] == TXT('@') )
			{
				ptranslateInfo->SetResultHtml( translatedText+1 );
			}
			else
			{
				ptranslateInfo->SetResult( translatedText );
			}
		}
	}
}

#endif // USE_SCALEFORM