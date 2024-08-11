/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include <string>
#include <vector>


namespace DLCTool
{
	///////////////////////////////////////////////////////////////////////////////

	class DLCFileStream;

	///////////////////////////////////////////////////////////////////////////////
	/*
	class DLCImporter
	{
	private:
		std::wstring						m_gameDirectory;

		UINT								m_dataOffset;
		std::vector< std::wstring >			m_installedFiles;
		std::vector< DWORD >				m_fileSizes;

		UINT				m_id;						// id
		std::wstring		m_name;						// name
		std::wstring		m_localPath;				// local download path
		std::wstring		m_additionalDlcLocalPath;	// additional DLC local download path
		DWORD				m_additionalDlcSize;		// additional DLC size
		std::wstring		m_promoCode;				// Promo code
		unsigned long long	m_version;					// file version
		unsigned long long	m_requiredVersion;			// game version required to install this file
		DWORD64				m_checksum;					// crc32

	public:
		DLCImporter( const WCHAR* gameDirectory );
		~DLCImporter();

		bool InstallDLC();

		void UpdateRegistry();
	};*/

	///////////////////////////////////////////////////////////////////////////////

	void CreateArchive( const char* iniFile, const char* outputFile );

	void ExtractArchive( const WCHAR* gameDirectory );

} // DLCTool
