# Jak działa model przepływu wody (FlowModel) — model rurowy (pipe model)

## Wstęp — po co to?

Symulujemy jak woda spływa po terenie. Mamy siatkę komórek (np. 200×200), każda
komórka ma:
- **wysokość terenu** (np. góra = 100m, dolina = 10m)
- **głębokość wody** na wierzchu terenu
- **całkowita wysokość** = teren + woda

Woda płynie z miejsc wyżej do miejsc niżej — tak jak w prawdziwym życiu.

---

## Zasada działania — rury między komórkami

Wyobraź sobie, że między każdymi dwoma sąsiednimi komórkami jest **rurka**.
Każda komórka ma 4 rurki: w lewo, w prawo, do góry, na dół.

```
        [góra]
          |
[lewo] --[Ja]-- [prawo]
          |
        [dół]
```

Przez każdą rurkę płynie woda z jakąś **prędkością przepływu** (flux, oznaczamy Q).
Ta prędkość jest **zapamiętywana** między krokami symulacji — woda ma „rozpęd" (momentum).

---

## Krok symulacji — co się dzieje co klatkę

### Krok 1: Aktualizacja przepływów w rurkach

Dla każdej rurki liczymy nowy przepływ według wzoru:

```
Q_nowy = Q_stary × tłumienie + dt × A × g × Δh / L
```

Gdzie:
- **Q_stary** — przepływ z poprzedniego kroku (rurka „pamięta" swój rozpęd)
- **tłumienie** = e^(-tarcie × dt) — im większe tarcie, tym szybciej woda hamuje
- **dt** — krok czasowy (np. 0.016s = 1/60 sekundy)
- **A** — przekrój rurki (= rozmiar komórki, bo rurka jest tak szeroka jak komórka)
- **g** = 9.81 m/s² — przyspieszenie grawitacyjne (jak na lekcji fizyki!)
- **Δh** = różnica całkowitych wysokości między moją komórką a sąsiadem
- **L** — długość rurki (= rozmiar komórki)

Jeśli moja komórka jest **wyżej** niż sąsiad → Δh > 0 → woda płynie DO sąsiada.
Jeśli niżej → Δh < 0 → przepływ się zmniejsza (ale nie poniżej zera — ssanie
jest obsługiwane przez rurkę sąsiada w drugą stronę).

### Krok 1b: Zachowanie masy (najważniejsza rzecz!)

Po obliczeniu wszystkich 4 przepływów, sprawdzamy:

```
Ile wody chce wypłynąć = (Q_lewo + Q_prawo + Q_góra + Q_dół) × dt
Ile wody mamy          = głębokość_wody × pole_komórki
```

Jeśli chce wypłynąć **więcej niż mamy** — skalujemy WSZYSTKIE przepływy w dół:

```
współczynnik = woda_dostępna / woda_wypływająca
Q_lewo  *= współczynnik
Q_prawo *= współczynnik
...
```

Dzięki temu **nigdy nie wypłynie więcej wody niż jest** — zachowujemy masę!

### Krok 2: Aktualizacja głębokości wody

Dla każdej komórki liczymy:
- **Wpływ** = suma przepływów z rurek sąsiadów, które prowadzą DO mnie
- **Wypływ** = suma moich przepływów na zewnątrz

```
ΔV = dt × (wpływ - wypływ)     ← zmiana objętości
Δh = ΔV / pole_komórki          ← zmiana głębokości
nowa_głębokość = stara + Δh
```

### Krok 3: Obliczenie prędkości wody

Prędkość liczymy z różnicy przepływów:

```
vx = (wpływ_z_lewej + wypływ_w_prawo - wpływ_z_prawej - wypływ_w_lewo) / (2 × L × głębokość)
```

Analogicznie dla vy. Dzięki temu prędkość jest powiązana z **faktycznym przepływem**,
a nie z gradientem terenu.

---

## Dodatkowe mechanizmy

### Deszcz
Deszcz po prostu dodaje wodę na każdą komórkę:
```
Δh = intensywność_deszczu × dt
```
Np. intensywność 0.5 = 0.5 metra wody na sekundę (to mega ulewa!).

### Infiltracja (wsiąkanie)
Woda wsiąka w ziemię (ale nie w rzeki, źródła ani przeszkody):
```
Δh = -szybkość_wsiąkania × dt
```

### Źródła wody
Źródła utrzymują minimalny poziom wody — jeśli spadnie poniżej, jest uzupełniany.

---

## Dlaczego model rurowy jest lepszy niż prosty model?

| Cecha | Stary model | Model rurowy |
|-------|-------------|-------------|
| Równanie | Q = k × Δh (liniowe) | Q = Q_stary + dt × A × g × Δh / L (fizyczne) |
| Momentum | Brak — woda natychmiast reaguje | Jest — woda ma „rozpęd" z poprzedniego kroku |
| Masa | NIE zachowana — 4 sąsiadów mogło zabrać 4× więcej wody niż było | ZAWSZE zachowana — przepływy skalowane w dół |
| Grawitacja | Brak — sztuczny współczynnik k | Tak — g = 9.81 m/s², fizycznie poprawne |
| Prędkość | Z gradientu terenu (niepowiązana z przepływem) | Z faktycznych przepływów w rurkach |
| Tarcie | 0.999^n (zależy od ilości kroków) | e^(-f×dt) (niezależne od kroku czasowego) |
| Oscylacje | Częste | Tłumione przez momentum + tarcie |

---

## Parametry do regulacji w UI

- **pipeFriction** (tarcie) — jak szybko woda hamuje. 0 = frictionless, 0.5 = domyślne, >1 = bardzo lepka woda
- **dt** (krok czasowy) — mniejszy = dokładniejszy ale wolniejszy
- **globalRainIntensity** — ile wody spada na sekundę (w metrach głębokości)
- **infiltrationRate** — ile wody wsiąka na sekundę

---

## Literatura

Model oparty na artykule:
> *"Fast Hydraulic Erosion Simulation and Visualization on GPU"*
> Xing Mei, Philippe Decaudin, Bao-Gang Hu (2007)

