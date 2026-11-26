#include "BmpMiniParser.h"

STATIC
UINT16
ReadLe16 (
  IN CONST UINT8 *P
  )
{
  return (UINT16)(P[0] | (P[1] << 8));
}

STATIC
UINT32
ReadLe32 (
  IN CONST UINT8 *P
  )
{
  return (UINT32)P[0]
       | ((UINT32)P[1] << 8)
       | ((UINT32)P[2] << 16)
       | ((UINT32)P[3] << 24);
}

EFI_STATUS
ParseBmpMini (
  IN  CONST UINT8   *Buffer,
  IN  UINTN         BufferSize,
  OUT BMP_MINI_INFO *Info
  )
{
  UINT32  FileSize;
  UINT32  PixelOffset;
  UINT32  DibSize;
  UINT32  Width;
  INT32   Height;
  UINT16  BitsPerPixel;
  UINT32  Compression;
  UINT64  RowSize;
  UINT64  PixelDataSize;

  if (Buffer == NULL || Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize < 54) {
    return EFI_UNSUPPORTED;
  }

  // BITMAPFILEHEADER
  if (Buffer[0] != 'B' || Buffer[1] != 'M') {
    return EFI_UNSUPPORTED;
  }

  FileSize    = ReadLe32(&Buffer[2]);
  PixelOffset = ReadLe32(&Buffer[10]);

  // BITMAPINFOHEADER
  DibSize = ReadLe32(&Buffer[14]);
  Width   = ReadLe32(&Buffer[18]);
  Height  = (INT32)ReadLe32(&Buffer[22]);

  BitsPerPixel = ReadLe16(&Buffer[28]);
  Compression  = ReadLe32(&Buffer[30]);

  if (DibSize != 40 ||
      BitsPerPixel != 24 ||
      Compression != 0 ||
      Width == 0 ||
      Height <= 0) {
    return EFI_UNSUPPORTED;
  }

  if (PixelOffset < 54 ||
      PixelOffset > BufferSize ||
      FileSize > BufferSize ||
      FileSize < PixelOffset) {
    return EFI_UNSUPPORTED;
  }

  // Cada pixel ocupa 3 bytes.
  RowSize = (UINT64)Width * 3;

  // Cada linha BMP precisa ter tamanho múltiplo de 4.
  RowSize = (RowSize + 3) & ~((UINT64)3);

  // Verifica overflow antes da multiplicação.
  if (RowSize > (~(UINT64)0) / (UINT32)Height) {
    return EFI_UNSUPPORTED;
  }

  PixelDataSize = RowSize * (UINT32)Height;

  if ((PixelDataSize > (UINT64)(BufferSize - PixelOffset)) ||
      (PixelDataSize > (UINT64)(FileSize - PixelOffset)) ||
      (RowSize > 0xFFFFFFFFULL)) {
    return EFI_UNSUPPORTED;
  }

  Info->Width         = Width;
  Info->Height        = (UINT32)Height;
  Info->PixelOffset   = PixelOffset;
  Info->RowSize       = (UINT32)RowSize;
  Info->PixelDataSize = PixelDataSize;

  return EFI_SUCCESS;
}


/*
 * Modela, sem corromper memória, a divergência entre:
 *
 *   - o tamanho de alocação calculado em aritmética de 32 bits; e
 *   - o tamanho necessário para um framebuffer GOP (4 bytes/pixel).
 *
 * A referência técnica é BRLY-LOGOFAIL-2023-027. Isto NÃO reproduz o OOB
 * write: apenas identifica a condição aritmética em uma entrada controlada.
 */
EFI_STATUS
ModelBmpAllocation (
  IN  CONST UINT8           *Buffer,
  IN  UINTN                 BufferSize,
  OUT BMP_ALLOCATION_MODEL *Model
  )
{
  UINT32  FileSize;
  UINT32  PixelOffset;
  UINT32  DibSize;
  UINT32  Width;
  UINT32  Height;
  UINT16  BitsPerPixel;
  UINT64  RequiredSize;

  if ((Buffer == NULL) || (Model == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Model->Width = 0;
  Model->Height = 0;
  Model->BitsPerPixel = 0;
  Model->WrappedAllocationSize = 0;
  Model->RequiredAllocationSize = 0;
  Model->AllocationMismatch = FALSE;

  if (BufferSize < 54) {
    return EFI_UNSUPPORTED;
  }

  if ((Buffer[0] != 'B') || (Buffer[1] != 'M')) {
    return EFI_UNSUPPORTED;
  }

  FileSize    = ReadLe32 (&Buffer[2]);
  PixelOffset = ReadLe32 (&Buffer[10]);
  DibSize     = ReadLe32 (&Buffer[14]);
  Width       = ReadLe32 (&Buffer[18]);
  Height      = ReadLe32 (&Buffer[22]);
  BitsPerPixel = ReadLe16 (&Buffer[28]);

  if ((DibSize != 40) ||
      (Width == 0) ||
      (Height == 0) ||
      (BitsPerPixel != 24) ||
      (ReadLe32 (&Buffer[30]) != 0) ||
      (PixelOffset < 54) ||
      (PixelOffset > BufferSize) ||
      (FileSize < PixelOffset) ||
      (FileSize > BufferSize)) {
    return EFI_UNSUPPORTED;
  }

  /*
   * Avoid overflow in the 64-bit reference calculation.
   * GOP BLT pixels occupy 4 bytes each.
   */
  if ((UINT64)Width > ((~(UINT64)0) / 4 / (UINT64)Height)) {
    return EFI_UNSUPPORTED;
  }

  RequiredSize = (UINT64)4 * (UINT64)Width * (UINT64)Height;

  /*
   * Deliberately model a 32-bit wrapped allocation-size expression.
   * Unsigned wrap is defined by C; the result is observed only, never used
   * as an allocation size.
   */
  Model->WrappedAllocationSize = (UINT32)(4U * Width * Height);
  Model->RequiredAllocationSize = RequiredSize;
  Model->Width = Width;
  Model->Height = Height;
  Model->BitsPerPixel = BitsPerPixel;
  Model->AllocationMismatch =
    (RequiredSize > (UINT64)Model->WrappedAllocationSize);

  return EFI_SUCCESS;
}
