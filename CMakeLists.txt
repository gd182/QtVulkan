cmake_minimum_required(VERSION 3.20)

project(QtVulkan VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Gui QuickControls2)
find_package(Vulkan REQUIRED)

# GLM — header-only
find_package(glm CONFIG QUIET)
if (NOT TARGET glm::glm)
    find_path(GLM_INCLUDE_FALLBACK glm/glm.hpp
        PATHS
            "D:/Programming/C++/vcpkg/installed/x64-windows/include"
            "C:/vcpkg/installed/x64-windows/include"
            "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
            "$ENV{VCPKG_ROOT}/installed/x64-mingw-dynamic/include"
    )
    if (GLM_INCLUDE_FALLBACK)
        message(STATUS "GLM found via path: ${GLM_INCLUDE_FALLBACK}")
        add_library(glm_iface INTERFACE)
        target_include_directories(glm_iface INTERFACE "${GLM_INCLUDE_FALLBACK}")
        add_library(glm::glm ALIAS glm_iface)
    else()
        message(FATAL_ERROR "GLM not found. Install with: vcpkg install glm")
    endif()
endif()

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appQtVulkan
    main.cpp
    src/VulkanCore.cpp   src/VulkanCore.h
    src/VulkanWindow.cpp src/VulkanWindow.h
    src/SceneController.cpp src/SceneController.h
    src/Camera.cpp       src/Camera.h
    src/vk3dSphere.cpp   src/vk3dSphere.h
    src/Physics.cpp      src/Physics.h
    src/TypesData.h
)

qt_add_qml_module(appQtVulkan
    URI QtVulkan
    QML_FILES
        Main.qml
)

set_target_properties(appQtVulkan PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appQtVulkan PRIVATE src)

target_link_libraries(appQtVulkan
    PRIVATE
        Qt6::Quick
        Qt6::Gui
        Qt6::QuickControls2
        Vulkan::Vulkan
        glm::glm
)

# ── Компиляция шейдеров / Compile shaders ─────────────────────────────────
set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/../Vulcan/shaders")
set(SHADER_OUT_DIR "${SHADER_SRC_DIR}/compiled")

if (Vulkan_GLSLC_EXECUTABLE)
    message(STATUS "glslc found: ${Vulkan_GLSLC_EXECUTABLE}")
    set(VERT_SPV "${SHADER_OUT_DIR}/sphere.vert.spv")
    set(FRAG_SPV "${SHADER_OUT_DIR}/sphere.frag.spv")

    add_custom_command(
        OUTPUT  "${VERT_SPV}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}"
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} --target-env=vulkan1.0 "${SHADER_SRC_DIR}/sphere.vert" -o "${VERT_SPV}"
        DEPENDS "${SHADER_SRC_DIR}/sphere.vert"
        COMMENT "Compiling sphere.vert -> sphere.vert.spv"
    )
    add_custom_command(
        OUTPUT  "${FRAG_SPV}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}"
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} --target-env=vulkan1.0 "${SHADER_SRC_DIR}/sphere.frag" -o "${FRAG_SPV}"
        DEPENDS "${SHADER_SRC_DIR}/sphere.frag"
        COMMENT "Compiling sphere.frag -> sphere.frag.spv"
    )
    add_custom_target(shaders DEPENDS "${VERT_SPV}" "${FRAG_SPV}")
    add_dependencies(appQtVulkan shaders)
else()
    message(WARNING "glslc not found — shaders must be compiled manually")
endif()

# ── Копировать скомпилированные шейдеры рядом с exe / Copy compiled shaders ──
add_custom_command(TARGET appQtVulkan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:appQtVulkan>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SHADER_OUT_DIR}"
            $<TARGET_FILE_DIR:appQtVulkan>/shaders
)

install(TARGETS appQtVulkan
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
