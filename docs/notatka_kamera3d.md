# 📷 Kamera 3D w FloodSim

Kamera zaimplementowana jest w klasie `OpenGLRenderer` (`app/Renderer/OpenGLRenderer.cpp/.h`).
Obsługuje dwa niezależne tryby pracy przełączane w locie.

---

## Tryby kamery (`CameraMode`)

| Tryb | Enum | Projekcja | Zastosowanie |
|---|---|---|---|
| Widok z góry | `TopDown` | Ortograficzna | Rysowanie i edycja terenu |
| Widok 3D | `Orbit` | Perspektywiczna (FOV 45°) | Eksploracja i podgląd symulacji |

Przełączanie odbywa się przez `setCameraMode()` lub `setCameraPanEnabled(bool)` — włączenie pana automatycznie przełącza na `Orbit`, wyłączenie wraca do `TopDown`.

---

## Stan kamery

```cpp
QVector3D cameraPosition;   // pozycja oka kamery w świecie
QVector3D cameraTarget;     // punkt na który patrzy kamera (look-at)
float     cameraYaw;        // kąt poziomy (stopnie), domyślnie -90°
float     cameraPitch;      // kąt pionowy  (stopnie), zawsze ujemny
float     cameraZoom;       // odległość oka od cameraTarget (Orbit) lub
                            // połowa rozmiaru widoku (TopDown)
QVector3D orbitFocusPoint;  // punkt obrotu w Orbit (worldspace, pod kursorem)
QVector3D panAnchorWorld;   // punkt "przyklejony" do kursora podczas pana
```

---

## Tryb TopDown — projekcja ortograficzna

### Macierz widoku
```
lookAt(cameraPosition, cameraTarget, up=(0,1,0))
```
Kamera zawieszona jest pionowo nad mapą na stałej wysokości `World::CAMERA_TOP_DOWN_HEIGHT = 800`.

### Macierz projekcji
Ortograficzna, symetryczna względem środka ekranu:
```
ortho(-zoom*aspect, +zoom*aspect, -zoom, +zoom, nearPlane, farPlane)
```
`zoom` mieści się w zakresie `[50, 800]`. `farPlane = World::CAMERA_TOP_DOWN_HEIGHT + maxWaterDepth`.

### Zoom (kółko myszy)
```
newZoom = clamp(zoom * zoomFactor, 10, 100)
zoomFactor = 1 - scrollAmount / 1200
```
Zmiana `zoom` bezpośrednio skaluje ortograficzne okno widoku — obiekty wyglądają na przybliżone/oddalone bez zmiany perspektywy.

### Pan (przeciąganie myszą)
```
sensitivity = zoom * 0.01
target.x -= deltaX * sensitivity
target.y += deltaY * sensitivity   // Qt: Y rośnie w dół, stąd +
```

---

## Tryb Orbit — projekcja perspektywiczna

### Układ współrzędnych
System Z-up (Z = góra). Teren leży w płaszczyźnie XY.

### Limity pitch
| Stała | Wartość | Znaczenie |
|---|---|---|
| `CAMERA_PITCH_MIN` | −89° | Prawie z góry (topdown) |
| `CAMERA_PITCH_MAX` | −5° | Prawie przy horyzoncie |

Pitch jest **zawsze ujemny** — kamera zawsze patrzy w dół na teren.

### Obliczanie pozycji kamery (`setupCamera`)
```cpp
yawRad   = radians(cameraYaw)
pitchRad = radians(cameraPitch)

offset = vec3(
    cos(yaw) * cos(pitch),      // X
    sin(yaw) * cos(pitch),      // Y
   -sin(pitch)                  // Z — negacja: ujemny pitch → dodatnie Z (ponad ziemią)
)

cameraPosition = cameraTarget + normalize(offset) * cameraZoom
```

```
lookAt(cameraPosition, cameraTarget, up=(0,0,1))
```

Wartości domyślne po resecie: `yaw = -90°`, `pitch = -45°` → klasyczny widok izometryczny z kierunku południa.

### Macierz projekcji
```
perspective(FOV=45°, aspect, near=0.5, far=5000)
```

### Zoom do kursora (kółko myszy) — styl Google Maps
Algorytm zachowuje punkt świata pod kursorem w tym samym miejscu ekranu po przybliżeniu:
```
zoomFactor = 1 - scrollAmount / 1200
newZoom    = clamp(oldZoom * zoomFactor, 5, 800)
ratio      = newZoom / oldZoom

cameraTarget    = cursorPt + (cameraTarget    - cursorPt) * ratio
orbitFocusPoint = cursorPt + (orbitFocusPoint - cursorPt) * ratio
```
Gdy `newZoom < oldZoom` (zbliżenie), `ratio < 1` → `cameraTarget` przesuwa się w stronę kursora.

Obsługa trackpada macOS: priorytet ma `pixelDelta` (mnożony ×4), fallback to `angleDelta` (120 na klik kółka).

`Shift + scroll` → pionowy pan (podnosi/opuszcza punkt `cameraTarget.z`):
```
cameraTarget.z += scrollAmount * 0.1 * 0.005 * zoom
```

### Orbit (LPM + przeciąganie)
Przy wciśnięciu LPM kamera zapamiętuje punkt świata pod kursorem jako `orbitFocusPoint` (raycast na płaszczyznę Z=0). Podczas ruchu:
```
cameraYaw   += deltaX * 0.4°
cameraPitch -= deltaY * 0.4°   // odwrócony deltaY: mysz w górę = kamera w górę
cameraPitch  = clamp(pitch, -89°, -5°)
cameraTarget = orbitFocusPoint   // obrót wokół punktu pod kursorem
```

### Pan (PPM + przeciąganie) — styl Google Maps
Punkt świata kliknięty prawym przyciskiem pozostaje "przyklejony" pod kursorem przez cały czas przeciągania:
```
shift = panAnchorWorld - currentGroundPt(cursor)
cameraTarget    += shift
orbitFocusPoint += shift
```
`currentGroundPt` wyliczany jest każdą klatkę przez raycast na płaszczyznę Z=0 — brak akumulacji błędu.

---

## Raycast na płaszczyznę terenu (`screenToGroundPlane`)

Używany przez: zoom do kursora, orbit, pan, screen→grid coords w trybie Orbit.

```
1. flipY: flippedY = height - screenY - 1        // Qt Y vs OpenGL Y
2. nearPt = unproject(screenX, flippedY, 0.0)    // punkt na near plane
3. farPt  = unproject(screenX, flippedY, 1.0)    // punkt na far plane
4. dir    = normalize(farPt - nearPt)
5. t      = -nearPt.z / dir.z                    // przecięcie z Z=0
6. worldPos = nearPt + t * dir
```
Zwraca `false` gdy promień jest równoległy do podłoża (`|dir.z| < 1e-6`) lub przecięcie jest za kamerą (`t < 0`).

---

## Mapowanie kursor → komórka siatki (`screenToGridCoords`)

**TopDown:**
```
ndcX = 2*screenX/width  - 1
ndcY = 1 - 2*screenY/height
worldPos = invView * invProj * vec4(ndcX, ndcY, 0, 1)
gridX = int(worldPos.x / cellSize)
gridY = int(worldPos.y / cellSize)
```

**Orbit:**
```
groundPt = screenToGroundPlane(screenX, screenY)
gridX = floor(groundPt.x / cellSize)
gridY = floor(groundPt.y / cellSize)
```

---

## Stałe

| Stała | Wartość | Opis |
|---|---|---|
| `World::CAMERA_ZOOM_MIN_ORTHO` | 50 | Min. zoom ortograficzny (m) |
| `World::CAMERA_ZOOM_MAX_ORTHO` | 1500 | Max. zoom ortograficzny (m) |
| `World::CAMERA_ZOOM_MIN_PERSP` | 10 | Min. odległość w Orbit (m) |
| `World::CAMERA_ZOOM_MAX_PERSP` | 5000 | Max. odległość w Orbit (m) |
| `World::CAMERA_TOP_DOWN_HEIGHT` | 3500 | Wysokość kamery TopDown (m) |
| `World::CAMERA_PITCH_MIN` | −89° | Prawie z góry |
| `World::CAMERA_PITCH_MAX` | −5° | Prawie przy horyzoncie |

