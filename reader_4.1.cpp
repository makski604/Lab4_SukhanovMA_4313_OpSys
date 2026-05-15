/* 
   Лабораторная работа 4. Межпроцессное взаимодействие
   Задание 4.1. Код: читатель

   Выполнял: Суханов М.А.
   Группа:   4313
   ВУЗ:      СПбГЭТУ "ЛЭТИ"
   Год:      2026
*/

#include <windows.h>
#include <iostream>
#include <fstream>
#pragma comment(lib, "winmm.lib")

const int NUM_PAGES = 10; // 3+1+3+2+1 (431321)

// Метаданные
struct SharedMeta { 
    int in;  // <-- |Критические
    int out; // <-- |            данные
};

int main() {
    setlocale(LC_ALL, "Russian");

    int processID = 0;
    std::cout << "Введите номер читателя: ";
    std::cin >> processID;
    
    // Ожидание глобального события старта:
    HANDLE hStartEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\StartSimSignal");
    if (hStartEvent == NULL) {
        std::cout << "Ожидание создания управляющего события...";
        while (hStartEvent == NULL) {
            std::cout << ".";
            Sleep(500);
            hStartEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\StartSimSignal");
        }
    }
    std::cout << "Читатель [" << processID << "] готов" << std::endl;
    WaitForSingleObject(hStartEvent, INFINITE);
    CloseHandle(hStartEvent);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD pageSize = sysInfo.dwPageSize;
    DWORD totalSize = (NUM_PAGES + 1) * pageSize; // 1 страница под метаданные + NUM_PAGES под буфер

    // Cоздаём объект типа "проецируемый файл"
    HANDLE hMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        totalSize,
        L"sharedbuff_41"
    );
    if (hMap == NULL) {
        std::cout << "Ошибка CreateFileMapping. Код ошибки: " << GetLastError() << std::endl;
        return 1;
    }

    // Проецируем файл
    void* pMap = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, totalSize);

    // Блокируем страницы буфера в ОЗУ
    char* bufferStart = (char*)pMap + pageSize;
    VirtualLock(bufferStart, NUM_PAGES * pageSize);

    SharedMeta* meta = (SharedMeta*)pMap;

    // Создание объектов синхронизации:
    // Семафор пустых ячеек: счётчик = NUM_PAGES
    HANDLE semEmpty = CreateSemaphore(NULL, NUM_PAGES, NUM_PAGES, L"semempty");

    // Семафор заполненных ячеек: счётчик = 0
    HANDLE semFull = CreateSemaphore(NULL, 0, NUM_PAGES, L"semfull");

    // Мьютекс для монопольного доступа к буферу
    HANDLE mutex = CreateMutex(NULL, FALSE, L"mutex41");

    std::ofstream logFile("reader_log.txt", std::ios::app); // создаём лог файл
    std::cout << "Читатель " << processID << " запущен. Нажмите Ctrl + C для выхода." << std::endl;

    while (true) {
        logFile << timeGetTime() << " : Читатель " << processID << " -> ожидание\n"; // <- логируем
        WaitForSingleObject(semFull, INFINITE); // ждем появление хотя бы одной записи

        WaitForSingleObject(mutex, INFINITE); // захватываем мьютекс

        /*##########################   Начало Критической секции ############################*/
        //#
        int currentPage = meta->out; //  Логируем ->:
        logFile << timeGetTime() << " : Читатель " << processID << " -> чтение(стр. " << currentPage << ")\n";

        // Задаём длительность выполнения операции
        // случайным числом от 0.5 до 1.5 сек:
        int sleepTime = 500 + rand() % 1001;
        Sleep(sleepTime); // <-- имитация чтения данных из буфера

        meta->out = (currentPage + 1) % NUM_PAGES; // меняем крит. данные (переход на след. стр.)
        logFile << timeGetTime() << " : Читатель " << processID << " -> освобождение\n"; // <- логируем
        //#
        /*##########################   Конец Критической секции ############################*/
        
        ReleaseMutex(mutex); // Выходим из крит. секции и освобождаем мьютекс

        ReleaseSemaphore(semEmpty, 1, NULL); // +1 пустая страница
        Sleep(1000);
    }
    VirtualUnlock(bufferStart, NUM_PAGES * pageSize);
    UnmapViewOfFile(pMap);
    CloseHandle(hMap);
    CloseHandle(semEmpty);
    CloseHandle(semFull);
    CloseHandle(mutex);

    return 0;
}