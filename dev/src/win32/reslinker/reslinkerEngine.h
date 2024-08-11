
#pragma once

// Resource linker engine
class CResLinkerEngine : public CBaseEngine, public IOutputDevice
{
protected:
	HANDLE m_stdOut;			//!< Std output
	String m_emailText;
	TDynArray< String > m_links;
	
public:
	CResLinkerEngine();
	~CResLinkerEngine();

	// Resource linker interface
	Bool Main( Int argCount, const char* argV[] );

	void AddToEmail(const Char *text);
	CWorld* LoadWorldSafely(String* path);
	Bool SaveWorldSafely(CWorld* world);
	Bool UnloadWorldSafely(CWorld* world);

	CResource* LoadResourceSafely(String* path);
	Bool SaveResourceSafely(CResource *resource);

protected:
	virtual void Write( const CName& type, const Char* str );
	virtual Bool AskYesNo( const CName& type, const Char* str ) { return false; }
	void SendEmail( const String &address );
	Bool ProcessFile( const String &absPath );

	//Bool SaveLayer(CLayer* layer, String path);
};
