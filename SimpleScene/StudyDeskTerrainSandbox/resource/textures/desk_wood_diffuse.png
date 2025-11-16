// src/terrain.h
#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>
#include <glm/glm.hpp>

class Terrain {
public:
    Terrain(int width, int height);
    void generateDisplacementMap();
    void generateRandomTerrain();
    const std::vector<float>& getVertices() const;

private:
    int width;
    int height;
    std::vector<float> vertices;
    void generateVertices();
};

#endif // TERRAIN_H