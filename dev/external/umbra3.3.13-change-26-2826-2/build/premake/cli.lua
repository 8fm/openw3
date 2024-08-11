
-- Umbra command line interface for optimizer

umbraproject("cli", { "Release", "Debug" }, "source")
folder "umbra3"
platforms { "x32", "x64" }
kind "ConsoleApp"
includedirs { "interface/optimizer", "interface/runtime" }
sourcedirs "utils/cli"
includedirs { "source/common" }
links { "optimizer", "common" }
