defaultproject("squirrel", nil, "source")
platforms { "x32", "x64" }
kind "StaticLib"
folder "external"
includedirs { "external/squirrel/include" }
defines { "_CRT_SECURE_NO_WARNINGS" }
files {
	"external/squirrel/squirrel/*.cpp",
	"external/squirrel/squirrel/*.h",
	"external/squirrel/include/*.h",
	"external/sqrat/include/**.h"
}

defaultproject("squirrelLib", nil, "source")
platforms { "x32", "x64" }
kind "StaticLib"
folder "external"
includedirs { "external/squirrel/include" }
links { "squirrel" }
defines { "_CRT_SECURE_NO_WARNINGS" }

files { "external/squirrel/sqstdlib/*.cpp", "external/squirrel/sqstdlib/*.h" }
