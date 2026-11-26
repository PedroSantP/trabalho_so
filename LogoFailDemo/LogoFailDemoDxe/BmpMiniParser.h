#ifndef LOGO_FAIL_DEMO_BMP_MINI_PARSER_H
#define LOGO_FAIL_DEMO_BMP_MINI_PARSER_H

#include <Uefi.h>

typedef struct {
  UINT32  Width;
  UINT32  Height;
  UINT32  PixelOffset;
  UINT32  RowSize;
  UINT64  PixelDataSize;
} BMP_MINI_INFO;

/*
 * Safe arithmetic model of the allocation-size calculation discussed in
 * Binarly BRLY-LOGOFAIL-2023-027.
 *
 * This structure records a mismatch; it never allocates a framebuffer using
 * the wrapped size and never performs an out-of-bounds access.
 */
typedef struct {
  UINT32  Width;
  UINT32  Height;
  UINT16  BitsPerPixel;
  UINT32  WrappedAllocationSize;
  UINT64  RequiredAllocationSize;
  BOOLEAN AllocationMismatch;
} BMP_ALLOCATION_MODEL;

EFI_STATUS
ParseBmpMini (
  IN  CONST UINT8   *Buffer,
  IN  UINTN         BufferSize,
  OUT BMP_MINI_INFO *Info
  );

EFI_STATUS
ModelBmpAllocation (
  IN  CONST UINT8           *Buffer,
  IN  UINTN                 BufferSize,
  OUT BMP_ALLOCATION_MODEL *Model
  );

#endif
