set(LIBRARY_SOURCES 
    tale/tale.hpp 
    tale/tale.cpp 
    tale/school.hpp
    tale/school.cpp
    tale/course.hpp
    tale/course.cpp
    tale/interactionstore.hpp
    tale/interactionstore.cpp
    shared/actor.hpp
    shared/actor.cpp
    shared/setting.hpp
    shared/tattletalecore.hpp
    shared/kernels/kernel.hpp
    shared/kernels/kernel.cpp
    shared/kernels/resourcekernels/resource.hpp
    shared/kernels/resourcekernels/resource.cpp
    shared/kernels/resourcekernels/emotion.hpp
    shared/kernels/resourcekernels/emotion.cpp
    shared/kernels/resourcekernels/relationship.hpp
    shared/kernels/resourcekernels/relationship.cpp
    shared/kernels/goal.hpp
    shared/kernels/goal.cpp
    shared/kernels/interactions/interaction.hpp
    shared/kernels/interactions/interaction.cpp
    shared/kernels/interactions/interactionprototype.hpp
    shared/kernels/interactions/interactionrequirement.hpp
    shared/kernels/interactions/interactioncontexttype.hpp
    shared/kernels/interactions/interactiontendency.hpp
    shared/random.hpp
    shared/random.cpp
    shared/chronicle.hpp
    shared/chronicle.cpp
    tattle/tattle.hpp 
    tattle/tattle.cpp
    tattle/curator.hpp
    tattle/curator.cpp
    tattle/curations/curation.hpp
    tattle/curations/raritycuration.hpp
    tattle/curations/raritycuration.cpp
    tattle/curations/absoluteinterestcuration.hpp
    tattle/curations/absoluteinterestcuration.cpp
    tattle/curations/tagcuration.hpp
    tattle/curations/tagcuration.cpp
    tattle/curations/catcuration.hpp
    tattle/curations/catcuration.cpp
    tattle/curations/randomcuration.hpp
    tattle/curations/randomcuration.cpp
)

set(ADDITIONAL_EXECTUABLE_SOURCES 
    main.cpp
)

include(FetchContent)

FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(rang
  GIT_REPOSITORY https://github.com/agauniyal/rang.git
  GIT_TAG master
)
FetchContent_MakeAvailable(rang)

FetchContent_Declare(robin_hood
  GIT_REPOSITORY https://github.com/martinus/robin-hood-hashing.git
  GIT_TAG master
)
FetchContent_MakeAvailable(robin_hood)


add_library(${LIBRARY_NAME} STATIC ${LIBRARY_SOURCES})


target_link_libraries(${LIBRARY_NAME} nlohmann_json::nlohmann_json fmt::fmt rang robin_hood)

add_executable(${EXECUTABLE_NAME} ${ADDITIONAL_EXECTUABLE_SOURCES})
# Uncomment/comment out for prints
target_compile_definitions(${LIBRARY_NAME} PUBLIC TATTLETALE_PROGRESS_PRINT_OUTPUT)
#target_compile_definitions(${LIBRARY_NAME} PUBLIC TATTLETALE_DEBUG_PRINT_OUTPUT)
target_compile_definitions(${LIBRARY_NAME} PUBLIC TATTLETALE_ERROR_PRINT_OUTPUT)
#target_compile_definitions(${LIBRARY_NAME} PUBLIC TATTLETALE_VERBOSE_PRINT_OUTPUT)

target_compile_definitions(${LIBRARY_NAME} PUBLIC NOMINMAX)


add_custom_command(
        TARGET ${EXECUTABLE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/resources
                $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/resources)

target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME})
