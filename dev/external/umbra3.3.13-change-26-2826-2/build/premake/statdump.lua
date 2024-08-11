
-- Umbra statdump application

umbraproject("statdump", "Release", "source")
folder "utils"
platforms { "x32", "x64" }
kind "ConsoleApp"
sourcedirs "utils/statdump"
includedirs { "source/runtime", "source/common" }
links { "runtime", "common" }
