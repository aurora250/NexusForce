# =============================================================================
# NexusForce DB Module Examples
# =============================================================================

# Helper macro to reduce boilerplate
macro(add_db_example NAME)
    add_executable(${NAME} ${ARGN})
    if(WIN32)
        set_target_properties(${NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
    elseif(UNIX AND NOT APPLE)
        set_target_properties(${NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
    endif()
    target_link_libraries(${NAME} PRIVATE NexusForce::NexusForce)
    nexusforce_compiler_options(${NAME})
endmacro()

# ---- SQL Builder ----
add_db_example(NexusForceSqlBuilderExample
    sql_builder_example.cpp
)

# ---- DB Config ----
add_db_example(NexusForceDbConfigExample
    db_config_example.cpp
)

# ---- CRUD + Transactions ----
add_db_example(NexusForceDbCrudExample
    db_crud_example.cpp
)

# ---- Transaction Guard ----
add_db_example(NexusForceTransactionGuardExample
    transaction_guard_example.cpp
)

# ---- Prepared Statement ----
add_db_example(NexusForcePreparedStatementExample
    prepared_statement_example.cpp
)

# ---- Batch Insert ----
add_db_example(NexusForceBatchInsertExample
    batch_insert_example.cpp
)

# ---- Connection Pool ----
add_db_example(NexusForceConnectionPoolExample
    connection_pool_example.cpp
)

# ---- Result Metadata ----
add_db_example(NexusForceResultMetadataExample
    result_metadata_example.cpp
)

# ---- Redis ----
add_db_example(NexusForceRedisExample
    redis_example.cpp
)
