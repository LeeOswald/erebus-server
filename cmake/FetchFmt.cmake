FetchContent_Declare(fmt
    GIT_REPOSITORY "https://github.com/fmtlib/fmt"
    GIT_TAG "master"
)
FetchContent_MakeAvailable(fmt)

set(fmt_INCLUDE_DIR "$<BUILD_INTERFACE:${fmt_SOURCE_DIR}/include>")
include_directories(${fmt_INCLUDE_DIR})