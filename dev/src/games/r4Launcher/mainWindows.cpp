/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/redSystem/compilerExtensions.h"
#include "../../common/core/loadingProfiler.h"
#include "mainRed.h"

#include <wtypes.h>

// One directory up to ensure it goes in bin/ and not bin/x64/
#ifdef RED_LOGGING_ENABLED
	Red::System::Log::File fileLogger( TXT( "..\\r4launcher_windows.log" ), true );
#endif

static const AnsiChar* PROGRAM_UNIQUE_MUTEX_NAME = "cdprojekt_red_the_witcher_3_wild_hunt";

void DisplayDuplicateInstanceError()
{
	Char locale[ LOCALE_NAME_MAX_LENGTH ];
	GetUserDefaultLocaleName( locale, LOCALE_NAME_MAX_LENGTH );

	const Char* caption = TXT( "The Witcher 3: Wild Hunt" );
	const Char* message = TXT( "Game is already running." );
	UINT type = MB_OK | MB_ICONWARNING;

	if( Red::System::StringSearch( locale, TXT( "de" ) ) == locale )
	{
		message = TXT( "Das Spiel wird bereits ausgeführt." );
	}
	else if( Red::System::StringSearch( locale, TXT( "it" ) ) == locale )
	{
		message = TXT( "Il gioco è già in esecuzione." );
	}
	else if( Red::System::StringSearch( locale, TXT( "fr" ) ) == locale )
	{
		message = TXT( "Le jeu est déjà en cours d'exécution." );
	}
	else if( Red::System::StringSearch( locale, TXT( "cz" ) ) == locale )
	{
		caption = TXT( "Zaklínač 3: Divoký hon" );
		message = TXT( "Hra už byla spuštěna." );
	}
	// Search for Spain explicitly
	else if( Red::System::StringSearch( locale, TXT( "es_ES" ) ) == locale )
	{
		message = TXT( "El juego ya se está ejecutando." );
	}
	// Otherwise it's Latin American Spanish
	else if( Red::System::StringSearch( locale, TXT( "es" ) ) == locale )
	{
		message = TXT( "El juego ya está en ejecución." );
	}
	else if( Red::System::StringSearch( locale, TXT( "zh" ) ) == locale )
	{
		caption = TXT( "巫師 3：狂獵" );
		message = TXT( "遊戲已在執行中。" );
	}
	else if( Red::System::StringSearch( locale, TXT( "ru" ) ) == locale )
	{
		caption = TXT( "Ведьмак 3: Дикая Охота" );
		message = TXT( "Игра уже запущена." );
	}
	else if( Red::System::StringSearch( locale, TXT( "hu" ) ) == locale )
	{
		message = TXT( "A játék már fut." );
	}
	else if( Red::System::StringSearch( locale, TXT( "jp" ) ) == locale )
	{
		caption = TXT( "ウィッチャー３　ワイルドハント" );
		message = TXT( "ゲームが既に起動しています。" );
	}
	else if( Red::System::StringSearch( locale, TXT( "kr" ) ) == locale )
	{
		caption = TXT( "더 위쳐 3: 와일드 헌트" );
		message = TXT( "게임이 이미 실행중입니다." );
	}
	// Brazilian (pt_BR)
	else if( Red::System::StringSearch( locale, TXT( "pt" ) ) == locale )
	{
		message = TXT( "O jogo já está sendo executado." );
	}
	else if( Red::System::StringSearch( locale, TXT( "ar" ) ) == locale )
	{
		message = TXT( "اللعبة تعمل بالفعل." );
		type |= MB_RIGHT | MB_RTLREADING;
	}

	MessageBox( NULL, message, caption, type );
}

int WINAPI wWinMain( HINSTANCE, HINSTANCE, wchar_t* commandLine, int )
{
	int programReturnValue = 0;

	// Create a mutex with a name unique to the game, if it fails, an instance is already running
	HANDLE mutexHandle = CreateMutexA( NULL, TRUE, PROGRAM_UNIQUE_MUTEX_NAME );

	if( GetLastError() != ERROR_ALREADY_EXISTS )
	{
		GLoadingProfiler.Start();
		programReturnValue = mainRed( commandLine );

		// Game has finished processing, release mutex ownership
		ReleaseMutex( mutexHandle );
	}
	else
	{
		// Another instance of the game already exists, shutdown before we load or initialise anything

		DisplayDuplicateInstanceError();

		programReturnValue = 1;
	}

	CloseHandle( mutexHandle );

	return programReturnValue;
}
