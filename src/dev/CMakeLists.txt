add_executable(mesh-test mesh_test.cpp)
target_link_libraries(mesh-test mesh)

add_executable(mathpresso-test mathpresso_test.cpp)
target_link_libraries(mathpresso-test mathpresso)
