#!/bin/bash

# monitor_threads.sh

# Компилируем программу
g++ -pthread main.cpp -o bitonic_sort

# Запускаем программу в фоне
./bitonic_sort 2048 4 &
PROG_PID=$!

echo "Program PID: $PROG_PID"
echo "Monitoring threads..."

# Мониторим каждую сотую секунды
while kill -0 $PROG_PID 2>/dev/null; do
    echo "$(date +%T%3N) - Number of threads: $(ls /proc/$PROG_PID/task | wc -l)"
    sleep 0.05
done

wait $PROG_PID
