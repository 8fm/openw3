#include "build.h"

#include "../../common/core/gameConfiguration.h"

struct GameConfigurationFixture : public ::testing::Test
{
	GameConfigurationFixture()
	{
		config.name = TXT( "MyName" );
		config.dataPathSuffix = TXT( "MyDataPath" );
		config.userPathSuffix = TXT("MyUserPath" );
		config.scriptsPathSuffix = TXT( "MyScriprPath" );
		config.configDirectoryName = TXT( "MyConfigDir" );
		config.gameClassName = TXT( "MyGameClassName" );
		config.playerClassName = TXT( "MyPlayerClassName" );
		config.telemetryClassName = TXT( "MyTelemetryClassName" );
		config.cameraDirectorClassName = TXT( "MyCameraDirectorClassName" );
	
		gameConfig.Initialize( config );
	}

	SGameConfigurationParameter config;
	CGameConfiguration gameConfig;
};

TEST_F( GameConfigurationFixture, GetName_return_correct_string )
{
	EXPECT_EQ( gameConfig.GetName(), config.name );
}

TEST_F( GameConfigurationFixture, GetDataPathSuffix_return_correct_string )
{
	EXPECT_EQ( gameConfig.GetDataPathSuffix(), config.dataPathSuffix );
}

TEST_F( GameConfigurationFixture, GetUserPathSuffix_return_correct_string )
{
	EXPECT_EQ( gameConfig.GetUserPathSuffix(), config.userPathSuffix );
}

TEST_F( GameConfigurationFixture, GetScriptPathSuffix_return_correct_string )
{
	EXPECT_EQ( gameConfig.GetScriptsPathSuffix(), config.scriptsPathSuffix );
}

TEST_F( GameConfigurationFixture, GetConfigDirName_return_correct_string )
{
	EXPECT_EQ( gameConfig.GetConfigDirName(), config.configDirectoryName );
}

TEST_F( GameConfigurationFixture, GetGameClassName_return_correct_string )
{
	EXPECT_EQ( gameConfig.GetGameClassName(), config.gameClassName );
}

TEST_F( GameConfigurationFixture, GetPlayerClassName_return_correct_string )
{
	EXPECT_EQ( gameConfig.GetPlayerClassName(), config.playerClassName );
}

TEST_F( GameConfigurationFixture, GetTelemetryClassName_return_correct_string )
{
	EXPECT_EQ( gameConfig.GetTelemetryClassName(), config.telemetryClassName );
}

TEST_F( GameConfigurationFixture, GetCameraDirectoryClassName_return_correct_string )
{
	EXPECT_EQ( gameConfig.GetCameraDirectorClassName(), config.cameraDirectorClassName );
}
