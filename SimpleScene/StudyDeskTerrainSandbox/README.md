#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>
#include <glm/glm.hpp>

class Terrain {
public:
    Terrain(int width, int height);
    void generateDisplacementMap(float scale);
    void generateRandomTerrain(float roughness);
    const std::vector<float>& getVertices() const;

private:
    int width;
    int height;
    std::vector<float> vertices;
    void generateHeightMap();
};

#endif // TERRAIN_H