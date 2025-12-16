# floodsim
## TODO: do zrobienia po zmergowaniu brancha tools
- [ ] dodać jakieś proste testy jednostkowe do folderu tests
- [ ] pokombinować można coś z kamerą, aby można było zobaczyć mapę 3d (ogólnie to wystarczy zmienić coś z kamerą lub projection matrix bo sama mapa już jest w 3d tylko jest wyświetlana od góry)
- [ ] dodać tools do dodawania deszczu (chyba, że deszcz to jest źródło wody u nas to wsm nie trzeba tego robić)
- [ ] dodać jakiegoś prostego loggera do logowania przydatnych wartośći np. że w danej komórce coś sie stało, żeby później można było zobaczyć w konsoli takie rzeczy
- [ ] oprócz wielkości pędzla dodać też jaką ma siłę (ile terenu lub wody ma usunąć/dodać)
- [ ] jak na razie pędzel tylko dodaje wartości jak się naciśnie lewym przyciskiem, nie ma jeszcze funkcji usuwania wartości (np. prawym przyciskiem myszy)
- [ ] dodać jakieś ikonki do przycisków
- [ ] ogarnąć jakąś instrukcję obsługi
- [ ] refactor tego w jaki sposób przesyłamy dane o komórce do shadera, sama tekstura może mieć maks 4 kanały RGBA, ale komórka ma o wiele więcej informacji. Trzeba albo zrobić to jako SSBO albo po prostu VBO z danymi komórek i w shadere odczytywać z tego VBO
- [ ] refactor MainWindow. Rozbić każdy panel na osobną klasę i w konstruktorze MainWindow tylko je inicjalizować i ustawiać layout