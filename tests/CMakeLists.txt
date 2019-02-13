project(tests)

add_executable(builder-test
        builder_test.cc
)

target_include_directories(builder-test
        PRIVATE
        ${hoglStatic}
)

target_link_libraries(builder-test
    hogl
)