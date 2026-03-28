find_package(Qt6 REQUIRED COMPONENTS Test Sql)

add_executable(DatabaseTest DatabaseTest.cpp)
target_link_libraries(DatabaseTest PRIVATE
    kiriflux_core
    Qt6::Test
    Qt6::Sql
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/secrets.h")
    add_executable(SyncTest SyncTest.cpp)
    target_link_libraries(SyncTest PRIVATE
        kiriflux_core
        Qt6::Test
        Qt6::Network
    )
    add_test(NAME SyncTest COMMAND SyncTest)
else()
    message(STATUS "secrets.h not found, skipping SyncTest")
endif()
