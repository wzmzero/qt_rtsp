add_executable(test_rtsp_pull_api
  test_rtsp_pull_api.cpp
)

target_include_directories(test_rtsp_pull_api PRIVATE
  ${CMAKE_SOURCE_DIR}/src/media_core
)

target_link_libraries(test_rtsp_pull_api PRIVATE media_core)
