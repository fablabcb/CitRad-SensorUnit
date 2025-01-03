add_executable(utest_FiFo utest_FiFo.cpp)

include_directories(../../../shared)

target_link_libraries(utest_FiFo
    PUBLIC boost_unit_test_framework
)

# run instructions: go to build directory and run:
#   ./test_SimpleClass --logger=JUNIT
# this will create a SimpleClass.xml with the results
