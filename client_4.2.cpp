/*
   Лабораторная работа 4. Межпроцессное взаимодействие
   Задание 4.2. Код: клиент
   Вариант 3 организации асинхронного в/в:
   «Тревожный» асинхронный ввод-вывод

   Выполнял: Суханов М.А.
   Группа:   4313
   ВУЗ:      СПбГЭТУ "ЛЭТИ"
   Год:      2026
*/

#include <windows.h>
#include <iostream>

char buffer[512]; // буфер для чтения данных


// Функция завершения, которая будет вызываться всякий раз 
// при завершении операции ввода-вывода:
VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

void MenuClient();

int main() {
    setlocale(LC_ALL, "Russian");

    HANDLE hPipe = INVALID_HANDLE_VALUE;
    OVERLAPPED ov{};

    int choice = 0;
    while (true) {
        MenuClient();
        std::cin >> choice;
        switch (choice) {
        case 1:
            std::cout << "Ожидание доступности канала...\n";
            if (WaitNamedPipe(L"\\\\.\\pipe\\pipe42", NMPWAIT_USE_DEFAULT_WAIT)) {
                // Подключаемся к каналу
                hPipe = CreateFile(
                    L"\\\\.\\pipe\\pipe42",
                    GENERIC_READ, // только чтение
                    0, // никто больше не сможет открыть файл
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED, // асинхронный режим чтения
                    NULL
                );
                if (hPipe != INVALID_HANDLE_VALUE) std::cout << "Успех! Соединение с сервером установлено.\n";
                else std::cout << "Ошибка! Не удалось подключиться. Код: " << GetLastError() << std::endl;
            }
            else std::cout << "Сервер не отвечает или канал не создан. Код: " << GetLastError() << std::endl;
            break;

        case 2:
            if (hPipe != INVALID_HANDLE_VALUE) {
                ZeroMemory(buffer, sizeof(buffer)); // очищаем буфер
                std::cout << "Переход в режим ожидания данных...\n";

                // Асинхронное чтение:
                BOOL res = ReadFileEx(hPipe, buffer, sizeof(buffer), &ov, FileIOCompletionRoutine);
                if (res) {
                    // Поток, запустивший операции чтения/записи должен приостановить себя
                    // с помощью SleepEx() и предоставить возможность 
                    // выполнения функции завершения FileIOCompletionRoutine
                    SleepEx(INFINITE, TRUE);
                }
                else std::cout << "Ошибка! ReadFileEx. Код: " << GetLastError() << std::endl;
            }
            else std::cout << "Сначала выполните подключение (Пункт 1).\n";
            break;

        case 3:
            if (hPipe != INVALID_HANDLE_VALUE) CloseHandle(hPipe);
            return 0;

        default:
            std::cout << "Неверный пункт меню.\n";
        }
    }
    return 0;
}

VOID WINAPI FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
    if (dwErrorCode == 0) {
        std::cout << "\n--- Уведомление от системы ---";
        std::cout << "\nАсинхронное чтение завершено успешно.";
        std::cout << "\nПолучено байт: " << dwNumberOfBytesTransfered; 
        std::cout << "\nСообщение от сервера: " << buffer << "\n";
    }
    else std::cout << "\nОшибка асинхронного чтения. Код: " << dwErrorCode << std::endl;
}

void MenuClient() {
    std::cout << "\n--- Меню Клиента ---\n";
    std::cout << "1. Подключиться к каналу\n";
    std::cout << "2. Считать данные (ReadFileEx)\n";
    std::cout << "3. Выход\n";
    std::cout << "Выбор: ";
}