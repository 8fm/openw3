
TOP              = $(_HERE_)/..

PATH            += $(TOP)/open64/bin;$(TOP)/nvvm;$(_HERE_);$(TOP)/lib;

INCLUDES        +=  "-I$(TOP)/include" $(_SPACE_)

LIBRARIES        =+ $(_SPACE_) "/LIBPATH:$(TOP)/lib/$(_WIN_PLATFORM_)" cudart.lib

CUDAFE_FLAGS    +=
OPENCC_FLAGS    +=
PTXAS_FLAGS     +=
