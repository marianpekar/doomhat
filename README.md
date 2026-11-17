# doomhat

Based on [doomgeneric](https://github.com/ozkl/doomgeneric) by [@ozkl](https://github.com/ozkl).

![doomhat](https://marianpekar.com/uploads/doomhat.gif)

This port is made for playing Doom on the [Waveshare 1.44inch LCD HAT](https://www.waveshare.com/wiki/1.44inch_LCD_HAT). I've built and tested it on a [Raspberry Pi Zero 2 W](https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/) with a [Waveshare UPS HAT (C)](https://www.waveshare.com/wiki/UPS_HAT_(C)) attached, so I can play Doom wherever I go.

It runs nice and smoothly on that tiny chip, but you might need to use zoom* when playing it, depending on your eyesight.

*) Moving your head closer to the object observed.

## Setup Guide

1. Get a microSD card (I have a 16 GB one, but a fast one; speed is more important than size here... \*cough\*) and use [rpi-imager](https://www.raspberrypi.com/software/) to flash the OS. At the time of making this, I used Raspberry Pi OS (64-bit) released 2026-10-06. Don't forget to enable SSH and provide credentials for your Wi-Fi (if you forgot them, ask your neighbors, they might know...)
2. If you have both the Waveshare 1.44inch LCD HAT and the Waveshare UPS HAT (C), make a sandwich where these two components are the bread and your Raspberry Pi Zero 2 W is the ham.
3. Gently insert the microSD card, then pull it back out because you tried to insert it the wrong way, flip it and insert it the correct way.
4. You might want to plug in a USB cable to give some juice to the battery on the UPS HAT and prevent a shutdown during the installation process.
5. Connect to your Raspberry Pi via SSH.
6. Enable the SPI interface:

```shell
sudo raspi-config
# Choose Interface Options -> SPI -> Yes to enable the SPI interface
```

7. Enable pull-up resistors on the GPIO pins used by the joystick and buttons:

```shell
sudo vi /boot/firmware/config.txt
# Add the following line:
gpio=6,19,5,26,13,21,20,16=pu
```

8. Save and exit vim... don't Google it: press [ESC], type `:wq!` and press [ENTER].
9. Reboot your Raspberry Pi:

```shell
sudo reboot
```

10. Clone this repository:

```shell
git clone https://github.com/marianpekar/doomhat
```

11. Get an IWAD file (e.g. `DOOM.WAD`, the shareware `doom1.wad`, or the free [Freedoom](https://freedoom.github.io/) `freedoom1.wad`) and copy it into the `doomhat` directory as `DOOM.WAD`. If the file is on your computer, run this on your computer, not on the Pi:

```shell
scp DOOM.WAD <user>@<pi-ip>:~/doomhat/DOOM.WAD
```

12. Back on the Pi, use `run.sh` to build and run the game:

```shell
cd doomhat
./run.sh
```

13. The game should be running now. Quit it with `Ctrl+C`, then run `install.sh`, which adds a systemd service that runs `run.sh` at boot. You can disable it later by running `./install.sh --uninstall`.

```shell
./install.sh
```

## Controls

| Input | Primary Action | Secondary Action |
|---|---|---|
| Joystick Up | Move forward | *-* |
| Joystick Down | Move backward | *-* |
| Joystick Left | Turn left | Strafe left |
| Joystick Right | Turn right | Strafe right |
| Joystick Press | Use / Open doors / Confirm | Toggle map |
| KEY1 | Fire weapon | Previous weapon (`,`) |
| KEY2 | **When held, the secondary action is applied.** | *-* |
| KEY3 | Open menu / Escape | Next weapon (`.`) |

### Save Game Hack

With only 8 buttons, typing is quite limited, and Doom won't let you save a game without a name. But with **KEY2** used as a modifier for cycling through weapons with **KEY1** and **KEY3**, a solution to this problem emerged on its own.

To save a game, press **KEY3** to open the menu and navigate to *Save Game* using **Joystick Up/Down**. Press **KEY1** to confirm, select a save slot, and confirm again. Then hold **KEY2** while pressing **KEY1** to type `,` or **KEY3** to type `.`, and confirm.

## Project Structure

```txt
.
├── doomgeneric/       copied from https://github.com/ozkl/doomgeneric
├── lib/               underlying hardware interface for the LCD HAT
│   ├── Config/        from https://files.waveshare.com/upload/f/fa/1.44inch-LCD-HAT-Code.7z
│   └── LCD/
├── port/
│   └── doomgeneric_st7735.c  platform implementation (DG_Init, DG_DrawFrame, etc.), see https://github.com/ozkl/doomgeneric#porting
├── .gitignore
├── /!\ DOOM.WAD        not included, provide your own IWAD
├── install.sh          installs run.sh as a start-up service
├── README.md           you are here
└── run.sh              builds and runs DOOM
```
