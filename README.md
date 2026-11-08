# DeadBabiesX Dreamcast — v0.1 test build

Native KallistiOS port of DeadBabiesX. Based on joesnose/DeadBabiesX-Windows-Native-port commit f6a59d4f95481d97cc96cb8d725be71a86beee80, with original artwork only.

Open `DeadBabiesX-DC-v0.1.cdi` in Flycast, or use a Dreamcast setup supporting self-booting homebrew CDI images. Physical Dreamcast compatibility and performance have not been verified. No commercial BIOS files are supplied. The ELF is included for developers using dcload or a debugger; it requires the disc filesystem at `/cd`.

## Controls

- Analog stick: move. D-pad: aim.
- R trigger: primary fire. L trigger: alternate fire.
- A: collect weapon / confirm menu selection.
- X / Y: special weapons.
- Start or B during play: pause. B in the pause menu: resume.
- Pause > Main Menu: return to the title without closing the application.
- Title > Exit to BIOS: quit. Start + A + B + X + Y also requests exit.

Choose 1–4 local players before starting. Each active player needs a controller. There is no LAN/online mode. Easy allows mega weapons up to level 5; Normal uses level 3; Hard gives every enemy a bomb. The original train base extends to the bottom of the screen.

## Scope and limits

Original art, 640×480 output, native PowerVR rendering and AICA effects/music. This build excludes the Windows HD, Alpine and cartoon packs. Long effects are resampled to fit AICA's 65,534-sample limit. Original music streams from the disc as stereo PCM; effects use Yamaha ADPCM. The first round loads assets from disc and can take a while; subsequent rounds reuse cached textures and sounds.

Scores are held in RAM for the session. VMU saving and rumble are not implemented. Motion blur is disabled. Extreme scenes are limited to 2,500 world quads plus up to 500 UI quads per frame to protect the 512 KiB submission buffer; excess draws are skipped, while simulation continues. This is an early test build, not a hardware-certified release.

See `docs/BUILD.md`, `docs/VALIDATION.md`, and `notices/` for provenance and verification. Original game credited to parlance / WaffleMind; Windows port and repository maintained by joesnose.
