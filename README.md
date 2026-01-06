# floodsim
## TODO: do zrobienia po zmergowaniu brancha tools
- [ ] oprócz wielkości pędzla dodać też jaką ma siłę (ile terenu lub wody ma usunąć/dodać)
- [ ] jak na razie pędzel tylko dodaje wartości jak się naciśnie lewym przyciskiem, nie ma jeszcze funkcji usuwania wartości (np. prawym przyciskiem myszy)
- [ ] ogarnąć jakąś instrukcję obsługi
- [ ] refactor tego w jaki sposób przesyłamy dane o komórce do shadera, sama tekstura może mieć maks 4 kanały RGBA, ale komórka ma o wiele więcej informacji. Trzeba albo zrobić to jako SSBO albo po prostu VBO z danymi komórek i w shadere odczytywać z tego VBO