find_package(Qt6 REQUIRED COMPONENTS Test Sql Network)

# 1. DatabaseManagerTest
add_executable(DatabaseManagerTest DatabaseManagerTest.cpp)
target_link_libraries(DatabaseManagerTest PRIVATE
    kiriflux_core
    Qt6::Test
    Qt6::Sql
)
add_test(NAME DatabaseManagerTest COMMAND DatabaseManagerTest)

# 2. ArticleListModelTest
add_executable(ArticleListModelTest ArticleListModelTest.cpp)
target_link_libraries(ArticleListModelTest PRIVATE
    kiriflux_core
    Qt6::Test
)
add_test(NAME ArticleListModelTest COMMAND ArticleListModelTest)

# 3. FeedListModelTest
add_executable(FeedListModelTest FeedListModelTest.cpp)
target_link_libraries(FeedListModelTest PRIVATE
    kiriflux_core
    Qt6::Test
)
add_test(NAME FeedListModelTest COMMAND FeedListModelTest)

# 4. CategoryListModelTest
add_executable(CategoryListModelTest CategoryListModelTest.cpp)
target_link_libraries(CategoryListModelTest PRIVATE
    kiriflux_core
    Qt6::Test
)
add_test(NAME CategoryListModelTest COMMAND CategoryListModelTest)

# 5. SyncTest (Integration, requires secrets)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/secrets.h")
    add_executable(SyncTest SyncTest.cpp)
    target_compile_definitions(SyncTest PRIVATE EXISTS_SECRETS)
    target_link_libraries(SyncTest PRIVATE
        kiriflux_core
        Qt6::Test
        Qt6::Network
    )
    add_test(NAME SyncTest COMMAND SyncTest)
else()
    add_executable(SyncTest SyncTest.cpp) # Built anyway, skip at runtime if no env/secrets
    target_link_libraries(SyncTest PRIVATE
        kiriflux_core
        Qt6::Test
        Qt6::Network
    )
    add_test(NAME SyncTest COMMAND SyncTest)
endif()

# 6. BatchSyncTest
add_executable(BatchSyncTest BatchSyncTest.cpp)
target_link_libraries(BatchSyncTest PRIVATE
    kiriflux_core
    Qt6::Test
    Qt6::Network
)
add_test(NAME BatchSyncTest COMMAND BatchSyncTest)
