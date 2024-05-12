Jak skompilować: 
  g++ -std=c++20 -Wall main.cpp parser.cpp

Jak uruchomić: 
  1) ./a.out graph.20.dat.txt
  2) ./a.out GRAF.30.txt
  3) ./a.out GRAF.200.txt

Uwagi:
  1) Żeby móc prześledzić stany poszczególnych zasobów, wystarczy odkomentować
  455. linię kodu w pliku nagłówkowym resourceAllocator.hpp.
  
  2) Wartość maxTime przekazywana do konstruktora przy tworzeniu instancji 
  klasy ResourceAllocator wpływa na to, jak szybko rośnie współczynnik 
  standaryzacyjny odpowiedzialny za czas wykonania zadania.
  
  3) Zgodnie z poleceniem zadania, na początku alokujemy dla zadania #0
  najszybszy zasób uniwersalny. Obserwacja dla grafu "graph.20.dat.txt":
  przydział pierwszemy zadaniu zasobu uniwersalnego powoduje zwiększenie
  czasu wykonania całego grafu zadań z teoretycznej, idealnej wartości równej
  150 do znacznie większysch wartości rzędu 1100-3500. Najszybszy zasób 
  uniwersalny wykonuje zadanie T0 w czasie 693. Następnie, kiedy losujemy metodę 
  wybierającą zasób z tych już zaalokowanych, zwiększa się prawdopodobieństwo 
  wybrania tego zasobu, bo ma mniejszy koszt wykonania zadania. Dlatego po 
  wylosowaniu takiej metody co najmniej 2 razy, mamy już total time ponad 1800.

  4) W tym zadaniu nie bierzemy pod uwagę szyn komunikacyjnych, gdyż nie wiemy
  jak należy postępować w przypadku, kiedy zostaje wybrany zasób, na który nie 
  da się przesłać danych.

Autor: Tair Yerniyazov
Data: 27 kwietnia 2024 r.