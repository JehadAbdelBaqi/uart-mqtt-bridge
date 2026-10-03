# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS "../include")

# Certificates the MQTT client (components/modules/mqtt_link.c) connects with, written by
# scripts/steps/setup-certs.sh. Embedded as '\0'-terminated text; also listed in platformio.ini
# (board_build.embed_txtfiles). PlatformIO only accepts these lines in this file.
target_add_binary_data(${COMPONENT_TARGET} "../include/secrets/ca.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "../include/secrets/client.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "../include/secrets/client.key" TEXT)
