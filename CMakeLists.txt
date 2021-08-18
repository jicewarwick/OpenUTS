cmake_minimum_required(VERSION 3.19)

project(
	OpenUTS
	LANGUAGES CXX
	VERSION 0.1.0
	DESCRIPTION "Unified Trading System"
)

# compiler settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_LINK_WHAT_YOU_USE TRUE)
# set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE include-what-you-use)
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy")

# testing options
option(BUILD_TESTING "enable testing" ON)
option(INSTALL_TESTING "install testing" OFF)

# dependencies
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(spdlog REQUIRED)
find_package(nlohmann_json 3.9.0 REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(CTP REQUIRED)
find_package(mariadbcpp QUIET)

# project code
configure_file(include/uts/version.h.in include/uts/version.h @ONLY)
add_subdirectory(src)
add_subdirectory(apps)
if(BUILD_TESTING)
	enable_testing()
	add_subdirectory(tests)
endif()

# installation
include(GNUInstallDirs)
install(
	DIRECTORY ./include/uts
	DESTINATION include
	FILES_MATCHING
	PATTERN "*.h"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/uts/version.h DESTINATION include/uts)

install(
	EXPORT ${PROJECT_NAME}Targets
	FILE ${PROJECT_NAME}Targets.cmake
	NAMESPACE ${PROJECT_NAME}::
	DESTINATION lib/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(Config.cmake.in ${PROJECT_NAME}Config.cmake INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})
write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake COMPATIBILITY AnyNewerVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
			  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" DESTINATION lib/cmake/${PROJECT_NAME}
)

export(
	EXPORT ${PROJECT_NAME}Targets
	FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Targets.cmake"
	NAMESPACE ${PROJECT_NAME}::
)

# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	set(DOXYGEN_EXCLUDE build/* tests/*)

	set(DOXYGEN_SOURCE_BROWSER YES)
	set(DOXYGEN_EXTRACT_PRIVATE NO)
	set(DOXYGEN_EXTRACT_LOCAL_CLASSES NO)
	set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/docs")

	doxygen_add_docs(docs ${PROJECT_SOURCE_DIR} COMMENT "Generate API-documents for ${PROJECT_NAME}.")
endif(DOXYGEN_FOUND)

# CPack
set(CPACK_PACKAGE_VENDOR "Ce Ji")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "UnifiedTradingSystem")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES /.git .idea/ .vs/ CMakeLists.txt.* CMakeSettings.json ./build/)

include(CPack)
