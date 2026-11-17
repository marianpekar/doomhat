# doomhat

Based on [doomgeneric](https://github.com/ozkl/doomgeneric) by [@ozkl](https://github.com/ozkl).

This port is made for playing doom on [Waveshare 1.44inch LCD HAT](https://www.waveshare.com/wiki/1.44inch_LCD_HAT). I've built and tested it on [Raspberry Pi Zero 2 W](https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/) with [Waveshare UPC Hat](https://www.waveshare.com/wiki/UPS_HAT_(C)) attached so I can play Doom wherever I go.

It runs nice and smoothly on that tiny chip but you might need to use zoom* when playing it, depends on you eyesight.

*) Moving your head closer to the object observed.

## Setup Guide

1. Get an MicroSD card (I have 16 GB one, but fast, speed is more important than size here... \*cough\*) and use [rpi-imager](https://www.raspberrypi.com/software/) to flash in the OS. By the time of making this, I used Raspberry Pi OS (64-bit) released 2026-10-06. Don't forget to enable SSH and provide credentials to your Wi-Fi (if you forgot them, ask your neighbors, they might know...)
2. If you have both Waveshare 1.44inch LCD HAT and Waveshare UPC Hat, make a sandwitch where these two components are bread and your Raspberry Pi Zero 2 W is a ham.
3. Gently insert MicroSD card, then pull back because you tried to insert it the wrong way, flip it and insert the correct way.
4. You might want to plug USB cable to give some juice to the Li-on batery on UPC hat and prevent shutdown during installation process.
5. Connect to your Raspberry Pi via SSH
6. Enable SPI interface

```shell
sudo raspi-config
# Choose Interfacing Options -> SPI -> Yes to enable SPI interface
```
7. Set gpio and pu pins:

```shell
sudo vi /boot/firmware/config.txt
#Add the following:
gpio=6,19,5,26,13,21,20,16=pu
```
8. Save and exit vim... don't Google it, press [ESC], type `wq!` and press [ENTER].
9. Reboot your Raspberry Pi
```
sudo reboot
```
10. Clone this repository and use `run.sh` to build and run the game.

```shell
git clone https://github.com/marianpekar/doomhat
cd doomhat
./run.sh
```

11. The game should run now, run `install.sh` which adds a systemd service that runs run.sh at boot. Later you can disable it by running `./install.sh --uninstall`.

```shell
./install.sh
```

## Controls

| Input | Action |
|---|---|
| Joystick Up | Move forward |
| Joystick Down | Move backward |
| Joystick Left | Turn left |
| Joystick Right | Turn right |
| Joystick Press | Use / Open doors / Confirm |
| KEY1 | Fire weapon |
| KEY2 | Strafe modifier (hold while turning) |
| KEY3 | Open menu / Escape |

## Project Structure

```txt
..
├── doomgeneric/       copied from https://github.com/ozkl/doomgeneric
├── lib/               hardware underlying interface for LCD HAT 
│   ├── Config/        from https://files.waveshare.com/upload/f/fa/1.44inch-LCD-HAT-Code.7z
│   └── LCD/ 
├── port/
│   └── doomgeneric_st7735.c  DG_Init, DG_DrawFrame, etc. see DG_DrawFrame https://github.com/ozkl/doomgeneric#porting
├── .gitignore
├── /!\ DOOM.WAD        not included, provide your own IWAD 
├── install.sh          install run.sh as start-up service
├── README.md           you are here
└── run.sh              build and run DOOM
```