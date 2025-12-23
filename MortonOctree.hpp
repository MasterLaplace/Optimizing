// Version linéarisée de DynamicOctree utilisant l'ordre Z (Morton order)
// ----------------------------------------------------------------------
// Objectif : supprimer la structure pointer-based et utiliser une
// représentation contiguë et cache-friendly via Morton codes.
// ----------------------------------------------------------------------

#pragma once

#include "BoundaryBox.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

// Fonction d'interleaving des bits pour le codage Morton 3D
inline uint64_t morton3D(uint32_t x, uint32_t y, uint32_t z)
{
    auto split = [](uint32_t a) {
        uint64_t x = a & 0x1fffff; // 21 bits
        x = (x | x << 32) & 0x1f00000000ffff;
        x = (x | x << 16) & 0x1f0000ff0000ff;
        x = (x | x << 8) & 0x100f00f00f00f00f;
        x = (x | x << 4) & 0x10c30c30c30c30c3;
        x = (x | x << 2) & 0x1249249249249249;
        return x;
    };
    return split(x) | (split(y) << 1) | (split(z) << 2);
}

// Structure linéaire d'octree (SVO compact)
template <typename OBJ_TYPE> struct MortonNode {
    uint64_t mortonCode; // position spatiale codée en Morton
    BoundaryBox box;     // bounding box réelle
    OBJ_TYPE value;      // contenu du voxel / objet
};

template <typename OBJ_TYPE> class MortonOctree {
public:
    explicit MortonOctree(const BoundaryBox &boundary, uint8_t maxDepth = 5) noexcept
        : _boundary(boundary), _maxDepth(maxDepth)
    {
        glm::vec3 size = boundary.getSize();
        _invCellSize = glm::vec3((1 << maxDepth)) / size; // inverse pour convertir coord -> morton index
    }

    inline void clear() noexcept { _nodes.clear(); }

    inline void insert(const OBJ_TYPE &item, const BoundaryBox &itemBox) noexcept
    {
        glm::vec3 relMin = (itemBox.getMin() - _boundary.getMin()) * _invCellSize;
        glm::vec3 relMax = (itemBox.getMax() - _boundary.getMin()) * _invCellSize;

        uint32_t x = static_cast<uint32_t>(relMin.x);
        uint32_t y = static_cast<uint32_t>(relMin.y);
        uint32_t z = static_cast<uint32_t>(relMin.z);
        uint64_t code = morton3D(x, y, z);

        MortonNode<OBJ_TYPE> node{code, itemBox, item};
        _nodes.push_back(node);
    }

    [[nodiscard]] inline std::vector<OBJ_TYPE> search(const BoundaryBox &area) const noexcept
    {
        std::vector<OBJ_TYPE> results;
        for (const auto &n : _nodes)
        {
            if (area.overlaps(n.box))
                results.push_back(n.value);
        }
        return results;
    }

    inline void sortMortonOrder() noexcept
    {
        std::sort(_nodes.begin(), _nodes.end(), [](const auto &a, const auto &b) {
            return a.mortonCode < b.mortonCode;
        });
    }

    [[nodiscard]] inline size_t size() const noexcept { return _nodes.size(); }

    [[nodiscard]] inline const BoundaryBox &boundary() const noexcept { return _boundary; }

private:
    BoundaryBox _boundary;
    uint8_t _maxDepth;
    glm::vec3 _invCellSize{};
    std::vector<MortonNode<OBJ_TYPE>> _nodes; // structure linéaire
};
