add_executable(test_ast1 test_ast.cpp)
add_executable(test_logging test_logging.cpp)
target_link_libraries(test_ast1 syntax common)
target_link_libraries(test_logging common)

install(
    TARGETS test_logging
    RUNTIME DESTINATION bin
)

install(
    TARGETS test_ast1
    RUNTIME DESTINATION bin
)
