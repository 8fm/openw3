/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CAppearanceComponent;

class IAppearanceComponentModifierModifier
{
public:
	virtual void OnAppearanceChange( CAppearanceComponent* appearanceComponent ) = 0;
	virtual void AppearanceComponentOnFinalize( CAppearanceComponent* appearanceComponent ) = 0;
};

///////////////////////////////////////////////////////////

/// Appearance component modifier
class CAppearanceComponentModifierManager
{
private:
	THashMap< Uint32, TDynArray<IAppearanceComponentModifierModifier*> > m_modifiers;

	friend class CAppearanceComponent;

public:
	CAppearanceComponentModifierManager(){}
	~CAppearanceComponentModifierManager(){}

	Bool RegisteModifier( const String& entityTemplateName, IAppearanceComponentModifierModifier* appearanceComponentModifier );
	Bool UnregisteModifier( const String& entityTemplateName, IAppearanceComponentModifierModifier* appearanceComponentModifier );
	Bool IsModifierRegistered( const String& entityTemplatePath ) const;

private:
	void OnAppearanceChange( CAppearanceComponent* appearanceComponent );
	void AppearanceComponentOnFinalize( CAppearanceComponent* appearanceComponent );
};

typedef TSingleton< CAppearanceComponentModifierManager, TDefaultLifetime, TCreateUsingNew > SAppearanceComponentModifierManager;
