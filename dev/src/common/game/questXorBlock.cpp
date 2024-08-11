#include "build.h"
#include "questGraphSocket.h"
#include "questXorBlock.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/gameSave.h"

IMPLEMENT_ENGINE_CLASS( CQuestXorBlock )

void CQuestXorBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_wasInputActivated;
	compiler << i_wasOutputActivated;
}

void CQuestXorBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_wasInputActivated ] = false;
	instanceData[ i_wasOutputActivated ] = false;
}

void CQuestXorBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	if ( data[ i_wasOutputActivated ] )
	{
		return;
	}

	if ( data[ i_wasInputActivated ] == false )
	{
		// activate an input only if one input was activated
		data[ i_wasInputActivated ] = true;
	}
}


void CQuestXorBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	if ( data[ i_wasOutputActivated ] )
	{
		return;
	}
	if ( data[ i_wasInputActivated ] == true )
	{
		data[ i_wasOutputActivated ] = true;
		ActivateOutputWithoutExiting( data, CNAME( Out ) );
	}
}

void CQuestXorBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	TBaseClass::SaveGame( data, saver );
	saver->WriteValue( CNAME(wasInputActivated), data[ i_wasInputActivated ] );
	saver->WriteValue( CNAME(wasOutputActivated), data[ i_wasOutputActivated ] );
}

void CQuestXorBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	TBaseClass::LoadGame( data, loader );
	loader->ReadValue( CNAME(wasInputActivated), data[ i_wasInputActivated ] );
	loader->ReadValue( CNAME(wasOutputActivated), data[ i_wasOutputActivated ] );
}

