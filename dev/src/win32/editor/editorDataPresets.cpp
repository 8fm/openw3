#include "build.h"
#include "editorDataPresets.h"
#include "..\..\common\core\factory.h"
#include "..\..\common\engine\environmentManager.h"


class CDataPresetsFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CDataPresetsFactory, IFactory, 0 );

public:
	CDataPresetsFactory()
	{
		m_resourceClass = ClassID< CDataPresets >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options )
	{
		CDataPresets* dataPreset = ::CreateObject< CDataPresets >( options.m_parentObject );
		return dataPreset;
	}
};

BEGIN_CLASS_RTTI( CDataPresetsFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()


IMPLEMENT_ENGINE_CLASS( CDataPresets );
IMPLEMENT_ENGINE_CLASS( CStoryScenePresets );
IMPLEMENT_ENGINE_CLASS( CDataPresetsFactory );
