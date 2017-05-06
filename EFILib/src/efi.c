#include "../include/efi.h"

#define EFILIB_PRINTF_BUFFER_SIZE 1024

static EFI_HANDLE* hndl;
static EFI_SYSTEM_TABLE* systbl;
static UINTN cursor_x = 0, cursor_y = 0;

CHAR16 chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

UINT32 uint32ToString(UINT32 v, UINT8 base, CHAR16* buffer) {

	UINT32 num = 0;

	while (v > 0) {
		if (num > 10) break;
		buffer[num++] = chars[v % base];
		v /= base;
	}

	CHAR16 tmp[10];
	memcpy(tmp, buffer, sizeof(tmp));

	UINT32 start = 0;

	if (base == 16) {
		for (UINT32 i = 0; i < 9; i++) buffer[i] = '0';
		start = 8 - num;
	}

	for (UINT32 i = 0; i < num; i++) {
		buffer[i + start] = tmp[num - i - 1];
	}

	return num;
}

UINT32 uint64ToString(UINT64 v, UINT8 base, CHAR16* buffer) {

	UINT32 num = 0;

	while (v > 0) {
		if (num > 19) break;
		buffer[num++] = chars[v % base];
		v /= base;
	}

	CHAR16 tmp[19];
	memcpy(tmp, buffer, sizeof(tmp));

	UINT32 start = 0;

	if (base == 16) {
		for (UINT32 i = 0; i < 17; i++) buffer[i] = '0';
		start = 16 - num;
	}

	for (UINT32 i = 0; i < num; i++) {
		buffer[i + start] = tmp[num - i - 1];
	}

	return num;
}

#pragma function(memset)
VOID* memset(VOID* dest, INT32 v, UINTN size) {
	for (UINTN i = 0; i < size; i++) ((UINT8*)dest)[i] = v;
	return dest;
}

#pragma function(memcpy)
VOID* memcpy(VOID* dest, CONST VOID* src, UINTN size) {
	for (UINTN i = 0; i < size; i++) ((UINT8*)dest)[i] = ((UINT8*)src)[i];
	return dest;
}

BOOLEAN InitializeLib(EFI_HANDLE* handle, EFI_SYSTEM_TABLE* systemTable) {
	hndl = handle;
	systbl = systemTable;

	if (hndl == 0) return 0;
	if (systbl == 0) return 0;

	return 1;
}

#pragma function(strlen)
UINTN strlen(CONST CHAR16* string) {
	UINTN len = 0;
	
	while (string[len++] != 0);

	return len;
}

VOID SetCursor(UINTN x, UINTN y) {
	systbl->ConOut->SetCursorPosition(systbl->ConOut, cursor_x = x, cursor_y = y);
}

VOID print(CONST CHAR16* string) {
	UINTN len = strlen(string);

	BOOLEAN alloc = FALSE;

	for (UINTN i = 0; i < len; i++) {
		if (string[i] == '\n') {
			alloc = TRUE;
			break;
		}
	}

	if (alloc) {
		CHAR16* tmp = 0;
		UINTN size = len * sizeof(CHAR16);
		tmp = AllocPool(EfiBootServicesData, size);
		memset(tmp, 0, size);


		UINTN last = 0;

		for (UINTN i = 0; i < len; i++) {
			if (string[i] == (CHAR16)'\n') {
				UINTN size = (i - last) * sizeof(CHAR16);
				memcpy(tmp, string + last, size);
				if (size != 0) systbl->ConOut->OutputString(systbl->ConOut, tmp);
				memset(tmp, 0, size);

				SetCursor(0, cursor_y + 1);

				last = i + 1;
			}
		}

		if (last <= len) systbl->ConOut->OutputString(systbl->ConOut, string + last);
		
		Free((VOID*)tmp, 0);
	} else {
		systbl->ConOut->OutputString(systbl->ConOut, string);
	}

}

VOID println(CONST CHAR16* string) {
	print(string);
	SetCursor(0, cursor_y + 1);
}


VOID printf(CONST CHAR16* format, ...) {

	va_list list;
	va_start(list, format);

	vprintf(format, list);
}

VOID vprintf(CONST CHAR16* format, va_list list) {
	CHAR16* buffer = AllocPool(EfiBootServicesData, EFILIB_PRINTF_BUFFER_SIZE);

	vsprintf(buffer, EFILIB_PRINTF_BUFFER_SIZE, format, list);
	print(buffer);

	Free(buffer, 0);
}

UINTN sprintf(CHAR16* buffer, UINTN bufferSize, CONST CHAR16* format, ...) {

	va_list list;
	va_start(list, format);

	return vsprintf(buffer, bufferSize, format, list);
}

UINTN vsprintf(CHAR16* buffer, UINTN bufferSize, CONST CHAR16* format, va_list list) {

	UINTN len = strlen(format);
	UINTN printed = 0;

	for (UINTN i = 0; i < len; i++) {
		if (format[i] == '%') {
			i++;
			switch (format[i]) {
				case 'c':
				{
					buffer[printed++] = (CHAR16)va_arg(list, UINTN);
					break;
				}
				case 'C':
				{
					buffer[printed++] = (CHAR16)va_arg(list, UINTN);
					break;
				}
				case 'u':
				{
					CHAR16 tmp[10];

					UINT32 num = uint32ToString((UINT32)va_arg(list, UINTN), 10, tmp);

					memcpy(buffer + printed, tmp, num * sizeof(CHAR16));
					printed += num;
					break;
				}					
				case 'U':
				{
					CHAR16 tmp[19];

					UINT32 num = uint64ToString(va_arg(list, UINT64), 10, tmp);

					memcpy(buffer + printed, tmp, num * sizeof(CHAR16));
					printed += num;
					break;
				}
				case 'h':
				{
					CHAR16 tmp[10];

					uint32ToString((UINT32)va_arg(list, UINTN), 16, tmp);

					memcpy(buffer + printed, tmp, 8 * sizeof(CHAR16));
					printed += 8;
					break;
				}
				case 'H':
				{
					CHAR16 tmp[19];

					uint64ToString(va_arg(list, UINT64), 16, tmp);

					memcpy(buffer + printed, tmp, 16 * sizeof(CHAR16));
					printed += 16;
					break;
				}
				case 'S':
				case 's':
				{
					CHAR16* string = va_arg(list, CHAR16*);
					UINTN len = strlen(string);

					memcpy(buffer + printed, string, len * sizeof(CHAR16));
					printed += len;
					break;
				}
			}
		} else {
			buffer[printed++] = format[i];
		}
	}

	buffer[printed++] = 0;

	return printed;
}

VOID* AllocPages(EFI_ALLOCATE_TYPE type, EFI_MEMORY_TYPE memType, UINTN pages) {
	UINT8* data = 0;
	if (systbl->BootServices->AllocatePages(AllocateAnyPages, EfiBootServicesData, pages, (EFI_PHYSICAL_ADDRESS*)&data) == EFI_SUCCESS)	return (VOID*)data;
	return 0;
}

VOID* AllocPool(EFI_MEMORY_TYPE type, UINTN size) {
	UINT8* data = 0;
	systbl->BootServices->AllocatePool(type, size, (VOID**)&data);
	return (VOID*)data;
}

VOID Free(VOID* address, UINTN pages) {
	if (address == 0) return;
	if (pages > 0) {
		systbl->BootServices->FreePages((EFI_PHYSICAL_ADDRESS*)address, pages);
	} else {
		systbl->BootServices->FreePool(address);
	}
}

VOID Sleep(UINTN microseconds) {
	systbl->BootServices->Stall(microseconds);
}

BOOLEAN ExitBootServices(UINTN key) {

	EFI_STATUS res = systbl->BootServices->ExitBootServices(hndl, key);

	if (res == EFI_SUCCESS) {
		systbl->BootServices = 0;
		systbl->ConIn = 0;
		systbl->ConOut = 0;
		systbl->ConsoleInHandle = 0;
		systbl->ConsoleOutHandle = 0;
		systbl->StandardErrorHandle = 0;
		systbl->StdErr = 0;
		return TRUE;
	}

	return FALSE;
}