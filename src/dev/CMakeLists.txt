add_executable(mesh-test mesh_test.cpp)
target_link_libraries(mesh-test mesh)

add_executable(mathpresso-test mathpresso_test.cpp)
target_link_libraries(mathpresso-test mathpresso)

add_executable(glm-test glm_test.cpp)
target_link_libraries(glm-test glm::glm)
