idf_build_get_property(target IDF_TARGET)


set(srcs            app_main.cpp)

#set(src_dirs        .)

set(include_dirs    ../../esp-dl/include
                    ../../esp-dl/include/tool
                    ../../esp-dl/include/nn
                    ../../esp-dl/include/layer
                    ../../esp-dl/include/image
                    ../../esp-dl/include/typedef
                    ../../esp-dl/include/detect
                    ../../esp-dl/include/model_zoo)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

set(lib     libhuman_face_detect.a
            libdl.a)

if(${IDF_TARGET} STREQUAL "esp32")
    set(links   "-L ${CMAKE_CURRENT_SOURCE_DIR}/../../esp-dl/lib/esp32")

elseif(${IDF_TARGET} STREQUAL "esp32s2")
    set(links   "-L ${CMAKE_CURRENT_SOURCE_DIR}/../../esp-dl/lib/esp32s2")

elseif(${IDF_TARGET} STREQUAL "esp32s3")
    set(links   "-L ${CMAKE_CURRENT_SOURCE_DIR}/../../esp-dl/lib/esp32s3")

elseif(${IDF_TARGET} STREQUAL "esp32c3")    
    set(links   "-L ${CMAKE_CURRENT_SOURCE_DIR}/../../esp-dl/lib/esp32c3")

endif()


target_link_libraries(${COMPONENT_TARGET} ${links})
target_link_libraries(${COMPONENT_TARGET} ${lib})