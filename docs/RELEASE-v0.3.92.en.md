# 0.3.92 · build 101

- Fix the audio buffer-release notification race that could leave pending work asleep.
- Return guest memory pages to the system when mappings or heap ranges are released.
- Close the Mac settings window immediately when Play is selected.
- Correct red/blue channel ordering for BGRA/BGRX movie target textures.
- Validate Bundle 2 ranges, declared sizes and bounded decompression.
- Preserve adjusted UE region sizes when initialization sets the request argument again after the store.
- Retain the adaptive decorating furniture-menu background fix. Existing external data remains compatible.

Local test build; not pushed or released. Outdoor edge-object pop-in under adaptive display remains unresolved. Physical phone validation is pending.
