set(CATCH2_DIR "${CMAKE_SOURCE_DIR}/third_party/Catch2")

add_executable(test_fep
    test_lexer.cpp
    test_parser.cpp
    test_eval.cpp
    "${CATCH2_DIR}/extras/catch_amalgamated.cpp"
)

target_link_libraries(test_fep PRIVATE fep_core)

target_include_directories(test_fep PRIVATE
    "${CATCH2_DIR}/extras"
)
