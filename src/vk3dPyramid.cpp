#include "vk3dPyramid.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
static constexpr float kPi = 3.14159265358979323846f;
#else
static constexpr float kPi = static_cast<float>(M_PI);
#endif

// Конструктор / Constructor

/**
 * @brief Конструктор с начальными параметрами / Constructor with initial parameters.
 */
vkObj::Vk3dPyramid::Vk3dPyramid(const Params& pyramidParams)
    : pyramidParams(pyramidParams)
{}

// Генерация меша / Mesh generation

/**
 * @brief Сгенерировать меш N-гранной (возможно, косой) пирамиды.
 *        Generate mesh for an N-sided (possibly oblique) pyramid.
 *
 * Алгоритм / Algorithm:
 *
 * 1. Вычисляем позиции:
 *    apex = ( apexOffsetX, +height/2, apexOffsetZ )
 *    base[i] = ( radius*cos(2π*i/N), -height/2, radius*sin(2π*i/N) )
 *    base_centre = ( 0, -height/2, 0 )
 *
 * 2. Боковые грани (lateral, flat-shaded):
 *    Треугольник i: apex → base[i+1] → base[i]
 *    Нормаль: cross( base[i+1]-apex, base[i]-apex ), затем normalize
 *    Каждая грань — 3 уникальных вершины (нормали разные у всех трёх граней)
 *
 * 3. Основание (base):
 *    Треугольник i: base_centre → base[i] → base[(i+1)%N]
 *    Нормаль: (0, -1, 0).
 *    Вершины основания разделяются (одна нормаль для всех)
 *
 * 1. Compute positions:
 *    apex = ( apexOffsetX, +height/2, apexOffsetZ )
 *    base[i] = ( radius*cos(2π*i/N), -height/2, radius*sin(2π*i/N) )
 *    base_centre = ( 0, -height/2, 0 )
 *
 * 2. Lateral faces (flat-shaded):
 *    Triangle i: apex → base[i+1] → base[i]
 *    Normal = normalize( cross( base[i+1]-apex, base[i]-apex ) )
 *    3 unique vertices per face (normals differ across faces)
 *
 * 3. Base:
 *    Triangle i: base_centre → base[i] → base[(i+1)%N]
 *    Normal = (0, -1, 0)
 *    Base vertices are shared (one normal for all)
 */
void vkObj::Vk3dPyramid::makePyramid(const Params& p,
                                      std::vector<typesData::Vertex>& outV,
                                      std::vector<uint32_t>& outI)
{
    outV.clear();
    outI.clear();

    // Гарантируем минимум 3 грани / Ensure at least 3 faces
    const int N = std::max(p.sides, 3);

    const float halfH = p.heightValue * 0.5f;
    const float step  = 2.0f * kPi / static_cast<float>(N);

    // Ключевые позиции / Key positions
    const float axV[3] = { p.apexOffsetX, halfH, p.apexOffsetZ }; // вершина / apex
    const float bcV[3] = { 0.0f, -halfH, 0.0f}; // центр основания / base centre

    // Предварительно вычисляем base-вершины в XZ-плоскости
    // Pre-compute base vertices in the XZ plane
    //   base[i] = ( radius*cos(i*step), -halfH, radius*sin(i*step) )

    // =========================================================
    // 1. Боковые грани (flat-shading) / Lateral faces (flat-shading)
    // =========================================================
    // Индексы 0 .. 3N-1 (по 3 вершины на грань)
    // Indices 0 .. 3N-1 (3 vertices per face)
    for (int i = 0; i < N; ++i) {
        const float a0 = static_cast<float>(i)     * step;
        const float a1 = static_cast<float>(i + 1) * step;

        const float b0[3] = { p.radius * std::cos(a0), -halfH, p.radius * std::sin(a0) };
        const float b1[3] = { p.radius * std::cos(a1), -halfH, p.radius * std::sin(a1) };

        // Вектора для вычисления нормали грани / Edge vectors for face normal
        // a = base[i+1] - apex,  b = base[i] - apex
        const float ea[3] = { b1[0] - axV[0], b1[1] - axV[1], b1[2] - axV[2] };
        const float eb[3] = { b0[0] - axV[0], b0[1] - axV[1], b0[2] - axV[2] };

        // Нормаль: cross(ea, eb) — CCW снаружи → нормаль наружу
        // Normal: cross(ea, eb) — CCW from outside → outward normal
        float nx = ea[1] * eb[2] - ea[2] * eb[1];
        float ny = ea[2] * eb[0] - ea[0] * eb[2];
        float nz = ea[0] * eb[1] - ea[1] * eb[0];
        const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 1e-7f) { nx /= len; ny /= len; nz /= len; }

        // 3 вершины грани: apex, base[i+1], base[i]
        // 3 face vertices: apex, base[i+1], base[i]
        const uint32_t base = static_cast<uint32_t>(outV.size());

        outV.push_back({{ axV[0], axV[1], axV[2] }, { nx, ny, nz }});
        outV.push_back({{ b1[0],  b1[1],  b1[2]  }, { nx, ny, nz }});
        outV.push_back({{ b0[0],  b0[1],  b0[2]  }, { nx, ny, nz }});

        outI.push_back(base);
        outI.push_back(base + 1);
        outI.push_back(base + 2);
    }

    // 2. Основание (triangle fan) / Base (triangle fan)
    // Вершины основания разделяются (одинаковая нормаль 0,-1,0)
    // Base vertices are shared (all share normal 0, -1, 0)

    const float bn[3] = { 0.0f, -1.0f, 0.0f }; // нормаль основания / base normal

    // Индекс центра основания / Base-centre vertex index
    const uint32_t centreIdx = static_cast<uint32_t>(outV.size());
    outV.push_back({{ bcV[0], bcV[1], bcV[2] }, { bn[0], bn[1], bn[2] }});

    // Вершины основания (индексы centreIdx+1 .. centreIdx+N)
    // Base circle vertices (indices centreIdx+1 .. centreIdx+N)
    for (int i = 0; i < N; ++i) {
        const float a = static_cast<float>(i) * step;
        outV.push_back({{ p.radius * std::cos(a), -halfH, p.radius * std::sin(a) },
                        { bn[0], bn[1], bn[2] }});
    }

    // Индексы треугольников основания / Base triangle indices
    // centre -> base[i] -> base[(i+1)%N]  ->  нормаль (0,-1,0)
    for (int i = 0; i < N; ++i) {
        outI.push_back(centreIdx);
        outI.push_back(centreIdx + 1 + static_cast<uint32_t>(i));
        outI.push_back(centreIdx + 1 + static_cast<uint32_t>((i + 1) % N));
    }
}
