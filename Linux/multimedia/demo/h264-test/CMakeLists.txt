project(h264-test)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0 gio-unix-2.0 gstreamer-0.10 gstreamer-app-0.10)

target_include_directories(${PROJECT_NAME} PRIVATE ${GLIB_INCLUDE_DIRS})
target_compile_options(${PROJECT_NAME} PRIVATE ${GLIB_CFLAGS_OTHER})
target_link_libraries(${PROJECT_NAME} ${GLIB_LIBRARIES})

