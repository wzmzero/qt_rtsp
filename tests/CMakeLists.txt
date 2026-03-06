add_executable(test_rtsp_pull_api
  test_rtsp_pull_api.cpp
)

target_include_directories(test_rtsp_pull_api PRIVATE
  ${CMAKE_SOURCE_DIR}/src/media_core
)

target_link_libraries(test_rtsp_pull_api PRIVATE media_core)

find_package(Qt6 QUIET COMPONENTS Core Multimedia Network)
if(Qt6_FOUND)
  add_executable(test_qt_stream_worker
    test_qt_stream_worker.cpp
    ${CMAKE_SOURCE_DIR}/src/client/media/stream_worker.cpp
  )
  target_include_directories(test_qt_stream_worker PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
  )
  target_link_libraries(test_qt_stream_worker PRIVATE Qt6::Core Qt6::Multimedia Qt6::Network)
endif()
