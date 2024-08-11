
#pragma once

class CEdItemsThumbnailGenerator
{
public:
	CEdItemsThumbnailGenerator( wxWindow* parent, CContextMenuDir* dir );

	Bool Execute();

	static void ChooseOutputDir( wxWindow* parent, String& outputPath );
	static void OutputToPNG( TDynArray< CThumbnail* >& thumbnails, Uint32 width, Uint32 height, const String& outputPath );
		
private:
	wxWindow* m_parent;
	CContextMenuDir* m_dir;

	struct ItemInfo
	{
		struct IconDef
		{
			Int32 width;
			Int32 height;
		};

		TDynArray< IconDef > iconDefs;
		TDynArray< String > entitySearchDirs;
		String environment;
		Float cameraAngle;
		Float cameraPitch;
		Float cameraFov;
		Float lightAngle;
		Vector backgroundColor;
	};

	THashMap< String, ItemInfo > m_itemInfos;

	TOptional<String> ReadConfigFile();
	TOptional<String> ReadItemsFile( const String& path, THashSet< String >& warnings );
	void GenerateThumbnail( const String& outputPath, const String& category, const String& entityTemplate, const String& originalIconPath, THashSet< String >& warnings );
};
