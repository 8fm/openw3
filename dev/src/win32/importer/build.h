/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "..\..\common\core\core.h"
#include "..\..\common\engine\engine.h"
#include "..\..\common\game\game.h"
#include "..\..\common\gpuApiDX10\gpuApi.h"
#include "..\..\win32\platform\win32.h"
#include "../../common/core/factory.h"

#define LOG_IMPORTER( format, ... ) RED_LOG( Importer, format, ##__VA_ARGS__ )
#define ERR_IMPORTER( format, ... ) RED_LOG_ERROR( Importer, format, ##__VA_ARGS__ )
#define WARN_IMPORTER( format, ... ) RED_LOG_WARNING( Importer, format, ##__VA_ARGS__ );

#include "importerTypeRegistry.h"

