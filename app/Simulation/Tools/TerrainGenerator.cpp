#include "TerrainGenerator.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

#include "FastNoiseLite.h"
#include "../Grid/Grid.h"
#include "../Grid/Cell.h"

// ─────────────────────────────────────────────────────────────────────────────
TerrainGenerator::TerrainGenerator(uint32_t seed)
    : m_seed(seed), m_cfg{} {}

TerrainGenerator::TerrainGenerator(uint32_t seed, Config cfg)
    : m_seed(seed), m_cfg(cfg) {}

// ─────────────────────────────────────────────────────────────────────────────
// PUBLIC
// ─────────────────────────────────────────────────────────────────────────────
void TerrainGenerator::generateTerrain(Grid& grid) {
    const int W = static_cast<int>(grid.getWidth());
    const int H = static_cast<int>(grid.getHeight());

    // buildHeightmap już normalizuje do [0,1] i stosuje power curve
    auto hm = buildHeightmap(W, H);
    smoothHeightmap(hm, W, H);
    // Po smoothingu renormalizuj żeby zachować zakres [0,1]
    normalise(hm);

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const float n  = hm[y * W + x];
            const float th = n * m_cfg.landHeightScale - m_cfg.seaLevelShift;

            Cell cell;
            cell.setTerrainHeight(th);
            cell.setType(LAND);
            // NIE wypełniamy wodą terenu poniżej 0 przy generowaniu –
            // mini ciapki wody w dołkach powodowały natychmiastowy przelew.
            // Woda trafia na mapę wyłącznie przez rzeki i źródła wody.

            grid.setCell(x, y, cell);
        }
    }

    auto seeds = findValleySeeds(hm, W, H);
    const int rivers = std::min(m_cfg.numRivers, static_cast<int>(seeds.size()));
    for (int i = 0; i < rivers; ++i) {
        carveRiver(hm, grid, W, H, seeds[i], m_cfg.mountainThreshold);
    }

    placeSources(hm, grid, W, H);
}

// ─────────────────────────────────────────────────────────────────────────────
// HEIGHTMAP CONSTRUCTION
// ─────────────────────────────────────────────────────────────────────────────
std::vector<float> TerrainGenerator::buildHeightmap(int w, int h) const {
    std::vector<float> hm(static_cast<size_t>(w) * h, 0.0f);

    // FBM – główny kształt terenu z pagórkami
    FastNoiseLite fbm;
    fbm.SetSeed(static_cast<int>(m_seed));
    fbm.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    fbm.SetFractalType(FastNoiseLite::FractalType_FBm);
    fbm.SetFractalOctaves(6);
    fbm.SetFractalLacunarity(2.0f);
    fbm.SetFractalGain(0.5f);
    fbm.SetFrequency(0.005f);   // niższa częstotliwość = szersze, bardziej spójne wzgórza

    // Ridge – wyraźne pasma górskie
    FastNoiseLite ridge;
    ridge.SetSeed(static_cast<int>(m_seed + 7));
    ridge.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    ridge.SetFractalType(FastNoiseLite::FractalType_Ridged);
    ridge.SetFractalOctaves(4);
    ridge.SetFractalLacunarity(2.0f);
    ridge.SetFractalGain(0.4f);
    ridge.SetFrequency(0.008f);  // niższa freq = szersze pasma górskie

    // Drobny noise na szczegóły
    FastNoiseLite detail;
    detail.SetSeed(static_cast<int>(m_seed + 31));
    detail.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    detail.SetFractalType(FastNoiseLite::FractalType_FBm);
    detail.SetFractalOctaves(3);
    detail.SetFrequency(0.018f);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float fx = static_cast<float>(x);
            const float fy = static_cast<float>(y);

            const float base      = fbm.GetNoise(fx, fy);     // -1..1
            const float ridgeVal  = ridge.GetNoise(fx, fy);   // -1..1
            const float detailVal = detail.GetNoise(fx, fy);  // -1..1

            // 65% FBM + 25% ridge + 10% detal
            const float combined = 0.65f * base + 0.25f * ridgeVal + 0.10f * detailVal;

            // Edge-fade – opadanie przy krawędziach
            const float nx       = (fx / static_cast<float>(w)) * 2.0f - 1.0f;
            const float ny       = (fy / static_cast<float>(h)) * 2.0f - 1.0f;
            const float edgeDist = std::max(std::abs(nx), std::abs(ny));
            const float fade     = 1.0f - std::pow(edgeDist, 4.0f);

            hm[y * w + x] = combined * std::clamp(fade, 0.0f, 1.0f);
        }
    }

    // Normalizacja do [0,1] przed power curve
    normalise(hm);

    // Power curve: n^0.9 – wartość <1.0 rozszerza doliny kosztem gór
    for (auto& v : hm) {
        v = std::pow(v, 0.9f);
    }

    return hm;
}

// ─────────────────────────────────────────────────────────────────────────────
void TerrainGenerator::normalise(std::vector<float>& hm) const {
    const float lo    = *std::min_element(hm.begin(), hm.end());
    const float hi    = *std::max_element(hm.begin(), hm.end());
    const float range = (hi - lo) < 1e-6f ? 1.0f : (hi - lo);
    for (auto& v : hm) { v = (v - lo) / range; }
}

// ─────────────────────────────────────────────────────────────────────────────
void TerrainGenerator::smoothHeightmap(std::vector<float>& hm, int w, int h) const {
    std::vector<float> tmp(hm.size());
    for (int pass = 0; pass < m_cfg.smoothPasses; ++pass) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float sum = 0.0f;
                int   cnt = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                            sum += hm[ny * w + nx];
                            ++cnt;
                        }
                    }
                }
                tmp[y * w + x] = sum / static_cast<float>(cnt);
            }
        }
        hm = tmp;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// RIVER CARVING
// ─────────────────────────────────────────────────────────────────────────────
std::vector<TerrainGenerator::Point>
TerrainGenerator::findValleySeeds(const std::vector<float>& hm, int w, int h) const {
    std::vector<Point> seeds;
    const int stripW = w / m_cfg.numRivers;
    std::mt19937 rng(m_seed + 99);

    for (int s = 0; s < m_cfg.numRivers; ++s) {
        const int xStart = s * stripW;
        const int xEnd   = (s == m_cfg.numRivers - 1) ? w : xStart + stripW;

        // Szukamy najniższego punktu w pasie – preferujemy doliny (below ~0.45)
        float bestH = 1.0f;
        Point best  = {xStart + (xEnd - xStart) / 2, h / 2};

        for (int y = 10; y < h - 10; ++y) {
            for (int x = xStart + 5; x < xEnd - 5; ++x) {
                const float v = hm[y * w + x];
                if (v < bestH) {
                    bestH = v;
                    best  = {x, y};
                }
            }
        }
        best.x = std::clamp(best.x + static_cast<int>(rng() % 11) - 5, 1, w - 2);
        best.y = std::clamp(best.y + static_cast<int>(rng() % 11) - 5, 1, h - 2);
        seeds.push_back(best);
    }
    return seeds;
}

// ─────────────────────────────────────────────────────────────────────────────
void TerrainGenerator::carveRiver(std::vector<float>& hm, Grid& grid,
                                   int w, int h, Point start,
                                   float mountainThreshold) const {
    std::mt19937 rng(m_seed ^ static_cast<uint32_t>(start.x * 1000 + start.y));

    const int DX[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int DY[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

    auto idx      = [&](int x, int y) { return y * w + x; };
    auto inBounds = [&](int x, int y) { return x >= 0 && x < w && y >= 0 && y < h; };

    int cx = start.x;
    int cy = start.y;
    int stuckCount = 0;
    const int maxSteps = w * h;

    for (int step = 0; step < maxSteps; ++step) {
        Cell* cell = grid.getCell(cx, cy);
        if (cell != nullptr) {
            // Dno rzeki: tyle poniżej 0, żeby tafla wody (th + waterDepth) = 0
            // Używamy riverWaterDepth jako głębokości koryta.
            const float riverBed = -m_cfg.riverWaterDepth;
            cell->setTerrainHeight(riverBed);
            cell->setType(RIVER);
            cell->setWaterDepth(m_cfg.riverWaterDepth);  // tafla = riverBed + depth = 0
        }

        // Poszerzenie koryta – 3 poziomy głębokości (promień 2 komórki)
        for (int dy2 = -2; dy2 <= 2; ++dy2) {
            for (int dx2 = -2; dx2 <= 2; ++dx2) {
                if (dx2 == 0 && dy2 == 0) { continue; }
                const int nx2 = cx + dx2;
                const int ny2 = cy + dy2;
                if (!inBounds(nx2, ny2)) { continue; }
                Cell* nb = grid.getCell(nx2, ny2);
                if (nb == nullptr || nb->getType() == RIVER || nb->getType() == WATER_SOURCE) { continue; }
                const float nv = hm[idx(nx2, ny2)];
                if (nv >= mountainThreshold) { continue; }

                const bool isInnerBank = (std::abs(dx2) <= 1 && std::abs(dy2) <= 1);
                if (isInnerBank) {
                    // Wewnętrzny brzeg (r=1): półgłębokość, tafla = 0
                    const float bankBed = -m_cfg.riverWaterDepth * 0.5f;
                    nb->setTerrainHeight(bankBed);
                    nb->setType(RIVER);
                    nb->setWaterDepth(m_cfg.riverWaterDepth * 0.5f);
                } else {
                    // Zewnętrzny brzeg (r=2): płytka woda, łagodne zejście
                    const float outerBed = -m_cfg.riverWaterDepth * 0.25f;
                    nb->setTerrainHeight(outerBed);
                    nb->setType(RIVER);
                    nb->setWaterDepth(m_cfg.riverWaterDepth * 0.25f);
                }
            }
        }

        if (cx <= 0 || cx >= w - 1 || cy <= 0 || cy >= h - 1) { break; }

        // Wybierz kierunek do najniższego sąsiada (nie góra)
        int   bestDir = -1;
        float bestVal = 1e9f;

        int order[8] = {0, 1, 2, 3, 4, 5, 6, 7};
        for (int i = 7; i > 0; --i) {
            const int j = static_cast<int>(rng() % static_cast<unsigned>(i + 1));
            std::swap(order[i], order[j]);
        }

        for (const int d : order) {
            const int nx2 = cx + DX[d];
            const int ny2 = cy + DY[d];
            if (!inBounds(nx2, ny2)) { continue; }
            const float v = hm[idx(nx2, ny2)];
            if (v >= mountainThreshold) { continue; }
            const float jitter    = (static_cast<float>(rng() & 0xFF) / 255.0f) * 0.03f;
            const float effective = v + jitter;
            if (effective < bestVal) {
                bestVal = effective;
                bestDir = d;
            }
        }

        if (bestDir == -1) {
            ++stuckCount;
            if (stuckCount > 5) { break; }
            hm[idx(cx, cy)] -= 0.05f;
            continue;
        }

        stuckCount = 0;
        cx += DX[bestDir];
        cy += DY[bestDir];
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// WATER SOURCES
// ─────────────────────────────────────────────────────────────────────────────
void TerrainGenerator::placeSources(const std::vector<float>& hm, Grid& grid,
                                     int w, int h) const {
    struct Candidate { int x, y; float v; };
    std::vector<Candidate> candidates;
    candidates.reserve(512);

    // Zbocza – zakres poniżej progu szczytów, ale powyżej dolin
    constexpr float slopeMin = 0.35f;
    constexpr float slopeMax = 0.62f;

    for (int y = 5; y < h - 5; ++y) {
        for (int x = 5; x < w - 5; ++x) {
            const float v = hm[y * w + x];
            if (v >= slopeMin && v <= slopeMax) {
                candidates.push_back({x, y, v});
            }
        }
    }
    if (candidates.empty()) { return; }

    // Losowa kolejność – źródła nie skupiają się zawsze w tym samym miejscu
    std::mt19937 rng(m_seed ^ 0xDEADBEEF);
    std::shuffle(candidates.begin(), candidates.end(), rng);

    int placed = 0;
    const int minDist = w / (m_cfg.numSources + 1);
    std::vector<Candidate> chosen;

    for (auto& c : candidates) {
        if (placed >= m_cfg.numSources) { break; }

        // Pomiń jeśli komórka jest rzeką
        const Cell* existing = grid.getCell(c.x, c.y);
        if (existing == nullptr || existing->getType() == RIVER) { continue; }

        bool tooClose = false;
        for (const auto& ch : chosen) {
            const int ddx = c.x - ch.x;
            const int ddy = c.y - ch.y;
            if (ddx * ddx + ddy * ddy < minDist * minDist) { tooClose = true; break; }
        }
        if (tooClose) { continue; }
        chosen.push_back(c);
        ++placed;

        Cell* cell = grid.getCell(c.x, c.y);
        if (cell != nullptr) {
            cell->setType(WATER_SOURCE);
            cell->setSourceStrength(m_cfg.sourceStrength);
            cell->setWaterDepth(m_cfg.sourceStrength);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ASCII DEBUG MAP
// ─────────────────────────────────────────────────────────────────────────────
void TerrainGenerator::printAsciiMap(const Grid& grid) const {
    const int W    = static_cast<int>(grid.getWidth());
    const int H    = static_cast<int>(grid.getHeight());
    const int step = 4;

    int rivers = 0, sources = 0, land = 0, low = 0;

    std::printf("\n=== TerrainGenerator ASCII map (sample every %d cells) ===\n", step);
    std::printf("Legend:  ^ mountain  . land  ~ river  * source  _ basin(water)\n\n");

    for (int y = 0; y < H; y += step) {
        for (int x = 0; x < W; x += step) {
            const Cell* cell = grid.getCell(x, y);
            if (cell == nullptr) { std::putchar('?'); continue; }

            const CellType t  = cell->getType();
            const float    th = cell->getTerrainHeight();

            char ch = '.';
            if      (t == RIVER)        { ch = '~'; ++rivers;  }
            else if (t == WATER_SOURCE) { ch = '*'; ++sources; }
            else if (th > 3.0f)         { ch = '^'; ++land;    }
            else if (th > 0.0f)         { ch = '.'; ++land;    }
            else                        { ch = '_'; ++low;     }

            std::putchar(ch);
        }
        std::putchar('\n');
    }

    std::printf("\n--- Stats (sampled) ---\n");
    std::printf("River: %d  |  Source: %d  |  Land: %d  |  Basin: %d\n",
                rivers, sources, land, low);

    int totalRivers = 0, totalSources = 0, totalBasin = 0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const Cell* c = grid.getCell(x, y);
            if (c == nullptr) { continue; }
            if (c->getType() == RIVER)        { ++totalRivers;  }
            if (c->getType() == WATER_SOURCE) { ++totalSources; }
            if (c->getTerrainHeight() <= 0.0f && c->getWaterDepth() > 0.0f) { ++totalBasin; }
        }
    }
    std::printf("Full grid: RIVER=%d  WATER_SOURCE=%d  BASIN_WATER=%d\n\n",
                totalRivers, totalSources, totalBasin);
}
