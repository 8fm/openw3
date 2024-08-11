
-- Umbra visibility cacher application

umbraproject("visibilitycacher", nil, "source")
folder "utils"
if os.is("linux")
then
  platforms { "x64" }
else
  platforms { "x32", "x64" }
end
kind "ConsoleApp"
sourcedirs "utils/visibilitycacher"
includedirs { "utils/testdb", "source/common" }
links { "common", "runtime", "optimizer", "testdb" }

configuration { "windows" }
includedirs { "external/opengl", "external/glut/include" }
defines { "GLUT_NO_LIB_PRAGMA" }
links { "glut", "winmm", "opengl32", "glu32" }

configuration { "linux" }
links { "glut", "GLU", "GL" }

configuration { "macosx" }
links { "GLUT.framework", "OpenGL.framework" }
