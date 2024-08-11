
-- Umbra command line interface for optimizer

umbraproject("dpvs", {"Release", "Debug"}, "source")
folder "umbra3"
platforms { "x32", "x64" }
kind "ConsoleApp"
includedirs { "interface/optimizer", "interface/runtime" }
sourcedirs "utils/dpvs"
includedirs { "source/optimizer", "source/common" }
links { "optimizer", "common", "runtime" }

configuration { "windows" }
includedirs { "external/opengl", "external/glut/include" }
defines { "GLUT_NO_LIB_PRAGMA" }
links { "glut", "winmm", "opengl32", "glu32", "comctl32" }
