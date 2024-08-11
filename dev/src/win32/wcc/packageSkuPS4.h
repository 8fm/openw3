/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/redSystem/utility.h"
#include "packageConstants.h"
#include "packageFiles.h"
#include "packageLanguagesPS4.h"
#include "packagePlayGoChunksBuilder.h"

struct SPatchParamsPS4
{
	PackageHelpers::SPackageInfo	m_basePackage;
	String							m_appPkgPath;
	String							m_latestPatchPath;
	Bool							m_isDayOne;

	SPatchParamsPS4()
		: m_isDayOne(false)
	{}
};

struct SPackageSkuPS4 : private Red::System::NonCopyable
{
	EPackageType					m_packageType;
	const SPackageFiles&			m_packageFiles;
	const SPackageLanguagesPS4&		m_packageLanguages;
	String							m_contentID;				//!< E.g., V0002-NPXS29038_00-SIMPLESHOOTINGAM
	String							m_passcode;					//!< E.g., vE6xCpZxd96scOUGuLPbuLp8O800B0s
	String							m_entitlementKey;			//!< DLC only. E.g., 00112233445566778899aabbccddeeff
	SPatchParamsPS4					m_patchParams;				//!< Patch only stuff

	SPackageSkuPS4( EPackageType packageType, const SPackageFiles& packageFiles, const SPackageLanguagesPS4& packageLanguages )
		: m_packageType( packageType )
		, m_packageFiles( packageFiles )
		, m_packageLanguages( packageLanguages )
	{}
};
