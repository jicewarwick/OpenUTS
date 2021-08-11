# Try to find CTP
# Once done, the following will be defined
#
# CTP_FOUND        		- system has CTP
# CTP_INCLUDE_DIRS		- CTP include directories
# CTP_QUOTE_LIBRARIES	- quote libraries needed to use CTP
# CTP_TRADE_LIBRARIES	- trade libraries needed to use CTP

if(NOT CTP_FOUND)
	find_path(
		CTP_INCLUDE_DIRS CTP/ThostFtdcTraderApi.h
		HINTS ${CTP_ROOT_DIR} "/usr/local"
		DOC "CTP header files"
		PATH_SUFFIXES include
	)

	find_library(
		CTP_TRADE_LIBRARIES thosttraderapi_se
		HINTS ${CTP_ROOT_DIR} "/usr/local"
		DOC "CTP trader library"
		PATH_SUFFIXES ${CMAKE_INSTALL_LIBDIR}
	)

	find_library(
		CTP_QUOTE_LIBRARIES thostmduserapi_se
		DOC "CTP market data library"
		HINTS ${CTP_ROOT_DIR} "/usr/local"
		PATH_SUFFIXES ${CMAKE_INSTALL_LIBDIR}
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(CTP DEFAULT_MSG CTP_INCLUDE_DIRS CTP_TRADE_LIBRARIES CTP_QUOTE_LIBRARIES)
	mark_as_advanced(CTP_INCLUDE_DIRS CTP_TRADE_LIBRARIES CTP_QUOTE_LIBRARIES)

	# trader api
	add_library(CTP::CTPTraderAPI SHARED IMPORTED)
	set_target_properties(
		CTP::CTPTraderAPI
		PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
				   IMPORTED_IMPLIB "${CTP_TRADE_LIBRARIES}"
				   IMPORTED_LOCATION "${CTP_TRADE_LIBRARIES}"
				   INTERFACE_INCLUDE_DIRECTORIES "${CTP_INCLUDE_DIRS}"
	)

	# market data api
	add_library(CTP::CTPMarketDataAPI SHARED IMPORTED)
	set_target_properties(
		CTP::CTPMarketDataAPI
		PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
				   IMPORTED_IMPLIB "${CTP_QUOTE_LIBRARIES}"
				   IMPORTED_LOCATION "${CTP_QUOTE_LIBRARIES}"
				   INTERFACE_INCLUDE_DIRECTORIES "${CTP_INCLUDE_DIRS}"
	)

endif()
