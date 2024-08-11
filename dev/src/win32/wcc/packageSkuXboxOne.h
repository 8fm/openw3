/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/redSystem/utility.h"
#include "packageConstants.h"
#include "packageFiles.h"
#include "packageLanguagesXboxOne.h"

struct SPackageSkuXboxOne : private Red::System::NonCopyable
{
	EPackageType						m_packageType;
	const SPackageFiles&				m_packageFiles;
	const SPackageLanguagesXboxOne&		m_packageLanguages;

	SPackageSkuXboxOne( EPackageType packageType, const SPackageFiles& packageFiles, const SPackageLanguagesXboxOne& packageLanguages )
		: m_packageType( packageType )
		, m_packageFiles( packageFiles )
		, m_packageLanguages( packageLanguages )
	{}
};
