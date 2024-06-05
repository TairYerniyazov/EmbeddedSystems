# Opis plików

W katalogu `project/` są pliki dotyczące samego projektu. Plan dostępny jest 
w pliku `project/todo.txt`.

# Jak uruchomić (Linux)
W tej chwili mamy zaimplementowane parsowanie dowolnego grafu zadań. Powstałe
struktury są widoczne na wyjściu `cerr`. Za parsowanie odpowiada klasa `Parser`.

Żeby zobaczyć dane wypisywane na `cerr` w celu debugowania kodu, wystarczy
usunąć część `2>/dev/null` odpowiedzialną za przekierowanie strumienia błędów.

Program uruchamiamy zgodnie ze wzorem:
```
./[program.out] [task graph filepath] [max time] [max cost] [1/2] 2>dev/null
```
gdzie w ostatnich nawiasach opcja `1` uruchamia algorytm konstrukcyjny, a `2` 
uruchamia przydział nieprzewidzianych zadań.
 
```shell
cd project
g++ -std=c++20 -Wall main.cpp parser.cpp

./a.out data/test_structural_1.txt 1000 600 1 2>/dev/null

./a.out data/test_structural_2.txt 100 1000 1 2>/dev/null

./a.out data/test_structural_3.txt 10000 5000 1 2>/dev/null

./a.out data/test_structural_4.txt 100000 100000 1 2>/dev/null

./a.out data/test_unpredicted.txt 100000 100000 2 2>/dev/null
```

## Autorzy
&copy; 2024 Przemysław Wlazły, Tair Yerniyazov