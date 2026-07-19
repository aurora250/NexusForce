add_nexusforce_example(NexusForceIntegrationExample
    main.cpp
    task_store.cpp
    ws_broadcaster.cpp
)

nexusforce_reflect_scan(
    TARGET NexusForceIntegrationExample
    HEADERS ${CMAKE_CURRENT_SOURCE_DIR}
)
