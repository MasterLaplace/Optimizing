#include <iostream>
#include <vector>
#include <array>
#include <list>
#include <cmath>
#include <memory>
#include <random>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <SFML/Graphics.hpp>

class BoundaryBox {
public:
    BoundaryBox() = default;
    BoundaryBox(const BoundaryBox &other) = default;
    BoundaryBox(BoundaryBox &&other) = default;
    BoundaryBox &operator=(const BoundaryBox &other) = default;
    BoundaryBox &operator=(BoundaryBox &&other) = default;

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
        return (point.x >= _min.x && point.x <= _max.x &&
                point.y >= _min.y && point.y <= _max.y &&
                point.z >= _min.z && point.z <= _max.z);
    }
    [[nodiscard]] inline bool overlaps(const BoundaryBox &other) const noexcept
    {
        return (_min.x <= other._max.x && _max.x >= other._min.x &&
                _min.y <= other._max.y && _max.y >= other._min.y &&
                _min.z <= other._max.z && _max.z >= other._min.z);
    }
    [[nodiscard]] inline bool contains(const BoundaryBox &other) const noexcept
    {
        return (_min.x <= other._min.x && _max.x >= other._max.x &&
                _min.y <= other._min.y && _max.y >= other._max.y &&
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

struct SomeObjectWithArea
{
    sf::Vector3f vPos;
    sf::Vector3f vVel;
    sf::Vector3f vSize;
    sf::Color colour;
};

template <typename T>
struct TreeItemLocation
{
    typename std::list<std::pair<BoundaryBox, T>> *container;
    typename std::list<std::pair<BoundaryBox, T>>::iterator iterator;
};

constexpr uint8_t MAX_DEPTH = 5;
constexpr uint8_t MAX_CAPACITY = 4;

template <typename OBJ_TYPE>
class DynamicOctree
{
private:
    enum class INDEX : uint8_t
    {
        SWD, // South-West-Down (min corner)
        SED, // South-East-Down
        NWD, // North-West-Down
        NED, // North-East-Down
        SWU, // South-West-Up
        SEU, // South-East-Up
        NWU, // North-West-Up
        NEU  // North-East-Up (max corner)
    };

public:
    //* COMPLETED
    DynamicOctree(const BoundaryBox &boundary, const uint8_t capacity = MAX_CAPACITY, const uint8_t depth = MAX_DEPTH)
        : _boundary(boundary), _DEPTH(depth), _CAPACITY(capacity)
    {
        resize(boundary);
    }

     //* COMPLETED
    void resize(const BoundaryBox &rArea)
    {
        clear();
        _boundary = rArea;

        sf::Vector3f size = _boundary.getSize() * 0.5f;
        sf::Vector3f pos = _boundary.getMin();

        _rNodes[static_cast<size_t>(INDEX::SWD)] = BoundaryBox(pos, size);
        _rNodes[static_cast<size_t>(INDEX::SED)] = BoundaryBox({pos.x + size.x, pos.y, pos.z}, size);
        _rNodes[static_cast<size_t>(INDEX::NWD)] = BoundaryBox({pos.x, pos.y + size.y, pos.z}, size);
        _rNodes[static_cast<size_t>(INDEX::NED)] = BoundaryBox({pos.x + size.x, pos.y + size.y, pos.z}, size);
        _rNodes[static_cast<size_t>(INDEX::SWU)] = BoundaryBox({pos.x, pos.y, pos.z + size.z}, size);
        _rNodes[static_cast<size_t>(INDEX::SEU)] = BoundaryBox({pos.x + size.x, pos.y, pos.z + size.z}, size);
        _rNodes[static_cast<size_t>(INDEX::NWU)] = BoundaryBox({pos.x, pos.y + size.y, pos.z + size.z}, size);
        _rNodes[static_cast<size_t>(INDEX::NEU)] = BoundaryBox(pos + size, size);
    }

    //* COMPLETED
    void clear()
    {
        _pItems.clear();

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_nodes[i])
                _nodes[i]->clear();

            _nodes[i].reset();
        }
    }

    //* COMPLETED
    size_t size() const
    {
        size_t size = _pItems.size();

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_nodes[i])
                size += _nodes[i]->size();
        }

        return size;
    }

    //* COMPLETED
    TreeItemLocation<OBJ_TYPE> insert(const OBJ_TYPE &item, const BoundaryBox &itemsize)
    {
        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_DEPTH == 0 || _pItems.size() < _CAPACITY)
                break;

            else if (!_rNodes[i].contains(itemsize))
                continue;

            if (!_nodes[i])
                _nodes[i] = std::make_unique<DynamicOctree<OBJ_TYPE>>(_rNodes[i], _CAPACITY, _DEPTH - 1);

            return _nodes[i]->insert(item, itemsize);
        }

        _pItems.emplace_back(itemsize, item);
        return {&_pItems, std::prev(_pItems.end())};
    }

    std::list<OBJ_TYPE> search(const BoundaryBox &rArea) const
    {
        std::list<OBJ_TYPE> listItems;
        search(rArea, listItems);
        return listItems;
    }

    void search(const BoundaryBox &rArea, std::list<OBJ_TYPE> &listItems) const
    {
        for (const auto &[rItem, item] : _pItems)
        {
            if (rArea.overlaps(rItem))
                listItems.emplace_back(item);
        }

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (!_nodes[i])
                continue;

            if (rArea.contains(_rNodes[i]))
                _nodes[i]->items(listItems);
            else if (rArea.overlaps(_rNodes[i]))
                _nodes[i]->search(rArea, listItems);
        }
    }

    void items(std::list<OBJ_TYPE> &listItems) const
    {
        for (const auto &[rItem, item] : _pItems)
            listItems.emplace_back(item);

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_nodes[i])
                _nodes[i]->items(listItems);
        }
    }

    //* COMPLETED
    const BoundaryBox &area() const
    {
        return _boundary;
    }

    //* COMPLETED
    bool remove(OBJ_TYPE pItem)
    {
        auto it = std::find_if(_pItems.begin(), _pItems.end(), [pItem](const std::pair<BoundaryBox, OBJ_TYPE> &p) { return p.second == pItem; });

        if (it != _pItems.end())
        {
            _pItems->erase(it);
            return true;
        }

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_nodes[i] && _nodes[i]->remove(pItem))
                return true;
        }

        return false;
    }

protected:
    const uint8_t _DEPTH = 1;
    const uint8_t _CAPACITY = 4;

    BoundaryBox _boundary;

    std::array<BoundaryBox, 8u> _rNodes{};

    std::array<std::unique_ptr<DynamicOctree<OBJ_TYPE>>, 8u> _nodes{};

    std::list<std::pair<BoundaryBox, OBJ_TYPE>> _pItems;
};

template <typename T>
struct TreeItem
{
    T item;
    TreeItemLocation<typename std::list<TreeItem<T>>::iterator> pItem;
};

template <typename BOUNDARY_TYPE, typename OBJ_TYPE>
class DynamicOctreeContainer
{
public:
    using TreeContainer = std::list<TreeItem<OBJ_TYPE>>;

    DynamicOctreeContainer(const BOUNDARY_TYPE &size, const uint8_t capacity = MAX_CAPACITY, const uint8_t depth = MAX_DEPTH)
        : _root(size, capacity, depth) {}

    void resize(const BOUNDARY_TYPE &rArea)
    {
        _root.resize(rArea);
    }

    size_t size() const
    {
        return _allItems.size();
    }

    bool empty() const
    {
        return _allItems.empty();
    }

    void clear()
    {
        _root.clear();
        _allItems.clear();
    }

    typename TreeContainer::iterator begin()
    {
        return _allItems.begin();
    }

    typename TreeContainer::iterator end()
    {
        return _allItems.end();
    }

    typename TreeContainer::const_iterator cbegin() const
    {
        return _allItems.cbegin();
    }

    typename TreeContainer::const_iterator cend() const
    {
        return _allItems.cend();
    }

    void insert(const OBJ_TYPE &item, const BOUNDARY_TYPE &itemsize)
    {
        TreeItem<OBJ_TYPE> newItem;
        newItem.item = item;
        _allItems.emplace_back(newItem);
        _allItems.back().pItem = _root.insert(std::prev(_allItems.end()), itemsize);
    }

    std::vector<typename TreeContainer::iterator> search(const BOUNDARY_TYPE &rArea) const
    {
        std::vector<typename TreeContainer::iterator> listItemsPointers;
        _root.search(rArea, listItemsPointers);
        return listItemsPointers;
    }

    void remove(typename TreeContainer::iterator item)
    {
        item->pItem.container->erase(item->pItem.iterator);
        _allItems.erase(item);
    }

    void relocate(typename TreeContainer::iterator &item, const BOUNDARY_TYPE &itemsize)
    {
        item->pItem.container->erase(item->pItem.iterator);
        item->pItem = _root.insert(item, itemsize);
    }

protected:
    TreeContainer _allItems;
    DynamicOctree<typename TreeContainer::iterator> _root;
};

class TestDynamicOctree
{
public:
    TestDynamicOctree(size_t nbObjects = 1000, float maxArea = 100.0f)
        : _octree(BoundaryBox(0, 0, 0, maxArea, maxArea, maxArea), MAX_CAPACITY, MAX_DEPTH)
    {
        objects.reserve(nbObjects);

        for (size_t i = 0; i < nbObjects; ++i)
        {
            SomeObjectWithArea obj;
            obj.vPos = {randfloat(0, maxArea), randfloat(0, maxArea), randfloat(0, maxArea)};
            obj.vVel = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
            obj.vSize = {randfloat(0, 5 + 1), randfloat(0, 5 + 1), randfloat(0, 5 + 1)};
            obj.colour = sf::Color(rand() % 255, rand() % 255, rand() % 255, 255);

            objects.emplace_back(obj);
            _octree.insert(obj, BoundaryBox(obj.vPos, obj.vSize));
        }
    }

    ~TestDynamicOctree() = default;

    void run()
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto &obj : objects)
        {
            // Display logic here
            objCount++;
        }
        std::chrono::duration<float> duration = std::chrono::high_resolution_clock::now() - start;

        // Log to file if duration exceeds threshold
        if (duration.count() > 0.1f) // Example threshold of 0.1 seconds
        {
            std::ofstream logFile("TestDynamicOctree.log", std::ios_base::app);
            logFile << "QuadTree: " << objCount << " objects displayed in " << duration.count() << " seconds\n";
        }

        objCount = 0;
    }

private:
    [[nodiscard]] static float randfloat(const float min, const float max)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen);
    };

private:
    std::vector<SomeObjectWithArea> objects;
    DynamicOctreeContainer<BoundaryBox, SomeObjectWithArea> _octree;
    sf::Vector3f pos{0, 0, 0};
    size_t objCount = 0;
};

#if RUN_TEST 1
int main(void)
{
    TestDynamicOctree test;
    test.run();
    return 0;
}
#endif
