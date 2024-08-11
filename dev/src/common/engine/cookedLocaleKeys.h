/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace CookedLocaleKeys
{
	static void GetSecureKeys( const String& lang, Uint32& fileKey, Uint32& langKey )
	{
		if ( lang == TXT( "pl" ) ) { fileKey = 0x83496237; langKey = 0x73946816; return; } // PL
		if ( lang == TXT( "en" ) ) { fileKey = 0x43975139; langKey = 0x79321793; return; } // EN
		if ( lang == TXT( "de" ) ) { fileKey = 0x75886138; langKey = 0x42791159; return; } // DE
		if ( lang == TXT( "it" ) ) { fileKey = 0x45931894; langKey = 0x12375973; return; } // IT
		if ( lang == TXT( "fr" ) ) { fileKey = 0x23863176; langKey = 0x75921975; return; } // FR
		if ( lang == TXT( "cz" ) ) { fileKey = 0x24987354; langKey = 0x21793217; return; } // CZ
		if ( lang == TXT( "es" ) ) { fileKey = 0x18796651; langKey = 0x42387566; return; } // ES
		if ( lang == TXT( "zh" ) ) { fileKey = 0x18632176; langKey = 0x16875467; return; } // ZH
		if ( lang == TXT( "ru" ) ) { fileKey = 0x63481486; langKey = 0x42386347; return; } // RU
		if ( lang == TXT( "hu" ) ) { fileKey = 0x42378932; langKey = 0x67823218; return; } // HU
		if ( lang == TXT( "jp" ) ) { fileKey = 0x54834893; langKey = 0x59825646; return; } // JP

		fileKey = 0; langKey = 0;
	}

	static Uint32 GetLanguageKey( Uint32 fileKey )
	{
		if( fileKey == 0x0        ) return 0x0;
		if( fileKey == 0x83496237 ) return 0x73946816; // PL
		if( fileKey == 0x43975139 ) return 0x79321793; // EN
		if( fileKey == 0x75886138 ) return 0x42791159; // DE
		if( fileKey == 0x45931894 ) return 0x12375973; // IT
		if( fileKey == 0x23863176 ) return 0x75921975; // FR
		if( fileKey == 0x24987354 ) return 0x21793217; // CZ
		if( fileKey == 0x18796651 ) return 0x42387566; // ES
		if( fileKey == 0x18632176 ) return 0x16875467; // ZH
		if( fileKey == 0x63481486 ) return 0x42386347; // RU
		if( fileKey == 0x42378932 ) return 0x67823218; // HU
		if( fileKey == 0x54834893 ) return 0x59825646; // JP

		return 0x0;
	}
};