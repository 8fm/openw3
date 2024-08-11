/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

struct SPackageGameFiles
{
	TDynArray< String >										m_bundleFiles;			//!< *.bundle files and related files
	TDynArray< String >										m_scriptFiles;			//!< *.redscripts
	TDynArray< String >										m_cacheFiles;			//!< *.cache files
	TDynArray< String >										m_miscFiles;			//!< Files not fitting any other category
	TDynArray< String >										m_stringsFiles;			//!< *.w3strings
	TDynArray< String >										m_speechFiles;			//!< *.w3speech
};

struct SPackageFiles
{
	TDynArray< String >										m_gameBinFiles;			//!< bin/*.* files, excluding any system files
	TDynArray< String >										m_dynLibFiles;			//!< SCE *.prx files or Windows DLLs. Dynamic libraries.
	TDynArray< String >										m_exeFiles;				//!< SCE ELF or Windows EXE files. Executables.
	THashMap< String, TDynArray< String > >					m_exeDepFiles;			//!< Extra files needed by the EXE. On Xbox One, appdata.bin, AppxManifest.xml, logo PNGs etc
	TDynArray< String >										m_sysFiles;				//!< System files common across EXEs. On PS4, to go under sce_sys
	TDynArray< String >										m_prefetchFiles;		//!< Hacky: Files for content0 prefetch chunk. Must be in content0.
	TArrayMap< Uint32, SPackageGameFiles >					m_gameContentFilesMap;	//!< Map of package name to game files
	TArrayMap< Uint32, TDynArray< String > >				m_gamePatchFilesMap;	//!< Map of package name to patch files
};
