ADD_DEFINITIONS(
	-DWEBP_DLL
)

ADD_LIBRARY(${LIBWEBP_LIBRARY_NAME} SHARED ${LIBWEBP_HEADERS} ${LIBWEBP_SOURCES})
TARGET_LINK_LIBRARIES(${LIBWEBP_LIBRARY_NAME} m)

