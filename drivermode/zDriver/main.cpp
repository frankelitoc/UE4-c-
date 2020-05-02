#include "main.h"

//#pragma alloc_text(INIT, DriverEntry)

//This is a .asm file
/* 
.code
DispatchHook proc
	add rsp, 8h
	mov rax, 0DEADBEEFCAFEBEEFh
	jmp rax
DispatchHook endp

end
*/

extern "C" void DispatchHook();

PDRIVER_DISPATCH ACPIOriginalDispatch = 0;





NTSTATUS ProcessReadWriteMemory(PEPROCESS SourceProcess, PVOID SourceAddress, PEPROCESS TargetProcess, PVOID TargetAddress, SIZE_T Size)
{
	SIZE_T Bytes = 0;

	if (NT_SUCCESS(MmCopyVirtualMemory(SourceProcess, SourceAddress, TargetProcess, TargetAddress, Size, UserMode, &Bytes)))
		return STATUS_SUCCESS;
	else
		return STATUS_ACCESS_DENIED;
}


NTSTATUS CustomDispatch(PDEVICE_OBJECT device, PIRP irp)
{
	PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);
	NTSTATUS Status;
	ULONG BytesIO = 0;

	//Here you can do your custom calls

	if (ioc->Parameters.DeviceIoControl.IoControlCode == IOCTL_DISK_GET_DRIVE_GEOMETRY)
	{
		CUSTOM_IOCTL_CALL* Buffer = (CUSTOM_IOCTL_CALL*)irp->AssociatedIrp.SystemBuffer;

		if (Buffer->Filter == 0xDEADBEEFCAFEBEEF)
		{
			if (Buffer->ControlCode == READ_PROCESS_MEMORY_IOCTL)
			{
				READ_PROCESS_MEMORY* UserlandBuffer = (READ_PROCESS_MEMORY*)irp->AssociatedIrp.SystemBuffer;

				PEPROCESS TargetProcess = 0;

				if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)UserlandBuffer->ProcessId, &TargetProcess))) {

					Status = ProcessReadWriteMemory(TargetProcess, (PVOID)UserlandBuffer->ProcessAddress, IoGetCurrentProcess(), (PVOID)UserlandBuffer->OutBuffer, UserlandBuffer->Length);
					ObfDereferenceObject(TargetProcess);
				}
				Status = STATUS_SUCCESS;
				BytesIO = sizeof(READ_PROCESS_MEMORY);
			}
			else if (Buffer->ControlCode == WRITE_PROCESS_MEMORY_IOCTL)
			{
				WRITE_PROCESS_MEMORY* UserlandBuffer = (WRITE_PROCESS_MEMORY*)irp->AssociatedIrp.SystemBuffer;

				PEPROCESS TargetProcess = 0;

				if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)UserlandBuffer->ProcessId, &TargetProcess))) {
					Status = ProcessReadWriteMemory(IoGetCurrentProcess(), (PVOID)UserlandBuffer->InBuffer, TargetProcess, (PVOID)UserlandBuffer->ProcessAddress, UserlandBuffer->Length);

					ObfDereferenceObject(TargetProcess);
				}
				Status = STATUS_SUCCESS;
				BytesIO = sizeof(WRITE_PROCESS_MEMORY);
			}
			else if (Buffer->ControlCode == GET_PROCESS_BASE_IOCTL)
			{
				GET_PROCESS_BASE* UserlandBuffer = (GET_PROCESS_BASE*)irp->AssociatedIrp.SystemBuffer;

				PEPROCESS TargetProcess = 0;

				UserlandBuffer->ProcessBaseAddres = -1;

				if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)UserlandBuffer->ProcessId, &TargetProcess))) {
					UserlandBuffer->ProcessBaseAddres = (unsigned __int64)PsGetProcessSectionBaseAddress(TargetProcess);

					ObfDereferenceObject(TargetProcess);
				}
				Status = STATUS_SUCCESS;
				BytesIO = sizeof(GET_PROCESS_BASE);
			}
			else if (Buffer->ControlCode == GET_PROCESS_PEB_IOCTL)
			{
				GET_PROCESS_PEB* UserlandBuffer = (GET_PROCESS_PEB*)irp->AssociatedIrp.SystemBuffer;

				PEPROCESS TargetProcess = 0;

				UserlandBuffer->ProcessBaseAddres = -1;

				if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)UserlandBuffer->ProcessId, &TargetProcess))) {
					UserlandBuffer->ProcessBaseAddres = (unsigned __int64)PsGetProcessPeb(TargetProcess);

					ObfDereferenceObject(TargetProcess);
				}
				Status = STATUS_SUCCESS;
				BytesIO = sizeof(GET_PROCESS_PEB);
			}

			irp->IoStatus.Status = Status;
			irp->IoStatus.Information = BytesIO;

			IofCompleteRequest(irp, IO_NO_INCREMENT);
			return Status;
		}
	}

	return ACPIOriginalDispatch(device, irp);
}

NTSTATUS DriverEntry(PVOID lpBaseAddress, DWORD32 dwSize)
{
	RetrieveMmUnloadedDriversData();
	ClearPiDDBCacheTable();

	UNICODE_STRING iqvw64e = RTL_CONSTANT_STRING(L"iqvw64e.sys");
	ClearMmUnloadedDrivers(&iqvw64e, true);

	PDRIVER_OBJECT ACPIDriverObject = nullptr;

	UNICODE_STRING DriverObjectName = RTL_CONSTANT_STRING(L"\\Driver\\ACPI");
	ObReferenceObjectByName(&DriverObjectName, OBJ_CASE_INSENSITIVE, 0, 0, *IoDriverObjectType, KernelMode, 0, (PVOID*)&ACPIDriverObject);

	if (ACPIDriverObject)
	{
		ACPIOriginalDispatch = ACPIDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];

		ULONG64 DispatchHookAddr = (ULONG64)DispatchHook;

		*(ULONG64*)(DispatchHookAddr + 0x6) = (ULONG64)CustomDispatch;

		ULONG64 TraceMessageHookInst = FindPattern((UINT64)ACPIDriverObject->DriverStart, ACPIDriverObject->DriverSize, (BYTE*)"\xB8\x0C\x00\x00\x00\x44\x0F\xB7\xC8\x8D\x50\x00", "xxxxxxxxxxx?");

		if (TraceMessageHookInst)
		{
			TraceMessageHookInst += 0xC;

			ULONG64 pfnWppTraceMessagePtr = (ULONG64)ResolveRelativeAddress((PVOID)TraceMessageHookInst, 3, 7);

			if (pfnWppTraceMessagePtr)
			{
				*(ULONG64*)(pfnWppTraceMessagePtr) = DispatchHookAddr;

				ACPIDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)TraceMessageHookInst;

				Printf("ACAPI IRP_MJ_DEVICE_CONTROL Hooked!\n");
			}
		}
	}
	return STATUS_SUCCESS;
}
