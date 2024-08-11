
-- Umbra optimizer library

apifiles = {
	"source/optimizer/umbraLicense.cpp",
	"source/optimizer/umbraTask.cpp",
	"source/optimizer/umbraLocalComputation.cpp",
	"source/optimizer/umbraCloudComputation.cpp",   -- todo split into its own lib
	"source/optimizer/umbraScene.cpp",
	"source/optimizer/umbraBuilder.cpp",
	"source/optimizer/umbraComputationArgs.cpp",
	"source/optimizer/umbraOptimizerTextCommand.cpp",
	"source/optimizer/umbraOptimizerInfoString.cpp",
	"source/optimizer/umbraObjectGrouper.cpp", 
	"source/optimizer/umbraDPVSBuilder.cpp"
}

umbraproject("optimizer_core", nil, "source")
folder "umbra3"
kind "StaticLib"
platforms { "x32", "x64" }
includedirs  "interface/runtime"
sourcedirs { "source/optimizer" }
includedirs { "source/common", "source/runtime", "external/eigen3" }
files { "interface/optimizer/*.hpp" }
excludes(apifiles)
-- Release the core lib for mac
-- Todo: On Windows the core lib is merged into optimizer_static and thus not needed. 
-- How to make it happen with gnu toolchain and premake?
if os.get() == "macosx" then
	project().releasebinary = "full"
end

umbraproject("optimizer", { "Release", "Debug" }, "source")
folder "umbra3"
kind "SharedLib"
includedirs  "interface/runtime"
platforms { "x32", "x64" }
includedirs { "source/common", "source/runtime" }
defines { "UMBRA_UNLOCKED", "UMBRA_DLLEXPORT" }
files(apifiles)
links { "optimizer_core", "runtime", "common" }
configuration { "windows", "not StaticLib", "x32" }
libdirs     "external/curl/win32/lib"
configuration { "windows", "not StaticLib", "x64" }
libdirs     "external/curl/win64/lib"

configuration { "windows", "not StaticLib", "x32 or x64", "Release or Test" }
links "libcurl"
configuration { "windows", "not StaticLib", "x32 or x64", "Debug" }
links "libcurl_d"


umbraproject("optimizer_static", { "Release", "Debug" }, "source")
folder "umbra3"
kind "StaticLib"
platforms { "x32", "x64" }
includedirs { "interface/runtime", "source/common", "source/runtime" }
defines { "UMBRA_UNLOCKED" }
files(apifiles)
links { "optimizer_core" }

umbraproject("optimizer_l", { "Release", "Debug" }, "source")
folder "umbra3"
kind "SharedLib"
platforms { "x32", "x64" }
includedirs { "interface/runtime", "source/common", "source/runtime" }
defines { "UMBRA_DLLEXPORT" }
files(apifiles)
links { "optimizer_core", "runtime", "common" }
configuration { "windows", "not StaticLib", "x32" }
libdirs     "external/curl/win32/lib"
configuration { "windows", "not StaticLib", "x64" }
libdirs     "external/curl/win64/lib"

configuration { "windows", "not StaticLib", "x32 or x64", "Release or Test" }
links "libcurl"
configuration { "windows", "not StaticLib", "x32 or x64", "Debug" }
links "libcurl_d"


umbraproject("optimizer_static_l", { "Release", "Debug" }, "source")
folder "umbra3"
kind "StaticLib"
platforms { "x32", "x64" }
includedirs { "interface/runtime", "source/common", "source/runtime" }
files(apifiles)
links { "optimizer_core" }

