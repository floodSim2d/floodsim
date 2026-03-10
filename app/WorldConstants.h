#ifndef FLOODSIM_WORLDCONSTANTS_H
#define FLOODSIM_WORLDCONSTANTS_H

/**
 * @brief Centralne stałe definiujące skalę i układ współrzędnych symulacji.
 *
 * WAŻNE: Wartości wewnętrzne (teren, woda, kamera) są w "jednostkach symulacji".
 * Do wyświetlania w UI mnoży się je przez DISPLAY_SCALE (= 10),
 * żeby użytkownik widział realistyczne metry.
 *
 * Wewnętrznie: teren 0–300, woda 0–100, mapa 200×200 cells × 5 = 1000 units
 * Wyświetlanie: teren 0–3000 m, woda 0–1000 m, mapa 10 km × 10 km
 */
namespace World {

    // ─── Mnożnik wyświetlania ──────────────────────────────────────────
    // Wewnętrzna wartość × DISPLAY_SCALE = wartość wyświetlana użytkownikowi (metry)
    constexpr float DISPLAY_SCALE        = 10.0F;

    // ─── Siatka (Grid) ─────────────────────────────────────────────────
    constexpr int   DEFAULT_GRID_WIDTH   = 200;       // komórek
    constexpr int   DEFAULT_GRID_HEIGHT  = 200;       // komórek
    constexpr float DEFAULT_CELL_SIZE    = 5.0F;      // jednostek na komórkę
    // => mapa wewnętrzna: 1000 × 1000 units
    // => wyświetlana:     10 km × 10 km

    // ─── Teren ─────────────────────────────────────────────────────────
    constexpr float TERRAIN_MIN_HEIGHT   = -50.0F;    // wewnętrznie (wyświetlane: -500 m)
    constexpr float TERRAIN_MAX_HEIGHT   = 300.0F;    // wewnętrznie (wyświetlane: 3000 m)

    // ─── Woda ──────────────────────────────────────────────────────────
    constexpr float DEFAULT_WATER_DEPTH  = 50.0F;     // wewnętrznie (wyświetlane: 500 m)
    constexpr float MAX_WATER_DEPTH      = 100.0F;    // wewnętrznie (wyświetlane: 1000 m)
    constexpr float MIN_WATER_DEPTH      = 5.0F;      // wewnętrznie (wyświetlane: 50 m)

    // ─── Kamera ────────────────────────────────────────────────────────
    constexpr float CAMERA_TOP_DOWN_HEIGHT = 800.0F;

    // Zoom – ortograficzny (top-down)
    constexpr float CAMERA_ZOOM_MIN_ORTHO  =  50.0F;
    constexpr float CAMERA_ZOOM_MAX_ORTHO  = 800.0F;

    // Zoom – perspektywiczny (orbit)
    constexpr float CAMERA_ZOOM_MIN_PERSP  =  10.0F;
    constexpr float CAMERA_ZOOM_MAX_PERSP  = 2000.0F;

    // Orbit tilt limits (stopnie)
    constexpr float CAMERA_PITCH_MIN       = -89.0F;
    constexpr float CAMERA_PITCH_MAX       = -5.0F;

    // ─── Narzędzia malowania ───────────────────────────────────────────
    constexpr float PAINT_TERRAIN_STEP     = 2.5F;    // wewnętrznie (wyśw: 25 m)
    constexpr float PAINT_RIVER_STEP       = 2.5F;    // wewnętrznie (wyśw: 25 m)
    constexpr float PAINT_SOURCE_STEP      = 1.0F;    // wewnętrznie (wyśw: 10 m)

    // ─── Generator terenu ──────────────────────────────────────────────
    constexpr float TERRAIN_GEN_HEIGHT_SCALE = 200.0F;  // wewnętrznie (wyśw: 2000 m)
    constexpr float TERRAIN_GEN_SEA_SHIFT    = 60.0F;   // wewnętrznie (wyśw: 600 m)

    // ─── Jednostki wyświetlania ────────────────────────────────────────
    inline const char* UNIT_LABEL = "m";

    // Pomocnicza funkcja: wartość wewnętrzna → wartość wyświetlana
    constexpr float toDisplay(float internalValue) { return internalValue * DISPLAY_SCALE; }

}  // namespace World

#endif  // FLOODSIM_WORLDCONSTANTS_H

