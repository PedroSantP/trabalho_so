/*
LogoFAILDemo - demonstração didática e primitiva LogoFAIL

AVISO: TRECHOS DESTE CÓDIGO FORAM DELIBERADAMENTE OMITIDOS POR RAZÕES ÉTICAS E DE SEGURANÇA

As partes omitidas correspondem ao segundo estágio da cadeia de ataque (conversão em ACE) e derivam do código público de terceiros, 
utilizado como referência para análise em ambiente controlado. Tais trechos NÃO são redistribuídos neste artefato.

O que este módulo demonstra:
BMP na ESP -> parser -> modelagem aritmética -> detecção de inconsistência -> transferência de controle para marcador interno

Não há corrupção de memória, desvio real de ponteiro de instrução e nem payload.

AUTORIA

O código presente neste arquivo é de autoria própria; os créditos a terceiros referem-se exclusivamente
aos trechos omitidos, não redistribuídos.

CRÉDITOS

A concepção das técnicas do segundo estágio (ACE) é de autoria de TheMalwareGuardian, publicada em:
https://github.com/TheMalwareGuardian/Exploitation-Technique-UEFI-SecureBoot-Bypass-gSecurity2-Corruption, sob licença GPL-3.0.
Todos os créditos pelas técnicas originais são do referido autor.

Este trabalho aplica-se apenas em contexto acadêmico delimitado e não reivindica autoria sobre essas técnicas.

ESCOPO

Este código é read-only quanto a variáveis de segurança e não deve ser redistribuído fora do escopo do projeto.
Disponibilizado em repositório privado institucional, para avaliação acadêmica.

*/

#include <Uefi.h>
#include "BmpMiniParser.h"

#include <Guid/FileInfo.h>
#include <Library/BmpSupportLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>


#define LOGO_FILE_PATH       L"\\EFI\\LogoFailDemo\\logo.bmp"
#define MAX_BMP_FILE_SIZE    (4U * 1024U * 1024U)

STATIC
EFI_STATUS
CheckBootOrder (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT16      *BootOrder;
  UINTN       BootOrderSize;
  UINTN       Index;
  UINT32      Attributes;

  BootOrder     = NULL;
  BootOrderSize = 0;

  //
  // First query: obtain the required buffer size.
  //
  Status = gRT->GetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  &Attributes,
                  &BootOrderSize,
                  NULL
                  );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    Print (
      L"[LogoFailDemo] BootOrder size query failed: %r\n",
      Status
      );
    return Status;
  }

  if ((BootOrderSize < 2 * sizeof (UINT16)) ||
      ((BootOrderSize % sizeof (UINT16)) != 0)) {
    return EFI_COMPROMISED_DATA;
  }

  BootOrder = AllocatePool (BootOrderSize);

  if (BootOrder == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read current BootOrder.
  //
  Status = gRT->GetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  &Attributes,
                  &BootOrderSize,
                  BootOrder
                  );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Print (L"\n[LogoFailDemo] BootOrder:\n");

  for (Index = 0;
       Index < BootOrderSize / sizeof (UINT16);
       Index++) {
    Print (L"  Boot%04X\n", BootOrder[Index]);
  }

Done:

  FreePool (BootOrder);
  return Status;
}


STATIC
VOID
CheckSecureBootStatus (
  VOID
  )
{
  EFI_STATUS Status;
  UINT8      SecureBoot;
  UINTN      Size;
  UINT32     Attributes;

  Size = sizeof (SecureBoot);

  Status = gRT->GetVariable (
                  L"SecureBoot",
                  &gEfiGlobalVariableGuid,
                  &Attributes,
                  &Size,
                  &SecureBoot
                  );

  if (EFI_ERROR (Status)) {
    Print (L"[LogoFailDemo] GetVariable failed: %r\n", Status);
    return;
  }

  Print (L"[LogoFailDemo] SecureBoot state: %u\n", SecureBoot);

  Status = CheckBootOrder ();

  DEBUG ((
    DEBUG_ERROR,
    "[LogoFailDemo] BootOrder test: %r\n",
    Status
  ));

}

STATIC
EFI_STATUS
SimulatedAceMarker (
  IN CONST BMP_ALLOCATION_MODEL  *Model
  )
{
  DEBUG ((
    DEBUG_ERROR,
    "\n[LogoFailDemo] ==========================================\n"
    "[LogoFailDemo] SIMULATED ACE MARKER REACHED\n"
    "[LogoFailDemo] Trigger: BMP allocation arithmetic mismatch\n"
    "[LogoFailDemo] Dimensions: %u x %u; bpp: %u\n"
    "[LogoFailDemo] Modeled allocation: %u bytes\n"
    "[LogoFailDemo] Required BLT size: %Lu bytes\n"
    "[LogoFailDemo] SIMULATION ONLY: control-flow redirection\n"
    "[LogoFailDemo] No native payload execution performed.\n"
    "[LogoFailDemo] ==========================================\n\n",
    Model->Width,
    Model->Height,
    Model->BitsPerPixel,
    Model->WrappedAllocationSize,
    Model->RequiredAllocationSize
    ));

  Print (L"\n\n");
  Print (L"============================================\n");
  Print (L"       LOGOFAIL - SIMULATED ACE\n");
  Print (L"============================================\n\n");
  Print (L"BMP allocation mismatch detected.\n\n");

  gBS->Stall (3 * 1000 * 1000);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ReadAndDecodeFromVolume (
  IN EFI_HANDLE  FileSystemHandle
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  EFI_FILE_PROTOCOL                *Root;
  EFI_FILE_PROTOCOL                *File;
  EFI_FILE_INFO                    *FileInfo;
  UINTN                            FileInfoSize;
  UINTN                            BmpSize;
  UINTN                            ReadSize;
  VOID                             *BmpData;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Blt;
  UINTN                            BltSize;
  UINTN                            PixelHeight;
  UINTN                            PixelWidth;
  RETURN_STATUS                    DecodeStatus;
  BMP_MINI_INFO                    MiniInfo;
  BMP_ALLOCATION_MODEL            AllocationModel;

  Root       = NULL;
  File       = NULL;
  FileInfo   = NULL;
  BmpData    = NULL;
  Blt        = NULL;
  FileSystem = NULL;

  DEBUG ((DEBUG_ERROR, "[LogoFailDemo] Opening filesystem protocol\n"));

  Status = gBS->HandleProtocol (
                  FileSystemHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FileSystem
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[LogoFailDemo] HandleProtocol: %r\n", Status));
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  DEBUG ((DEBUG_ERROR, "[LogoFailDemo] OpenVolume: %r\n", Status));

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Root->Open (
                  Root,
                  &File,
                  LOGO_FILE_PATH,
                  EFI_FILE_MODE_READ,
                  0
                  );

  DEBUG ((DEBUG_ERROR, "[LogoFailDemo] Open file: %r\n", Status));

  if (EFI_ERROR (Status)) {
    Root->Close (Root);
    return Status;
  }

  DEBUG ((DEBUG_ERROR, "[LogoFailDemo] Found %s\n", LOGO_FILE_PATH));

  FileInfoSize = 0;
  Status = File->GetInfo (
                   File,
                   &gEfiFileInfoGuid,
                   &FileInfoSize,
                   NULL
                   );

  DEBUG ((
    DEBUG_ERROR,
    "[LogoFailDemo] GetInfo size query: %r; required: %Lu\n",
    Status,
    (UINT64)FileInfoSize
    ));

  if (Status != EFI_BUFFER_TOO_SMALL) {
    goto Done;
  }

  FileInfo = AllocatePool (FileInfoSize);
  if (FileInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = File->GetInfo (
                   File,
                   &gEfiFileInfoGuid,
                   &FileInfoSize,
                   FileInfo
                   );

  DEBUG ((DEBUG_ERROR, "[LogoFailDemo] GetInfo: %r\n", Status));

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  DEBUG ((
    DEBUG_ERROR,
    "[LogoFailDemo] File size reported: %Lu bytes\n",
    FileInfo->FileSize
    ));

  if ((FileInfo->FileSize == 0) ||
      (FileInfo->FileSize > MAX_BMP_FILE_SIZE)) {
    DEBUG ((
      DEBUG_ERROR,
      "[LogoFailDemo] Rejected BMP size: %Lu bytes\n",
      FileInfo->FileSize
      ));
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  BmpSize = (UINTN)FileInfo->FileSize;
  BmpData = AllocatePool (BmpSize);

  if (BmpData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  ReadSize = BmpSize;
  Status = File->Read (File, &ReadSize, BmpData);

  DEBUG ((
    DEBUG_ERROR,
    "[LogoFailDemo] Read: %r; requested: %Lu; received: %Lu\n",
    Status,
    (UINT64)BmpSize,
    (UINT64)ReadSize
    ));

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if (ReadSize != BmpSize) {
    Status = EFI_END_OF_FILE;
    goto Done;
  }

  DEBUG ((
    DEBUG_ERROR,
    "[LogoFailDemo] Read %Lu BMP bytes\n",
    (UINT64)BmpSize
    ));

  /*
   * First run the non-destructive arithmetic model. A mismatch is reported
   * as a simulated trigger and is NOT passed to the real BMP decoder.
   */
  Status = ModelBmpAllocation (BmpData, BmpSize, &AllocationModel);
  if (!EFI_ERROR (Status) && AllocationModel.AllocationMismatch) {
    DEBUG ((
      DEBUG_ERROR,
      "[LogoFailDemo] Modeled allocation mismatch detected\n"
      ));
    Status = SimulatedAceMarker (&AllocationModel);
    goto Done;
  }

  /*
   * For ordinary images, parse and validate the BMP before handing it to
   * EDK II's decoder.
   */
  Status = ParseBmpMini (BmpData, BmpSize, &MiniInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[LogoFailDemo] ParseBmpMini rejected input: %r\n",
      Status
      ));
    goto Done;
  }

  DEBUG ((
    DEBUG_ERROR,
    "[LogoFailDemo] MiniParser: %u x %u; offset=%u; row=%u; pixel-data=%Lu\n",
    MiniInfo.Width,
    MiniInfo.Height,
    MiniInfo.PixelOffset,
    MiniInfo.RowSize,
    MiniInfo.PixelDataSize
    ));

  Blt         = NULL;
  BltSize     = 0;
  PixelWidth  = 0;
  PixelHeight = 0;

  DecodeStatus = TranslateBmpToGopBlt (
                   BmpData,
                   BmpSize,
                   &Blt,
                   &BltSize,
                   &PixelHeight,
                   &PixelWidth
                   );

  DEBUG ((
    DEBUG_ERROR,
    "[LogoFailDemo] TranslateBmpToGopBlt: %r\n",
    DecodeStatus
    ));

  if (DecodeStatus != RETURN_SUCCESS) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  DEBUG ((
    DEBUG_ERROR,
    "[LogoFailDemo] BMP decoded: %Lu x %Lu; GOP BLT buffer: %Lu bytes\n",
    (UINT64)PixelWidth,
    (UINT64)PixelHeight,
    (UINT64)BltSize
    ));

  Status = EFI_SUCCESS;

Done:

  DEBUG ((DEBUG_ERROR, "[LogoFailDemo] Volume final status: %r\n", Status));

  if (Blt != NULL) {
    FreePool (Blt);
  }

  if (BmpData != NULL) {
    FreePool (BmpData);
  }

  if (FileInfo != NULL) {
    FreePool (FileInfo);
  }

  if (File != NULL) {
    File->Close (File);
  }

  if (Root != NULL) {
    Root->Close (Root);
  }

  return Status;
}


EFI_STATUS
EFIAPI
LogoFailDemoEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *Handles;
  UINTN       HandleCount;
  UINTN       Index;
  BOOLEAN     Found;

  Handles     = NULL;
  HandleCount = 0;
  Found       = FALSE;

  DEBUG ((DEBUG_ERROR, "[LogoFailDemo] DXE entry point reached\n"));

  CheckSecureBootStatus ();

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[LogoFailDemo] LocateHandleBuffer failed: %r\n",
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_ERROR,
    "[LogoFailDemo] SimpleFS handles: %Lu\n",
    (UINT64)HandleCount
    ));

  for (Index = 0; Index < HandleCount; Index++) {
    DEBUG ((
      DEBUG_ERROR,
      "[LogoFailDemo] Processing volume %Lu\n",
      (UINT64)Index
      ));

    Status = ReadAndDecodeFromVolume (Handles[Index]);

    if (!EFI_ERROR (Status)) {
      Found = TRUE;
      break;
    }

    DEBUG ((
      DEBUG_ERROR,
      "[LogoFailDemo] Volume %Lu failed: %r\n",
      (UINT64)Index,
      Status
      ));
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }

  if (!Found) {
    DEBUG ((
      DEBUG_ERROR,
      "[LogoFailDemo] No volume successfully processed the BMP\n"
      ));
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_ERROR,
    "[LogoFailDemo] Baseline BMP test completed\n"
    ));

  return EFI_SUCCESS;
}
