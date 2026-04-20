#pragma once

#include "vk3dObject.h"
#include "TypesData.h"

#include <cstdint>
#include <vector>

namespace vkObj {

/**
 * @brief Класс N-гранной пирамиды с возможностью смещения вершины.
 *        N-sided pyramid class with apex offset support.
 *
 * Пирамида состоит из:
 *  - N боковых треугольников (flat-shading, уникальные нормали).
 *  - N треугольников основания (веер из центра, нормаль вниз).
 *
 * The pyramid consists of:
 *  - N lateral triangles (flat-shaded, unique normals per face).
 *  - N base triangles (fan from centre, normal pointing down).
 *
 * Смещение вершины (apexOffsetX, apexOffsetZ) создаёт косую пирамиду.
 * Apex offset (apexOffsetX, apexOffsetZ) produces an oblique pyramid.
 *
 * @see Vk3DObject
 */
class Vk3dPyramid : public Vk3DObject {
public:
    /**
     * @brief Параметры формы пирамиды / Pyramid shape parameters.
     *
     * Все параметры влияют на геометрию меша и требуют перестройки
     * GPU-буферов при изменении.
     *
     * All parameters affect mesh geometry and require GPU buffer
     * rebuild when changed.
     */
    struct Params {
        int   sides = 4;    // Число боковых граней (≥ 3) / Number of lateral faces (≥ 3)
        float radius = 1.0f; // Радиус основания / Base radius
        float height = 1.5f; // Высота пирамиды / Pyramid height
        float apexOffsetX = 0.0f; // Смещение вершины по X / Apex X offset from base centre
        float apexOffsetZ = 0.0f; // Смещение вершины по Z / Apex Z offset from base centre
    };

    /** @brief Конструктор по умолчанию / Default constructor. */
    Vk3dPyramid() = default;

    /**
     * @brief Конструктор с начальными параметрами / Constructor with initial parameters.
     * @param params Начальные параметры формы / Initial shape parameters.
     */
    explicit Vk3dPyramid(const Params& pyramidParams);

    /** @brief Деструктор / Destructor. */
    ~Vk3dPyramid() override = default;

    // Параметры формы / Shape params

    /** @brief Получить параметры (изменяемые) / Get mutable parameters. */
    Params& getPyramidParams() noexcept { return pyramidParams; }

    /** @brief Получить параметры (константные) / Get const parameters. */
    const Params& getPyramidParams() const noexcept { return pyramidParams; }

    /** @brief Задать параметры / Set parameters. */
    void setParams(const Params& p) { pyramidParams = p; }

    // Генерация меша / Mesh generation

    /**
     * @brief Сгенерировать меш пирамиды / Generate pyramid mesh.
     *
     * Боковые грани: плоские нормали (flat shading), по 3 уникальных вершины
     * на каждый треугольник.
     * Lateral faces: flat normals, 3 unique vertices per triangle.
     *
     * Основание: веер треугольников из центра, нормаль (0, -1, 0).
     * Base: triangle fan from centre, normal (0, -1, 0).
     *
     * Порядок обхода / Winding order:
     *  - Боковые: apex → base[i+1] → base[i] (CCW снаружи / CCW from outside)
     *  - Основание: centre → base[i] → base[i+1] (CCW снизу / CCW from below)
     *
     * @param p    Параметры формы / Shape parameters.
     * @param outV Выходной буфер вершин / Output vertex buffer.
     * @param outI Выходной буфер индексов / Output index buffer.
     */
    static void makePyramid(const Params& p,
                            std::vector<typesData::Vertex>& outV,
                            std::vector<uint32_t>& outI);

private:
    Params pyramidParams; // Текущие параметры формы / Current shape parameters
};

} // namespace vkObj
