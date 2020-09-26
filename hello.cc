#include <napi.h>
#include "build\stdafx.h"
#include "build\imageUtilities.h"

// input: none
// output:
//	focusedWindow: handle of the focused windows
//	rc:	window rectangle coordinates
//	focusedPID: process ID of the window
//
HRESULT _getFocusedWindowInfo(HWND *focusedWindow, RECT *rc, DWORD *focusedPID)
{
	*focusedWindow = GetForegroundWindow();
	GetWindowRect(*focusedWindow, rc);
	GetWindowThreadProcessId(*focusedWindow, focusedPID);
	return S_OK;
}

// input:
//	pszExeFileName: process executable file path
// output:
//	imageBuffer: contains the icon in RGB32
//	width, height, size of the icon
//
HRESULT _getProcessIcon(LPCSTR pszExeFileName, BYTE **imageBuffer, DWORD *width, DWORD *height, DWORD *imageSize)
{
	HICON icon = ExtractIconA(GetModuleHandle(NULL), pszExeFileName, 0);
	auto iconInfo = imageUtilities::_MyGetIconInfo(icon);
	auto hbmp = imageUtilities::_HICONtoHBITMAP(icon, iconInfo.nWidth, iconInfo.nWidth);

	//create
	HDC hdcScreen = GetDC(NULL);
	HDC hdc = CreateCompatibleDC(hdcScreen);
	SelectObject(hdc, hbmp);

	BYTE *imgBuffer = new BYTE[iconInfo.nWidth * iconInfo.nHeight * 4];

	auto x = imageUtilities::_bitmapToBytes(hbmp, (int&)*width, (int&)*height);
	memcpy(imgBuffer, &x[0], (*width)*(*height) * 4);

	*imageBuffer = imgBuffer;
	*imageSize = x.size();

	//release
	DeleteDC(hdc);
	DeleteObject(hbmp);
	ReleaseDC(NULL, hdcScreen);
	return S_OK;
}


// input:
//	focusedWindow, rc
// output:
//	imageBuffer: contains the snapshot of the window in RGB32
//	width, height, size of the snapshot
//
HRESULT _getWindowSnapshot(HWND focusedWindow, RECT rc, BYTE **imageBuffer, DWORD *width, DWORD *height, DWORD *imageSize)
{
	//create
	HDC hdcScreen = GetDC(NULL);
	HDC hdc = CreateCompatibleDC(hdcScreen);
	HBITMAP hbmp = CreateCompatibleBitmap(hdcScreen,
		(rc.right - rc.left), (rc.bottom - rc.top));
	SelectObject(hdc, hbmp);

	auto imgSize = (rc.right - rc.left) * (rc.bottom - rc.top) * 4;
	BYTE *imgBuffer = new BYTE[(rc.right - rc.left) * (rc.bottom - rc.top) * 4];

	//Print to memory hdc
	PrintWindow(focusedWindow, hdc, PW_RENDERFULLCONTENT);

	auto x = imageUtilities::_bitmapToBytes(hbmp, (int&)*width, (int&)*height);
	memcpy(imgBuffer, &x[0], (*width)*(*height)*4);

	*imageBuffer = imgBuffer;
	*imageSize = x.size();

	//release
	DeleteDC(hdc);
	DeleteObject(hbmp);
	ReleaseDC(NULL, hdcScreen);
	return S_OK;
}

// input:
//	PID: process ID
// output:
//	filename: file path of the process
//
HRESULT _getProcessFilename(DWORD PID, TCHAR *filename)
{
	auto processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
	if (processHandle != NULL)
	{
		if (GetModuleFileNameEx(processHandle, NULL, filename, MAX_PATH) == 0)
		{
			printf("GetModuleFileName failed\n");
		}
		else
		{
			//	printf("Module filename is: %S\n", filename);
		}
		CloseHandle(processHandle);
	}
	else
	{
		printf("OpenProcess failed\n");
	}
	return S_OK;
}

// input: none
// output:	object with keys
//	filename (Napi::String): the path of the executable
//	
Napi::Object getFocusedImageAndDetail(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::Object obj = Napi::Object::New(env);

	// get  focused window info

	HWND focusedWindow;  RECT rc; DWORD focusedPID;
	_getFocusedWindowInfo(&focusedWindow, &rc, &focusedPID);

	// get file name of the program

	TCHAR filename[MAX_PATH];
	_getProcessFilename(focusedPID, filename);

	// get snapshot of the window

	BYTE *imageBuffer = nullptr; DWORD imageSize = 0, snapshotWidth = 0, snapshotHeight = 0;
	_getWindowSnapshot(focusedWindow, rc, &imageBuffer, &snapshotWidth, &snapshotHeight, &imageSize);

	// get icon of the process

	BYTE *iconBuffer = nullptr; DWORD iconSize = 0, iconWidth = 0, iconHeight = 0;
	_getProcessIcon(filename, &iconBuffer, &iconWidth, &iconHeight, &iconSize);
	
	obj.Set(Napi::String::New(env, "filename"), filename);
	obj.Set(Napi::String::New(env, "snapshot"), Napi::Buffer<uint8_t>::New(env, imageBuffer, imageSize));
	obj.Set(Napi::String::New(env, "snapshotWidth"), snapshotWidth);
	obj.Set(Napi::String::New(env, "snapshotHeight"), snapshotHeight);
	obj.Set(Napi::String::New(env, "icon"), Napi::Buffer<uint8_t>::New(env, iconBuffer, iconSize));
	obj.Set(Napi::String::New(env, "iconWidth"), iconWidth);
	obj.Set(Napi::String::New(env, "iconHeight"), iconHeight);

	return obj;
}

// input: none
// output:	object with keys
//	
Napi::Object getFocusedDetail(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::Object obj = Napi::Object::New(env);
	uint8_t *x = new uint8_t[4];
	return obj;
}

// input: none
// output: true (microphone active) / false (not active)
//
Napi::Boolean isMicrophoneActive(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	return Napi::Boolean::New(env, false);
}

// input: none
// output: true (webcam active) / false (not active)
//
Napi::Boolean isWebcamActive(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	return Napi::Boolean::New(env, false);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	exports.Set(Napi::String::New(env, "getFocusedImageAndDetail"),
		Napi::Function::New(env, getFocusedImageAndDetail));
	exports.Set(Napi::String::New(env, "getFocusedDetail"),
		Napi::Function::New(env, getFocusedDetail));
	exports.Set(Napi::String::New(env, "isMicrophoneActive"),
		Napi::Function::New(env, isMicrophoneActive));
	exports.Set(Napi::String::New(env, "isWebcamActive"),
		Napi::Function::New(env, isWebcamActive));

	SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
	int result = SetProcessDPIAware();

	return exports;
}

NODE_API_MODULE(hello, Init)
