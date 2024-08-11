#pragma once

#ifndef NO_HEIGHTMAP_EDIT

struct SHeightmapImageEntryParams
{
	// parameters that can be used to modify behavior of image loading
	Bool normalizeImage;		// reinterpret image data in a way, that darkest pixel will be "0"

	SHeightmapImageEntryParams() : normalizeImage( false ) { }
};

template < typename T >
struct SHeightmapImageEntry : public Red::System::NonCopyable
{
	T*							m_data;
	SHeightmapImageEntryParams	m_params;

	SHeightmapImageEntry();
	~SHeightmapImageEntry();
};

class CHeightmapUtils
{
public:
	CHeightmapUtils() {}
	~CHeightmapUtils() {}

	Bool LoadHeightmap( const String& absolutePath, /*out*/SHeightmapImageEntry<Uint16>& entry, /*out*/Uint32* width /*= NULL*/, /*out*/Uint32* height /*= NULL*/ );
	Bool LoadColor( const String& absolutePath, /*out*/SHeightmapImageEntry<Uint32>& entry, /*out*/Uint32* width /*= NULL*/, /*out*/Uint32* height /*= NULL*/ );

	Bool ResizeImage( Uint16* data, Uint32 currentWidth, Uint32 currentHeight, Uint32 desiredWidth, Uint32 desiredHeight, /*out*/SHeightmapImageEntry<Uint16>& entry );
	Bool ResizeImage( Uint32* data, Uint32 currentWidth, Uint32 currentHeight, Uint32 desiredWidth, Uint32 desiredHeight, /*out*/SHeightmapImageEntry<Uint32>& entry );

	// save image to file
	Bool SaveImage( const String& absolutePath, Uint16* data, Uint32 width, Uint32 height );

	// save Uint32 image to file
	Bool SaveImage( const String& absolutePath, Uint32* data, Uint32 width, Uint32 height );

	Bool GetImageSize( const String& absolutePath, Uint32& width, Uint32& height );
};

typedef TSingleton< CHeightmapUtils > SHeightmapUtils;

#endif //!NO_HEIGHTMAP_EDIT
