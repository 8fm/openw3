
#pragma once

class CResVerifierOutput : public IOutputDevice
{
	FILE* m_resVerifiacationLogFile;
	FILE* m_resVerifiacationErrFile;
	FILE* m_resVerifiacationErrFileCSV;
	HANDLE m_stdOut;			//!< Std output

protected:
	void Write( const CName& type, const Char* str );
	virtual Bool AskYesNo( const CName& type, const Char* str ) { return false; }

public:
	static CName TYPE_LOG;
	static CName TYPE_ERR;
	static CName TYPE_RES;

	CResVerifierOutput();
	~CResVerifierOutput();

};

class CResVerifierCaseCategory
{
protected:
	typedef THashMap< String, TSet<String> > TReferences;
	TReferences m_references;
	String		m_fileName;
public:
	CResVerifierCaseCategory( const Char* fileName );
	virtual ~CResVerifierCaseCategory();

	void ReportResource ( const String& resPath, const String& referencedBy );
};

class CResVerifierJunkCategory : public CResVerifierCaseCategory
{
public:
	CResVerifierJunkCategory();
};

typedef TSingleton< CResVerifierOutput, TPheonixLifetime > SResVerifierOutput;

// Resource linker engine
class CResVerifierEngine : public CBaseEngine
{
	IViewport *m_viewport;
	static Uint m_totalFiles;
	static Uint m_processedFiles;
	static CResVerifierEngine* SResVerifierEngineInstance;
	CResVerifierJunkCategory m_junkResources;
public:
	CResVerifierEngine();
	~CResVerifierEngine();

	CWorld* SaftyLoadWorld(String* path);
	Bool SaftyUnloadWorld(CWorld* world);

	CResource* SafetyLoadResource(String* path);
	void SafetyUnloadResource(CResource* resource);

	void ParseDepot( String depotSubpathToParse = String::EMPTY );

	static Uint32 GetDepth();
	static const TList<String>& GetLoadTrace();
	static Uint32 GetProcessedFilesCount();
	virtual Bool Initialize();
	virtual void Shutdown();

	void ReportJunkResource();
private:
	void ParseResource( String resourcePath, Bool topLevel = false );
	Bool LoadResource( String resourceLocalPath );

	TList< String > m_resPaths;
	TSortedSet< String > m_loadableResources;
};

#define RESV_ERR( format, ... ) { SResVerifierOutput::GetInstance().Log( CResVerifierOutput::TYPE_ERR, format, ## __VA_ARGS__ ); }
#define RESV_REPORT_JUNK() { ReportJunkResource(); }
#define RESV_LOG( format, ... ) { SResVerifierOutput::GetInstance().Log( CResVerifierOutput::TYPE_LOG, format, ## __VA_ARGS__ ); }
#define RESV_RES( format, ... ) { SResVerifierOutput::GetInstance().Log( CResVerifierOutput::TYPE_RES, format, ## __VA_ARGS__ ); }