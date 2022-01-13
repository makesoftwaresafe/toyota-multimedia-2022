#######
# Dlt - Diagnostic Log and Trace
# @licence make begin@
#
# Copyright (C) 2011-2012, BMW AG - Alexander Wenzel <alexander.aw.wenzel@bmw.de>
# 
# Contributions are licensed to the GENIVI Alliance under one or more
# Contribution License Agreements.
# 
# This Source Code Form is subject to the terms of the
# Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed with
# this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# @licence end@
########

if(WITH_SMOKETEST)
	set(SMOKETEST_FILES_DIR "/opt/platform" )
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/smoketest/ DESTINATION ${SMOKETEST_FILES_DIR} FILES_MATCHING PATTERN "*.sh" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE  WORLD_READ WORLD_EXECUTE PATTERN "CMakeFiles" EXCLUDE )
	message(STATUS "smoke tests will be installed to ${SMOKETEST_FILES_DIR} after make install" )


# install checkers for user group settings
set(CHECKER_CONFIG_DIR "/opt/platform/user_group_checkers" )
install(FILES ${CMAKE_SOURCE_DIR}/../meta-adit/meta-adit-oss-userland/recipes/dlt/files/dlt-service-checker.sh
DESTINATION ${CHECKER_CONFIG_DIR} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ GROUP_EXECUTE )
install(FILES ${CMAKE_SOURCE_DIR}/../meta-adit/meta-adit-oss-userland/recipes/dlt/files/dlt-process-checker.sh
DESTINATION ${CHECKER_CONFIG_DIR} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ GROUP_EXECUTE )


endif(WITH_SMOKETEST)






















