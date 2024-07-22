FILE(GLOB edge_impulse_sources *.c)
target_sources(app PRIVATE ${edge_impulse_sources})

# Control the size of the buffer that's used to read from the signal
add_compile_definitions(EI_DSP_IMAGE_BUFFER_STATIC_SIZE=1024)
# Statically allocate the neural network model
add_compile_definitions(EI_CLASSIFIER_ALLOCATION_STATIC=1)
# Below macros are needed to link it from C code
add_compile_definitions(EIDSP_SIGNAL_C_FN_POINTER=1)
add_compile_definitions(EI_C_LINKAGE=1)

zephyr_include_directories(${PROJECT_SOURCE_DIR}/src/ext_drivers/fusion/Fusion)
zephyr_sources(${PROJECT_SOURCE_DIR}/src/ext_drivers/fusion/Fusion/FusionOffset.c)
zephyr_sources(${PROJECT_SOURCE_DIR}/src/ext_drivers/fusion/Fusion/FusionCompass.c)
zephyr_sources(${PROJECT_SOURCE_DIR}/src/ext_drivers/fusion/Fusion/FusionAhrs.c)

# port below

if(NOT TARGET app)
     message(FATAL_ERROR "Please create a target named 'app' (ex: add_executable(app)) before adding this file")
endif()

include(edge-impulse-sdk/cmake/utils.cmake)
add_subdirectory(edge-impulse-sdk/cmake/zephyr)

RECURSIVE_FIND_FILE_APPEND(MODEL_SOURCE "tflite-model" "*.cpp")
target_include_directories(app PRIVATE .)

# add all sources to the project
target_sources(app PRIVATE ${MODEL_SOURCE})
