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

#define DEBUG_RAYTRACING

class Raytracing {
public:
    struct CreateInfo {
        glm::dvec3 position;
        glm::dvec3 direction;
        glm::dvec3 background_color;
        double fov;
        uint8_t depth;
        uint16_t width;
        uint16_t height;
        uint16_t ray_per_pixel;
        // Je vais le changer pour me basé sur la position de la caméra et les cellules de la world partition qui sont
        // chargées pour calculer la boundary box totale de la scène, se qui va me permettre de faire le raytracing sur
        // la scène entière (et pas seulement sur la cellule courante).
        glm::dvec3
            scene_pos_min; // Will be replaced by the current position of the camera - the world partition's cell * 0.5f
        glm::dvec3 scene_scene_max; // Will be replaced by the current position of the camera + the world partition's
                                    // cell * 0.5f
    };

private:
    struct Vector {
        double x, y, z;

        constexpr Vector() : x(0.0), y(0.0), z(0.0) {}
        constexpr Vector(double x) : x(x), y(0.0), z(0.0) {}
        constexpr Vector(double x, double y) : x(x), y(y), z(0.0) {}
        constexpr Vector(double x, double y, double z) : x(x), y(y), z(z) {}
        constexpr Vector(const glm::dvec3 &v) : x(v.x), y(v.y), z(v.z) {}
        constexpr Vector(const glm::vec3 &v) : x(v.x), y(v.y), z(v.z) {}

        // produit scalaire (dot product)
        double dot(const Vector &v) const { return x * v.x + y * v.y + z * v.z; }

        // produit vectoriel (cross product)
        Vector cross(const Vector &v) const { return Vector(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }

        // multiplication vectorielle
        Vector multiply(const Vector &v) const { return Vector(x * v.x, y * v.y, z * v.z); }

        // multiplication scalaire
        Vector operator*(double s) const { return Vector(x * s, y * s, z * s); }

        // multiplication élément par élément (vectorielle)
        Vector operator*(const Vector &v) const { return Vector(x * v.x, y * v.y, z * v.z); }

        // division vectorielle
        Vector operator/(const Vector &v) const { return Vector(x / v.x, y / v.y, z / v.z); }

        // division scalaire
        Vector operator/(double s) const { return Vector(x / s, y / s, z / s); }

        // addition vectorielle
        Vector operator+(const Vector &v) const { return Vector(x + v.x, y + v.y, z + v.z); }

        // soustraction vectorielle
        Vector operator-(const Vector &v) const { return Vector(x - v.x, y - v.y, z - v.z); }

        double operator[](size_t i) const
        {
            switch (i)
            {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: return 0.0;
            }
        }

        // normalisation
        Vector &normalize() { return *this = *this * (1.0 / sqrt(x * x + y * y + z * z)); }
    };

    struct Ray {
        Vector origin;
        Vector direction;

        Ray(Vector o, Vector d) : origin(o), direction(d) {}
    };

    struct Camera {
        Vector position;
        Vector orientation;

        Vector axis_x;
        Vector axis_y;
        Vector axis_z;

        const uint16_t _VIEWPORT_WIDTH = 0;
        const uint16_t _VIEWPORT_HEIGHT = 0;

        const double _FOV = 0.0f;

        Camera(glm::dvec3 p, glm::dvec3 o, const uint16_t viewport_height, const uint16_t viewport_width,
               const double fov)
            : position(p), orientation(o), _VIEWPORT_WIDTH(viewport_width), _VIEWPORT_HEIGHT(viewport_height), _FOV(fov)
        {
            calibrate();
        }

        // fonction qui permet de calibrer la caméra en fonction la valeur courante de ses attributs
        void calibrate()
        {
            axis_z = orientation;
            axis_x = Vector(_VIEWPORT_WIDTH * _FOV / _VIEWPORT_HEIGHT);
            axis_y = axis_x.cross(axis_z).normalize() * _FOV;
        }
    };

public:
    Raytracing(const CreateInfo &params)
        : _MAX_DEPTH(params.depth), _CAMERA_FOV(params.fov), _CAMERA_POSITION(params.position),
          _CAMERA_ORIENTATION(params.direction), _BACKGROUND_COLOR(params.background_color), _IMAGE_WIDTH(params.width),
          _IMAGE_HEIGHT(params.height), _RAY_PER_PIXEL(params.ray_per_pixel),
          _PIXEL_COUNT(params.width * params.height),
          _camera(params.position, params.direction, params.height, params.width, params.fov)
    {
        _pixels.reserve(_PIXEL_COUNT * 4u);

        constexpr double anchor = 1e5;
        constexpr double wall_radius = anchor;

        constexpr double box_size_x = 100.0;
        constexpr double box_size_y = 81.6;
        constexpr double box_size_z = 81.6;

        constexpr double box_x_min = 0.0;
        constexpr double box_x_max = box_size_x;
        constexpr double box_y_min = 0.0;
        constexpr double box_y_max = box_size_y;
        constexpr double box_z_min = 0.0;
        constexpr double box_z_max = box_size_z;

        constexpr double box_center_x = (box_x_max - box_x_min) / 2.0;
        constexpr double box_center_y = (box_y_max - box_y_min) / 2.0;
        constexpr double box_center_z = (box_z_max - box_z_min) / 2.0;

        size_t nbObjects = 1000;
        sf::Vector3f pos = {0, 0, 0};
        sf::Vector3f size = {800, 600, 600};
        sf::Vector3f maxArea = pos + size;
        std::vector<SpatialObject> objects;

        objects.reserve(nbObjects);

        for (size_t i = 0; i < nbObjects; ++i)
        {
            SpatialObject obj;
            obj.position = {randdouble(pos.x, maxArea.x), randdouble(pos.y, maxArea.y), randdouble(pos.z, maxArea.z)};
            obj.velocity = {randdouble(0, 10), randdouble(0, 10), randdouble(0, 10)};
            obj.size = {randdouble(20, 200), randdouble(20, 200), randdouble(20, 200)};
            obj.colour = glm::vec4(rand() % 255, rand() % 255, rand() % 255, 255);
            obj.emission = {randdouble(0, 15), randdouble(0, 15), randdouble(0, 15)};
            obj.material = static_cast<SurfaceType>(rand() % 3);
            obj.radius = obj.size;

            objects.emplace_back(obj);
        }

        objects.emplace_back(SpatialObject(22.5, glm::dvec3(30, 30, 40), glm::dvec3(0), glm::dvec3(1.0, 1.0, 1.0),
                                           SurfaceType::SPECULAR)); // sphère mirroir
        objects.emplace_back(SpatialObject(17.5, glm::dvec3(75, 40, 75), glm::dvec3(0), glm::dvec3(1.0, 1.0, 1.0),
                                           SurfaceType::REFRACTION)); // sphère de verre

        objects.emplace_back(SpatialObject(600, glm::dvec3(0, 0, 0), glm::dvec3(15, 15, 15), glm::dvec3(0.0, 0.0, 0.0),
                                           SurfaceType::DIFFUSE)); // sphère lumineuse

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
    [[nodiscard]] inline double clamp(double x) const noexcept { return x < 0 ? 0 : x > 1 ? 1 : x; }

    [[nodiscard]] inline uint8_t format_color_component(double value) const noexcept
    {
        // clamp la valeur entre 0 et 1
        value = clamp(value);

        // appliquer la correction gamma
        value = pow(clamp(value), _GAMMA_CORRECTION);

        // convertir la valeur dans l'espace de couleur
        value = value * 255.0 + 0.5;

        // conversion numérique de réel vers entier
        return static_cast<uint8_t>(value);
    }

    [[nodiscard]] inline double intersect(const Ray &ray, const BoundaryBox &box) const noexcept
    {
        double tmin = 0, tmax = std::numeric_limits<double>::infinity();

        for (uint8_t i = 0; i < 3; ++i)
        {
            double origin = (&ray.origin.x)[i];
            double dir = (&ray.direction.x)[i];
            double invD = 1.0 / dir;
            double t0 = ((&box.getMin().x)[i] - origin) * invD;
            double t1 = ((&box.getMax().x)[i] - origin) * invD;

            if (invD < 0)
                std::swap(t0, t1);

            tmin = t0 > tmin ? t0 : tmin;
            tmax = t1 < tmax ? t1 : tmax;

            if (tmax <= tmin)
                return 0.0;
        }
        return tmin > 1e-4 ? tmin : 0.0;
    }

    [[nodiscard]] inline double intersect(const Ray &ray, const SpatialObject &object) const noexcept
    {
        // Calculer l'intersection entre le rayon et la boîte
        if (object.type == SpatialObject::Type::CUBE)
            return intersect(ray, object.getBoundingBox());

        // distance de l'intersection la plus près si elle existe
        double distance;

        // seuil de tolérance numérique du test d'intersection
        double epsilon = 1e-4f;

        // distance du point d'intersection
        double t;

        // vecteur entre le centre de la sphère et l'origine du rayon
        Vector delta = Vector(object.position) - ray.origin;

        // calculer a
        double a = delta.dot(delta);

        // calculer b
        double b = delta.dot(ray.direction);

        // calculer c
        double c = object.radius.x * object.radius.y;

        // calculer le discriminant de l'équation quadratique
        double discriminant = b * b - a + c;

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
            distance = (t > epsilon) ? t : 0;
        }

        // retourner la distance du point d'intersection
        return distance;
    }

    void init_cornell_box()
    {
        constexpr double anchor = 1e5;
        constexpr double wall_radius = anchor;

        constexpr double box_size_x = 100.0;
        constexpr double box_size_y = 81.6;
        constexpr double box_size_z = 81.6;

        constexpr double box_x_min = 0.0;
        constexpr double box_x_max = box_size_x;
        constexpr double box_y_min = 0.0;
        constexpr double box_y_max = box_size_y;
        constexpr double box_z_min = 0.0;
        constexpr double box_z_max = box_size_z;

        constexpr double box_center_x = (box_x_max - box_x_min) / 2.0;
        constexpr double box_center_y = (box_y_max - box_y_min) / 2.0;
        constexpr double box_center_z = (box_z_max - box_z_min) / 2.0;

        Vector centre(50, 40, 75);
        constexpr double half = 15.0;

        // vider la scène de son contenu
        _scene.clear();

        // génération du contenu de la scène
        _scene.insert(
            _scene.begin(),
            {

                // approximation d'une boîte de Cornell avec des sphères surdimensionnées qui simulent des
                // surfaces planes
                SpatialObject(wall_radius, glm::dvec3(box_center_x, anchor, box_size_z), glm::dvec3(0.),
                              glm::dvec3(0.75, 0.75, 0.75),
                              SurfaceType::DIFFUSE), // plancher
                SpatialObject(wall_radius, glm::dvec3(box_center_x, -anchor + box_size_y, box_size_z), glm::dvec3(0.),
                              glm::dvec3(0.75, 0.75, 0.75), SurfaceType::DIFFUSE), // plafond
                SpatialObject(wall_radius, glm::dvec3(anchor + 1, box_center_y, box_size_z), glm::dvec3(0.),
                              glm::dvec3(0.75, 0.25, 0.25),
                              SurfaceType::DIFFUSE), // mur gauche
                SpatialObject(wall_radius, glm::dvec3(box_center_x, box_center_y, anchor), glm::dvec3(0.),
                              glm::dvec3(0.25, 0.75, 0.25),
                              SurfaceType::DIFFUSE), // mur arrière
                SpatialObject(wall_radius, glm::dvec3(-anchor + 99, box_center_y, box_size_z), glm::dvec3(0.),
                              glm::dvec3(0.25, 0.25, 0.75),
                              SurfaceType::DIFFUSE), // mur droit
                SpatialObject(wall_radius, glm::dvec3(box_center_x, box_center_y, -anchor + 170), glm::dvec3(0.),
                              glm::dvec3(0.),
                              SurfaceType::DIFFUSE), // mur avant

                // ensemble des sphères situées à l'intérieur de la boîte de Cornell
                SpatialObject(22.5, glm::dvec3(30, 30, 40), glm::dvec3(0.), glm::dvec3(1.),
                              SurfaceType::SPECULAR), // sphère mirroir
                SpatialObject(17.5, glm::dvec3(75, 40, 75), glm::dvec3(0.), glm::dvec3(1.),
                              SurfaceType::REFRACTION), // sphère de verre

                SpatialObject(600, glm::dvec3(box_center_x, 600.0 + box_size_z - 0.27, box_size_z),
                              glm::dvec3(15, 15, 15), glm::dvec3(0.),
                              SurfaceType::DIFFUSE), // sphère lumineuse

                // ajoute un cube au milieu de la scène
                SpatialObject(10, glm::dvec3(centre.x - half, centre.y - half, centre.z - half), glm::dvec3(0.),
                              glm::dvec3(0.8, 0.8, 0.2), SurfaceType::DIFFUSE, SpatialObject::Type::CUBE),
            });
    }

    void render() noexcept
    {
        std::cout << "render start" << std::endl;

        uint32_t index = 0;

        float progression = 0.0f;

        double r1, r2 = 0.0f;
        double dx, dy = 0.0f;

        Vector radiance;

        Vector distance;

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
                        radiance = Vector();

                        // itération des sur les rayons par pixel
                        for (uint16_t s = 0u; s < _RAY_PER_PIXEL; ++s)
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
                                radiance +
                                compute_radiance(Ray(_camera.position + distance * 140.0, distance.normalize()), 0u) *
                                    (1.0 / _RAY_PER_PIXEL);
                        }

                        _pixels[index] =
                            _pixels[index] + Vector(clamp(radiance.x), clamp(radiance.y), clamp(radiance.z)) * 0.25;
                    }
                }
            }
        }

        std::cout << "\nrender done" << std::endl;
    }

    Vector compute_radiance(const Ray &ray, uint8_t depth)
    {
        // distance de l'intersection
        double distance;

        // identifiant de la géométrie en intersection
        uint32_t id = 0;

        // valider s'il n'y a pas intersection
        if (!raycast(ray, distance, id))
            return Vector{}; // couleur par défault (noir)

        // référence sur une géométrie en intersection avec un rayon
        const SpatialObject &obj = _scene[id];

        // calculer les coordonnées du point d'intersection
        Vector x = ray.origin + ray.direction * distance;

        // attributs de l'objet touché
        Vector n;             // normale
        Vector emission;      // emission
        Vector colour;        // couleur
        SurfaceType material; // type de surface

        if (obj.type != SpatialObject::Type::CUBE)
        {
            n = (x - obj.position).normalize();
            emission = obj.emission;
            colour = Vector(obj.colour.r, obj.colour.g, obj.colour.b);
            material = obj.material;
        }
        else
        {
            const BoundaryBox bb = obj.getBoundingBox();
            // normale selon l'axe de la face impactée
            Vector d = x - Vector(bb.getMin() + bb.getMax()) * 0.5;
            double ax = fabs(d.x), ay = fabs(d.y), az = fabs(d.z);
            if (ax > ay && ax > az)
                n = Vector(d.x > 0 ? 1 : -1, 0, 0);
            else if (ay > az)
                n = Vector(0, d.y > 0 ? 1 : -1, 0);
            else
                n = Vector(0, 0, d.z > 0 ? 1 : -1);
            emission = obj.emission;
            colour = Vector(obj.colour.r, obj.colour.g, obj.colour.b);
            material = obj.material;
        }

        // ajustement de la direction de la normale
        Vector nl = n.dot(ray.direction) < 0 ? n : n * -1;

        // isoler la composante de couleur la plus puissante
        Vector f = Vector(obj.colour.r, obj.colour.g, obj.colour.b);
        double threshold = std::max(f.x, std::max(f.y, f.z));

        // valider si la limite du nombre de récursions est atteinte
        if (++depth > _MAX_DEPTH)
        {
            // test de probabilité
            if (_random01(_rng) < threshold)
                f = f * (1 / threshold);
            else
                return obj.emission;
        }

        if (obj.material == SurfaceType::DIFFUSE)
        {
            // matériau avec réflexion diffuse

            double r1 = 2.f * M_PI * _random01(_rng);
            double r2 = _random01(_rng);
            double r2s = sqrt(r2);

            Vector w = nl;
            Vector u = ((fabs(w.x) > 0.1 ? Vector(0, 1) : Vector(1)).cross(w)).normalize();
            Vector v = w.cross(u);
            Vector d = (u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2)).normalize();

            return Vector(obj.emission) + f.multiply(compute_radiance(Ray(x, d), depth));
        }
        else if (obj.material == SurfaceType::SPECULAR)
        {
            // matériau avec réflexion spéculaire

            Vector refl_dir = ray.direction - n * 2.0 * n.dot(ray.direction);
            return emission + f.multiply(compute_radiance(Ray(x, refl_dir), depth));
        }
        else if (obj.material == SurfaceType::REFRACTION)
        {
            // matériau avec réflexion réfraction

            Ray reflection_ray(x, ray.direction - n * 2.0 * n.dot(ray.direction));

            bool into = n.dot(nl) > 0;

            double ior = 1.5; // indice de réfraction du verre
            double nc = 1.0;
            double nt = ior;
            double nnt = into ? nc / nt : nt / nc;
            double ddn = ray.direction.dot(nl);
            double cos2t;

            if ((cos2t = 1.0 - nnt * nnt * (1.0 - ddn * ddn)) < 0.0)
            {
                return Vector(obj.emission) + f.multiply(compute_radiance(reflection_ray, depth));
            }

            Vector tdir = (ray.direction * nnt - n * ((into ? 1.0 : -1.0) * (ddn * nnt + sqrt(cos2t)))).normalize();

            // effet de fresnel
            double a = nt - nc;
            double b = nt + nc;
            double R0 = a * a / (b * b);
            double c = 1.0 - (into ? -ddn : tdir.dot(n));
            double Re = R0 + (1.0 - R0) * c * c * c * c * c;
            double Tr = 1 - Re;
            double P = 0.25 + 0.5 * Re;
            double RP = Re / P;
            double TP = Tr / (1.0 - P);

            if (depth > 2)
            {
                if (_random01(_rng) < P)
                {
                    return emission + f.multiply(compute_radiance(reflection_ray, depth) * RP);
                }
                else
                {
                    return emission + f.multiply(compute_radiance(Ray(x, tdir), depth) * TP);
                }
            }

            return emission + f.multiply(compute_radiance(reflection_ray, depth) * Re +
                                         compute_radiance(Ray(x, tdir), depth) * Tr);
        }

        return Vector();
    }

    bool raycast(const Ray &ray, double &distance, uint32_t &id)
    {
        // variable temporaire pour la distance d'une intersection entre un rayon et une sphère
        double d;

        // initialiser la distance à une valeur suffisamment éloignée pour qu'on la considère comme l'infinie
        distance = std::numeric_limits<double>::max();
        double infinity = distance;

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
        return distance < infinity;
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
    const double _CAMERA_FOV = 0.5135; // ~30 degrés
    const Vector _CAMERA_POSITION = Vector(50, 50, 300);
    const Vector _CAMERA_ORIENTATION = Vector(0, -0.042612, -1).normalize();
    const Vector _BACKGROUND_COLOR = Vector(0.0, 0.0, 0.0);
    static constexpr double _GAMMA_CORRECTION = 1.f / 2.2f;

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
    std::vector<Vector> _pixels;
    std::vector<uint8_t> _image;
    Camera _camera;

    // source d'entropie
    std::random_device _rd;

    // générateur de nombres pseudo-aléatoires (algorithme mersenne twister)
    std::mt19937 _rng{_rd()};

    // distribution uniforme entre 0 et 1
    std::uniform_real_distribution<double> _random01{0.0, 1.0};
};
