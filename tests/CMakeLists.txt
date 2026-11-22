find_package(Qt6 REQUIRED COMPONENTS Test Sql Network Quick Qml Gui)

function(kiriflux_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE
        kiriflux_core
        Qt6::Test
        ${ARGN}
    )
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-data"
    )
endfunction()

kiriflux_add_test(DatabaseManagerTest Qt6::Sql)
kiriflux_add_test(AppControllerTest Qt6::Qml)
kiriflux_add_test(ArticleListModelTest)
kiriflux_add_test(FeedListModelTest)
kiriflux_add_test(CategoryListModelTest)
kiriflux_add_test(SyncTest Qt6::Network)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/secrets.h")
    target_compile_definitions(SyncTest PRIVATE EXISTS_SECRETS)
endif()
kiriflux_add_test(BatchSyncTest Qt6::Network)
kiriflux_add_test(SettingsManagerTest)
kiriflux_add_test(NetworkManagerTest Qt6::Network)
kiriflux_add_test(ImagePrefetchTest)

add_executable(ImageProviderTest
    ImageProviderTest.cpp
    ${CMAKE_SOURCE_DIR}/src/KirifluxImageProvider.cpp
)
target_include_directories(ImageProviderTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
)
target_link_libraries(ImageProviderTest PRIVATE
    kiriflux_core
    Qt6::Test
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    Qt6::Network
    Qt6::Sql
)
add_test(NAME ImageProviderTest COMMAND ImageProviderTest)
set_tests_properties(ImageProviderTest PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-data"
)
