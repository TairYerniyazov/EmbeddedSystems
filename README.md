# Opis plików

W katalogu `structural/` znajduje się implementacja algorytmu konstrukcyjnego.

W katalogu `refining/` znajdziemy implementację algorytmu rafinacyjnego.

W katalogu `project/` możemy zamieszczać pliki dotyczące samego projektu.
Później pozostałe pliki zostaną usunięte. Klasa `ResourceAllocator` z 
implementacji algorytmu konstrukcyjnego zawiera elementy, które mogą być
przydatne, ewentualnie posłużyć za punkt początkowy. Z kolei analogiczna
klasa utworzona na potrzeby algorytmu rafinacyjnego zawiera mnóstwo (nie zbyt
eleganckich) metod składowych zapobiegających wystąpieniu problemów związanych z
łącznością dwóch bądź kilku węzłów: następnika i poprzednika lub też następnika 
i wszystkich jego "dzieci". Ten program z katalogu `refining/` został 
przetestowany na grafach reprezentujących ponad 300 zadań, dlatego zakładam, że
działa w miarę dobrze i w sensownym czasie.

Szczegółowe opisy są dostępne w osobnych plikach README.

Pliki źródłowe prawie nie zawierają żadnych komentarzy, ale będę od tego czasu
zamieszczał takie w nowych plikach, żebyśmy wiedzieli, o co w nich chodzi. 
Jeśli chodzi o styl (formatowanie) kodu, to używam konwencji i zasad
zdefiniowanych przez Google i stosowanych w projektach tej korporacji 
(https://google.github.io/styleguide/cppguide.html).

Plan dostępny jest w pliku `project/todo.txt`.

# Jak uruchomić (Linux)
W tej chwili mamy zaimplementowane parsowanie dowolnego grafu zadań. Powstałe
struktury są widoczne na wyjściu `cerr`. Za parsowanie odpowiada klasa `Parser`.
```shell
cd project
g++ -std=c++20 -Wall main.cpp parser.cpp
./a.out data/test_1.txt
./a.out data/test_2.txt
./a.out data/test_3.txt
```

## Autorzy
&copy; 2024 Przemysław Wlazły, Tair Yerniyazov