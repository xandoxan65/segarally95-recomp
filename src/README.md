# `src/` — lifted semantic C

Hand-edited C ports derived from the lift pipeline. Shared types live in `i960_lift.h`.

```
src/
  i960_lift.h          # u32, g14/fp/sp, LEAF_ENTER()
  libc/                # auto-lifted libc (@rom annotated)
    libc_strcpy.c
    libc_memcpy.c
    libc_printf.c
```

Regenerate:

```bash
./build.sh lift --pilot
./build.sh lift --function NAME   # symbols/functions.yaml
```

Lift writes C to `src/libc/` by default. IR JSON, `.lifted` pseudocode, and reports go to `out/lift/`.

Coverage metrics pick up `// @rom` tags in `src/**/*.c`.
