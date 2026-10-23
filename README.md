# Лабораторная работа №3. Вариант 8

**Тема:** многопоточность в Linux (POSIX Threads).

## Задание

Дано `K` массивов по `N` элементов. Нужно сложить их поэлементно с использованием потоков. Количество потоков должно зависеть от объёма вычислений и не превышать предел, переданный при запуске программы.

Пример: `[1, 2, 3] + [4, 5, 6] = [5, 7, 9]`.

## Компиляция

Все команды выполняются **из папки `lab03`** в Ubuntu / WSL:

```bash
mkdir -p build
gcc -std=c11 -O2 -Wall -Wextra -pthread src/sum_arrays.c -o build/sum_arrays
```

## Запуск

```bash
./build/sum_arrays --arrays 8 --length 300000 --threads 4
```

- `--arrays` — количество массивов (`K`);
- `--length` — длина каждого массива (`N`);
- `--threads` — максимальное количество вычислительных потоков (включая основной).

Программа заполняет массивы тестовыми значениями, складывает их, проверяет результат по однопоточному вычислению и выводит время, ускорение и эффективность.

## Проверка результата

```bash
./build/sum_arrays --arrays 3 --length 4 --threads 2
```

Для этого примера первые элементы результата:

```text
First result elements: -288 -285 -282 -279
```

Правильность результата дополнительно проверяется в коде сравнением с однопоточным вариантом (`memcmp`).

## Выбор количества потоков

Программа оценивает количество операций как `K * N` и выбирает примерно один поток на 100 000 операций. При этом соблюдает лимит `--threads` и не создаёт больше вычислительных потоков, чем элементов в массиве.

```bash
./build/sum_arrays --arrays 2 --length 1000 --threads 8
./build/sum_arrays --arrays 8 --length 300000 --threads 2
./build/sum_arrays --arrays 8 --length 300000 --threads 4
./build/sum_arrays --arrays 8 --length 300000 --threads 8
```

В первом случае будет использован один поток, поскольку объём работы маленький.

## Проверка потоков через Linux

```bash
./build/sum_arrays --arrays 16 --length 3000000 --threads 4 &
pid=$!
ps -T -p "$pid"
wait "$pid"
```

`ps -T` показывает потоки процесса: для четырёх вычислительных потоков должны отображаться четыре строки с одним `PID` и разными `SPID`, если команда выполнена во время параллельного вычисления.

Программа работает быстро, поэтому процесс может закончиться до вызова `ps`. **Только для демонстрации** можно временно добавить `#include <unistd.h>` и в начало `sum_part()` после `Task *task = arg;`:

```c
if (task->begin != 0) {
    sleep(5);
}
```

После изменения код нужно перекомпилировать. **Перед измерениями удалить `sleep(5)` и снова собрать программу**, иначе результаты времени, ускорения и эффективности будут искажены.

## Измерения для отчёта

```bash
./build/sum_arrays --arrays 4 --length 100000 --threads 1
./build/sum_arrays --arrays 4 --length 100000 --threads 2
./build/sum_arrays --arrays 4 --length 100000 --threads 4

./build/sum_arrays --arrays 8 --length 300000 --threads 1
./build/sum_arrays --arrays 8 --length 300000 --threads 2
./build/sum_arrays --arrays 8 --length 300000 --threads 4
./build/sum_arrays --arrays 8 --length 300000 --threads 8

./build/sum_arrays --arrays 16 --length 500000 --threads 1
./build/sum_arrays --arrays 16 --length 500000 --threads 2
./build/sum_arrays --arrays 16 --length 500000 --threads 4
./build/sum_arrays --arrays 16 --length 500000 --threads 8
```



