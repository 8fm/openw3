#include "build.h"

namespace DLCTool
{
	enum ELanguage
	{
		LANG_CZ = 0,
		LANG_DE,
		LANG_EN,
		LANG_ES,
		LANG_FR,
		LANG_HU,
		LANG_IT,
		LANG_JP,
		LANG_PL,
		LANG_RU,
		LANG_ZH,

		LANG_MAX
	};

	const Char* LANGS[] = { TEXT("CZ"), TEXT("DE"), TEXT("EN"), TEXT("ES"), TEXT("FR"), TEXT("HU"), TEXT("IT"), TEXT("JP"), TEXT("PL"), TEXT("RU"), TEXT("ZH"), };

	ELanguage GLanguage( LANG_EN );

	ELanguage GetLanguage( const Char* name ) 
	{ 
		for ( int i = 0; i < LANG_MAX; ++i )
		{
			if ( _wcsicmp( name, LANGS[ i ] ) == 0 )
			{
				return ELanguage( i );
			}
		}

		return LANG_EN;
	}

	void SetLanguage( const Char* name ) 
	{
		GLanguage = GetLanguage( name );
	};

	// Strings for English language
	namespace EN
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
				case String_GameTitle: return TEXT("The Witcher 3");
				case String_ToolTitle: return TEXT("Activation DLC");
				case String_ErrorTilte: return TEXT("Critical Error");
				case String_Error_IOReadError: return TEXT("IO Read Error occurred at position %i, trying to read %i bytes");
				case String_Error_IOWriteError: return TEXT("IO Write Error occurred at position %i, trying to write %i bytes");
				case String_Error_IODirError: return TEXT("Unable to create directory '%s'");
				case String_Error_IOReadOnly: return TEXT("Unable to write to read only file '%s'");
				case String_Error_NoDLCData: return TEXT("No patch data was found");
				case String_Error_CorruptedDLCData: return TEXT("DLC data corrupted");
				case String_Error_NoGameInstalled: return TEXT("Game installation direcotry was not found");
				case String_Error_NotEnoughDiskSpace: return TEXT("Not enough disk space to patch the game");
				case String_Error_CannotCreateTemporaryFile: return TEXT("Error creating temporary file");
				case String_Error_NoPrivledges: return TEXT("Not enough privledges to patch the game");
				case String_Error_InvalidGameVersion: return TEXT("Installed game version is invalid or incomplete");
				case String_Error_FailedOpeningPackFile: return TEXT("Failed to open package file '%s'");
				case String_Error_FailedApplyingDLCChange: return TEXT("Failed applying patch change to '%s'");
				case String_Error_FailedCreatingTempFile: return TEXT("Failed to create temporary file. Check disk space.");
				case String_Error_InternalError: return TEXT("Internal error at %s, line %i");
				case String_State_Initializing: return TEXT("Initializing...");
				case String_State_Checking: return TEXT("Checking game version...");
				case String_State_Decompressing: return TEXT("Decompressing data...");
				case String_State_Applying: return TEXT("Applying patch to game...");
				case String_State_Closing: return TEXT("Shutting down...");
				case String_Info_DLCInstalled: return TEXT("Game was successfully patched!");
				case String_Info_DLCInstallFailure: return TEXT("Game is already up to date");
				case String_Error_CannotCreateActivateFile: return TEXT("Cannot create activation.exe file.");
				case String_Error_CannotOpenLauncherProcess: return TEXT("Cannot open launcher process.");
				case String_Info_RegistryUpdateFailure: return TEXT("Cannot update registry.");
				case String_Updating_Registry: return TEXT("Updating registry.");
				case String_Parsing_Ini: return TEXT("Parsing the ini file.");
				case String_Adding_Files: return TEXT("Adding files to the archive.");
				case String_Adding_Ini: return TEXT("Adding ini to the archive.");
				case String_Reading_Layout: return TEXT("Reading the layout.");
				case String_Installing_Files: return TEXT("Installing files.");
				case String_Deleting_Files: return TEXT("Deleting files.");
			}

			return TEXT("Unknown error");
		}
	}

	// Strings for Polish language
	namespace PL
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
			case String_GameTitle: return TEXT("Wiedźmin 2");
			case String_ToolTitle: return TEXT("Pakiet aktywacyjny");
			case String_ErrorTilte: return TEXT("Krytyczny błąd");
			case String_Error_IOReadError: return TEXT("Na pozycji %i, przy próbie czytania %i bajtów, wystąpił Błąd Czytania IO.");
			case String_Error_IOWriteError: return TEXT("Na pozycji %i, przy próbie czytania %i bajtów, wystąpił Błąd Zapisu IO.");
			case String_Error_IODirError: return TEXT("Nie można stworzyć folderu '%s'.");
			case String_Error_IOReadOnly: return TEXT("Plik '%s' posiada atrybut tylko do odczytu. Nie można do niego zapisywać.");
			case String_Error_NoDLCData: return TEXT("Nie znaleziono danych aktualizacji.");
			case String_Error_CorruptedDLCData: return TEXT("Dane aktualizacji są uszkodzone.");
			case String_Error_NoGameInstalled: return TEXT("Nie znaleziono folderu z zainstalowaną grą.");
			case String_Error_NotEnoughDiskSpace: return TEXT("Na dysku jest za mało miejsca, żeby zaktualizować grę.");
			case String_Error_CannotCreateTemporaryFile: return TEXT("Błąd tworzenia pliku tymczasowego.");
			case String_Error_NoPrivledges: return TEXT("Brak uprawnień do zaktualizowania gry.");
			case String_Error_InvalidGameVersion: return TEXT("Zainstalowana wersja gry jest nieprawidłowa lub niekompletna.");
			case String_Error_FailedOpeningPackFile: return TEXT("Niepowodzenie przy otwieraniu pakietu plików '%s'.");
			case String_Error_FailedApplyingDLCChange: return TEXT("Niepowodzenie przy aktualizowaniu '%s'.");
			case String_Error_FailedCreatingTempFile: return TEXT("Niepowodzenie przy tworzeniu pliku tymczasowego. Sprawdź dostępną przestrzeń na dysku.");
			case String_Error_InternalError: return TEXT("Błąd wewnętrzny w '%s', linia '%i'.");
			case String_State_Initializing: return TEXT("Inicjalizacja...");
			case String_State_Checking: return TEXT("Sprawdzanie wersji gry...");
			case String_State_Decompressing: return TEXT("Rozpakowywanie danych...");
			case String_State_Applying: return TEXT("Aktualizacja gry...");
			case String_State_Closing: return TEXT("Zamykanie...");
			case String_Info_DLCInstalled: return TEXT("Gra została zaktualizowana!");
			case String_Info_DLCInstallFailure: return TEXT("Dysponujesz aktualną wersją.");
			case String_Error_CannotCreateActivateFile: return TEXT("Nie można stworzyć pliku activation.exe.");
			case String_Error_CannotOpenLauncherProcess: return TEXT("Nie można otworzyć procesu launchera.");
			case String_Info_RegistryUpdateFailure: return TEXT("Nie można odświerzyć rejestru.");
			case String_Updating_Registry: return TEXT("Update rejestru");
			case String_Parsing_Ini: return TEXT("Parsowanie pliku ini.");
			case String_Adding_Files: return TEXT("Dodawanie plikow do archiwum.");
			case String_Adding_Ini: return TEXT("Dodawanie pliku ini do archiwum.");
			case String_Reading_Layout: return TEXT("Odczytywanie layout'u");
			case String_Installing_Files: return TEXT("Instalowanie plikow.");
			case String_Deleting_Files: return TEXT("Kasowanie plikow.");
			}

			return TEXT("Nieznany błąd.");
		}
	}

	// Strings for Deutch language
	namespace DE
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
			case String_GameTitle: return TEXT("The Witcher 3");
			case String_ToolTitle: return TEXT("Aktivierungspaket");
			case String_ErrorTilte: return TEXT("Kritischer Fehler");
			case String_Error_IOReadError: return TEXT("E/A-Lesefehler bei Position %i beim Leseversuch von %i Bytes");
			case String_Error_IOWriteError: return TEXT("E/A-Schreibfehler bei Position %i beim Schreibversuch von %i Bytes");
			case String_Error_IODirError: return TEXT("Verzeichnis '%s' konnte nicht erstellt werden");
			case String_Error_IOReadOnly: return TEXT("Auf die schreibgeschützte Datei '%s' kann nicht geschrieben werden");
			case String_Error_NoDLCData: return TEXT("Keine DLC-Daten gefunden");
			case String_Error_CorruptedDLCData: return TEXT("DLC-Daten beschädigt");
			case String_Error_NoGameInstalled: return TEXT("Spielinstallationsverzeichnis nicht gefunden");
			case String_Error_NotEnoughDiskSpace: return TEXT("Nicht genügend Speicherplatz zum DLCen des Spiels");
			case String_Error_CannotCreateTemporaryFile: return TEXT("Fehler beim Erstellen der temporären Datei");
			case String_Error_NoPrivledges: return TEXT("Keine ausreichenden Rechte zum DLCen des Spiels");
			case String_Error_InvalidGameVersion: return TEXT("Installierte Spielversion ist ungültig oder unvollständig");
			case String_Error_FailedOpeningPackFile: return TEXT("Paketdatei '%s' konnte nicht geöffnet werden");
			case String_Error_FailedApplyingDLCChange: return TEXT("DLC-Änderung konnte nicht auf '%s' angewendet werden");
			case String_Error_FailedCreatingTempFile: return TEXT("Temporäre Datei konnte nicht erstellt werden. Speicherplatz überprüfen.");
			case String_Error_InternalError: return TEXT("Interner Fehler bei %s, Zeile %i");
			case String_State_Initializing: return TEXT("Initialisieren ...");
			case String_State_Checking: return TEXT("Spielversion prüfen ...");
			case String_State_Decompressing: return TEXT("Daten dekomprimieren ...");
			case String_State_Applying: return TEXT("DLC auf Spiel anwenden ...");
			case String_State_Closing: return TEXT("Beenden ...");
			case String_Info_DLCInstalled: return TEXT("Der DLC wurde erfolgreich aufgespielt!");
			case String_Info_DLCInstallFailure: return TEXT("Das Spiel ist bereits aktuell");
			case String_Info_RegistryUpdateFailure: return TEXT("Nie można odświerzyć rejestru.");
			case String_Updating_Registry: return TEXT("Updating registry.");
			case String_Parsing_Ini: return TEXT("Parsing the ini file.");
			case String_Adding_Files: return TEXT("Adding files to the archive.");
			case String_Adding_Ini: return TEXT("Adding ini to the archive.");
			case String_Reading_Layout: return TEXT("Reading the layout.");
			case String_Installing_Files: return TEXT("Installing files.");
			case String_Deleting_Files: return TEXT("Deleting files.");
			}

			return TEXT("Fehler");
		}
	}

	// Strings for French language
	namespace FR
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
			case String_GameTitle: return TEXT("The Witcher 3");
			case String_ToolTitle: return TEXT("Pack d'activation");
			case String_ErrorTilte: return TEXT("Erreur critique");
			case String_Error_IOReadError: return TEXT("Erreur de lecture d'entrée/sortie à %i, en essayant de lire %i octets.");
			case String_Error_IOWriteError: return TEXT("Erreur d'écriture d'entrée/sortie à %i, en essayant d'écrire %i octets.");
			case String_Error_IODirError: return TEXT("Création du répertoire '%s' impossible");
			case String_Error_IOReadOnly: return TEXT("Écriture sur le fichier en lecture seule '%s' impossible");
			case String_Error_NoDLCData: return TEXT("Aucun correctif trouvé");
			case String_Error_CorruptedDLCData: return TEXT("Données du correctif corrompues");
			case String_Error_NoGameInstalled: return TEXT("Répertoire d'installation du jeu introuvable");
			case String_Error_NotEnoughDiskSpace: return TEXT("Espace disque insuffisant pour installer le correctif du jeu");
			case String_Error_CannotCreateTemporaryFile: return TEXT("Erreur pendant la création du fichier temporaire");
			case String_Error_NoPrivledges: return TEXT("Vous ne disposez pas des droits suffisants pour installer le correctif du jeu");
			case String_Error_InvalidGameVersion: return TEXT("La version du jeu installée est invalide ou incomplète");
			case String_Error_FailedOpeningPackFile: return TEXT("Échec de l'ouverture du fichier '%s' du pack");
			case String_Error_FailedApplyingDLCChange: return TEXT("Échec de l'application du correctif à '%s'");
			case String_Error_FailedCreatingTempFile: return TEXT("Échec de la création d'un fichier temporaire. Vérification de l'espace disque...");
			case String_Error_InternalError: return TEXT("Erreur interne à %s, ligne %i");
			case String_State_Initializing: return TEXT("Initialisation en cours...");
			case String_State_Checking: return TEXT("Vérification de la version du jeu en cours...");
			case String_State_Decompressing: return TEXT("Extraction des données en cours...");
			case String_State_Applying: return TEXT("Application du correctif au jeu...");
			case String_State_Closing: return TEXT("Fermeture en cours...");
			case String_Info_DLCInstalled: return TEXT("Le correctif du jeu a été correctement installé.");
			case String_Info_DLCInstallFailure: return TEXT("Le jeu est à jour");
			case String_Info_RegistryUpdateFailure: return TEXT("Nie można odświerzyć rejestru.");
			case String_Updating_Registry: return TEXT("Updating registry.");
			case String_Parsing_Ini: return TEXT("Parsing the ini file.");
			case String_Adding_Files: return TEXT("Adding files to the archive.");
			case String_Adding_Ini: return TEXT("Adding ini to the archive.");
			case String_Reading_Layout: return TEXT("Reading the layout.");
			case String_Installing_Files: return TEXT("Installing files.");
			case String_Deleting_Files: return TEXT("Deleting files.");
			}

			return TEXT("Erreur");
		}
	}

	namespace ES
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
			case String_GameTitle: return TEXT("The Witcher 3");
			case String_ToolTitle: return TEXT("Paquete de activación");
			case String_ErrorTilte: return TEXT("Error crítico");
			case String_Error_IOReadError: return TEXT("Error de lectura de E/S en la posición %i al intentar leer %i bytes");
			case String_Error_IOWriteError: return TEXT("Error de escritura de E/S en la posición %i al intentar escribir %i bytes");
			case String_Error_IODirError: return TEXT("Imposible crear el directorio '%s'");
			case String_Error_IOReadOnly: return TEXT("Imposible escribir en el archivo de solo lectura '%s'");
			case String_Error_NoDLCData: return TEXT("No se encuentran datos del parche");
			case String_Error_CorruptedDLCData: return TEXT("Datos del parche dañados");
			case String_Error_NoGameInstalled: return TEXT("No se encuentra el directorio de instalación del juego");
			case String_Error_NotEnoughDiskSpace: return TEXT("No hay espacio suficiente en el disco para parchear el juego");
			case String_Error_CannotCreateTemporaryFile: return TEXT("Error al crear archivo temporal");
			case String_Error_NoPrivledges: return TEXT("Privilegios insuficientes para parchear el juego");
			case String_Error_InvalidGameVersion: return TEXT("La versión del juego instalada no es válida o está incompleta");
			case String_Error_FailedOpeningPackFile: return TEXT("Error al abrir el archivo de paquete '%s'");
			case String_Error_FailedApplyingDLCChange: return TEXT("Error al aplicar el cambio del parche a '%s'");
			case String_Error_FailedCreatingTempFile: return TEXT("Imposible crear archivo temporal; comprueba el espacio en el disco");
			case String_Error_InternalError: return TEXT("Error interno en '%s', línea '%i'");
			case String_State_Initializing: return TEXT("Inicializando...");
			case String_State_Checking: return TEXT("Comprobando versión del juego...");
			case String_State_Decompressing: return TEXT("Descomprimiendo datos...");
			case String_State_Applying: return TEXT("Aplicando parche al juego...");
			case String_State_Closing: return TEXT("Finalizando...");
			case String_Info_DLCInstalled: return TEXT("¡El juego se ha parcheado correctamente!");
			case String_Info_DLCInstallFailure: return TEXT("El juego ya está actualizado");
			case String_Info_RegistryUpdateFailure: return TEXT("No se puede cargar el registro.");
			case String_Updating_Registry: return TEXT("Updating registry.");
			case String_Parsing_Ini: return TEXT("Parsing the ini file.");
			case String_Adding_Files: return TEXT("Adding files to the archive.");
			case String_Adding_Ini: return TEXT("Adding ini to the archive.");
			case String_Reading_Layout: return TEXT("Reading the layout.");
			case String_Installing_Files: return TEXT("Installing files.");
			case String_Deleting_Files: return TEXT("Deleting files.");
			}

			return TEXT("Error");
		}
	}

	namespace RU
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
			case String_GameTitle: return TEXT("Ведьмак 2");
			case String_ToolTitle: return TEXT("Активация");
			case String_ErrorTilte: return TEXT("Критическая ошибка");
			case String_Error_IOReadError: return TEXT("В %i произошла ошибка чтения IO при попытке прочитать %i б.");
			case String_Error_IOWriteError: return TEXT("В %i произошла ошибка записи IO при попытке записать %i б.");
			case String_Error_IODirError: return TEXT("Не удается создать папку '%s'.");
			case String_Error_IOReadOnly: return TEXT("Не удается сделать запись в файл '%s'. Этот файл только для чтения.");
			case String_Error_NoDLCData: return TEXT("Данные обновлений не обнаружены.");
			case String_Error_CorruptedDLCData: return TEXT("Данные обновлений повреждены.");
			case String_Error_NoGameInstalled: return TEXT("Папка для установки игры не найдена.");
			case String_Error_NotEnoughDiskSpace: return TEXT("Недостаточно места для обновления игры.");
			case String_Error_CannotCreateTemporaryFile: return TEXT("При создании временного файла произошла ошибка.");
			case String_Error_NoPrivledges: return TEXT("Недостаточно прав, чтобы установить обновление.");
			case String_Error_InvalidGameVersion: return TEXT("Установлена недействительная или неполная версия игры.");
			case String_Error_FailedOpeningPackFile: return TEXT("Не удалось открыть архив '%s'.");
			case String_Error_FailedApplyingDLCChange: return TEXT("Не удалось применить обновление в файле '%s'.");
			case String_Error_FailedCreatingTempFile: return TEXT("Не удалось создать временный файл. Проверьте место на жестком диске.");
			case String_Error_InternalError: return TEXT("Внутренняя ошибка в %s, строка %i.");
			case String_State_Initializing: return TEXT("Запуск...");
			case String_State_Checking: return TEXT("Проверка версии игры...");
			case String_State_Decompressing: return TEXT("Распаковка данных...");
			case String_State_Applying: return TEXT("Актуализация обновлений...");
			case String_State_Closing: return TEXT("Выключение...");
			case String_Info_DLCInstalled: return TEXT("Обновление успешно установлено!");
			case String_Info_DLCInstallFailure: return TEXT("Игра уже обновлена.");
			case String_Info_RegistryUpdateFailure: return TEXT("Nie można odświerzyć rejestru.");
			case String_Updating_Registry: return TEXT("Updating registry.");
			case String_Parsing_Ini: return TEXT("Parsing the ini file.");
			case String_Adding_Files: return TEXT("Adding files to the archive.");
			case String_Adding_Ini: return TEXT("Adding ini to the archive.");
			case String_Reading_Layout: return TEXT("Reading the layout.");
			case String_Installing_Files: return TEXT("Installing files.");
			case String_Deleting_Files: return TEXT("Deleting files.");
			}

			return TEXT("ошибка");
		}
	}

	namespace IT
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
			case String_GameTitle: return TEXT("The Witcher 3");
			case String_ToolTitle: return TEXT("Pagina di attivazione");
			case String_ErrorTilte: return TEXT("Errore critico");
			case String_Error_IOReadError: return TEXT("Errore di lettura alla posizione %i, cercando di leggere %i byte.");
			case String_Error_IOWriteError: return TEXT("Errore di scrittura alla posizione %i, cercando di scrivere %i byte.");
			case String_Error_IODirError: return TEXT("Impossibile creare il percorso '%s'");
			case String_Error_IOReadOnly: return TEXT("Impossibile scrivere il file di sola lettura '%s'");
			case String_Error_NoDLCData: return TEXT("Nessun dato dell'aggiornamento trovato");
			case String_Error_CorruptedDLCData: return TEXT("Dati della patch corrotti");
			case String_Error_NoGameInstalled: return TEXT("Percorso di installazione del gioco non trovato");
			case String_Error_NotEnoughDiskSpace: return TEXT("Spazio su disco insufficiente per applicare l'aggiornamento");
			case String_Error_CannotCreateTemporaryFile: return TEXT("Errore durante la creazione del file temporaneo");
			case String_Error_NoPrivledges: return TEXT("Privilegi insufficienti per applicare l'aggiornamento");
			case String_Error_InvalidGameVersion: return TEXT("La versione del gioco installata è incompleta o non valida");
			case String_Error_FailedOpeningPackFile: return TEXT("Impossibile aprire il pacchetto '%s'");
			case String_Error_FailedApplyingDLCChange: return TEXT("Impossibile applicare i cambi dell'aggiornamento a '%s'");
			case String_Error_FailedCreatingTempFile: return TEXT("Impossibile creare il file temporaneo, controllare lo spazio disponibile su disco");
			case String_Error_InternalError: return TEXT("Errore interno a '%s', linea '%i'");
			case String_State_Initializing: return TEXT("Inizializzazione in corso...");
			case String_State_Checking: return TEXT("Controllo versione del gioco...");
			case String_State_Decompressing: return TEXT("Decompressione dati...");
			case String_State_Applying: return TEXT("Applicazione patch in corso...");
			case String_State_Closing: return TEXT("Chiusura in corso...");
			case String_Info_DLCInstalled: return TEXT("Il gioco è stato aggiornato!");
			case String_Info_DLCInstallFailure: return TEXT("Il gioco è già aggiornato.");
			case String_Info_RegistryUpdateFailure: return TEXT("Nie można odświerzyć rejestru.");
			case String_Updating_Registry: return TEXT("Updating registry.");
			case String_Parsing_Ini: return TEXT("Parsing the ini file.");
			case String_Adding_Files: return TEXT("Adding files to the archive.");
			case String_Adding_Ini: return TEXT("Adding ini to the archive.");
			case String_Reading_Layout: return TEXT("Reading the layout.");
			case String_Installing_Files: return TEXT("Installing files.");
			case String_Deleting_Files: return TEXT("Deleting files.");
			}

			return TEXT("Errore");
		}
	}

	namespace HU
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
			case String_GameTitle: return TEXT("The Witcher 3");
			case String_ToolTitle: return TEXT("Aktivációs csomag");
			case String_ErrorTilte: return TEXT("Kritikus hiba");
			case String_Error_IOReadError: return TEXT("IO olvasási hiba történt a(z) %i pozícióban. %i bájt olvasásának megkísérlése...");
			case String_Error_IOWriteError: return TEXT("IO írási hiba történt a(z) %i pozícióban. %i bájt írásának megkísérlése...");
			case String_Error_IODirError: return TEXT("Nem lehet létrehozni az alábbi könyvtárat: '%s'");
			case String_Error_IOReadOnly: return TEXT("Nem lehet az írásvédett fájlba írni: '%s'");
			case String_Error_NoDLCData: return TEXT("Nem található frissítési adat.");
			case String_Error_CorruptedDLCData: return TEXT("A frissítési adat sérült.");
			case String_Error_NoGameInstalled: return TEXT("A játék telepítési könyvtára nem található.");
			case String_Error_NotEnoughDiskSpace: return TEXT("Nincs elég hely a játék frissítéséhez.");
			case String_Error_CannotCreateTemporaryFile: return TEXT("Hiba történt az ideiglenes fájl létrehozásakor.");
			case String_Error_NoPrivledges: return TEXT("Nincs megfelelő jogod a játék frissítéséhez.");
			case String_Error_InvalidGameVersion: return TEXT("A telepített játékverzió érvénytelen vagy hiányos.");
			case String_Error_FailedOpeningPackFile: return TEXT("Nem lehet megnyitni a csomagolt fájlt: '%s'");
			case String_Error_FailedApplyingDLCChange: return TEXT("Nem lehet frissíteni: '%s'");
			case String_Error_FailedCreatingTempFile: return TEXT("Hiba történt az ideiglenes fájl létrehozásakor. Lemezterület ellenőrzése.");
			case String_Error_InternalError: return TEXT("Belső hiba itt: %s, %i sor.");
			case String_State_Initializing: return TEXT("Indítás...");
			case String_State_Checking: return TEXT("Játékverzió ellenőrzése...");
			case String_State_Decompressing: return TEXT("Adatok kitömörítése...");
			case String_State_Applying: return TEXT("Frissítések illesztése...");
			case String_State_Closing: return TEXT("Bezárás...");
			case String_Info_DLCInstalled: return TEXT("A játék sikeresen frissült!");
			case String_Info_DLCInstallFailure: return TEXT("A játék már a lehető legfrissebb.");
			case String_Info_RegistryUpdateFailure: return TEXT("Nie można odświerzyć rejestru.");
			case String_Updating_Registry: return TEXT("Updating registry.");
			case String_Parsing_Ini: return TEXT("Parsing the ini file.");
			case String_Adding_Files: return TEXT("Adding files to the archive.");
			case String_Adding_Ini: return TEXT("Adding ini to the archive.");
			case String_Reading_Layout: return TEXT("Reading the layout.");
			case String_Installing_Files: return TEXT("Installing files.");
			case String_Deleting_Files: return TEXT("Deleting files.");
			}

			return TEXT("Hiba");
		}
	}

	namespace CZ
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
			case String_GameTitle: return TEXT("Zaklínač 2");
			case String_ToolTitle: return TEXT("Aktivační balíček");
			case String_ErrorTilte: return TEXT("Kritická chyba");
			case String_Error_IOReadError: return TEXT("Při čtení IO se stala chyba na pozici %i při pokusu přečíst %i bytů");
			case String_Error_IOWriteError: return TEXT("Při zápisu IO se stala chyba na pozici %i při pokusu zapsat %i bytů");
			case String_Error_IODirError: return TEXT("Nelze vytvořit složku s návzem '%s'");
			case String_Error_IOReadOnly: return TEXT("Nelze zapisovat do souboru jen pro čtení s názvem '%s'");
			case String_Error_NoDLCData: return TEXT("Nebyla nalezena data aktualizace");
			case String_Error_CorruptedDLCData: return TEXT("Data aktualizace jsou poškozena");
			case String_Error_NoGameInstalled: return TEXT("Nebyla nalezena složka s instalací hry");
			case String_Error_NotEnoughDiskSpace: return TEXT("Na pevném disku není dost místa na aktualizaci");
			case String_Error_CannotCreateTemporaryFile: return TEXT("Chyba při vytváření dočasného souboru");
			case String_Error_NoPrivledges: return TEXT("Nedostatečná práva pro aktualizaci");
			case String_Error_InvalidGameVersion: return TEXT("Nainstalovaná verze hry je poškozená nebo nekompletní");
			case String_Error_FailedOpeningPackFile: return TEXT("Nepodařilo se otevřít balíček souborů s názvem '%s'");
			case String_Error_FailedApplyingDLCChange: return TEXT("Nepodařilo se aktualizovat soubor '%s'");
			case String_Error_FailedCreatingTempFile: return TEXT("Nepodařilo se vytvořit dočasný soubor. Zkontrolujte místo na pevném disku.");
			case String_Error_InternalError: return TEXT("Interní chyba v %s, řádka %i");
			case String_State_Initializing: return TEXT("Spouštění...");
			case String_State_Checking: return TEXT("Kontrola verze hry...");
			case String_State_Decompressing: return TEXT("Dekomprese dat...");
			case String_State_Applying: return TEXT("Aktualizace hry...");
			case String_State_Closing: return TEXT("Zavírání...");
			case String_Info_DLCInstalled: return TEXT("Hra byla úspěšně aktualizována!");
			case String_Info_DLCInstallFailure: return TEXT("Hra už je aktuální");
			case String_Info_RegistryUpdateFailure: return TEXT("Nie można odświerzyć rejestru.");
			case String_Updating_Registry: return TEXT("Updating registry.");
			case String_Parsing_Ini: return TEXT("Parsing the ini file.");
			case String_Adding_Files: return TEXT("Adding files to the archive.");
			case String_Adding_Ini: return TEXT("Adding ini to the archive.");
			case String_Reading_Layout: return TEXT("Reading the layout.");
			case String_Installing_Files: return TEXT("Installing files.");
			case String_Deleting_Files: return TEXT("Deleting files.");
			}

			return TEXT("Chyba");
		}
	}

	namespace ZH
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
			case String_GameTitle: return TEXT("巫師2");
			case String_ToolTitle: return TEXT("啟動包");
			case String_ErrorTilte: return TEXT("致命錯誤");
			case String_Error_IOReadError: return TEXT(" %i位置IO 讀取錯誤，嘗試讀取 %i 位元");
			case String_Error_IOWriteError: return TEXT(" %i位置IO 寫入錯誤，嘗試寫入 %i 位元");
			case String_Error_IODirError: return TEXT("無法創建目錄 '%s'");
			case String_Error_IOReadOnly: return TEXT("無法寫入唯讀檔案 '%s'");
			case String_Error_NoDLCData: return TEXT("找不到更新資料");
			case String_Error_CorruptedDLCData: return TEXT("更新資料損毀");
			case String_Error_NoGameInstalled: return TEXT("找不到遊戲安裝路徑");
			case String_Error_NotEnoughDiskSpace: return TEXT("沒足夠空間更新遊戲");
			case String_Error_CannotCreateTemporaryFile: return TEXT("創建暫時檔案錯誤");
			case String_Error_NoPrivledges: return TEXT("沒有特定權限更新遊戲");
			case String_Error_InvalidGameVersion: return TEXT("遊戲安裝版本不存在或不完整");
			case String_Error_FailedOpeningPackFile: return TEXT("開啟'%s'檔案失敗");
			case String_Error_FailedApplyingDLCChange: return TEXT("嘗試更新 '%s'改變失敗");
			case String_Error_FailedCreatingTempFile: return TEXT("創建暫時檔案失敗請檢查硬碟空間。");
			case String_Error_InternalError: return TEXT("網際網路錯誤 %s， %i行");
			case String_State_Initializing: return TEXT("初始化...");
			case String_State_Checking: return TEXT("檢查遊戲版版本...");
			case String_State_Decompressing: return TEXT("資料解壓...");
			case String_State_Applying: return TEXT("進行遊戲更新...");
			case String_State_Closing: return TEXT("關閉...");
			case String_Info_DLCInstalled: return TEXT("遊戲沒更新成功!");
			case String_Info_DLCInstallFailure: return TEXT("遊戲已更新到最新版本");
			case String_Info_RegistryUpdateFailure: return TEXT("Nie można odświerzyć rejestru.");
			case String_Updating_Registry: return TEXT("Updating registry.");
			case String_Parsing_Ini: return TEXT("Parsing the ini file.");
			case String_Adding_Files: return TEXT("Adding files to the archive.");
			case String_Adding_Ini: return TEXT("Adding ini to the archive.");
			case String_Reading_Layout: return TEXT("Reading the layout.");
			case String_Installing_Files: return TEXT("Installing files.");
			case String_Deleting_Files: return TEXT("Deleting files.");
			}

			return TEXT("致命錯誤");
		}
	}

	namespace JP
	{
		const WCHAR* GetStringById( StringID id )
		{
			switch ( id )
			{
			case String_GameTitle: return TEXT("The Witcher 3");
			case String_ToolTitle: return TEXT("Activation DLC");
			case String_ErrorTilte: return TEXT("Critical Error");
			case String_Error_IOReadError: return TEXT("IO Read Error occurred at position %i, trying to read %i bytes");
			case String_Error_IOWriteError: return TEXT("IO Write Error occurred at position %i, trying to write %i bytes");
			case String_Error_IODirError: return TEXT("Unable to create directory '%s'");
			case String_Error_IOReadOnly: return TEXT("Unable to write to read only file '%s'");
			case String_Error_NoDLCData: return TEXT("No patch data was found");
			case String_Error_CorruptedDLCData: return TEXT("DLC data corrupted");
			case String_Error_NoGameInstalled: return TEXT("Game installation direcotry was not found");
			case String_Error_NotEnoughDiskSpace: return TEXT("Not enough disk space to patch the game");
			case String_Error_CannotCreateTemporaryFile: return TEXT("Error creating temporary file");
			case String_Error_NoPrivledges: return TEXT("Not enough privledges to patch the game");
			case String_Error_InvalidGameVersion: return TEXT("Installed game version is invalid or incomplete");
			case String_Error_FailedOpeningPackFile: return TEXT("Failed to open package file '%s'");
			case String_Error_FailedApplyingDLCChange: return TEXT("Failed applying patch change to '%s'");
			case String_Error_FailedCreatingTempFile: return TEXT("Failed to create temporary file. Check disk space.");
			case String_Error_InternalError: return TEXT("Internal error at %s, line %i");
			case String_State_Initializing: return TEXT("Initializing...");
			case String_State_Checking: return TEXT("Checking game version...");
			case String_State_Decompressing: return TEXT("Decompressing data...");
			case String_State_Applying: return TEXT("Applying patch to game...");
			case String_State_Closing: return TEXT("Shutting down...");
			case String_Info_DLCInstalled: return TEXT("Game was successfully patched!");
			case String_Info_DLCInstallFailure: return TEXT("Game is already up to date");
			case String_Info_RegistryUpdateFailure: return TEXT("Cannot update registry.");
			case String_Updating_Registry: return TEXT("Updating registry.");
			case String_Parsing_Ini: return TEXT("Parsing the ini file.");
			case String_Adding_Files: return TEXT("Adding files to the archive.");
			case String_Adding_Ini: return TEXT("Adding ini to the archive.");
			case String_Reading_Layout: return TEXT("Reading the layout.");
			case String_Installing_Files: return TEXT("Installing files.");
			case String_Deleting_Files: return TEXT("Deleting files.");
			}

			return TEXT("Unknown error");
		}
	}

	const WCHAR* GetStringById( StringID id )
	{
		switch ( GLanguage )
		{
			case LANG_CZ: return CZ::GetStringById( id );
			case LANG_DE: return DE::GetStringById( id );
			case LANG_ES: return ES::GetStringById( id );
			case LANG_FR: return FR::GetStringById( id );
			case LANG_HU: return HU::GetStringById( id );
			case LANG_IT: return IT::GetStringById( id );
			case LANG_JP: return JP::GetStringById( id );
			case LANG_PL: return PL::GetStringById( id );
			case LANG_RU: return RU::GetStringById( id );
			case LANG_ZH: return ZH::GetStringById( id );
			case LANG_EN: default: return EN::GetStringById( id );
		}
	}
}