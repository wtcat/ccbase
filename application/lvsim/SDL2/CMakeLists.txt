
include_directories(
	include
	include/SDL2
)

add_library(sdl2_win STATIC IMPORTED)
set_property(TARGET sdl2_win PROPERTY
             IMPORTED_LOCATION "lib/x86/SDL2.lib")