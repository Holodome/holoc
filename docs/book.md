# holoc

Это должна быть книга, описывающая компилятор для языка С, служащая как пример создания компиялтора с описанием техник и мотиваций к их использованию
Материал, изложенный здесь преподносится с более практической точки зрения, чем большинство книг, которые обычно используются для обучения по теме

## Содержание 

1. Зачем писать компиляторы
2. Язык с - его современное состояние
3. Трансляция (ее этапы)
4. Лексинг
5. Парсинг
6. Intermediate representation (bytecode)
7. LLVM бекенд
8. x86 бекенд
9. Расширение языка
10. Статический анализ
11. Оптимизация

## Доп аспекты, описанные в этой книге

1. Многопоточность
2. Управление памятью
3. Работа с строками
4. Дизайн, ориентированный на данные
5. Подход к структуризации данных

# 0. Введение

Языки программирования на сегодняшний день представлены в большом разнообразии. Оно выражается и в множестве сфер применения, в технических аспектах, идеологических и реализационных. 
Но, к сожалению, соврменное сосотояние языков программировния сложно назвать благополучным. Нет такого языка, который стал бы однозначно доминирующим в какой-то одной сфере, а поэтому постоянно есть нужда в специалистах в создании новых языков. 
Знание теории создания языков программирования на низком уровне и алгоритмов проведения отпимизация, общая осведомленность об внутреннем устройстве процессора, влияние которого на производительность программы, как ни странно, крайне велико, важны для прогпаммиста, занимающимся программирование высокоэффективных систем примерно так же как понятие производной и интеграла для всей математики.

# 1. Язык С - его современное состонияе
Язык С был выбран для этого проекта потому что он прост в анализе, и явно показывает все процессы, происходящие в программе.


# 2. Этапы трансляции
Этапы трансляции - это крупные подпрограммы, которые занимаются тем, что переводят 
## 2.1 Лексинг
Лексинг (lexing) - это обычно первый этап в работе с программой, который присутствует в подовляющем множестве компиляторов.
Лексер этого компилятора собран вручную для работы с С
Он реализует лексинг исходного текста в один проход

Задача лексинга в С может быть названа сложной - ведь в нее входит не просто разбиение текста на токены, но и препроцессинг, который, в свою очередь, не является контекстно-независимым и должен запускаться в контексте языка и использовать некоторые его особенности.
На самом деле аналогичные С препроцессоры применяются так же и в некоторых реализациях ассемблера, поэтому было принято решение реализовать его как обособленную часть программы

Алгоритм лексинга:
1) Все символы, которые не являются частью препроцессинга разбиваются на токены согласно правилам парсинга
2) 