# Lifted semantic C from the i960 disasm lift pipeline.

Regenerate with:

```bash
./build.sh lift --pilot
./build.sh lift --function libc_printf
```

| File | ROM range | Source |
|------|-----------|--------|
| `libc_strcpy.c` | `0x05cdc8` + `0x3c` | lift |
| `libc_memcpy.c` | `0x05daa0` + `0x140` | lift |
| `libc_printf.c` | `0x05cec0` + `0x50` | lift |

These are semantic starting points (`#include "../i960_lift.h"`), not byte-matched ROM replacements.

Intermediate IR and reports live under `out/lift/`.
