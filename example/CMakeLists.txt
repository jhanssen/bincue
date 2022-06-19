set(SOURCES main.cpp)

add_executable(bincue_example ${SOURCES})
target_compile_features(bincue_example PRIVATE cxx_std_17)
target_link_libraries(bincue_example bincue)
