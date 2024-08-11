/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

const String STRINGS_FILE_PATH_POSTFIX = TXT( ".w3strings" );
const String SPEECH_FILE_PATH_POSTFIX_DURANGO = TXT( "xboxone.w3speech" );
const String SPEECH_FILE_PATH_POSTFIX_ORBIS = TXT( "ps4.w3speech" );
const String SPEECH_FILE_PATH_POSTFIX_PC = TXT( "pc.w3speech" );
#if defined(RED_PLATFORM_DURANGO)
const String CURRENT_SPEECH_FILE_PATH_POSTFIX = SPEECH_FILE_PATH_POSTFIX_DURANGO;
#elif defined(RED_PLATFORM_ORBIS)
const String CURRENT_SPEECH_FILE_PATH_POSTFIX = SPEECH_FILE_PATH_POSTFIX_ORBIS;
#elif defined(RED_PLATFORM_WINPC)
const String CURRENT_SPEECH_FILE_PATH_POSTFIX = SPEECH_FILE_PATH_POSTFIX_PC;
#elif
#error Unsupported platform!
#endif

#if defined( RED_PLATFORM_WINPC )
# define STATIC_SHADER_CACHE_FILENAME TXT("staticshader.cache")
# define SHADER_CACHE_FILENAME TXT("shader.cache")
#elif defined( RED_PLATFORM_ORBIS )
# define STATIC_SHADER_CACHE_FILENAME TXT("staticshaderps4.cache")
# define SHADER_CACHE_FILENAME TXT("shaderps4.cache")
#elif defined( RED_PLATFORM_DURANGO )
# define STATIC_SHADER_CACHE_FILENAME TXT("staticshaderxboxone.cache")
# define SHADER_CACHE_FILENAME TXT("shaderxboxone.cache")
#else
# error Unsupported platform!
#endif