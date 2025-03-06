# Digital Rain Gauge with battery, solar panel and eink display

This project is a digital rain gauge that uses a tipping bucket mechanism to measure the amount of rain that has fallen. The rain gauge is powered by a battery and a solar panel and displays the amount of rain that has fallen on an eink display.

Note - the project details are not complete instructions.  I often uses scrap box parts, or parts I get from my local electronics store.  This is more of a guide to show what I did.

## Hardware

The hardware used in this project is as follows:

- STM32F103 microcontroller (initially developed with a blue pill board, then a custom board was created with only the required components).
- DFROBOT Micro Solar Power Manager / Charger with Regulated Output (DFR0579).
- Waveshare 250x122, 2.13inch E-Ink display HAT for Raspberry Pi (12915) - the one with the 8 pin connector.
- 500mAh LiPo battery.
- Reed switch to sense the rain bucket tipping.

## Software

The files are in RainGaugeDigital.  I use VisualGDB hosted in Visual Studio.

The program spends most of its time in low power mode.  It will wake when the button tips (reed switch), or when one of the UI buttons is pressed.

The STM32F103 doesn't have any permanent storage, eg EEPROM.  The data collected is stored in ram.  If the battery is removed, the data is lost.

The display showing the mm of rain is not updated until 1 minute after the first tip.  This is to reduce power use.

The DFROBOT Micro Solar Power Manager can harvest very low voltages to charge the battery.  The built in voltage regulator powers the circuit at 3.3V.

The eink display code is based on the samples in the wiki.  A few changes were made to the code more flexible, and increase performance by using DMA transfers.

## Schematic

Files and a PDF schematic are in the EInkRainGaugePCB folder.  I use CircuitStudio.

## 3D Printed Parts

Alibre Design files are in the 3dModel\Alibre folder.  Step files in the 3dModel\Step folder.  Stl files in the 3dModel\Stl folder.

Note - not many fastners are shown in the 3D model.  I used M3 screws and heat set inserts.

The catchment funnel has 10 holes around the topmost edge to install 20cm 0.8mm spring steel (piano wire) pieces to discourage birds from perching on the funnel.

This is a 3d render of the model:
<img src="images\render.jpg">