cmake_minimum_required(VERSION 3.28...3.30)

set(_name "gamepad-monitor")
set(_version "1.0.0")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

if(APPLE)
  add_link_options("-F${CMAKE_CURRENT_SOURCE_DIR}/Frameworks")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  set(WrapOpenGL_AGL "/System/Library/Frameworks/OpenGL.framework" CACHE FILEPATH "" FORCE)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

# Use system SDL2 from Homebrew
set(SDL2_INCLUDE_DIR "/opt/homebrew/include/SDL2")
set(SDL2_LIBRARY "/opt/homebrew/lib/libSDL2.a")

include_directories(${SDL2_INCLUDE_DIR})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
  src/plugin-main.cpp
  src/gamepad-manager.cpp
  src/gamepad-settings-dialog.cpp
  src/thirdparty/mongoose.c
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${SDL2_LIBRARY})

if(APPLE)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(GAMECONTROLLER_LIBRARY GameController)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
    ${IOKIT_LIBRARY} 
    ${COREFOUNDATION_LIBRARY}
    ${GAMECONTROLLER_LIBRARY}
  )
endif()

if(APPLE)
  set_source_files_properties(src/thirdparty/mongoose.c PROPERTIES COMPILE_FLAGS "-w")
elseif(MSVC)
  set_source_files_properties(src/thirdparty/mongoose.c PROPERTIES COMPILE_FLAGS "/W0")
endif()

set(WrapOpenGL_AGL "/System/Library/Frameworks/OpenGL.framework" CACHE FILEPATH "" FORCE)
set(OPENGL_agl_LIBRARY "/System/Library/Frameworks/OpenGL.framework" CACHE FILEPATH "" FORCE)

if(APPLE)
  get_target_property(LIBS ${CMAKE_PROJECT_NAME} LINK_LIBRARIES)
  if(LIBS)
    list(REMOVE_ITEM LIBS "-framework AGL")
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_LIBRARIES "${LIBS}")
  endif()
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
