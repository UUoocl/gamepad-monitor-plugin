cmake_minimum_required(VERSION 3.28...3.30)

set(_name "gamepad-monitor")
set(_version "1.0.0")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

if(APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Frameworks")
  add_link_options("-F${CMAKE_CURRENT_SOURCE_DIR}/Frameworks")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
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

# Use source-based SDL2 via FetchContent to ensure Universal Binary support
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

if(TRUE) # Force FetchContent
  set(SDL_WERROR OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  
  FetchContent_Declare(
    SDL2
    URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.2/SDL2-2.30.2.tar.gz
  )
  FetchContent_MakeAvailable(SDL2)
  if(TARGET SDL2-static)
    set(SDL2_LIBRARY SDL2-static)
    target_compile_options(SDL2-static PRIVATE 
      $<$<C_COMPILER_ID:Clang,AppleClang,GNU>:-Wno-error -Wno-sign-compare>
      $<$<C_COMPILER_ID:MSVC>:/WX- /wd4018 /wd4244 /wd4267>
    )
  elseif(TARGET SDL2)
    set(SDL2_LIBRARY SDL2)
    target_compile_options(SDL2 PRIVATE 
      $<$<C_COMPILER_ID:Clang,AppleClang,GNU>:-Wno-error -Wno-sign-compare>
      $<$<C_COMPILER_ID:MSVC>:/WX- /wd4018 /wd4244 /wd4267>
    )
  endif()
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
  src/plugin-main.cpp
  src/gamepad-manager.cpp
  src/gamepad-settings-dialog.cpp
  src/thirdparty/mongoose.c
)

# Apply -Werror only to our own plugin target on CI
if(DEFINED ENV{CI})
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE 
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
  )
endif()

# Use target-based includes if SDL2_INCLUDE_DIR isn't manually set (FetchContent case)
if(SDL2_INCLUDE_DIR)
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIR})
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${SDL2_LIBRARY})

if(WIN32)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE winmm imm32 version setupapi)
endif()

if(APPLE)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(GAMECONTROLLER_LIBRARY GameController)
  find_library(COREAUDIO_LIBRARY CoreAudio)
  find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
  find_library(FORCEFEEDBACK_LIBRARY ForceFeedback)
  find_library(COREVIDEO_LIBRARY CoreVideo)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(CARBON_LIBRARY Carbon)
  find_library(METAL_LIBRARY Metal)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
    ${IOKIT_LIBRARY} 
    ${COREFOUNDATION_LIBRARY}
    ${GAMECONTROLLER_LIBRARY}
    ${COREAUDIO_LIBRARY}
    ${AUDIOTOOLBOX_LIBRARY}
    ${FORCEFEEDBACK_LIBRARY}
    ${COREVIDEO_LIBRARY}
    ${COCOA_LIBRARY}
    ${CARBON_LIBRARY}
    ${METAL_LIBRARY}
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
