#pragma once

class CAnalyzerOutputList;

struct SDLCLanguagePack
{
	DECLARE_RTTI_STRUCT( SDLCLanguagePack );

	TDynArray< String > m_textLanguages;
	TDynArray< String > m_speechLanguages;
};

BEGIN_CLASS_RTTI( SDLCLanguagePack );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_textLanguages, TXT("Text langauges"), TXT("TextLanguageSelection") );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_speechLanguages, TXT("Speech languages"), TXT("SpeechLanguageSelection") );
END_CLASS_RTTI();
