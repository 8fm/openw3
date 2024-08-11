/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/core/hashmap.h"

struct SPackageLanguagesXboxOne
{
	THashMap< String, String >	m_gameToLocaleMap;				//!< Game language to Xbox locale. E.g., EN->en-US, BR->pt-BR
	THashMap< String, String >	m_languageFileToGameLangMap;	//!< E.g., en.w3strings -> "EN", pl.w3speech -> "PL"
	String						m_defaultSpeechLanguage;			//!< Default game language if system language unsupported
	TDynArray< String >			m_supportedSpeechLanguages;		//!< Supported game language codes. E.g., EN, PL, AR
};
