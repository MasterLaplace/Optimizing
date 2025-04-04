#pragma once

#include <SFML/Graphics.hpp>

class BoundaryBox {
public:
    BoundaryBox() = default;
    BoundaryBox(const BoundaryBox &other) = default;
    BoundaryBox(BoundaryBox &&other) = default;
    BoundaryBox &operator=(const BoundaryBox &other) = default;
    BoundaryBox &operator=(BoundaryBox &&other) = default;

    [[nodiscard]] inline bool operator==(const BoundaryBox &other) const noexcept
    {
        return _min == other._min && _max == other._max;
    }

    BoundaryBox(float x, float y, float z, float w, float h, float d) noexcept
        : _min(x, y, z), _max(x + w, y + h, z + d)
    {
        _min.x = std::min(_min.x, _max.x);
        _min.y = std::min(_min.y, _max.y);
        _min.z = std::min(_min.z, _max.z);
        _max.x = std::max(_min.x, _max.x);
        _max.y = std::max(_min.y, _max.y);
        _max.z = std::max(_min.z, _max.z);
    }

    BoundaryBox(const sf::Vector3f &pos, const sf::Vector3f &size) noexcept
        : _min(pos), _max(pos.x + size.x, pos.y + size.y, pos.z + size.z)
    {
        _min.x = std::min(_min.x, _max.x);
        _min.y = std::min(_min.y, _max.y);
        _min.z = std::min(_min.z, _max.z);
        _max.x = std::max(_min.x, _max.x);
        _max.y = std::max(_min.y, _max.y);
        _max.z = std::max(_min.z, _max.z);
    }

    ~BoundaryBox() = default;

    [[nodiscard]] inline bool contains(const sf::Vector3f &point) const noexcept
    {
        return (point.x >= _min.x && point.x <= _max.x && point.y >= _min.y && point.y <= _max.y && point.z >= _min.z &&
                point.z <= _max.z);
    }
    [[nodiscard]] inline bool overlaps(const BoundaryBox &other) const noexcept
    {
        return (_min.x <= other._max.x && _max.x >= other._min.x && _min.y <= other._max.y && _max.y >= other._min.y &&
                _min.z <= other._max.z && _max.z >= other._min.z);
    }
    [[nodiscard]] inline bool contains(const BoundaryBox &other) const noexcept
    {
        return (_min.x <= other._min.x && _max.x >= other._max.x && _min.y <= other._min.y && _max.y >= other._max.y &&
                _min.z <= other._min.z && _max.z >= other._max.z);
    }

    [[nodiscard]] inline const sf::Vector3f &getPosition() const noexcept { return _min; }
    [[nodiscard]] inline const sf::Vector3f &getMin() const noexcept { return _min; }
    [[nodiscard]] inline const sf::Vector3f &getMax() const noexcept { return _max; }
    [[nodiscard]] inline sf::Vector3f getSize() const noexcept { return _max - _min; }
    [[nodiscard]] inline sf::Vector3f getCenter() const noexcept { return (_min + _max) * 0.5f; }

    [[nodiscard]] inline float getWidth() const noexcept { return _max.x - _min.x; }
    [[nodiscard]] inline float getHeight() const noexcept { return _max.y - _min.y; }
    [[nodiscard]] inline float getDepth() const noexcept { return _max.z - _min.z; }

protected:
    sf::Vector3f _min{0, 0, 0};
    sf::Vector3f _max{1, 1, 1};
};

struct SomeObjectWithArea {
    sf::Vector3f vPos;
    sf::Vector3f vVel;
    sf::Vector3f vSize;
    sf::Color colour;
};
