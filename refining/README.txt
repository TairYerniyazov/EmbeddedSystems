KOMPILACJA
================================================================================
  $ g++ -std=c++20 -Wall main.cpp parser.cpp

URUCHOMIENIE
================================================================================
  Ogólny wzór: 
    $ ./a.out [task graph] [max time]
  Przykłady:
    $ ./a.out data/test_1.txt 2000 2>/dev/null
    $ ./a.out data/test_2.txt 300 2>/dev/null
    $ ./a.out data/graph.20.dat.txt 700 2>/dev/null > results/graph20.txt
    $ ./a.out data/GRAF.30.txt 1204 2>/dev/null > results/graph30.txt
    $ ./a.out data/GRAF.200.txt 150000 2>/dev/null > results/graph200.txt
  Uwagi:
    1) Powyższe polecenia zostały przetestowane na Linux. W przypadku 
    uruchomienia na Windows, należy podmienić /dev/null na nazwę dowolnego 
    pliku, w którym zostaną przechowane dane pozwalające na prześledzenie 
    działania całego algorytmu rafinacyjnego krok po kroku.
    2) Ostatnie trzy przykłady zapisują wyniki w katalogu results/. W przypadku
    przeglądu danych dotyczących dużego grafu GRAF.200.txt polecam wyświetlić
    kilka pierwszych i ostatnich linijek:
      $ head -205 results/graph200.txt
      $ tail -10 results/graph200.txt 
    3) Pierwsze dwa przykłady zostały wygenerowane za pomocą jednego z
    pierwszych programów napisanych w trakcie kursu. Graf zadań z pliku 
    test_1.txt został rozpisany ręcznie. Wyniki zostały sprawdzone. 
    4) Plik test_2.txt oferuje graf zadań o pięciu kanałach komunikacyjnych.
    W każdym z tych kanałów brakuje możliwości podpięcia większości z
    dostępnych jednostek obliczeniowych. Dlatego w wyniku działania algorytmu
    znaczna częśc zadań wciąż będzie wykonywana na tych zasobach, które zostały
    wybrane na początku, gdyż brak elastyczności ogranicza ruchy 
    optymalizacyjne.

WADY IMPLEMENTACJI
================================================================================
  1) Jeśli algorytm zostanie przerwany wcześniej ze względu na przekroczenie
  maksymalnego dopuszczalnego czasu, to w przypadku dużej liczby dostępnych
  szyn komunikacyjnych, istnieje szansa, że niektóre zasoby będą miały zbędne
  podpięcia, które były potrzebne do komunikacji z następnikami bądź
  poprzednikami zgodnie z pierwotnym planem. Te podpięcia nie są usuwane, więc
  mogą nieco zwiększyć prawdziwą wartość całkowitego kosztu.
  2) Na samym początku działania algorytmu, kiedy wyznaczany jest idealny
  scenariusz, to przydzielane są najszybsze zasoby. Szynę komunikacyjną jednak
  wybiera się taką, która oferuje najmniejszy koszt podpięcia, a nie
  największą przepustowość. Wynika to z tego, że później ta sama funkcja 
  składowa klasy ResourceAllocator odpowiedzialna za wybór odpowiedniej szyny
  komunikacyjnej jest wykorzystywana w trakcie wykonywania właściwych czynności 
  optymalizacyjnych. Ponadto, preferujemy heurystycznie dla rozważanych
  zadań te kanały, które nie wymagają dodatkowego podpięcia do nich 
  poprzednika, o ile poprzednik już posiada inny kanał komunikacyjny spełniający
  warunek łączności między dwoma zasobami. 

ZALETY IMPLEMENTACJI
================================================================================
  Program uwzględnia wszelkie problemy, które mogłyby powstać w wyniki podpięcia
  do nieodpowiedniej szyny danych. Np. jeśli następnik i poprzednik w którymś
  momencie nie mogą wymieniać się danymi w żaden sposób, to dla rozważanego 
  zadania nie jest alokowany nowy zasób. Jeżeli natomiast, jakikolwiek kanał
  pozwalałby na połączenie tych zasobów, na których te zadania są wykonywane, to
  wtedy wybiera się najtańszy (w sensie kosztu podpięcia) zasób z listy 
  dostępnych. 
  
  Taka metoda sprawdzenia warunku łączności jest sprawdzana również
  dla innych następników rozważanego zadania, nawet jak następniki te
  wcześniej nie stanowiły żadnej ścieżki. To, czy stanowią teraz, sprawdza się,
  obliczając czasy wykonywania poszczególnych zadań. Wtedy dynamicznie
  sprawdzany są poprzedniki, które dla swoich następników będą odgrywały
  kluczową rolę, gdyż ich wykonania skończy się wcześniej, określając w ten
  sposób punkt startowy dla kolejnego zadania. 
  
  Podczas określania czasu rozpoczęcia zadań, uwzględniana jest ilość 
  przekazywanych danych oraz przepustowość oferowana na wybranym kanale.

********************************************************************************
                            Autor: Tair Yerniyazov
                             Data: 1 maja 2024 r.
********************************************************************************