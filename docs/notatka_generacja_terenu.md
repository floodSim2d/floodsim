# 🏔️ Generowanie terenu (TerrainGenerator)

## Krok 1 — Heightmapa (`buildHeightmap`)

Używa biblioteki **FastNoiseLite** z trzema warstwami szumu:

| Warstwa | Typ | Rola | Waga |
|--------|-----|------|------|
| `fbm` | OpenSimplex2 + FBm, 6 oktaw, freq=0.005 | Ogólny kształt pagórków i dolin | 65% |
| `ridge` | OpenSimplex2 + Ridged, 4 oktawy, freq=0.008 | Ostre pasma górskie | 25% |
| `detail` | OpenSimplex2 + FBm, 3 oktawy, freq=0.018 | Drobne szczegóły powierzchni | 10% |

Następnie stosowane są dwa zabiegi:

- **Edge-fade** — krawędzie mapy opadają do wody (`fade = 1 - edgeDist^4`), dzięki czemu mapa wygląda jak wyspa
- **Power curve** (`n^0.9`) — rozszerza doliny kosztem gór (wartość `<1.0` „spłaszcza" środek)

---

## Krok 2 — Wygładzanie (`smoothHeightmap`)

Filtr uśredniający w oknie **3×3**, wykonywany wielokrotnie (`smoothPasses`). Usuwa ostre artefakty szumu.

---

## Krok 3 — Rzeźbienie rzek (`carveRiver`)

- Algorytm szuka najniższego punktu w każdym pionowym pasie mapy jako punktu startowego
- Idzie zachłannie w dół (do sąsiada o najniższej wysokości) z małym jitterem losowym — żeby rzeki nie wyglądały sztucznie prosto
- Koryto rzeki ma **3 poziomy głębokości** (promień 2 komórki):

| Strefa | Głębokość |
|--------|-----------|
| Centrum | `terrainHeight = -riverWaterDepth` (pełna głębokość) |
| Wewnętrzny brzeg (`r=1`) | 50% głębokości |
| Zewnętrzny brzeg (`r=2`) | 25% głębokości |

---

## Krok 4 — Źródła wody (`placeSources`)

Umieszczane na zboczach (zakres wysokości **0.35–0.62** znormalizowanej wysokości), z minimalną odległością między sobą, żeby były rozłożone równomiernie po mapie.

---

# 🎨 Renderowanie terenu (shadery)

## Vertex shader (`grid.vert`)

- Siatka 2D wierzchołków jest wynoszona w górę zgodnie z wysokością odczytaną z tekstury heightmapy (kanał `G`)
- Normalne liczone są z gradientu wysokości sąsiednich pikseli:

```
N = normalize((hL-hR)/dx, (hD-hU)/dy, 1.0)
```

---

## Fragment shader (`grid.frag`)

Kolory biome dobierane są na podstawie wysokości z płynnym przejściem (blending):

| Wysokość | Biome |
|----------|-------|
| `< 0` | Ciemna mulista ziemia (pod wodą) |
| `0–50` | Łąka (zielona trawa z plamami brudu) |
| `50–100` | Brąz (sucha ziemia, gleba, Voronoi crack pattern) |
| `100–150` | Skaliste góry z mchem |
| `150+` | Śnieg z odsłoniętą skałą |

Granice biome są celowo rozmyte szumem (`transitionNoise = fbm * 20 - 10`), żeby nie tworzyć prostych linii.

Oświetlenie realizowane jest modelem **Blinn-Phong** (ambient + diffuse lambertian + specular).