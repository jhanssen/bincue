set(SOURCES CueParser.cpp)
set(INCLUDES ${CMAKE_CURRENT_LIST_DIR})

add_library(bincue ${SOURCES})

target_include_directories(bincue PUBLIC ${INCLUDES})
target_compile_features(bincue PRIVATE cxx_std_17)

set_target_properties(bincue PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
