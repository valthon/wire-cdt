SET(SRC_FILENAMES algorithm.cpp any.cpp bind.cpp charconv.cpp chrono.cpp condition_variable.cpp functional.cpp
                  future.cpp hash.cpp ios.cpp iostream.cpp locale.cpp memory.cpp mutex.cpp new.cpp optional.cpp
                  regex.cpp stdexcept.cpp string.cpp strstream.cpp system_error.cpp exception.cpp
                  typeinfo.cpp utility.cpp valarray.cpp variant.cpp vector.cpp sysio.cpp)

SET(SRC_FILES "")
FOREACH(FN ${SRC_FILENAMES})
   LIST(APPEND SRC_FILES "cdt-libcxx/src/${FN}")
ENDFOREACH(FN)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-everything")

add_library(c++
            ${SRC_FILES})


target_include_directories(c++
                           PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}/cdt-libcxx/include
                           ${CMAKE_SOURCE_DIR}/libc/cdt-musl/include
                           ${CMAKE_SOURCE_DIR}/libc/cdt-musl/src/internal
                           ${CMAKE_SOURCE_DIR}/libc/cdt-musl/src/crypt
                           ${CMAKE_SOURCE_DIR}/libc/cdt-musl/arch/eos)

target_link_libraries(c++ c)
target_compile_definitions(c++ PUBLIC -D_LIBCPP_BUILDING_LIBRARY)
add_custom_command( TARGET c++ POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:c++> ${BASE_BINARY_DIR}/lib )

if (ENABLE_NATIVE_COMPILER)
   add_native_library(native_c++
               ${SRC_FILES})

   target_include_directories(native_c++
                              PUBLIC
                              ${CMAKE_CURRENT_SOURCE_DIR}/cdt-libcxx/include
                              ${CMAKE_SOURCE_DIR}/libc/cdt-musl/include
                              ${CMAKE_SOURCE_DIR}/libc/cdt-musl/src/internal
                              ${CMAKE_SOURCE_DIR}/libc/cdt-musl/src/crypt
                              ${CMAKE_SOURCE_DIR}/libc/cdt-musl/arch/eos)

   target_link_libraries(native_c++ native_c)
   target_compile_definitions(native_c++ PUBLIC -D_LIBCPP_BUILDING_LIBRARY)
   target_compile_options(native_c++ PUBLIC -allow-sse)
   add_custom_command( TARGET native_c++ POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:native_c++> ${BASE_BINARY_DIR}/lib )
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cdt-libcxx/include/ DESTINATION ${BASE_BINARY_DIR}/include/libcxx)
