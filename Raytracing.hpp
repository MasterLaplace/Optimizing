/**************************************************************************
 * Optimizing v0.0.0
 *
 * Optimizing is a C/CPP software package, part of the Laplace-Project.
 * It is designed to provide a set of tools and utilities for optimizing
 * various aspects of software development, including performance,
 * memory usage, and code organization.
 *
 * This file is part of the Optimizing project that is under Anti-NN License.
 * https://github.com/MasterLaplace/Anti-NN_LICENSE
 * Copyright © 2025 by @MasterLaplace, All rights reserved.
 *
 * Optimizing is a free software: you can redistribute it and/or modify
 * it under the terms of the Anti-NN License as published by MasterLaplace.
 * See the Anti-NN License for more details.
 *
 * @file Raytracing.hpp
 * @brief Raytracing Implementation for 3D Rendering using World Partitioning
 * and Dynamic Octree.
 *
 * This is an experimental implementation of a raytracer using optimizing features
 * such as world partitioning and dynamic octree for efficient rendering.
 *
 * The implementation is based on the work of philvoyer, which can be
 * found in the following tutorial:
 * - https://github.com/philvoyer/IFT3100H25/blob/main/module09/EX01/IFT3100H25_Raytracer/raytracer.cpp
 *
 * @author @MasterLaplace
 * @version 0.0.0
 * @date 2025-04-03
 **************************************************************************/

#pragma once

#include "WorldPartition.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

class Raytracing {
public:
    struct CreateInfo {
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 background_color;
        float fov;
        uint8_t depth;
        uint16_t width;
        uint16_t height;
        uint8_t ray_per_pixel;
        // Je vais le changer pour me basé sur la position de la caméra et les cellules de la world partition qui sont
        // chargées pour calculer la boundary box totale de la scène, se qui va me permettre de faire le raytracing sur
        // la scène entière (et pas seulement sur la cellule courante).
        glm::vec3
            scene_pos_min; // Will be replaced by the current position of the camera - the world partition's cell * 0.5f
        glm::vec3 scene_scene_max; // Will be replaced by the current position of the camera + the world partition's
                                   // cell * 0.5f
    };

private:
    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;

        Ray(glm::vec3 o, glm::vec3 d) : origin(o), direction(d) {}
    };

    struct Camera {
        glm::vec3 position;
        glm::vec3 orientation;
        glm::vec3 target;

        glm::vec3 axis_x;
        glm::vec3 axis_y;
        glm::vec3 axis_z;

        glm::vec3 up;

        const float _VIEWPORT_WIDTH = 0.0f;
        const float _VIEWPORT_HEIGHT = 0.0f;

        const float _FOV = 0.0f;

        Camera(glm::vec3 p, glm::vec3 o, const uint16_t viewport_height, const uint16_t viewport_width, const float fov)
            : position(p), orientation(o), target(glm::vec3(0.0f)), up(glm::vec3(0.0f, 1.0f, 0.0f)),
              _VIEWPORT_WIDTH(static_cast<float>(viewport_width)),
              _VIEWPORT_HEIGHT(static_cast<float>(viewport_height)), _FOV(fov)
        {
            calibrate();
        }

        // fonction qui permet de calibrer la caméra en fonction la valeur courante de ses attributs
        void calibrate()
        {
            axis_z = orientation;
            axis_x = glm::vec3(_VIEWPORT_WIDTH * _FOV / _VIEWPORT_HEIGHT);
            axis_y = glm::normalize(glm::cross(axis_x, axis_z)) * _FOV;
        }
    };

public:
    Raytracing(const CreateInfo &params)
        : _MAX_DEPTH(params.depth), _CAMERA_FOV(params.fov), _CAMERA_POSITION(params.position),
          _CAMERA_ORIENTATION(glm::normalize(params.direction)), _BACKGROUND_COLOR(params.background_color),
          _IMAGE_WIDTH(params.width), _IMAGE_HEIGHT(params.height), _RAY_PER_PIXEL(params.ray_per_pixel),
          _PIXEL_COUNT(params.width * params.height),
          _camera(params.position, params.direction, params.height, params.width, params.fov)
    {
        _pixels.reserve(_PIXEL_COUNT * 4u);

        size_t nbObjects = 10;
        sf::Vector3f pos = {0, 0, 0};
        sf::Vector3f size = {800, 600, 50};
        sf::Vector3f maxArea = pos + size;
        std::vector<SpatialObject> objects;

        objects.reserve(nbObjects);

        for (size_t i = 0; i < nbObjects; ++i)
        {
            SpatialObject obj;
            obj.position = {randfloat(pos.x, maxArea.x), randfloat(pos.y, maxArea.y), randfloat(pos.z, maxArea.z)};
            obj.velocity = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
            obj.size = {randfloat(0, 20), randfloat(0, 20), randfloat(0, 20)};
            obj.colour = glm::vec4(rand() % 255, rand() % 255, rand() % 255, 255);
            obj.material = static_cast<SurfaceType>(rand() % 3);

            objects.emplace_back(obj);
        }

        _worldPartition.insert(objects);
    }

    inline void update(sf::RenderWindow &window) noexcept
    {
        // faire un rendu de la scène par lancer de rayons
        auto start = std::chrono::high_resolution_clock::now();

        // rendu de la scène
        render();

        // temps de calcul
        auto render_time = std::chrono::high_resolution_clock::now() - start;

        std::cout << "render time : " << std::chrono::duration_cast<std::chrono::seconds>(render_time).count() << "s"
                  << std::endl;

        // procédure post-rendu (sauvegarde de l'image et désallocation de la mémoire)
        post_render(window);
    }

private:
    [[nodiscard]] inline float clamp(float x) const noexcept { return x < 0 ? 0 : x > 1 ? 1 : x; }

    [[nodiscard]] inline uint8_t format_color_component(float value) const noexcept
    {
        // clamp la valeur entre 0 et 1
        value = clamp(value);

        // appliquer la correction gamma
        value = pow(clamp(value), _GAMMA_CORRECTION);

        // convertir la valeur dans l'espace de couleur
        value = value * 255.f + 0.5f;

        // conversion numérique de réel vers entier
        return static_cast<uint8_t>(value);
    }

    [[nodiscard]] inline float intersect(const Ray &ray, const BoundaryBox &box) const noexcept
    {
        // Initialiser les limites de l'intersection
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();

        // Parcourir chaque axe (x, y, z)
        for (uint8_t i = 0; i < 3u; ++i)
        {
            // Inverser la direction du rayon pour éviter la division par zéro
            float invD = 1.0f / ray.direction[i];
            float t0 = (box.getMin()[i] - ray.origin[i]) * invD;
            float t1 = (box.getMax()[i] - ray.origin[i]) * invD;

            // Assurer que t0 est le plus petit et t1 le plus grand
            if (invD < 0.0f)
                std::swap(t0, t1);

            // Mettre à jour les limites de l'intersection
            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);

            // Si les limites se croisent, il n'y a pas d'intersection
            if (tMax <= tMin)
                return 0.0f;
        }

        // Retourner la distance minimale de l'intersection
        return tMin;
    }

    [[nodiscard]] inline float intersect(const Ray &ray, const SpatialObject &object) const noexcept
    {
#ifndef DEBUG_RAYTRACING
        // Créer une boîte englobante pour l'objet
        BoundaryBox box(object.position, object.size);

        // Calculer l'intersection entre le rayon et la boîte
        return intersect(ray, box);
#else
        // distance de l'intersection la plus près si elle existe
        float distance;

        // seuil de tolérance numérique du test d'intersection
        float epsilon = 1e-6f;

        // distance du point d'intersection
        float t;

        // vecteur entre le centre de la sphère et l'origine du rayon
        glm::vec3 delta = object.position - ray.origin;

        // calculer a
        float a = glm::dot(delta, delta);

        // calculer b
        float b = glm::dot(delta, ray.direction);

        // calculer c
        float c = object.radius * object.radius;

        // calculer le discriminant de l'équation quadratique
        float discriminant = b * b - a + c;

        // valider si le discriminant est négatif
        if (discriminant < 0)
        {
            // il n'y a pas d'intersection avec cette sphère
            return distance = 0;
        }

        // calculer la racine carrée du discriminant seulement si non négatif
        discriminant = sqrt(discriminant);

        // déterminer la distance de la première intersection
        t = b - discriminant;

        // valider si la distance de la première intersection est dans le seuil de tolérance
        if (t > epsilon)
            distance = t;
        else
        {
            // déterminer la distance de la première intersection
            t = b + discriminant;

            // valider si la distance de la seconde intersection est dans le seuil de tolérance
            if (t > epsilon)
                distance = t;
            else
                distance = 0;
        }

        // retourner la distance du point d'intersection
        return distance;
#endif
    }

    void init_cornell_box()
    {
        constexpr float anchor = 1e5;
        constexpr float wall_radius = anchor;

        constexpr float box_size_x = 100.0;
        constexpr float box_size_y = 81.6;
        constexpr float box_size_z = 81.6;

        constexpr float box_x_min = 0.0;
        constexpr float box_x_max = box_size_x;
        constexpr float box_y_min = 0.0;
        constexpr float box_y_max = box_size_y;
        constexpr float box_z_min = 0.0;
        constexpr float box_z_max = box_size_z;

        constexpr float box_center_x = (box_x_max - box_x_min) / 2.0;
        constexpr float box_center_y = (box_y_max - box_y_min) / 2.0;
        constexpr float box_center_z = (box_z_max - box_z_min) / 2.0;

        // vider la scène de son contenu
        _scene.clear();

        // génération du contenu de la scène
        _scene.insert(
            _scene.begin(),
            {

                // approximation d'une boîte de Cornell avec des sphères surdimensionnées qui simulent des surfaces
                // planes
                SpatialObject(wall_radius, glm::vec3(box_center_x, anchor, box_size_z), glm::vec3(),
                              glm::vec3(0.75, 0.75, 0.75),
                              SurfaceType::DIFFUSE), // plancher
                SpatialObject(wall_radius, glm::vec3(box_center_x, -anchor + box_size_y, box_size_z), glm::vec3(),
                              glm::vec3(0.75, 0.75, 0.75), SurfaceType::DIFFUSE), // plafond
                SpatialObject(wall_radius, glm::vec3(anchor + 1, box_center_y, box_size_z), glm::vec3(),
                              glm::vec3(0.75, 0.25, 0.25),
                              SurfaceType::DIFFUSE), // mur gauche
                SpatialObject(wall_radius, glm::vec3(box_center_x, box_center_y, anchor), glm::vec3(),
                              glm::vec3(0.25, 0.75, 0.25),
                              SurfaceType::DIFFUSE), // mur arrière
                SpatialObject(wall_radius, glm::vec3(-anchor + 99, box_center_y, box_size_z), glm::vec3(),
                              glm::vec3(0.25, 0.25, 0.75),
                              SurfaceType::DIFFUSE), // mur droit
                SpatialObject(wall_radius, glm::vec3(box_center_x, box_center_y, -anchor + 170), glm::vec3(),
                              glm::vec3(0.0, 0.0, 0.0),
                              SurfaceType::DIFFUSE), // mur avant

                // ensemble des sphères situées à l'intérieur de la boîte de Cornell
                SpatialObject(22.5, glm::vec3(30, 30, 40), glm::vec3(), glm::vec3(1.0, 1.0, 1.0),
                              SurfaceType::SPECULAR), // sphère mirroir
                SpatialObject(17.5, glm::vec3(75, 40, 75), glm::vec3(), glm::vec3(1.0, 1.0, 1.0),
                              SurfaceType::REFRACTION), // sphère de verre

                SpatialObject(600, glm::vec3(box_center_x, 600.0 + box_size_z - 0.27, box_size_z),
                              glm::vec3(15, 15, 15), glm::vec3(0.0, 0.0, 0.0), SurfaceType::DIFFUSE) // sphère lumineuse
            });
    }

    void render() noexcept
    {
        std::cout << "render start" << std::endl;

        uint16_t index = 0;

        float progression = 0.0f;

        float r1, r2 = 0.0f;
        float dx, dy = 0.0f;

        glm::vec3 radiance;

        glm::vec3 distance;

#ifndef DEBUG_RAYTRACING
        _scene.clear();
        _worldPartition.getAllObects(_scene);
#else
        init_cornell_box();
#endif

        // itération sur les rangées de pixels
        for (uint16_t y = 0u; y < _IMAGE_HEIGHT; ++y)
        {
            // calculer le pourcentage de progression
            progression = 100.0f * y / (_IMAGE_HEIGHT - 1.0f);

            // afficher le pourcentage de progression du rendu dans la console
            fprintf(stderr, "\rraytracing (%d rays per pixel) : %4.1f %%", _RAY_PER_PIXEL, progression);

            // itération sur les colonnes de pixels
            for (uint16_t x = 0u; x < _IMAGE_WIDTH; ++x)
            {
                // déterminer l'index du pixel
                index = (_IMAGE_HEIGHT - y - 1) * _IMAGE_WIDTH + x;

                // itération sur les rangées du bloc de 2x2 échantillons
                for (uint8_t sy = 0u; sy < 2u; ++sy)
                {
                    // itération sur les colonnes du bloc de 2x2 échantillons
                    for (uint8_t sx = 0u; sx < 2u; ++sx)
                    {
                        // initialiser la radiance
                        radiance = glm::vec3{};

                        // itération des sur les rayons par pixel
                        for (uint8_t s = 0u; s < _RAY_PER_PIXEL; ++s)
                        {
                            // filtre de la tente
                            r1 = 2.0f * _random01(_rng);
                            dx = r1 < 1.0 ? sqrt(r1) - 1.0 : 1.0 - sqrt(2.0 - r1);

                            r2 = 2.0 * _random01(_rng);
                            dy = r2 < 1.0 ? sqrt(r2) - 1.0 : 1.0 - sqrt(2.0 - r2);

                            // calculer la distance de l'échantillon
                            distance = _camera.axis_x * (((sx + 0.5f + dx) / 2.0f + x) / _IMAGE_WIDTH - 0.5f) +
                                       _camera.axis_y * (((sy + 0.5f + dy) / 2.0f + y) / _IMAGE_HEIGHT - 0.5f) +
                                       _camera.axis_z;

                            // appel récursif du calcul de la radiance
                            radiance =
                                radiance + compute_radiance(
                                               Ray(_camera.position + distance * 140.f, glm::normalize(distance)), 0u) *
                                               (1.0f / _RAY_PER_PIXEL);
                        }

                        _pixels[index] =
                            _pixels[index] + glm::vec3(clamp(radiance.x), clamp(radiance.y), clamp(radiance.z)) * 0.25f;
                    }
                }
            }
        }

        std::cout << "\nrender done" << std::endl;
    }

    glm::vec3 compute_radiance(const Ray &ray, uint8_t depth)
    {
        // valeur de la radiance
        glm::vec3 radiance;

        // distance de l'intersection
        float distance;

        // identifiant de la géométrie en intersection
        uint16_t id = 0;

        // valider s'il n'y a pas intersection
        if (!raycast(ray, distance, id))
            return glm::vec3{}; // couleur par défault (noir)

        // référence sur une géométrie en intersection avec un rayon
        const SpatialObject &obj = _scene[id];

        // calculer les coordonnées du point d'intersection
        glm::vec3 x = ray.origin + ray.direction * distance;

        // calculer la normale au point d'intersection
        glm::vec3 n = glm::normalize(glm::vec3(x - obj.position));

        // ajustement de la direction de la normale
        glm::vec3 na = glm::dot(n, ray.direction) < 0.f ? n : n * -1.f;

        // isoler la composante de couleur la plus puissante
        glm::vec3 f = obj.colour;
        float threshold = f.x > f.y && f.x > f.z ? f.x : f.y > f.z ? f.y : f.z;

        // valider si la limite du nombre de récursions est atteinte
        if (++depth > _MAX_DEPTH)
        {
            // test de probabilité
            if (_random01(_rng) < threshold)
                f = f * glm::vec3(1 / threshold);
            else
                return obj.emission;
        }

        if (obj.material == SurfaceType::DIFFUSE)
        {
            // matériau avec réflexion diffuse

            float r1 = 2.f * M_PI * _random01(_rng);
            float r2 = _random01(_rng);
            float r2s = sqrt(r2);

            glm::vec3 w = na;
            glm::vec3 u = glm::normalize(glm::cross(fabs(w.x) > 0.1f ? glm::vec3(0, 1, 0) : glm::vec3(1), w));
            glm::vec3 v = glm::cross(w, u);
            glm::vec3 d =
                glm::normalize(u * float(cos(r1)) * r2s + v * float(sin(r1)) * r2s + w * float(sqrt(1.f - r2)));

            radiance = obj.emission + f * compute_radiance(Ray(x, d), depth);

            return radiance;
        }
        else if (obj.material == SurfaceType::SPECULAR)
        {
            // matériau avec réflexion spéculaire

            radiance = obj.emission +
                       f * compute_radiance(Ray(x, ray.direction - n * 2.f * glm::dot(n, ray.direction)), depth);

            return radiance;
        }
        else if (obj.material == SurfaceType::REFRACTION)
        {
            // matériau avec réflexion réfraction

            Ray reflection_ray(x, ray.direction - n * 2.f * glm::dot(n, ray.direction));

            bool into = glm::dot(n, na) > 0;

            float ior = 1.5f; // indice de réfraction du verre
            float nc = 1.f;
            float nt = ior;
            float nnt = into ? nc / nt : nt / nc;
            float ddn = glm::dot(ray.direction, na);
            float cos2t;

            if ((cos2t = 1.f - nnt * nnt * (1.f - ddn * ddn)) < 0.f)
            {
                radiance = obj.emission + f * compute_radiance(reflection_ray, depth);

                return radiance;
            }

            glm::vec3 tdir =
                glm::normalize(ray.direction * nnt - n * float((into ? 1.f : -1.f) * (ddn * nnt + sqrt(cos2t))));

            // effet de fresnel
            float a = nt - nc;
            float b = nt + nc;
            float r0 = a * a / (b * b);
            float c = 1.0 - (into ? -ddn : glm::dot(tdir, n));
            float re = r0 + (1.0 - r0) * c * c * c * c * c;
            float tr = 1 - re;
            float p = 0.25 + 0.5 * re;
            float rp = re / p;
            float tp = tr / (1.0 - p);

            radiance =
                obj.emission + f * (depth > 2u ? (_random01(_rng) < p ? compute_radiance(reflection_ray, depth) * rp :
                                                                        compute_radiance(Ray(x, tdir), depth) * tp) :
                                                 compute_radiance(reflection_ray, depth) * re +
                                                     compute_radiance(Ray(x, tdir), depth) * tr);

            return radiance;
        }
        else
        {
            return radiance;
        }
    }

    bool raycast(const Ray &ray, float &distance, uint16_t &id)
    {
        // variable temporaire pour la distance d'une intersection entre un rayon et une sphère
        float d;

        // initialiser la distance à une valeur suffisamment éloignée pour qu'on la considère comme l'infinie
        float infinity = distance = 1e20;

        // nombre d'éléments dans la scène
        uint32_t n = static_cast<uint32_t>(_scene.size());

        // parcourir tous les éléments de la scène
        for (uint32_t index = 0; index < n; ++index)
        {
            // test d'intersection entre le rayon et la géométrie à cet index
            d = intersect(ray, _scene[index]);

            // valider s'il y a eu intersection et si la distance est inférieure aux autres intersections
            if (d && d < distance)
            {
                // nouvelle valeur courante de la distance et l'index de l'intersection la plus rapprochée de la caméra
                distance = d;
                id = index;
            }
        }

        // il y a eu intersection si la distance est plus petite que l'infini
        if (distance < infinity)
            return true;
        else
            return false;
    }

    void post_render(sf::RenderWindow &window) noexcept
    {
        _image.resize(_PIXEL_COUNT * 4u);

        for (uint32_t i = 0u; i < _PIXEL_COUNT; ++i)
        {
            _image[i * 4u + 0u] = format_color_component(_pixels[i].x);
            _image[i * 4u + 1u] = format_color_component(_pixels[i].y);
            _image[i * 4u + 2u] = format_color_component(_pixels[i].z);
            _image[i * 4u + 3u] = 255;
        }
        _texture.create(_IMAGE_WIDTH, _IMAGE_HEIGHT);
        _texture.update(_image.data());

        std::cout << "raytracer task is done" << std::endl;

        _sprite.setTexture(_texture);
        window.draw(_sprite);
    }

private:
    const uint8_t _MAX_DEPTH = 5u;
    const float _CAMERA_FOV = 0.5135f; // ~30 degrés
    const glm::vec3 _CAMERA_POSITION = glm::vec3(50, 50, 300);
    const glm::vec3 _CAMERA_ORIENTATION = glm::normalize(glm::vec3(0, -0.042612, -1));
    const glm::vec3 _BACKGROUND_COLOR = glm::vec3(0.0, 0.0, 0.0);
    static constexpr float _GAMMA_CORRECTION = 1.f / 2.2f;

    // variables du programme
    const uint16_t _IMAGE_WIDTH = 0u;
    const uint16_t _IMAGE_HEIGHT = 0u;
    const uint8_t _RAY_PER_PIXEL = 0u;
    const uint32_t _PIXEL_COUNT = 0u;

    WorldPartition _worldPartition;
    std::vector<SpatialObject> _scene;

    // framebuffer de SFML
    sf::Texture _texture;
    sf::Sprite _sprite;
    std::vector<glm::vec3> _pixels;
    std::vector<uint8_t> _image;
    Camera _camera;

    // source d'entropie
    std::random_device _rd;

    // générateur de nombres pseudo-aléatoires (algorithme mersenne twister)
    std::mt19937 _rng{_rd()};

    // distribution uniforme entre 0 et 1
    std::uniform_real_distribution<float> _random01{0.0, 1.0};
};
