# Validation — 2026-11-08

- Built and linked successfully as a statically linked little-endian Renesas SH ELF with KallistiOS 2.3.0 / GCC 15.2.0.
- Self-booting CDI reached the native title menu in Flycast 2.7 with its HLE BIOS, software OpenGL rendering, and one emulated Dreamcast controller.
- 26 existing weapon/collision checks passed on the Dreamcast executable: 0 failures. See the included serial log.
- Manually exercised title navigation, player selection, rounds configured for one and two players, movement, primary fire, pause, resume, and pause > Main Menu. Returning to the title kept the application running.
- Initial assets load on the first round; the loading message displays. Later rounds use cached assets.
- Original texture set: 114 textures including font, 3,404,160 bytes of image data. PVR reported 2,661,736 bytes free after the game assets loaded. Converted effects: see asset-manifest.json.
- The final runtime caps submissions at 3,000 quads/frame (480,000 bytes of headers/vertices), with 500 reserved for UI. This is a submission memory limit, not a measured performance guarantee.

An initial Exit to BIOS test exposed a return-to-loader restart. The final build explicitly selects KallistiOS ARCH_EXIT_MENU. A real BIOS/system-menu exit still needs hardware or a full BIOS emulator check.

Not verified: physical Dreamcast boot/performance, disc burning, actual two-to-four-controller input, VMU saving, audible output quality, every weapon in a prolonged game, or sustained worst-case effect loads. Emulator audio used a dummy host output; native stream startup and effect loading completed without reported failures. Hardware testing remains necessary.
