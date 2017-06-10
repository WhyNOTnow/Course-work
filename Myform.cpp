#include "Myform.h"
#include "StdAfx.h"

DWORD PID;
char buf[1024];

int showSV(DWORD pid, char* name)
{
	PID = pid;
	strcpy(buf,name);
	TCHAR szClassName[] = "Мой класс"; // строка с именем класса
	HWND hMainWnd; // создаём дескриптор будущего окошка
	MSG msg; // создём экземпляр структуры MSG для обработки сообщений
	WNDCLASSEX wc; // создаём экземпляр, для обращения к членам класса WNDCLASSEX
	wc.cbSize = sizeof(wc); // размер структуры (в байтах)
	wc.style = CS_HREDRAW | CS_VREDRAW; // стиль класса окошка
	wc.lpfnWndProc = WndProc; // указатель на пользовательскую функцию
	wc.lpszMenuName = NULL; // указатель на имя меню (у нас его нет)
	wc.lpszClassName = szClassName; // указатель на имя класса
	wc.cbWndExtra = NULL; // число освобождаемых байтов в конце структуры
	wc.cbClsExtra = NULL; // число освобождаемых байтов при создании экземпляра приложения
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO); // декриптор пиктограммы
	wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO); // дескриптор маленькой пиктограммы (в трэе)
	wc.hCursor = LoadCursor(NULL, IDC_ARROW); // дескриптор курсора
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // дескриптор кисти для закраски фона окна
	wc.hInstance = NULL; // указатель на строку, содержащую имя меню, применяемого для класса
	RegisterClassEx(&wc);
	
	char title[1024] = "Информация о процессе: ";
	// Функция, создающая окошко:
	hMainWnd = CreateWindow(
		szClassName, // имя класса
		strcat(title,name), // имя окошка (то что сверху)
		WS_OVERLAPPEDWINDOW , // режимы отображения окошка
		CW_USEDEFAULT, // позиция окошка по оси х
		NULL, // позиция окошка по оси у (раз дефолт в х, то писать не нужно)
		650, // ширина окошка
		400, // высота окошка (раз дефолт в ширине, то писать не нужно)
		(HWND)NULL, // дескриптор родительского окна
		NULL, // дескриптор меню
		HINSTANCE(NULL), // дескриптор экземпляра приложения
		NULL); // ничего не передаём из WndProc
	if (!hMainWnd){
		// в случае некорректного создания окошка (неверные параметры и тп):
		MessageBox(NULL, "Не получилось создать окно!", "Ошибка", MB_OK);
		return NULL;
	}
	ShowWindow(hMainWnd, SW_SHOWNORMAL); // отображаем окошко
	UpdateWindow(hMainWnd); // обновляем окошко
	while (GetMessage(&msg, NULL, NULL, NULL)){ // извлекаем сообщения из очереди, посылаемые фу-циями, ОС
		TranslateMessage(&msg); // интерпретируем сообщения
		DispatchMessage(&msg); // передаём сообщения обратно ОС
	}
	return msg.wParam; // возвращаем код выхода из приложения
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	PROCESS_MEMORY_COUNTERS pmc;
	HANDLE proc;

	HDC hDC; // создаём дескриптор ориентации текста на экране
	PAINTSTRUCT ps; // структура, сод-щая информацию о клиентской области (размеры, цвет и тп)
	RECT rect; // стр-ра, определяющая размер клиентской области
	COLORREF colorText = RGB(0, 0, 0); // задаём цвет текста

	char tmp[30];
	sprintf(tmp, "%d", PID);
	proc = OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_VM_READ,	FALSE, PID);
	if (NULL == proc)
		return NULL;
	GetProcessMemoryInfo(proc, &pmc, sizeof(pmc));
	int res = GetPriorityClass(proc);
	
	switch (uMsg){
	case WM_PAINT: // если нужно нарисовать, то:
		hDC = BeginPaint(hWnd, &ps); // инициализируем контекст устройства
		SetTextColor(hDC, colorText); // устанавливаем цвет контекстного устройства
		rect = { 0, 10, 400, 30 };
		DrawText(hDC, "Имя процесса: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 10, 600, 30 };
		DrawText(hDC, buf, -1, &rect, DT_LEFT); // рисуем текст
		rect = { 0, 30, 400, 50 };
		DrawText(hDC, "Идентификатор процесса: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 30, 600, 50 };
		DrawText(hDC, tmp, -1, &rect, DT_LEFT); // рисуем текст

		rect = { 0, 50, 400, 70 };
		DrawText(hDC, "Число дефектных страниц памяти: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 50, 600, 70 };
		sprintf(tmp, "%d", pmc.PageFaultCount);
		DrawText(hDC, tmp, -1, &rect, DT_LEFT); // рисуем текст

		rect = { 0, 70, 400, 90 };
		DrawText(hDC, "Пик использования физической памяти: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 70, 600, 90 };
		sprintf(tmp, "%d B", pmc.PeakWorkingSetSize);
		DrawText(hDC, tmp, -1, &rect, DT_LEFT); // рисуем текст

		rect = { 0, 90, 400, 110 };
		DrawText(hDC, "Текущее использование памяти: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 90, 600, 110 };
		sprintf(tmp, "%d B", pmc.WorkingSetSize);
		DrawText(hDC, tmp, -1, &rect, DT_LEFT); // рисуем текст

		rect = { 0, 110, 400, 130 };
		DrawText(hDC, "Пик использования выгружаемого пула ядра: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 110, 600, 130 };
		sprintf(tmp, "%d B", pmc.QuotaPeakPagedPoolUsage);
		DrawText(hDC, tmp, -1, &rect, DT_LEFT); // рисуем текст

		rect = { 0, 130, 400, 150 };
		DrawText(hDC, "Текущее использование выгружаемого пула ядра: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 130, 600, 150 };
		sprintf(tmp, "%d B", pmc.QuotaPagedPoolUsage);
		DrawText(hDC, tmp, -1, &rect, DT_LEFT); // рисуем текст

		rect = { 0, 150, 400, 170 };
		DrawText(hDC, "Пик использования невыгружаемого пула ядра: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 150, 600, 170 };
		sprintf(tmp, "%d B", pmc.QuotaPeakNonPagedPoolUsage);
		DrawText(hDC, tmp, -1, &rect, DT_LEFT); // рисуем текст

		rect = { 0, 170, 400, 190 };
		DrawText(hDC, "Текущее использование невыгружаемого пула ядра: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 170, 600, 190 };
		sprintf(tmp, "%d B", pmc.QuotaNonPagedPoolUsage);
		DrawText(hDC,tmp, -1, &rect, DT_LEFT); // рисуем текст

		rect = { 0, 190, 400, 210 };
		DrawText(hDC, "Текущеее использование файла подкачки: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 190, 600, 210 };
		sprintf(tmp, "%d B", pmc.PagefileUsage);
		DrawText(hDC, tmp, -1, &rect, DT_LEFT); // рисуем текст

		rect = { 0, 210, 400, 230 };
		DrawText(hDC, "Пик использования файла подкачки: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 210, 600, 230 };
		sprintf(tmp, "%d B", pmc.PeakPagefileUsage);
		DrawText(hDC, tmp, -1, &rect, DT_LEFT); // рисуем текст

		rect = { 0, 230, 400, 250 };
		DrawText(hDC, "Приоритет процесса: ", -1, &rect, DT_LEFT); // рисуем текст
		rect = { 400, 230, 600, 250 };
		sprintf(tmp, "%d", res);
		DrawText(hDC, tmp, -1, &rect, DT_LEFT); // рисуем текст

		EndPaint(hWnd, &ps); // заканчиваем рисовать
		break;
	case WM_DESTROY: // если окошко закрылось, то:
		//PostQuitMessage(NULL); // отправляем WinMain() сообщение WM_QUIT
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam); // если закрыли окошко
	}
	return NULL; // возвращаем значение
}
