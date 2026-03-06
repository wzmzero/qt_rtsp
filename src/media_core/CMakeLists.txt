add_library(media_core SHARED
  media_api.cpp
)

target_include_directories(media_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(media_pull_test
  main.cpp
)

target_link_libraries(media_pull_test PRIVATE media_core)
