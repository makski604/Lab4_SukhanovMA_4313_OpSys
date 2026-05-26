#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

int main() {
    setlocale(LC_ALL, "Russian");

    HANDLE hStartEvent = CreateEvent(
        NULL, 
        TRUE,  // сброс вручную
        FALSE, // изначально занятое
        L"Global\\StartSimSignal"
    );
    if (hStartEvent == NULL) {
        std::cerr << "Ошибка создания события: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Нажмите ENTER для одновременного старта...";
    std::cin.get();
    // переводим событие в свободное состояние
    if (SetEvent(hStartEvent)) std::cout << "Сигнал отправлен успешно! Все процессы начали работу.";
    else std::cout << "Ошибка SetEvent: " << GetLastError() << std::endl;
    Sleep(2000);
    CloseHandle(hStartEvent);

    return 0;
}
