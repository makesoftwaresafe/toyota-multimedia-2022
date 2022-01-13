ADD_DEFINITIONS(
	-DWEBP_DLL
)

# TODO: cpufeatures path
include_directories(
	/opt/android/android-ndk-r10d/sources/android/cpufeatures
)

LIST(APPEND LIBWEBP_SOURCES
	/opt/android/android-ndk-r10d/sources/android/cpufeatures/cpu-features.c
)

ADD_LIBRARY(${LIBWEBP_LIBRARY_NAME} SHARED ${LIBWEBP_HEADERS} ${LIBWEBP_SOURCES})

TARGET_LINK_LIBRARIES(${LIBWEBP_LIBRARY_NAME}
	m
)

