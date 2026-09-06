.program "String Manipulation"

; ШАГ 1: Модули
.require console
.require stringDevice

; ШАГ 2: ВСЕ слоты заранее
.var result         ; слот 0
.var index          ; слот 1
.var length         ; слот 2
.var compareResult  ; слот 3

.reg cons           ; слот 4
.reg str1           ; слот 5
.reg str2           ; слот 6
.reg str3           ; слот 7
.reg temp           ; слот 8

; ШАГ 3: Константы
.const GREETING = "Hello"
.const WORLD = "World"

; ШАГ 4: Точка входа
.entry main

; ШАГ 5: Код
main:
    ; Создание устройств (заполняем слоты-хендлы)
    console.new cons
    stringDevice.new str1, GREETING
    stringDevice.new str2, WORLD
    stringDevice.new str3, ""

    ; Использование
    str3.set str1
    str3.append " "
    str3.append str2

    cons.print str3
    cons.print "\n"

    ; Все операции используют предварительно объявленные слоты
    str3.length length
    cons.print "Length: "
    cons.printInt length
    cons.print "\n"

    str1.compare str2, compareResult
    if compareResult == 0:
        cons.print "Equal\n"
    else:
        cons.print "Not equal\n"
    endif

    halt



.program "Loop Demo"

; Модули
.require console
.require stringDevice

; ВСЕ слоты
.var counter        ; слот 0
.var limit          ; слот 1
.var sum            ; слот 2
.var temp           ; слот 3

.reg cons           ; слот 4
.reg output         ; слот 5 (строка для вывода)

; Константы
.const MAX_ITERATIONS = 10

.entry main

main:
    console.new cons
    stringDevice.new output, ""

    ; Инициализация слотов
    counter = 0
    limit = MAX_ITERATIONS
    sum = 0

    ; Цикл
    while counter < limit:
        sum + counter
        counter + 1
    endwhile

    ; Вывод результата
    output.set "Sum: "
    int.toStr sum, temp
    output.append temp

    cons.print output
    cons.print "\n"

    halt



Насколько интересный синтаксис?