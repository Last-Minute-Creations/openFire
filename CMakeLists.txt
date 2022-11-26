cmake_minimum_required(VERSION 3.14.0)
project(openFire LANGUAGES C)

if(NOT AMIGA)
	message(SEND_ERROR "This project only compiles for Amiga")
endif()

# ACE
add_subdirectory(deps/ace ace)

set(CMAKE_C_STANDARD 11)
file(GLOB_RECURSE SOURCES src/*.c src/*.h)

if(ELF2HUNK)
	set(GAME_EXECUTABLE openFire.elf)
	set(GAME_OUTPUT_EXECUTABLE openFire.exe)
	add_executable(${GAME_EXECUTABLE} ${SOURCES})
	target_link_libraries(${GAME_EXECUTABLE} -Wl,-Map=openFire.map)

	if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug") AND NOT (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
		set(ELF2HUNK_ARGS ${ELF2HUNK_ARGS} -s)
	endif()

	add_custom_command(
		TARGET ${GAME_EXECUTABLE} POST_BUILD
		COMMAND ${ELF2HUNK} ${GAME_EXECUTABLE} ${GAME_OUTPUT_EXECUTABLE} ${ELF2HUNK_ARGS}
	)
	add_custom_command(
		TARGET ${GAME_EXECUTABLE} POST_BUILD
		COMMAND ${OBJDUMP} --disassemble -S ${GAME_EXECUTABLE} > openFire.s
	)
else()
	SET(GAME_EXECUTABLE openFire)
	SET(GAME_OUTPUT_EXECUTABLE openFire)
	add_executable(${GAME_EXECUTABLE} ${SOURCES})
endif()

# Version stuff
string(TIMESTAMP YEAR "%y")
string(TIMESTAMP DAY "%d")
string(TIMESTAMP MONTH "%m")
MATH(EXPR VER_MAJOR "0 + ${YEAR}")
MATH(EXPR VER_MINOR "0 + ${MONTH}")
MATH(EXPR VER_FIX "0 + ${DAY}")
set(VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_FIX}")
target_compile_definitions(${GAME_EXECUTABLE} PRIVATE GAME_VERSION="${VERSION}")

target_include_directories(${GAME_EXECUTABLE} PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(${GAME_EXECUTABLE} PUBLIC -Wall)
target_link_libraries(${GAME_EXECUTABLE} ace)
if(GAME_DEBUG)
	target_compile_definitions(${GAME_EXECUTABLE} PRIVATE GAME_DEBUG)
	target_compile_definitions(ace PUBLIC ACE_DEBUG_UAE)
endif()

set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(WARRIOR_DIR ${GEN_DIR}/warrior)
set(THUNDER_DIR ${GEN_DIR}/thunder)
file(MAKE_DIRECTORY ${DATA_DIR} ${GEN_DIR} ${WARRIOR_DIR} ${THUNDER_DIR})

file(COPY "data" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Generating ZIP
set(GAME_PACKAGE_NAME "${CMAKE_PROJECT_NAME} ${VER_MAJOR}_${VER_MINOR}_${VER_FIX}")
add_custom_target(generateZip COMMAND
	${CMAKE_COMMAND} -E tar "cf" "${GAME_PACKAGE_NAME}.zip" --format=zip
	"${CMAKE_CURRENT_BINARY_DIR}/${GAME_OUTPUT_EXECUTABLE}" "${DATA_DIR}"
	COMMENT "Generating ZIP file ${GAME_PACKAGE_NAME}.zip"
)

# Generating ADF
set(ADF_DIR "${CMAKE_CURRENT_BINARY_DIR}/adf")
add_custom_target(generateAdf
	COMMAND ${CMAKE_COMMAND} -E make_directory "${ADF_DIR}/s"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${GAME_OUTPUT_EXECUTABLE}" "${ADF_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${DATA_DIR}" "${ADF_DIR}/data"
	# COMMAND ${CMAKE_COMMAND} -E echo "c:add21k" > "${ADF_DIR}/s/startup-sequence"
	COMMAND ${CMAKE_COMMAND} -E echo "${GAME_OUTPUT_EXECUTABLE}" > "${ADF_DIR}/s/startup-sequence"
	COMMAND exe2adf -l ${CMAKE_PROJECT_NAME} -a "${GAME_PACKAGE_NAME}.adf" -d ${ADF_DIR}
	COMMAND ${CMAKE_COMMAND} -E rm -rf "${ADF_DIR}"
	COMMENT "Generating ADF file ${GAME_PACKAGE_NAME}.adf"
)
