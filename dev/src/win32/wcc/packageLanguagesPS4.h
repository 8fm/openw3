/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/core/hashmap.h"

struct SPackageLanguagesPS4
{
	THashMap< String, String >	m_gameToPlayGoLangMap;			//!< Game language to PlayGo code. E.g., "EN"->"en-US"
	THashMap< String, String >	m_languageFileToGameLangMap;	//!< E.g., en.w3strings -> "EN", pl.w3speech -> "PL"

	String						m_defaultSpeechLanguage;			//!< Default game language if system language unsupported
	TDynArray< String >			m_supportedSpeechLanguages;		//!< Supported game language codes. E.g., EN, PL, AR
};
