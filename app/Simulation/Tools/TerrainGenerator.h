#ifndef FLOODSIM_TERRAINGENERATOR_H
#define FLOODSIM_TERRAINGENERATOR_H

#include <cstdint>
#include <vector>

class Grid;

/**
 * @brief Generates realistic terrain for flood simulation.
 *
 * Algorithm:
 * 1. FBM (fractal Brownian motion) heightmap using FastNoiseLite
 * 2. Ridge noise overlay for mountain ranges
 * 3. Gaussian blur smoothing pass
 * 4. Terrain classification: LAND / EMPTY based on sea level
 * 5. 3 river paths carved through valleys (below sea level)
 * 6. Several WATER_SOURCE cells placed on mountain peaks
 */
class TerrainGenerator {
public:
    struct Config {
        float mountainThreshold   = 0.55f;  // normalised [0,1] above which = mountain
        float sourceThreshold     = 0.65f;  // normalised [0,1] above which = water source candidate
        int   numRivers           = 5;      // więcej rzek (było 3)
        int   numSources          = 7;      // więcej źródeł wody (było 5)
        int   smoothPasses        = 2;      // gaussian blur iterations
        float riverWaterDepth     = 6.0f;   // 2x głębsze koryto = szersza rzeka (było 3.5)
        float sourceStrength      = 3.0f;   // mocniejsze źródła (było 2.5)
        float landHeightScale     = 40.0f;  // 2x wyższe góry niż poprzednio (było 20)
        // Cały teren jest obniżany o tę wartość po przeskalowaniu.
        // Obszary które spadną poniżej 0 automatycznie dostaną wodę.
        // Np. 8.0 = ~13% mapy będzie pod wodą (zależy od rozkładu noise).
        float seaLevelShift       = 12.0f;  // przywrócone (było 18.0)
    };

    // Two separate constructors – avoids clang bug with default Config{} arg
    explicit TerrainGenerator(uint32_t seed = 42);
    TerrainGenerator(uint32_t seed, Config cfg);

    /**
     * @brief Generates terrain and writes it directly into the grid.
     * @param grid  Must be a valid, already-constructed Grid(200,200,...).
     */
    void generateTerrain(Grid& grid);

    /** Dumps a small ASCII overview to stdout (for verification). */
    void printAsciiMap(const Grid& grid) const;

private:
    uint32_t m_seed;
    Config   m_cfg;

    // ---- internal helpers ----
    std::vector<float> buildHeightmap(int w, int h) const;
    void               smoothHeightmap(std::vector<float>& hm, int w, int h) const;
    void               normalise(std::vector<float>& hm) const;

    // River carving
    struct Point { int x, y; };
    std::vector<Point> findValleySeeds(const std::vector<float>& hm, int w, int h) const;
    void carveRiver(std::vector<float>& hm, Grid& grid,
                    int w, int h, Point start,
                    float mountainThreshold) const;

    // Source placement
    void placeSources(const std::vector<float>& hm, Grid& grid, int w, int h) const;
};

#endif // FLOODSIM_TERRAINGENERATOR_H
