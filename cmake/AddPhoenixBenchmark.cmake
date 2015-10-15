macro(AddPhoenixBenchmark benchmark)
  set(multiValueArgs DEFINITIONS RENAME)
  cmake_parse_arguments(bench
    ""
    ""
    "ARGS;DEFINITIONS;FILES"
    ${ARGN})

  foreach(type pthread tthread)
    set(target ${benchmark}-${type})
    add_executable(${target} ${bench_FILES})

    set(src ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(${target} PROPERTIES
      COMPILE_FLAGS "-march=native -mtune=native -O3 -pipe"
      COMPILE_DEFINITIONS "${bench_DEFINITIONS}"
      INCLUDE_DIRECTORIES "${PHOENIX_INCLUDE};${src};${bench_INCLUDES}"
      COMPILE_FEATURES cxx_variadic_macros cxx_static_assert cxx_auto_type
      LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT};phoenix")

    add_custom_target(bench-${benchmark}-${type}
      COMMAND ${CMAKE_COMMAND} -E
      time ./${benchmark}-${type} ${bench_ARGS})
  endforeach()
endmacro(AddPhoenixBenchmark)
