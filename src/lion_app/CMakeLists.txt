file(GLOB APP_ROOT_SOURCE *.c)
file(GLOB APP_ROOT_HEADER *.h)
file(GLOB APP_SLVR_SOURCE solver/*.c)
file(GLOB APP_SLVR_HEADER solver/*.h)

add_library(${PROJECT_APP_NAME} ${APP_ROOT_HEADER} ${APP_ROOT_SOURCE}
                                ${APP_SLVR_HEADER} ${APP_SLVR_SOURCE})

target_link_libraries(
  ${PROJECT_APP_NAME} PUBLIC ${PROJECT_MATH_NAME} ${PROJECT_UTILS_NAME}
                             ${GSL_LIBRARIES})
target_include_directories(
  ${PROJECT_APP_NAME}
  PUBLIC ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
         ${PROJECT_SOURCE_DIR_LOCATION} ${PROJECT_HEADERS} ${CMAKE_SOURCE_DIR}
         ${GSL_INCLUDE_DIRS})

