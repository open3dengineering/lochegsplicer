CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(APP_NAME LocheGSplicer)
PROJECT(${APP_NAME})

IF (WIN32)
   SET(CMAKE_DEBUG_POSTFIX  "d")
ENDIF (WIN32)

IF(APPLE)
   FIND_LIBRARY(CARBON_LIBRARY Carbon)
   FIND_LIBRARY(APPSERVICES_LIBRARY ApplicationServices)
   FIND_LIBRARY(IOKIT_LIBRARY IOKit)
   FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
   FIND_LIBRARY(AGL_LIBRARY AGL)
   FIND_LIBRARY(COCOA_LIBRARY Cocoa)

   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnewline-eof -pipe")

   SET(CMAKE_C_FLAGS "-pipe -Wnewline-eof")

   if (NOT CMAKE_OSX_ARCHITECTURES)
     set(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Build architectures for OSX" FORCE)
   endif()

   if (NOT CMAKE_OSX_SYSROOT)
     set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.5.sdk" CACHE STRING "Build SDK root" FORCE )
   endif()

   if (NOT CMAKE_OSX_DEPLOYMENT_TARGET)
     # use 10.5 because osg atomics won't build on less.
     set(CMAKE_OSX_DEPLOYMENT_TARGET "10.5" CACHE STRING "Build SDK root" FORCE)
   endif()

   SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-single_module -compatibility_version 3 -current_version 3" )
   SET(CMAKE_CXX_FLAGS_DEBUG "-gdwarf-2")
   SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -gdwarf-2")
   SET(CMAKE_C_FLAGS_DEBUG "-gdwarf-2")
   SET(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -gdwarf-2")
   INCLUDE_DIRECTORIES(/usr/include/malloc)
ELSE (APPLE)
   #We only want X11 if we are not running on OSX, but still with a unix-like environment
   IF(UNIX)
      FIND_PACKAGE(X11)
      FIND_LIBRARY(XXF86VM_LIBRARY Xxf86vm)
      SET(X11_LIBRARIES
          ${X11_LIBRARIES}
          ${XXF86VM_LIBRARY})
   ENDIF(UNIX)
ENDIF(APPLE)

IF(UNIX)
   # Some Unicies need explicit linkage to the Math library or the build fails.
   FIND_LIBRARY(MATH_LIBRARY m)

   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-non-virtual-dtor -Wreturn-type")
ENDIF (UNIX)

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(Qt4    REQUIRED)

SET(QT_USE_QTOPENGL "true")

SET(HEADER_PATH ${CMAKE_SOURCE_DIR}/inc)
SET(SOURCE_PATH ${CMAKE_SOURCE_DIR}/src)

SET(OUTPUT_BINDIR ${PROJECT_BINARY_DIR}/bin)
MAKE_DIRECTORY(${OUTPUT_BINDIR})

SET(OUTPUT_LIBDIR ${PROJECT_BINARY_DIR}/lib)
MAKE_DIRECTORY(${OUTPUT_LIBDIR})

# Project files
SET(HEADER_FILES
   ${HEADER_PATH}/GCodeObject.h
   ${HEADER_PATH}/GCodeParser.h
   ${HEADER_PATH}/qtlogo.h
   ${HEADER_PATH}/VisualizerView.h
   ${HEADER_PATH}/Window.h
)

SET(SOURCE_FILES
   ${SOURCE_PATH}/GCodeObject.cpp
   ${SOURCE_PATH}/GCodeParser.cpp
   ${SOURCE_PATH}/Main.cpp
   ${SOURCE_PATH}/qtlogo.cpp
   ${SOURCE_PATH}/VisualizerView.cpp
   ${SOURCE_PATH}/Window.cpp
)

QT4_WRAP_CPP(MOC_SOURCES ${HEADER_FILES})

SOURCE_GROUP("Auto-Generated" FILES ${MOC_SOURCES})

ADD_EXECUTABLE(${APP_NAME}
    ${HEADER_FILES}
    ${SOURCE_FILES}
	${MOC_SOURCES}
)

# Make the required external dependency headers visible to everything
INCLUDE_DIRECTORIES(
   ${CMAKE_SOURCE_DIR}/inc
   ${OPENGL_INCLUDE_DIR}
   ${QT_INCLUDE_DIR}
   ${QT_QTCORE_INCLUDE_DIR}
   ${QT_QTGUI_INCLUDE_DIR}
   ${QT_QTOPENGL_INCLUDE_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES( ${APP_NAME}
                       ${OPENGL_LIBRARY}
                       ${QT_QTCORE_LIBRARY}
                       ${QT_QTGUI_LIBRARY}
                       ${QT_QTOPENGL_LIBRARY}
)
