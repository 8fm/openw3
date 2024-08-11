

#ifdef SQLSTRINGDB_EXPORTS 
#define DLL __declspec(dllexport)
#else
#define DLL __declspec(dllimport)
#endif

class sqlStringInternal;
class sqlStringDbConnection;

struct sqlStringIdStruct
{
public:
	unsigned int stringId;
};

struct sqlStringKeyStruct
{
public:
	WCHAR *stringKey;
};

struct sqlReadStruct
{
public:
	WCHAR *text;
	size_t textSize;
	bool isFallback;
};


struct sqlLanguagesStruct
{
public:
	WCHAR **languages;
	size_t *textSizes;
	unsigned int *langIds;
	size_t languageNum;
};

struct sqlReadAllStringsStruct
{
public:
	WCHAR **texts;
	size_t *textSizes;
	unsigned int *textIds;
	unsigned int *allIds;
	bool *fallbacks;
	size_t textNum;
	size_t idsNum;
};

struct sqlReadLanguagePackStruct
{
	WCHAR*	text;
	WCHAR*	lipsyncName;
	WCHAR*	voiceoverName;
};

struct sqlReadAllLanguagePacksStruct
{
	sqlStringInternal* internalData;

	WCHAR**	texts;
	WCHAR**	voiceovers;
	WCHAR**	lipsyncs;
	WCHAR**	stringKeys;
	unsigned int*	ids;

	size_t	packsCount;
};

struct sqlReadAllStringsWithStringKeyStruct
{
	unsigned int*	ids;
	WCHAR**			categories;
	WCHAR**			stringKeys;

	size_t	packsCount;
};

struct sqlReadAllCategoriesStruct
{
	WCHAR** categories;

	size_t	packsCount;
};

struct sqlStringInfoStruct
{
	WCHAR* voiceoverName;
};

struct sqlSearchParams
{
	unsigned int numCategories;
	const WCHAR** categories;
	const WCHAR* searchQuery;
	const WCHAR* orderBy;
	unsigned int lang;
	
	// If false, will search strings instead
	bool searchKeys;
};

struct sqlSearchResults
{
	unsigned int* ids;
	WCHAR** keys;
	WCHAR** texts;

	size_t size;
};

// inits COM and SQL Connection, returns NULL if failed
DLL sqlStringDbConnection *sqlStringDbInit( const WCHAR* databaseAddress, const WCHAR* databaseName );


// closes COM and SQL Connection
DLL void sqlStringDbClose( sqlStringDbConnection * );


// inserts 
DLL int sqlStringInsert( sqlStringDbConnection *, unsigned int stringId, unsigned int lang, const WCHAR *text, const WCHAR *user );

/*
Creates new string in STRINGS table as a copy of existing string, all translations and historical versions are preserved.

\param sourceStringId Id of a source string.
\param destStrignId Id that is to be assigned to string created as a copy of source string.
\return 1 on success, -1 on failure.

This function copies all STRING table rows belonging to string sourceStringId and inserts them
to the same table with id destStringId. No STRING_INFO table row is created for new string.
*/
DLL int sqlCopyStringWithoutInfo( sqlStringDbConnection *sqlStringDbConn, unsigned int sourceStringId, unsigned int destStringId );

DLL int sqlStringInfoInsert( sqlStringDbConnection *, unsigned int stringId, const WCHAR *resource, const WCHAR *property_name, const WCHAR* voiceover_name, const WCHAR* string_key );

// updates
DLL int sqlStringInfoUpdate( sqlStringDbConnection *, unsigned int stringId, const WCHAR *resource, const WCHAR *property_name, const WCHAR* voiceover_name, const WCHAR* string_key );

// reads
DLL sqlReadStruct *sqlStringRead( sqlStringDbConnection *, unsigned int stringId, unsigned int lang, bool deep );

DLL sqlStringIdStruct *sqlStringReadId( sqlStringDbConnection *, const WCHAR* string_key );
DLL int sqlStringFreeReadId( sqlStringIdStruct *s );

DLL sqlStringKeyStruct *sqlStringReadStringKey( sqlStringDbConnection *, unsigned int stringId );
DLL int sqlStringFreeReadStringKey( sqlStringKeyStruct *s );

// call it after reading result!
DLL int sqlStringFreeReadResult( sqlReadStruct * );

// get next id
DLL int sqlStringGetNextID(sqlStringDbConnection *, unsigned int *id);

// check if string is in db
DLL int sqlStringIsInDb( sqlStringDbConnection *, unsigned int stringId );

// check if string has additional information
DLL bool sqlStringHasInfo( sqlStringDbConnection*, unsigned int stringId );


// reads
DLL sqlLanguagesStruct *sqlStringReadLanguages( sqlStringDbConnection * );

// call it after reading result!
DLL int sqlStringFreeLanguagesResult( sqlLanguagesStruct * );


// reads
DLL sqlReadAllStringsStruct *sqlReadAllStrings( sqlStringDbConnection *, unsigned int lang  );

// call it after reading result!
DLL int sqlStringFreeReadAllStringsResult( sqlReadAllStringsStruct * );


// reads whole language pack
DLL sqlReadLanguagePackStruct *sqlReadLanguagePack( sqlStringDbConnection *, unsigned int stringId, unsigned int lang, bool deep );

// call it after reading result!
DLL int sqlStringFreeReadLanguagePackResult( sqlReadLanguagePackStruct * );



DLL sqlReadAllLanguagePacksStruct* sqlReadAllLanguagePacks( sqlStringDbConnection* connection, unsigned int lang, const WCHAR* view );

DLL int sqlFreeReadAllLanguagePacksResult( sqlReadAllLanguagePacksStruct* packsStruct );

DLL sqlReadAllStringsWithStringKeyStruct* sqlReadAllStringsWithStringKey( sqlStringDbConnection* connection );
DLL int sqlFreeReadAllStringsWithStringKeyResult( sqlReadAllStringsWithStringKeyStruct* packsStruct );

DLL sqlReadAllCategoriesStruct* sqlReadAllCategories( sqlStringDbConnection* connection, bool OnlyStringsWithKeys );
DLL int sqlFreeReadAllCategoriesResult( sqlReadAllCategoriesStruct* packsStruct );

DLL sqlStringInfoStruct* sqlGetStringInfo( sqlStringDbConnection* connection, unsigned int stringId );
DLL int sqlFreeGetStringInfoResult( sqlStringInfoStruct* infoStruct );

DLL sqlSearchResults* sqlSearchStrings( sqlStringDbConnection *connection, const sqlSearchParams& searchParams );
DLL int sqlFreeSearchResults( sqlSearchResults* results );
