# STM32H7S78-DK_24bpp TBS

Performance testing can be done using the GPIO pins designated with the following signals in CN10 connector on the underside of the board:

- VSYNC_FREQ  - CN14-D2 (PF1)
- RENDER_TIME - CN14-D4 (PF2)
- FRAME_RATE  - CN14-D7 (PF3)
- MCU_ACTIVE  - CN15-D8 (PF4)

## Image format
Opaque images should be in ARGB8888 format to take full advantage of the color depth of the 24 bpp framebuffer. However, there is a bug in ToucHGFX Designer 4.24.1 that prevents this from being set as default. Instead, this TBS currently uses RGB565 as default image format. 

This can be worked around in two different ways:
- Set the format for each individual image to ARGB8888 in the Images tab in TouchGFX Designer.
- Set the "Opaque Image Format" to ARGB8888 in TouchGFX Designer -> Config -> Default Image Configuration. This needs to be re-done every time the project is reopened in TouchGFX Designer.

## Debugging in STM32CubeIDE
Debugging the code in an IDE can be complex because of the Bootloader and Application structure of the TBS for STM32H7S78-DK. To step through the code of the TouchGFX application in STM32CubeIDE, follow these steps:
1. Generate code in TouchGFX Designer
2. Open the project in STM32CubeIDE
3. Launch a debug session for the Boot project
4. Wait for the compilation and flashing to complete
5. Terminate the debug session (Ctrl + F2)
6. Launch a debug session for the Appli project
7. Wait for the compilation and flashing to complete
8. Click Resume (F8)
9. Press the black NRST button on the STM32H7S7-DK board
10. The application is now at a break point at the first line of main() in the Appli project. If not, click Resume (F8) once more
11. Proceed by e.g. clicking Resume (F8) or Step Over (F6)