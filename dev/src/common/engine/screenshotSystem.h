#pragma once

#include "../core/functor.h"

class CRenderFrameInfo;

enum ESaveFormat : CEnum::TValueType
{
	SF_BMP,
	SF_PNG,
	SF_JPG,
	SF_DDS,
};

BEGIN_ENUM_RTTI( ESaveFormat )
	ENUM_OPTION( SF_BMP );
	ENUM_OPTION( SF_PNG )
	ENUM_OPTION( SF_JPG )
	ENUM_OPTION( SF_DDS )
END_ENUM_RTTI();

enum EScreenshotCreationFlags
{
	SCF_SaveToDisk						= FLAG( 0 ),	//!< Save screenshot to HDD
	SCF_UseProvidedFilename				= FLAG( 1 ),	//!< Use filename from parameters instead of generating one automatically, valid only when SCF_SaveToDisk flag is set
	SCF_SaveToBuffer					= FLAG( 2 ),	//!< Save screenshot to buffer
	SCF_AddNewGamePlusWatermark			= FLAG( 3 ),	//!< Add new game plus watermark to the screenshot (the game is ready for it)
};

enum EScreenshotRenderFlags
{
	SRF_PostProcess		= FLAG( 0 ),
	SRF_GUI				= FLAG( 1 ),
	SRF_Debug			= FLAG( 2 ),

	SRF_Max				= FLAG( 3 ),

	SRF_PlainScreenshot = SRF_PostProcess,
	SRF_All				= SRF_PostProcess | SRF_GUI | SRF_Debug,
};

BEGIN_ENUM_RTTI( EScreenshotRenderFlags )
	ENUM_OPTION( SRF_PostProcess );
	ENUM_OPTION( SRF_GUI )
	ENUM_OPTION( SRF_Debug )
END_ENUM_RTTI();

// Save screenshot width and height. Don't change those values as on PS4
// copying screenshot to buffer assumes they are like that when calculating 
// strides and offsets for texture pixels. 
// Not my fault, it was like that before, just didn't have time to fix that yet.
#define SAVE_SCREENSHOT_WIDTH		228
#define SAVE_SCREENSHOT_HEIGHT		128

struct SScreenshotParameters
{
	Uint32		m_width;
	Uint32		m_height;
	Uint32		m_superSamplingSize;
	//Int32		m_pitch;
	//Int32		m_yaw;
	Float		m_fov;
	//Float		m_dof;
	String		m_fileName;
	ESaveFormat	m_saveFormat;
	Uint32*		m_buffer;				//!< Buffer, that will be filled with data
	Uint32		m_flags;
	size_t		m_bufferSize;
	size_t*		m_bufferSizeWritten;
	Red::Threads::CAtomic< Bool >* m_completionFlag;
	Bool		m_noWatermark;

	SScreenshotParameters( Uint32 width = 1920, Uint32 height = 1080, const String& fileName = TXT(""), ESaveFormat saveFormat = SF_BMP, Uint32 superSamplingSize = 4, Float fov = 70.0f, Uint32 flags = 0, Uint32* buffer = NULL, Bool noWatermark = false )
		: m_width( width )
		, m_height( height )
		, m_superSamplingSize( superSamplingSize )
		, m_fov( fov )
		, m_fileName( fileName )
		, m_saveFormat( saveFormat )
		, m_buffer( buffer )
		, m_flags( flags )
		, m_bufferSize( 0 )
		, m_bufferSizeWritten( nullptr )
		, m_completionFlag( nullptr )
		, m_noWatermark( noWatermark )
	{
	}

	/*
	SScreenshotParameters( Uint32 width = 1920, Uint32 height = 1080, const String& fileName = TXT(""), ESaveFormat saveFormat = SF_BMP, Uint32 superSamplingSize = 4, Int32 pitch = 1, Int32 yaw = 1, Float fov = 70.0f, Float dof = 1.0f )
		: m_width( width )
		, m_height( height )
		, m_fileName( fileName )
		, m_saveFormat( saveFormat )
		, m_superSamplingSize( superSamplingSize )
		, m_pitch( pitch )
		, m_yaw( yaw )
		, m_fov( fov )
		, m_dof( dof )
	{
	}
	*/

	RED_FORCE_INLINE Bool IsUberScreenshotRequired() const { return m_superSamplingSize > 1; }
	RED_FORCE_INLINE Bool ShouldPasteNgPlusWatermark() const { return 0 != ( m_flags & SCF_AddNewGamePlusWatermark ); }
};

struct SScreenshotRequest
{
	SScreenshotParameters												m_parameters;
	Uint32																m_saveFlags;
	Uint32																m_renderFlags;
	Bool																m_status;

	// parameters are: filename, pointer to screenshot data buffer, result of the operation, error message
	Functor4< void, const String&, const void*, Bool, const String& >	m_callback;
};

class CScreenshotSystem
{
public:
	CScreenshotSystem();
	virtual ~CScreenshotSystem();

public:
	RED_FORCE_INLINE Bool IsTakingScreenshot() const { return m_currentRequest != NULL; }

public:
	void	RequestSimpleScreenshot( );
	void	TakeSimpleScreenshot( const SScreenshotParameters& parameters, Uint32 saveFlags );
	void	TakeSimpleScreenshotNow( const SScreenshotParameters& parameters, Uint32 saveFlags );
	void	RequestSimpleScreenshot( const SScreenshotParameters& parameters, Uint32 saveFlags );
	void	RequestScreenshot( const SScreenshotParameters& parameters, Uint32 saveFlags, Uint32 renderFlags, Functor4< void, const String&, const void*, Bool, const String& >& callback );
	void	Tick();
	void	Flush();

	void	OverrideShowFlags( CRenderFrameInfo& frameInfo );
	
protected:
	Uint32	GetLastScreenshotNumber();
	void	TakeScreenshot( const SScreenshotParameters& parameters, Bool* requestFinished, Bool& status );
	String	GetNextScreenshotName( const String& extension );

private:
	void	IssueRequest( SScreenshotRequest& request );
	void	CurrentRequestFinished();

private:
	Red::Threads::CMutex			m_requestMutex;				//!< Mutex to lock access to the request queue
	TQueue< SScreenshotRequest >	m_requestsQueue;			//!< Queue of requests

	SScreenshotRequest*				m_currentRequest;
	Bool							m_currentRequestFinished;
	Uint32							m_lastScreenshotNumber;		//!< Last index of screenshot in the folder for easier filename generation
};

typedef TSingleton< CScreenshotSystem > SScreenshotSystem;