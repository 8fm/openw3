
-- Umbra validator application

umbraproject("validator", nil, "source")
folder "utils"
kind "ConsoleApp"
sourcedirs "utils/validator"
excludes "utils/validator/umbraConnectivityValidator.*"
includedirs { "source/common", "utils/testdb", "utils/main" }
links { "runtime", "testdb", "common" }

configuration { "x32 or x64" }
links "optimizer"
