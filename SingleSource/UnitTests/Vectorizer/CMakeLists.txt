llvm_singlesource()
set_property(TARGET runtime-checks PROPERTY CXX_STANDARD 17)
set_property(TARGET early-exit PROPERTY CXX_STANDARD 17)

# Enable epilogue vectorization to test resume value correctness.
# The -mllvm flag is Clang-specific.
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  target_compile_options(epilog-vectorization-reductions PRIVATE
    -mllvm -epilogue-vectorization-minimum-VF=1)
  target_compile_options(epilog-vectorization-narrow-interleave PRIVATE
    -mllvm -epilogue-vectorization-minimum-VF=1)
endif()

# The VPlan-native path is specific to llvm.
if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
  add_subdirectory(VPlanNativePath)
endif()
