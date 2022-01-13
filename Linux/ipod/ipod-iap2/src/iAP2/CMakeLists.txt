include_directories(
	link/iAP2Link
	link/iAP2Utility
	link/iAP2UtilityImplementation
)

add_subdirectory( link )
add_subdirectory( session )
add_subdirectory( transport )
add_subdirectory( interface )