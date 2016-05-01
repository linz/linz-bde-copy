CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# allow more human readable "if then else" etc
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)

SET(APP_NAME "bde_copy")
PROJECT(${APP_NAME})

SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "2")
SET(CPACK_PACKAGE_VERSION_PATCH "1")

# setup install paths

MESSAGE(STATUS ${CMAKE_BUILD_TYPE})

IF (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND WIN32)
  SET(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/MSWin32/${CMAKE_BUILD_TYPE}" CACHE PATH "install prefix" FORCE)
ENDIF()

IF(MSVC)
  ADD_DEFINITIONS(/W4)
  ADD_DEFINITIONS(/wd4996)
ELSE (MSVC)
  ADD_DEFINITIONS(-Wall)
ENDIF (MSVC)

IF (WIN32)
  SET (DEFAULT_BIN_SUBDIR     .)
  SET (DEFAULT_SYSCONF_SUBDIR .)
  SET (DEFAULT_DATA_SUBDIR    .)
ELSE(WIN32)
  IF (UNIX)
    SET (DEFAULT_BIN_SUBDIR     bin)
    SET (DEFAULT_SYSCONF_SUBDIR etc/bde-copy)
    SET (DEFAULT_DATA_SUBDIR    share/bde-copy)
  ENDIF(UNIX)
ENDIF(WIN32)

SET (BINDIR      ${DEFAULT_BIN_SUBDIR}   CACHE STRING "Directory where executables will be installed")
SET (DATADIR     ${DEFAULT_DATA_SUBDIR}  CACHE STRING "Directory where programme data will be installed")
SET (SYSCONFDIR  ${DEFAULT_SYSCONF_SUBDIR} CACHE STRING "Programme config directory")

# set full paths for config header
SET (DATADIR_FULL   ${CMAKE_INSTALL_PREFIX}/${DATADIR})

# find SCM revision number

SET (REVISION_FOUND FALSE)
FIND_FILE(SVN_MARKER entries PATHS ${CMAKE_SOURCE_DIR}/.svn)
SET (REVISION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/bde_copy_revision.h)

IF (SVN_MARKER)
  # See if we have svn installed
  FIND_PROGRAM(SVNVERSION svnversion PATHS "C:/Program Files/Subversion/bin")
  IF(SVNVERSION)
    IF(MSVC)
      ADD_CUSTOM_COMMAND(
        OUTPUT ${REVISION_HEADER}
        COMMAND for /f usebackq %%a in "(`\"${SVNVERSION}\"`)" do echo \#define REVISION \"%%a\" >${REVISION_HEADER}
        MAIN_DEPENDENCY ${SVN_MARKER}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      )
    ELSE(MSVC)
      ADD_CUSTOM_COMMAND(
        OUTPUT ${REVISION_HEADER}
        COMMAND echo \\\#define REVISION \\\"`${SVNVERSION}`\\\" >${REVISION_HEADER}
        MAIN_DEPENDENCY ${SVN_MARKER}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      )
    ENDIF(MSVC)
	SET (REVISION_FOUND TRUE)
  ENDIF (SVNVERSION)
ENDIF (SVN_MARKER)

FIND_FILE(GIT_MARKER index PATHS ${CMAKE_SOURCE_DIR}/.git)

IF (NOT REVISION_FOUND AND GIT_MARKER)
  FIND_PROGRAM(GIT git PATHS "C:/Program Files/Git/bin")
  IF(GIT)
    IF(MSVC)
      ADD_CUSTOM_COMMAND(
        OUTPUT ${REVISION_HEADER}
        COMMAND for /f \"usebackq tokens=1\" %%a in "(`\"${GIT}\" log -n1 --oneline`)" do echo \#define REVISION \"%%a\" >${REVISION_HEADER}
        MAIN_DEPENDENCY ${GIT_MARKER}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      )
    ELSE(MSVC)
      ADD_CUSTOM_COMMAND(
        OUTPUT ${REVISION_HEADER}
        COMMAND echo \\\#define REVISION \\\"$$\( ${GIT} log -n1 --pretty=%h \)\\\" > ${REVISION_HEADER}
        MAIN_DEPENDENCY ${GIT_MARKER}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      )
    ENDIF(MSVC)
	SET (REVISION_FOUND TRUE)
  ENDIF(GIT)
ENDIF ()

IF (NOT REVISION_FOUND)
  FILE(WRITE ${REVISION_HEADER}  "#define REVISION \"unknown\"")
ENDIF()

ADD_CUSTOM_TARGET(scm_revision ALL DEPENDS ${REVISION_HEADER})

# create bde_copy_config.h
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/bde_copy_config.h.in ${CMAKE_BINARY_DIR}/bde_copy_config.h)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

# zlib
FIND_PACKAGE(ZLIB)

IF (NOT ZLIB_FOUND)
  MESSAGE (SEND_ERROR "zlib library could not be found")
ENDIF ()

MESSAGE(STATUS "${APP_NAME} configure parameters:")
MESSAGE(STATUS ZLIB VERSION=         ${ZLIB_VERSION_STRING})
MESSAGE(STATUS CMAKE_INSTALL_PREFIX= ${CMAKE_INSTALL_PREFIX})
MESSAGE(STATUS SYSCONFIGDIR=         ${CMAKE_INSTALL_PREFIX}/${SYSCONFDIR})
MESSAGE(STATUS BINDIR=               ${CMAKE_INSTALL_PREFIX}/${BINDIR})
MESSAGE(STATUS DATADIR=              ${CMAKE_INSTALL_PREFIX}/${DATADIR})

SET(BDE_COPY_SRCS
  src/bde_copy.cpp
  src/bde_copy_funcs.cpp
  src/bde_copy_utils.cpp
  src/bde_copy_gzip.cpp
)

SET(BDE_COPY_DATA
  conf/bde_copy.help
  conf/bde_copy.cfg
  conf/bde_copy.cfg.clean
  conf/bde_copy.cfg.mssql
  conf/bde_copy.cfg.pgis
  conf/bde_copy.cfg.utf8
  conf/bde_copy.cfg.detect_utf8
  conf/bde_copy.cfg.control
)

INCLUDE_DIRECTORIES(
  src
  ${ZLIB_INCLUDE_DIRS}
  )

ADD_EXECUTABLE(${APP_NAME} ${BDE_COPY_SRCS})

TARGET_LINK_LIBRARIES(${APP_NAME}
  ${ZLIB_LIBRARIES}
)

ADD_DEPENDENCIES(${APP_NAME} scm_revision) 

INSTALL(TARGETS ${APP_NAME} RUNTIME DESTINATION ${BINDIR})
INSTALL(FILES ${BDE_COPY_DATA} DESTINATION ${DATADIR})

IF (WIN32)
  FIND_FILE(ZLIB_BIN zlib1.dll
  	PATH
  	  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\GnuWin32\\Zlib;InstallPath]/bin"
  	   NO_DEFAULT_PATH
  )
  INSTALL(FILES ${ZLIB_BIN} DESTINATION ${BINDIR})
ENDIF(WIN32)

