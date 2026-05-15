/*
   Лабораторная работа 4. Межпроцессное взаимодействие
   Задание 4.2. Код: сервер
   Вариант 1 организации асинхронного в/в через события

   Выполнял: Суханов М.А.
   Группа:   4313
   ВУЗ:      СПбГЭТУ "ЛЭТИ"
   Год:      2026
*/

#include <windows.h>
#include <iostream>
#include <string>

void MenuServer();

void WaitResult(DWORD wres);

int main() {
    setlocale(LC_ALL, "Russian");

    HANDLE hPipe = INVALID_HANDLE_VALUE;
    OVERLAPPED ov{}; // используем структуру перекрывающегося асинхронного в/в
    ov.hEvent = CreateEvent(
        NULL,
        TRUE,  // сброс вручную
        FALSE, // изначально занятое
        NULL
    );
    if (ov.hEvent == NULL) {
        std::cout << "Ошибка создания события. Код ошибки: " << GetLastError() << std::endl;
        return 1;
    }

    int choice = 0;
    while (true) {
        MenuServer();
        std::cin >> choice;
        switch (choice) {
        case 1:
            if (hPipe == INVALID_HANDLE_VALUE) {
                hPipe = CreateNamedPipe(
                    L"\\\\.\\pipe\\pipe42",
                    PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,  // запись | асинхронный режим
                    PIPE_TYPE_MESSAGE |  // данные записываются в канал как сообщения
                    PIPE_READMODE_MESSAGE |  // данные считываются из канала как сообщения
                    PIPE_WAIT,  // обмен происходит в блокирующем режиме
                    1,  // макс. кол-во каналов
                    512, 512,  // размер буферов на отправку и прием (со стороны сервера)
                    5000,  // время ожидания по умолчанию для функции WaitNamedPipe ()
                    NULL
                );
                if (hPipe != INVALID_HANDLE_VALUE) std::cout << "Успех! Канал создан.\n";
                else std::cout << "Ошибка создания канала. Код: " << GetLastError() << std::endl;
            }
            else std::cout << "Канал уже существует.\n";
            break;

        case 2:
            if (hPipe != INVALID_HANDLE_VALUE) {
                std::cout << "Ожидание подключения клиента...\n";
                ResetEvent(ov.hEvent); // переводим событие в занятое состояние
                ConnectNamedPipe(hPipe, &ov); // устанавливаем соединение
                DWORD waitResult = WaitForSingleObject(ov.hEvent, 10000); // ожидаем завершения подключения
                WaitResult(waitResult);
            }
            else std::cout << "Сначала создайте канал (Пункт 1).\n";
            break;

        case 3:
            if (hPipe != INVALID_HANDLE_VALUE) {
                std::string message;
                std::cout << "Введите сообщение: ";
                std::cin.ignore(); // очистка буфера от предыдущего ввода
                std::getline(std::cin, message);
                size_t lenmsg = message.length();
                DWORD lpBuf; // для информативности
                ResetEvent(ov.hEvent); // переводим событие в занятое состояние

                // Асинхронная запись:
                if (WriteFile(hPipe, message.c_str(), lenmsg + 1, &lpBuf, &ov)) {
                   std::cout << "Успех! Записано: " << lpBuf << " байт\n";
                }
                else if (GetLastError() == ERROR_IO_PENDING) {
                    std::cout << "Выполняется асинхронная запись...";
                    while (!HasOverlappedIoCompleted(&ov)) { // проверяем статус незавершенной операции 
                        std::cout << ".";                    // асинхронного в/в используя макрос
                        Sleep(100);
                    }
                    std::cout << "\nУспех! Запись завершена.\n";
                }
                else std::cout << "Ошибка! Запись не удалась. Код: " << GetLastError() << std::endl;
            }
            else std::cout << "Канал не создан или клиент не подключен.\n";
            break;

        case 4:
            if (hPipe != INVALID_HANDLE_VALUE) {
                DisconnectNamedPipe(hPipe); // откл. от канала
                std::cout << "Успех! Клиент отключен от канала.\n";
            }
            else std::cout << "Канал не существует.\n";
            break;

        case 5:
            if (hPipe != INVALID_HANDLE_VALUE) {
                DisconnectNamedPipe(hPipe); // откл. от канала
                CloseHandle(hPipe); // закрываем его дескриптор
            }
            CloseHandle(ov.hEvent);
            return 0;

        default:
            std::cout << "Неверный пункт меню.\n";
        }
    }
    return 0;
}

void MenuServer() {
    std::cout << "\n--- Меню Сервера ---\n";
    std::cout << "1. Создать именованный канал\n";
    std::cout << "2. Ожидать подключения клиента\n";
    std::cout << "3. Записать данные в канал (асинхронно)\n";
    std::cout << "4. Отключить клиента\n";
    std::cout << "5. Выход\n";
    std::cout << "Выбор: ";
}

void WaitResult(DWORD wres) {
    switch (wres) {
    case WAIT_OBJECT_0: std::cout << "Успех! Клиент подключен.\n"; break;
    case WAIT_TIMEOUT: std::cout << "Тайм-аут: никто не подключился за 10 секунд.\n"; break;
    case WAIT_FAILED: std::cout << "Ошибка синхронизации. Код: " << GetLastError() << std::endl; break;
    }
}