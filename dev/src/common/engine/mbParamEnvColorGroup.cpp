#include "build.h"
#include "mbParamEnvColorGroup.h"
#include "../../../../bin/shaders/include/globalConstants.fx"
#include "../../../../bin/shaders/include/globalConstantsPS.fx"
#include "../../../../bin/shaders/include/globalConstantsVS.fx"
#include "graphConnectionRebuilder.h"
#include "materialOutputSocket.h"
#include "environmentAreaParams.h"

IMPLEMENT_ENGINE_CLASS( CMaterialParameterEnvColorGroup );

CMaterialParameterEnvColorGroup::CMaterialParameterEnvColorGroup()
	: m_colorGroup( ECG_Default )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialParameterEnvColorGroup::OnPropertyPostChange( IProperty *property )
{
	// Pass to base class
	TBaseClass::OnPropertyPostChange( property );

	// Update layout
	OnRebuildSockets();
}

String CMaterialParameterEnvColorGroup::GetCaption() const
{
	return TXT("Env Color Group");
}

void CMaterialParameterEnvColorGroup::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Out ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( X ), Color::RED ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("yyyy"), CNAME( Y ), Color::GREEN ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("zzzz"), CNAME( Z ), Color::BLUE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( W ), Color( 127, 127, 127 ) ) );
}

#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CMaterialParameterEnvColorGroup::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{		
	return CodeChunk::Printf( true, "colorGroups[%i]", (Int32)m_colorGroup );
}

#endif
