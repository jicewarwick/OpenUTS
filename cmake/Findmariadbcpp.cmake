# Try to find mariadb-connector-cpp
# Once done, the following will be defined
# WARNING: for windows user, library MUST be compiled in release mode.
#
# MARIADBCPP_FOUND                - system has mariadb-connector-cpp
# MARIADBCPP_INCLUDE_DIRS         - mariadb-connector-cpp include directories
# MARIADBCPP_LIBRARIES            - mariadb-connector-cpp library
# mariadb::mariadbcpp             - CMake target

if(NOT MARIADBCPP_FOUND)
	find_path(
		MARIADBCPP_INCLUDE_DIRS mariadb/conncpp.hpp
		HINTS ${MARIADBCPP_ROOT_DIR} "/usr/local" "C:Program Files/MariaDB/MariaDB C++ Connector 64-bit/"
			  "C:Program Files/MariaDB/MariaDB C++ Connector 32-bit/"
		DOC "mariadbcpp header files"
		PATH_SUFFIXES include
	)

	find_library(
		MARIADBCPP_LIBRARIES mariadbcpp
		HINTS "/usr/lib/mariadb/" "C:Program Files/MariaDB/MariaDB C++ Connector 64-bit/"
			  "C:Program Files/MariaDB/MariaDB C++ Connector 32-bit/"
		DOC "mariadbcpp library"
		PATH_SUFFIXES ${CMAKE_INSTALL_LIBDIR}
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(mariadbcpp DEFAULT_MSG MARIADBCPP_INCLUDE_DIRS MARIADBCPP_LIBRARIES)
	mark_as_advanced(MARIADBCPP_INCLUDE_DIRS MARIADBCPP_LIBRARIES)

	# include(CMakePrintHelpers)
	# cmake_print_variables(MARIADBCPP_INCLUDE_DIRS MARIADBCPP_LIBRARIES)

	add_library(mariadb::mariadbcpp SHARED IMPORTED)
	set_target_properties(
		mariadb::mariadbcpp
		PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
				   IMPORTED_IMPLIB "${MARIADBCPP_LIBRARIES}"
				   IMPORTED_LOCATION "${MARIADBCPP_LIBRARIES}"
				   INTERFACE_INCLUDE_DIRECTORIES "${MARIADBCPP_INCLUDE_DIRS}"
	)

endif()
