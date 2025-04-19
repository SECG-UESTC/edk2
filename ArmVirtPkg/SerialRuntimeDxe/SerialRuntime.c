#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/PcdLib.h>
#include <Library/PL011UartLib.h>
#include <Library/SerialPortLib.h>
#include <Pi/PiBootMode.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Guid/EarlyPL011BaseAddress.h>

STATIC UINTN          mSerialBaseAddress;
STATIC RETURN_STATUS  mPermanentStatus = RETURN_SUCCESS;

RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  VOID                            *Hob;
  RETURN_STATUS                   Status;
  CONST EARLY_PL011_BASE_ADDRESS  *UartBase;
  UINTN                           SerialBaseAddress;
  UINT64                          BaudRate;
  UINT32                          ReceiveFifoDepth;
  EFI_PARITY_TYPE                 Parity;
  UINT8                           DataBits;
  EFI_STOP_BITS_TYPE              StopBits;

  if (mSerialBaseAddress != 0) {
    return RETURN_SUCCESS;
  }

  if (RETURN_ERROR (mPermanentStatus)) {
    return mPermanentStatus;
  }

  Hob = GetFirstGuidHob (&gEarlyPL011BaseAddressGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof *UartBase)) {
    Status = RETURN_NOT_FOUND;
    goto Failed;
  }

  UartBase = GET_GUID_HOB_DATA (Hob);

  SerialBaseAddress = (UINTN)UartBase->DebugAddress;
  if (SerialBaseAddress == 0) {
    Status = RETURN_NOT_FOUND;
    goto Failed;
  }

  BaudRate         = (UINTN)PcdGet64 (PcdUartDefaultBaudRate);
  ReceiveFifoDepth = 0; // Use the default value for Fifo depth
  Parity           = (EFI_PARITY_TYPE)PcdGet8 (PcdUartDefaultParity);
  DataBits         = PcdGet8 (PcdUartDefaultDataBits);
  StopBits         = (EFI_STOP_BITS_TYPE)PcdGet8 (PcdUartDefaultStopBits);

  Status = PL011UartInitializePort (
             SerialBaseAddress,
             FixedPcdGet32 (PL011UartClkInHz),
             &BaudRate,
             &ReceiveFifoDepth,
             &Parity,
             &DataBits,
             &StopBits
             );
  if (RETURN_ERROR (Status)) {
    goto Failed;
  }

  mSerialBaseAddress = SerialBaseAddress;
  return RETURN_SUCCESS;

Failed:
  mPermanentStatus = Status;
  return Status;
}


EFI_STATUS
EFIAPI 
InitializeSerial (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  );

EFI_STATUS
EFIAPI
SerialPortWrite (
  IN UINT8 *Buffer,
  IN UINTN NumberOfBytes
  )
{
  if (!RETURN_ERROR (SerialPortInitialize ())) {
    PL011UartWrite (mSerialBaseAddress, Buffer, NumberOfBytes);
    return EFI_SUCCESS;
  }
  
  return EFI_TIMEOUT;
}

EFI_STATUS
EFIAPI SerialPortRead (
  OUT UINT8 *Buffer,
  IN UINTN NumberOfBytes
  )
{
 if (!RETURN_ERROR (SerialPortInitialize ())) {
    PL011UartRead (mSerialBaseAddress, Buffer, NumberOfBytes);
    return EFI_SUCCESS;
  }
 
  return EFI_TIMEOUT;
}

EFI_STATUS
EFIAPI
InitializeSerial (
  IN EFI_HANDLE ImageHandle,		
  IN EFI_SYSTEM_TABLE *SystemTable)
{
  SerialPortInitialize ();

  SystemTable->RuntimeServices->SerialWrite=SerialPortWrite;
  SystemTable->RuntimeServices->SerialRead=SerialPortRead;
  return EFI_SUCCESS;
}
