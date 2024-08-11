/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// CScaleformTranslator
//////////////////////////////////////////////////////////////////////////
class CScaleformTranslator : public GFx::Translator
{
private:
	SFUInt m_caps;

public:
	explicit CScaleformTranslator( SFUInt wwMode = 0, SFUInt extraCaps = 0 );

	virtual SFUInt GetCaps() const;
	virtual void Translate( TranslateInfo* ptranslateInfo );
};

#endif // USE_SCALEFORM