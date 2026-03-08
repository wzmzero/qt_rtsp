add_library(media_core SHARED
  media_api.cpp
)

target_include_directories(media_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

