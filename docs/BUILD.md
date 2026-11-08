# Building

Tested with KallistiOS 38aeee7e9b4e9ad764c4f82a287edee8d228b355 and GCC 15.2.0 from pcercuei/dreamcast-toolchain:stable, image digest sha256:86485c68868b543b5fd973808b214aaaf0a0160702a10eb3b7abec8d18393a86.

1. Install/build KallistiOS and source its `environ.sh`. This project needs no kos-ports packages.
2. Obtain the Windows source/assets at commit f6a59d4f95481d97cc96cb8d725be71a86beee80 from https://github.com/joesnose/DeadBabiesX-Windows-Native-port.
3. On the host, install Python 3, Pillow, FFmpeg, and the DejaVu Sans Bold font. Build KOS's `utils/wav2adpcm` for the host. Run:

   `python3 tools/prepare_assets.py --assets /path/to/windows/assets --out disc --adpcm-tool /path/to/wav2adpcm`

4. Run `make`. Then:

   `sh-elf-objcopy -O binary DeadBabiesX.elf DeadBabiesX.bin`

   `$KOS_BASE/utils/scramble/scramble DeadBabiesX.bin disc/1ST_READ.BIN`

   `$KOS_BASE/utils/makeip/makeip ip.txt IP.BIN`

5. Build a self-booting disc with your preferred homebrew disc tools. For this build genisoimage 1.1.11 was used:

   `genisoimage -C 0,11702 -V DBX_DREAMCAST -G IP.BIN -l -J -r -o DeadBabiesX.iso disc`

   ISO to CDI used `create_cdi_image(iso, cdi, 11702)` from the external build tool https://github.com/Conkwer/lazyboot/blob/main/extra_tools/mkcdi-20240504/mkisofs_arg.py. This external tool is not bundled. No proprietary BIOS is required or bundled.

Place an empty `disc/selftest.flag` before building the disc to run the native collision/weapon regression tests on boot. Remove it for a normal disc. `--self-test` also runs the tests when arguments are supplied through a loader. The source archive excludes generated assets and toolchain binaries; regenerate assets from the pinned upstream commit.

Texture format: `DCTX`, little-endian uint32 width/height, then linear ARGB4444 pixels. Exact black is transparent to match the original renderer. Non-power-of-two images are resampled to power-of-two dimensions; rendering uses normalized coordinates. Asset manifest records the complete texture memory budget.
