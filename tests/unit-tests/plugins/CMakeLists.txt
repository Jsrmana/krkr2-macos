cmake_minimum_required(VERSION 3.16)
project(TestPlugins LANGUAGES CXX)

set(SOURCES
        shrinkCopy-test.cpp
)

# Asset-dependent tests are optional in source distributions that exclude
# third-party image and motion fixtures.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/psbfile-dll.cpp" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../test_files/emote/ezsave.pimg" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../test_files/emote/e-mote3.0バニラパジャマa.psb")
    list(APPEND SOURCES psbfile-dll.cpp)
endif()

string(REPLACE ".cpp" "" BASENAMES_SOURCES "${SOURCES}")
set(TARGETS ${BASENAMES_SOURCES})

foreach(name ${TARGETS})
    add_executable(${name} ${name}.cpp main.cpp)
endforeach()

set(ALL_TARGETS
        ${TARGETS}
)

foreach(name ${ALL_TARGETS})
    target_link_libraries(${name}
        PRIVATE
            Catch2::Catch2
        PUBLIC
            krkr2plugin krkr2core
    )
    target_include_directories(${name} PRIVATE "${TEST_CONFIG_DIR}")
    catch_discover_tests(${name})
endforeach()
