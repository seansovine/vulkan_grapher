add_executable(mesh-test mesh_test.cpp)
target_link_libraries(mesh-test mesh)

add_executable(mathpresso-test mathpresso_test.cpp)
target_link_libraries(mathpresso-test mathpresso mesh)

add_executable(glm-test glm_test.cpp)
target_link_libraries(glm-test mesh glm::glm)

add_executable(iceoryx2-test iceoryx2_test.cpp)
target_link_libraries(iceoryx2-test PRIVATE iceoryx2-cxx::shared-lib-cxx)
