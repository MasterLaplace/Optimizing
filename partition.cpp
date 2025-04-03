#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <limits>
#include <thread>
#include <mutex>
#include <future>
#include <vector>
#include <iostream>
#include <queue>
#include "DynamicOctree.cpp"

namespace std {
    template <>
    struct hash<sf::Vector2i>
    {
        std::size_t operator()(const sf::Vector2i& v) const noexcept
        {
            return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
        }
    };
}

class ThreadPool
{
public:
    ThreadPool(size_t threads) : stop(false)
    {
        for (size_t i = 0; i < threads; ++i)
            workers.emplace_back([this] {
                for (;;)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
    }

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

class Partition
{
public:
    Partition(const sf::Vector3f &pos, const sf::Vector3f &size)
        : _pos(pos), _size(size), _octree(BoundaryBox(pos, size), MAX_CAPACITY, MAX_DEPTH) {}

    void load_data()
    {
        _loaded = true;

        size_t nbObjects = 1000;
        sf::Vector3f maxArea = _pos + _size;

        _objects.reserve(nbObjects);

        for (size_t i = 0; i < nbObjects; ++i)
        {
            SomeObjectWithArea obj;
            obj.vPos = {randfloat(_pos.x, maxArea.x), randfloat(_pos.y, maxArea.y), randfloat(_pos.z, maxArea.z)};
            obj.vVel = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
            obj.vSize = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
            obj.colour = sf::Color(rand() % 255, rand() % 255, rand() % 255, 255);

            _objects.emplace_back(obj);
            _octree.insert(obj, BoundaryBox(obj.vPos, obj.vSize));
        }

        std::cout << "Cellule " << _pos.x << " " << _pos.y << " chargée." << std::endl;
    }

    void draw(sf::RenderWindow &window) const
    {
        if (!_loaded)
            return;

        for (const auto &obj : _objects)
        {
            sf::RectangleShape rect;
            rect.setPosition({obj.vPos.x, obj.vPos.y});
            rect.setSize({obj.vSize.x, obj.vSize.y});
            rect.setFillColor(obj.colour);
            window.draw(rect);
        }

        sf::RectangleShape rect;
        rect.setPosition({_pos.x, _pos.y});
        rect.setSize({_size.x, _size.y});
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1);
        window.draw(rect);
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
    sf::Vector3f _pos;
    sf::Vector3f _size;
    std::vector<SomeObjectWithArea> _objects;
    DynamicOctreeContainer<BoundaryBox, SomeObjectWithArea> _octree;
    bool _loaded = false;
};

class WorldPartition
{
public:
    WorldPartition()
        : _threadPool(std::thread::hardware_concurrency()) // Initialiser le pool de threads avec le nombre de threads disponibles
    {
    }

    void load_partition(sf::Vector2i grid)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_cells.find(grid) == _cells.end())
        {
            _cells[grid] = std::make_shared<Partition>(sf::Vector3f(grid.x * _size.x, grid.y * _size.y, 0), _size);
            _threadPool.enqueue(&Partition::load_data, _cells[grid]);
            std::cout << "Lancement du chargement de la cellule " << grid.x << ", " << grid.y << std::endl;
        }
    }

    void unload_partition(sf::Vector2i grid)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_cells.find(grid) != _cells.end())
        {
            _cells.erase(grid);
            std::cout << "Déchargement de la cellule " << grid.x << ", " << grid.y << std::endl;
        }
    }

    void update(sf::RectangleShape player_rect)
    {
        sf::Vector2i player_grid = {static_cast<int>(player_rect.getPosition().x / _size.x), static_cast<int>(player_rect.getPosition().y / _size.y)};

        for (int x = player_grid.x - 1; x <= player_grid.x + 1; ++x)
        {
            for (int y = player_grid.y - 1; y <= player_grid.y + 1; ++y)
            {
                load_partition({x, y});
            }
        }

        std::vector<sf::Vector2i> cells_to_unload;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            for (const auto &cell : _cells)
            {
                if (abs(cell.first.x - player_grid.x) > 1 || abs(cell.first.y - player_grid.y) > 1)
                {
                    cells_to_unload.push_back(cell.first);
                }
            }
        }

        for (const auto &cell : cells_to_unload)
        {
            unload_partition(cell);
        }
    }

    void draw(sf::RenderWindow &window)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto &cell : _cells)
        {
            cell.second->draw(window);
        }
    }

private:
    sf::Vector3f _size = {255, 255, std::numeric_limits<float>::max()};
    std::unordered_map<sf::Vector2i, std::shared_ptr<Partition>> _cells;
    std::mutex _mutex;
    ThreadPool _threadPool;
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Partition", sf::Style::Close);
    window.setFramerateLimit(60);

    WorldPartition worldPartition;

    sf::RectangleShape player_rect({10, 10});
    player_rect.setFillColor(sf::Color::Red);

    sf::Clock clock;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                window.close();
        }

        float deltaTime = clock.restart().asSeconds();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            player_rect.move(500 * deltaTime, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            player_rect.move(-500 * deltaTime, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            player_rect.move(0, -500 * deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            player_rect.move(0, 500 * deltaTime);

        worldPartition.update(player_rect);

        window.clear();
        worldPartition.draw(window);
        window.draw(player_rect);
        window.display();
    }

    return 0;
}

// Build : g++ -std=c++20 -o partition partition.cpp -lsfml-graphics -lsfml-window -lsfml-system
