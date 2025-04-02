// Tutorial: https://www.youtube.com/watch?v=ASAowY6yJII&ab_channel=javidx9

#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#include <array>
#include <memory>
#include <list>
#include <fstream>

struct rect : sf::RectangleShape
{
    rect() = default;

    rect(float x, float y, float w, float h)
    {
        setSize({w, h});
        setPosition(x, y);
        setFillColor(sf::Color::Transparent);
        setOutlineThickness(1);
        setOutlineColor(sf::Color::White);
    }

    rect(const sf::Vector2f &pos, const sf::Vector2f &size)
    {
        setSize(size);
        setPosition(pos);
        setFillColor(sf::Color::Transparent);
        setOutlineThickness(1);
        setOutlineColor(sf::Color::White);
    }

    bool contains(const sf::Vector2f &point) const
    {
        const auto &pos = getPosition();
        const auto &size = getSize();

        return !(point.x < pos.x || point.y < pos.y || point.x >= (pos.x + size.x) || point.y >= (pos.y + size.y));
    }

    bool contains(const rect &other) const
    {
        const auto &pos = getPosition();
        const auto &size = getSize();
        const auto &otherPos = other.getPosition();
        const auto &otherSize = other.getSize();

        return (otherPos.x >= pos.x) &&
               (otherPos.y >= pos.y) &&
               (otherPos.x + otherSize.x < pos.x + size.x) &&
               (otherPos.y + otherSize.y < pos.y + size.y);
    }

    bool overlaps(const rect &range) const
    {
        const auto &pos = getPosition();
        const auto &size = getSize();
        const auto &rangePos = range.getPosition();
        const auto &rangeSize = range.getSize();

        return (pos.x < rangePos.x + rangeSize.x) &&
               (pos.x + size.x >= rangePos.x) &&
               (pos.y < rangePos.y + rangeSize.y) &&
               (pos.y + size.y >= rangePos.y);
    }
};

struct SomeObjectWithArea
{
    sf::Vector2f vPos;
    sf::Vector2f vVel;
    sf::Vector2f vSize;
    sf::Color colour;
};

constexpr size_t MAX_DEPTH = 5;

// template <typename OBJ_TYPE>
// class StaticQuadTree
// {
// public:
//     StaticQuadTree(const rect &size = {0, 0, 100, 100}, const size_t depth = 0)
//         : _rect(size), _depth(depth)
//     {
//         resize(size);
//     }

//     void resize(const rect &rArea)
//     {
//         clear();
//         _rect = rArea;

//         sf::Vector2f size = _rect.getSize() / 2.0f;
//         sf::Vector2f pos = _rect.getPosition();

//         _rChild[0] = rect(pos, size);
//         _rChild[1] = rect({pos.x + size.x, pos.y}, size);
//         _rChild[2] = rect({pos.x, pos.y + size.y}, size);
//         _rChild[3] = rect(pos + size, size);
//     }

//     void clear()
//     {
//         _pItems.clear();

//         for (uint32_t i = 0; i < 4; ++i)
//         {
//             if (_pChild[i])
//                 _pChild[i]->clear();

//             _pChild[i].reset();
//         }
//     }

//     size_t size() const
//     {
//         size_t nCount = _pItems.size();

//         for (uint32_t i = 0; i < 4; ++i)
//         {
//             if (_pChild[i])
//                 nCount += _pChild[i]->size();
//         }

//         return nCount;
//     }

//     void insert(const OBJ_TYPE &item, const rect &itemsize)
//     {
//         for (uint32_t i = 0; i < 4; ++i)
//         {
//             if (_depth + 1 < MAX_DEPTH && _rChild[i].contains(itemsize))
//             {
//                 if (!_pChild[i])
//                     _pChild[i] = std::make_unique<StaticQuadTree<OBJ_TYPE>>(_rChild[i], _depth + 1);

//                 _pChild[i]->insert(item, itemsize);
//                 return;
//             }
//         }

//         _pItems.emplace_back(itemsize, item);
//     }

//     std::list<OBJ_TYPE> search(const rect &rArea) const
//     {
//         std::list<OBJ_TYPE> listItems;
//         search(rArea, listItems);
//         return listItems;
//     }

//     void search(const rect &rArea, std::list<OBJ_TYPE> &listItems) const
//     {
//         for (const auto &[rItem, item] : _pItems)
//         {
//             if (rArea.overlaps(rItem))
//                 listItems.emplace_back(item);
//         }

//         for (uint32_t i = 0; i < 4; ++i)
//         {
//             if (!_pChild[i])
//                 continue;

//             if (rArea.contains(_rChild[i]))
//                 _pChild[i]->items(listItems);
//             else if (rArea.overlaps(_rChild[i]))
//                 _pChild[i]->search(rArea, listItems);
//         }
//     }

//     void items(std::list<OBJ_TYPE> &listItems) const
//     {
//         for (const auto &[rItem, item] : _pItems)
//             listItems.emplace_back(item);

//         for (uint32_t i = 0; i < 4; ++i)
//         {
//             if (_pChild[i])
//                 _pChild[i]->items(listItems);
//         }
//     }

//     const rect &area() const
//     {
//         return _rect;
//     }

//     bool remove(OBJ_TYPE pItem)
//     {
//         auto it = std::find_if(_pItems.begin(), _pItems.end(), [pItem](const std::pair<rect, OBJ_TYPE> &p) { return p.second == pItem; });

//         if (it != _pItems.end())
//         {
//             _pItems.erase(it);
//             return true;
//         }

//         for (uint32_t i = 0; i < 4; ++i)
//         {
//             if (_pChild[i] && _pChild[i]->remove(pItem))
//                 return true;
//         }

//         return false;
//     }

// protected:
//     size_t _depth = 0;

//     rect _rect;

//     std::array<rect, 4u> _rChild{};

//     std::array<std::unique_ptr<StaticQuadTree<OBJ_TYPE>>, 4u> _pChild{};

//     std::vector<std::pair<rect, OBJ_TYPE>> _pItems;
// };

template <typename T>
struct QuadTreeItemLocation
{
    typename std::list<std::pair<rect, T>> *container;
    typename std::list<std::pair<rect, T>>::iterator iterator;
};

template <typename OBJ_TYPE>
class DynamicQuadTree
{
public:
    DynamicQuadTree(const rect &size = {0, 0, 100, 100}, const size_t depth = 0)
        : _rect(size), _depth(depth)
    {
        resize(size);
    }

    void resize(const rect &rArea)
    {
        clear();
        _rect = rArea;

        sf::Vector2f size = _rect.getSize() / 2.0f;
        sf::Vector2f pos = _rect.getPosition();

        _rChild[0] = rect(pos, size);
        _rChild[1] = rect({pos.x + size.x, pos.y}, size);
        _rChild[2] = rect({pos.x, pos.y + size.y}, size);
        _rChild[3] = rect(pos + size, size);
    }

    void clear()
    {
        _pItems.clear();

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (_pChild[i])
                _pChild[i]->clear();

            _pChild[i].reset();
        }
    }

    size_t size() const
    {
        size_t nCount = _pItems.size();

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (_pChild[i])
                nCount += _pChild[i]->size();
        }

        return nCount;
    }

    QuadTreeItemLocation<OBJ_TYPE> insert(const OBJ_TYPE &item, const rect &itemsize)
    {
        for (uint32_t i = 0; i < 4; ++i)
        {
            if (_depth + 1 < MAX_DEPTH && _rChild[i].contains(itemsize))
            {
                if (!_pChild[i])
                    _pChild[i] = std::make_unique<DynamicQuadTree<OBJ_TYPE>>(_rChild[i], _depth + 1);

                return _pChild[i]->insert(item, itemsize);
            }
        }

        _pItems.emplace_back(itemsize, item);
        return {&_pItems, std::prev(_pItems.end())};
    }

    std::list<OBJ_TYPE> search(const rect &rArea) const
    {
        std::list<OBJ_TYPE> listItems;
        search(rArea, listItems);
        return listItems;
    }

    void search(const rect &rArea, std::list<OBJ_TYPE> &listItems) const
    {
        for (const auto &[rItem, item] : _pItems)
        {
            if (rArea.overlaps(rItem))
                listItems.emplace_back(item);
        }

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (!_pChild[i])
                continue;

            if (rArea.contains(_rChild[i]))
                _pChild[i]->items(listItems);
            else if (rArea.overlaps(_rChild[i]))
                _pChild[i]->search(rArea, listItems);
        }
    }

    void items(std::list<OBJ_TYPE> &listItems) const
    {
        for (const auto &[rItem, item] : _pItems)
            listItems.emplace_back(item);

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (_pChild[i])
                _pChild[i]->items(listItems);
        }
    }

    const rect &area() const
    {
        return _rect;
    }

    bool remove(OBJ_TYPE pItem)
    {
        auto it = std::find_if(_pItems.begin(), _pItems.end(), [pItem](const std::pair<rect, OBJ_TYPE> &p) { return p.second == pItem; });

        if (it != _pItems.end())
        {
            _pItems.erase(it);
            return true;
        }

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (_pChild[i] && _pChild[i]->remove(pItem))
                return true;
        }

        return false;
    }

protected:
    size_t _depth = 0;

    rect _rect;

    std::array<rect, 4u> _rChild{};

    std::array<std::unique_ptr<DynamicQuadTree<OBJ_TYPE>>, 4u> _pChild{};

    std::list<std::pair<rect, OBJ_TYPE>> _pItems;
};

// template <typename OBJ_TYPE>
// class StaticQuadTreeContainer
// {
// public:
//     using QuadTreeContainer = std::list<OBJ_TYPE>;

//     StaticQuadTreeContainer(const rect &size = {0, 0, 100, 100}, const size_t depth = 0)
//         : _root(size) {}

//     void resize(const rect &rArea)
//     {
//         _root.resize(rArea);
//     }

//     size_t size() const
//     {
//         return _allItems.size();
//     }

//     bool empty() const
//     {
//         return _allItems.empty();
//     }

//     void clear()
//     {
//         _root.clear();
//         _allItems.clear();
//     }

//     typename QuadTreeContainer::iterator begin()
//     {
//         return _allItems.begin();
//     }

//     typename QuadTreeContainer::iterator end()
//     {
//         return _allItems.end();
//     }

//     typename QuadTreeContainer::const_iterator cbegin() const
//     {
//         return _allItems.cbegin();
//     }

//     typename QuadTreeContainer::const_iterator cend() const
//     {
//         return _allItems.cend();
//     }

//     void insert(const OBJ_TYPE &item, const rect &itemsize)
//     {
//         _allItems.emplace_back(item);
//         _root.insert(std::prev(_allItems.end()), itemsize);
//     }

//     std::list<typename QuadTreeContainer::iterator> search(const rect &rArea) const
//     {
//         std::list<typename QuadTreeContainer::iterator> listItemsPointers;
//         _root.search(rArea, listItemsPointers);
//         return listItemsPointers;
//     }

//     void remove(typename QuadTreeContainer::iterator item)
//     {
//         _root.remove(item);
//         _allItems.erase(item);
//     }

// protected:
//     QuadTreeContainer _allItems;
//     StaticQuadTree<typename QuadTreeContainer::iterator> _root;
// };

template <typename T>
struct QuadTreeItem
{
    T item;
    QuadTreeItemLocation<typename std::list<QuadTreeItem<T>>::iterator> pItem;
};

template <typename OBJ_TYPE>
class DynamicQuadTreeContainer
{
public:
    using QuadTreeContainer = std::list<QuadTreeItem<OBJ_TYPE>>;

    DynamicQuadTreeContainer(const rect &size = {0, 0, 100, 100}, const size_t depth = 0)
        : _root(size) {}

    void resize(const rect &rArea)
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

    typename QuadTreeContainer::iterator begin()
    {
        return _allItems.begin();
    }

    typename QuadTreeContainer::iterator end()
    {
        return _allItems.end();
    }

    typename QuadTreeContainer::const_iterator cbegin() const
    {
        return _allItems.cbegin();
    }

    typename QuadTreeContainer::const_iterator cend() const
    {
        return _allItems.cend();
    }

    void insert(const OBJ_TYPE &item, const rect &itemsize)
    {
        QuadTreeItem<OBJ_TYPE> newItem;
        newItem.item = item;
        _allItems.emplace_back(newItem);
        _allItems.back().pItem = _root.insert(std::prev(_allItems.end()), itemsize);
    }

    std::list<typename QuadTreeContainer::iterator> search(const rect &rArea) const
    {
        std::list<typename QuadTreeContainer::iterator> listItemsPointers;
        _root.search(rArea, listItemsPointers);
        return listItemsPointers;
    }

    void remove(typename QuadTreeContainer::iterator item)
    {
        item->pItem.container->erase(item->pItem.iterator);
        _allItems.erase(item);
    }

    void relocate(typename QuadTreeContainer::iterator &item, const rect &itemsize)
    {
        item->pItem.container->erase(item->pItem.iterator);
        item->pItem = _root.insert(item, itemsize);
    }

protected:
    QuadTreeContainer _allItems;
    DynamicQuadTree<typename QuadTreeContainer::iterator> _root;
};

class Game_JungleExplorer
{
public:
    struct sBush
    {
        sf::Vector2f vPos;
        sf::Vector2f vUnitSize;
        float scale;
        size_t nImage;
        sf::Color colour;
    };

public:
    Game_JungleExplorer()
    {
        // Create the window
        window.create(sf::VideoMode(800, 600), "QuadTree", sf::Style::Close);
        window.setFramerateLimit(60);

        auto rand_float = [](const float min, const float max) -> float
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(min, max);
            return dis(gen);
        };

        objects.reserve(1000000);
        quadtree.resize({0, 0, fArea, fArea});

        for (size_t i = 0; i < 1000000; ++i)
        {
            SomeObjectWithArea obj = {
                {rand_float(0, fArea), rand_float(0, fArea)},
                {0, 0},
                {rand_float(0.1f, 100.0f), rand_float(0.1f, 100.0f)},
                sf::Color(rand() % 255, rand() % 255, rand() % 255, 255)
            };

            objects.emplace_back(obj);
            quadtree.insert(obj, {obj.vPos, obj.vSize});
        }

        rScreen.setSize({static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)});
        rScreen.setPosition(0, 0);

        // Load the font
        if (!font.loadFromFile("/home/laplace/EIP/arial/ARIAL.TTF"))
            return;

        // Set the text properties
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
        text.setPosition(10, 10);

        // Set the rectangle properties
        m.setFillColor(sf::Color::Transparent);
        m.setOutlineColor(sf::Color::White);
        m.setOutlineThickness(1);
    }

    void run()
    {
        while (window.isOpen())
        {
            processEvents();
            update();
            render();
        }
    }

private:
    void processEvents()
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                window.close();

            if (event.type == sf::Event::KeyReleased)
            {
                if (event.key.code == sf::Keyboard::A)
                {
                    fSearchSize += 10.0f;
                    fSearchSize = std::clamp(fSearchSize, 10.0f, 500.0f);
                    vSearchArea = {fSearchSize, fSearchSize};
                }
                if (event.key.code == sf::Keyboard::Z)
                {
                    fSearchSize -= 10.0f;
                    fSearchSize = std::clamp(fSearchSize, 10.0f, 500.0f);
                    vSearchArea = {fSearchSize, fSearchSize};
                }
            }

            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Backspace)
                {
                    // m.setSize(vSearchArea * -fZoom);
                    sf::Vector2f pos = m.getPosition();
                    m.setPosition(mousePos - (vSearchArea * fZoom) / 2.0f + move);
                    for (auto &obj : quadtree.search(m))
                        quadtree.remove(obj);
                    m.setPosition(pos);
                }
            }

            if (event.type == sf::Event::MouseWheelScrolled)
            {
                if (event.mouseWheelScroll.delta > 0) {
                    fZoom *= 0.9f; // Zoom in
                    // viewRectangles.zoom(0.9f);
                }
                else if (event.mouseWheelScroll.delta < 0) {
                    fZoom *= 1.1f; // Zoom out
                    // viewRectangles.zoom(1.1f);
                }
            }
        }
    }

    void update()
    {
        float deltaTime = clock.restart().asSeconds();
        // Déplacement de la vue avec les touches directionnelles
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            move.x += -moveSpeed * fZoom * deltaTime;
            // viewRectangles.move(moveSpeed * fZoom * deltaTime, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            move.x += moveSpeed * fZoom * deltaTime;
            // viewRectangles.move(-moveSpeed * fZoom * deltaTime, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            move.y += moveSpeed * fZoom * deltaTime;
            // viewRectangles.move(0, -moveSpeed * fZoom * deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            move.y += -moveSpeed * fZoom * deltaTime;
            // viewRectangles.move(0, moveSpeed * fZoom * deltaTime);

        // Mettre à jour rScreen en fonction de la vue actuelle
        // sf::Vector2f viewCenter = viewRectangles.getCenter();
        // sf::Vector2f viewSize = viewRectangles.getSize();
        // rScreen.setPosition(viewCenter - viewSize / 2.0f);
        // rScreen.setSize(viewSize);

        mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    }

    void render()
    {
        window.clear();

        // Dessiner les rectangles avec la vue des rectangles
        // window.setView(viewRectangles);

        sf::Vector2f vPos = rScreen.getPosition();
        rScreen.setPosition(vPos + move);
        auto tpStart = std::chrono::system_clock::now();
        for (const auto &obj : quadtree.search(rScreen))
        {
            sf::RectangleShape shape(obj->item.vSize * fZoom);
            shape.setPosition(obj->item.vPos + move);
            shape.setFillColor(obj->item.colour);
            window.draw(shape);
            ++nObjCount;
        }
        std::chrono::duration<float> duration = std::chrono::system_clock::now() - tpStart;
        text.setString("QuadTree: " + std::to_string(nObjCount) + "/" + std::to_string(objects.size()) + " in " + std::to_string(duration.count()) + "s");

        // Log to file if duration exceeds threshold
        if (duration.count() > 0.1f) // Example threshold of 0.1 seconds
        {
            std::ofstream logFile("/home/laplace/EIP/Optimizing/QuadTree.log", std::ios_base::app);
            logFile << "QuadTree: " << nObjCount << " objects displayed in " << duration.count() << " seconds\n";
        }

        // Dessiner l'interface utilisateur avec la vue de l'interface utilisateur
        // window.setView(viewUI);
        window.draw(text);

        // Dessiner la zone de recherche
        m.setSize(vSearchArea * fZoom);
        m.setPosition(mousePos - (vSearchArea * fZoom) / 2.0f);
        window.draw(m);

        window.display();
        nObjCount = 0;
    }

private:
    sf::RenderWindow window;
    // sf::View viewRectangles = window.getDefaultView();
    // sf::View viewUI = window.getDefaultView();
    sf::Font font;
    sf::Text text;
    rect m;
    float fZoom = 1.0f;
    sf::Clock clock;
    sf::Vector2f mousePos;
    sf::Vector2f move;
    size_t nObjCount = 0;
    rect rScreen;
    const float moveSpeed = 500.0f; // Vitesse de déplacement en pixels par seconde
    std::vector<SomeObjectWithArea> objects;
    DynamicQuadTreeContainer<SomeObjectWithArea> quadtree;
    DynamicQuadTreeContainer<sBush> treeBushes;
    std::vector<sf::Sprite> sprites;
    float fArea = 100000.0f;
    float fSearchSize = 50.0f;
    sf::Vector2f vSearchArea = {fSearchSize, fSearchSize};
};

int main()
{
    Game_JungleExplorer game;
    game.run();

    return 0;
}

// Build : g++ -std=c++20 QuadTree.cpp -lsfml-graphics -lsfml-window -lsfml-system -o QuadTree
