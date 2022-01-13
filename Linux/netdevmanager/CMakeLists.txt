################################################################################
# CMakeLists.txt for Executable
#  see:  http://www.cmake.org/cmake/help/documentation.html
#
# 2012.09.21  新規作成                                                NTTD-MSE
# 2013.11.29  標準単体テスト環境対応                                  NTTD-MSE
################################################################################
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

# [for unittest CHANGE]
project(netdev_manager CXX C)

# cmake定義ファイル格納先パス定義
set(BUILDENVPATH CACHE STRING "BUILDTOOLS/env")

# モデル共通定義include
include("${BUILDENVPATH}/denso_common_definition.cmake")

# [for unittest ADD]
# 標準単体テスト環境セットアップinclude
# FIXME include("${BUILDENVPATH}/denso_unittest_setup.cmake")

# ビルド対象ファイル一覧作成
file(GLOB SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

# 依存ヘッダ一覧作成(ヘッダ変更時のmake向け)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

# includeパス設定
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../deliverables
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

# set(LIBDEF
# 	# 汎用ライブラリ librt.so
# 	rt
# 	# ローカルライブラリ
# 	sample_sharedlib
# 	sample_sharedlib_with_static
# )

# set(ARCHIVES
# 	sample_staticlib
# )

# ライブラリサーチパス設定
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../deliverables/lib ${CMAKE_CURRENT_SOURCE_DIR}/../deliverables/stalib/nopic)

# ターゲットを追加
add_executable(${PROJECT_NAME} ${SOURCE})

# # ヘッダーを依存させる。
# add_dependencies(${PROJECT_NAME} ${HEADERS}) 

# コンパイルオプション指定
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${CFLAGS})

# リンクオプション指定
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LDFLAGS} ${DENSO_CMAKE_RPATH_OPTION}")

# # 共有ライブラリリンク
# target_link_libraries(${PROJECT_NAME} ${LIBDEF})

# # スタティックライブラリリンク
# target_link_libraries(${PROJECT_NAME} ${ARCHIVES})

# オブジェクトインストール先設定 
install(TARGETS ${PROJECT_NAME}
		DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../deliverables/bin"
		)

# [for unittest ADD]
# main関数がある開発コード側のソースファイルパスと、疑似main関数があるテストコード側のソースファイル名を設定
# 後者は未指定(空文字設定)でも可
# FIXME denso_setup_fakemain(
# FIXME 	"${CMAKE_CURRENT_SOURCE_DIR}/src/sample_main.c"
# FIXME	"")

# [for unittest ADD]
# 単体テストセットアップ
# FIXME denso_setup_unittest( ${PROJECT_NAME} unittest "${LIBDEF}" ${SOURCE})
