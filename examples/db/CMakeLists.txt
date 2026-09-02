# ---- SQL Builder ----
add_nexusforce_example(NexusForceSqlBuilderExample db
    sql_builder_example.cpp
)

# ---- DB Config ----
add_nexusforce_example(NexusForceDbConfigExample db
    db_config_example.cpp
)

# ---- CRUD + Transactions ----
add_nexusforce_example(NexusForceDbCrudExample db
    db_crud_example.cpp
)

# ---- Transaction Guard ----
add_nexusforce_example(NexusForceTransactionGuardExample db
    transaction_guard_example.cpp
)

# ---- Prepared Statement ----
add_nexusforce_example(NexusForcePreparedStatementExample db
    prepared_statement_example.cpp
)

# ---- Batch Insert ----
add_nexusforce_example(NexusForceBatchInsertExample db
    batch_insert_example.cpp
)

# ---- Connection Pool ----
add_nexusforce_example(NexusForceConnectionPoolExample db
    connection_pool_example.cpp
)

# ---- Result Metadata ----
add_nexusforce_example(NexusForceResultMetadataExample db
    result_metadata_example.cpp
)

# ---- ORM ----
add_nexusforce_example(NexusForceOrmExample db
    orm_example.cpp
)

# ---- Redis ----
add_nexusforce_example(NexusForceRedisExample db
    redis_example.cpp
)
