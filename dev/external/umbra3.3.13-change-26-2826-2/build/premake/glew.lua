defaultproject("glew", nil, "source")
platforms { "x32", "x64" }
kind "StaticLib"
folder "external"
includedirs { "external/glew/include" }
links { "opengl32" }
defines { "GLEW_STATIC", "_CRT_SECURE_NO_WARNINGS", "GLEW_MX" }

files { "external/glew/include/GL/*.h" }
files { "external/glew/src/*.c" }
excludes {
	"external/glew/src/visualinfo.c",
	"external/glew/src/glewinfo.c"
}
