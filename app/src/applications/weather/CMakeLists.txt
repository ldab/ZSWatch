if (CONFIG_ZSWATCH_PCB_REV GREATER_EQUAL 4)
    FILE(GLOB app_sources *.c)
    target_sources(app PRIVATE ${app_sources})
endif()