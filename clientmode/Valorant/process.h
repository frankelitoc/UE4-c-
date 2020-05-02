#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <Psapi.h>
#include <stdio.h>
#pragma comment(lib, "Psapi.lib")

class PIDManager
{
public:
	PIDManager();
	~PIDManager();
	static int GetProcessIdByName(LPCTSTR szProcess);
	static BOOL EnableDebugPriv();
	static DWORD_PTR GetModuleBase(DWORD dwPid, LPCTSTR szModName);
	static int GetProcessThreadNumByID(DWORD dwPID);
	static int GetAowProcId();
	static void killProcessByName(LPCWSTR name);
};