/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

// TODO: Move into config?

const Char* const PLAYGO_DAT_FILE			= TXT("installer.dat");
const Char* const INSTALLER_DAT_FILE		= TXT("installer.dat");
const Char* const GAME_BIN_DIR				= TXT("bin");
const Char* const CONTENT_DIR				= TXT("content");
const Char* const PATCH_DIR					= TXT("patch");
const Char* const PATCH_NAME_PREFIX			= TXT("patch");
const Char* const CONTENT_NAME_PREFIX		= TXT("content");
const Char* const GAME_SPEECH_EXT			= TXT("w3speech");
const Char* const GAME_STRINGS_EXT			= TXT("w3strings");
const Char* const SYS_PRX_EXT				= TXT("prx");
const Char* const SYS_ELF_EXT				= TXT("elf");
const Char* const SYS_EXE_EXT				= TXT("exe");
const Char* const SYS_DLL_EXT				= TXT("dll");

// FIX: Hack instead of scanning.
const Char* const REGISTRATION_FILES_XBOX_APP[] = { 
	TXT("appdata.bin"),  TXT("AppxManifest.xml"), TXT("resources.pri")
};

const Char* const REGISTRATION_FILES_XBOX_DLC[] = { 
	TXT("AppxManifest.xml"), TXT("resources.pri"), TXT("smallLogo.png"), TXT("wideLogo.png"), TXT("logo.png")
};

const Uint32 DEFAULT_CONTENT_NAME_NUMBER	= 0;

// Technically bytes, but we don't allow unicode chars anyway (and not encoding utf8)
const Uint32 MAX_ENTRY_LENGTH	= 255; // 256 including null terminator
const Uint32 MAX_PATH_LENGTH	= 999; // 1000 including null terminator

const Uint32 MAX_DIR_DEPTH				= 8;
const Uint32 MAX_PATH_COMPONENTS_APP	= 0x20000;
const Uint32 MAX_PATH_COMPONENTS_PATCH	= 0x10000;
const Uint32 MAX_PATH_COMPONENTS_DLC	= 0x10000;
const Uint32 MAX_CHUNK_ID				= 99;
const Uint32 MIN_CHUNK_ID_XBOX			= 0x1; // confirmed from the MakePkg validation output
const Uint32 BIN_CHUNK_ID_XBOX			= MIN_CHUNK_ID_XBOX;
const Uint32 MAX_USED_CHUNK_ID_XBOX		= 0x200 - 1;
const Uint32 MAX_CHUNK_ID_XBOX			= 0x3FFFFFFF; // confirmed from the MakePkg validation output

	
const Uint32 MIN_COMPRESS_SIZE				= 64 * 1024; // Anything less than this is just a waste

enum EPackageType
{
	ePackageType_App,
	ePackageType_Patch,
	ePackageType_Dlc,
};