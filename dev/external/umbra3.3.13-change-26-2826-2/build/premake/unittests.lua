
-- Umbra unit tests

umbraproject("unittests", nil, "source")
folder "utils"
kind "ConsoleApp"

sourcedirs { "tests/unittests", "tests/unittests/tests", "tests/unittests/newtests", "tests/unittests/modeldata" }
includedirs { "interface/runtime", "source/common", "utils/testdb", "source/runtime", "utils/main" }
links { "common", "testdb", "runtime" }
files { "tests/unittests/unitfiles/*.unit" }

configuration "x32 or x64"
links { "optimizer_static" }

configuration { "x32 or x64", "linux or macosx" }
links { "optimizer_core" }

configuration { "windows", "not StaticLib", "x32" }
libdirs     "external/curl/win32/lib"
configuration { "windows", "not StaticLib", "x64" }
libdirs     "external/curl/win64/lib"

configuration { "windows", "not StaticLib", "x32 or x64", "Release or Test" }
links "libcurl"
configuration { "windows", "not StaticLib", "x32 or x64", "Debug" }
links "libcurl_d"
