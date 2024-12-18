# Zeal Music Tracker

## Arrangement View

![image](https://github.com/user-attachments/assets/c508b9b5-fa36-4935-9f4a-cd5369c775db)

### Tempo

This is expressed as "Frames per Quarter Note" (FPQ), the lowest value is "4", to represent the 4 steps per quarter (1/6th notes).

Tempo is adjusted in increments of 4.

A single frame is ~16.66ms (vsync).  At 32 FPQ, each quarter will take (32 * 16.66ms) or ~533.12ms.

You can then calculate BPM with (60,000 / 533.12) to get roughly 112.545BPM

Lower tempos are faster - 16 FPQ is roughly 224BPM

## Pattern View

![image](https://github.com/user-attachments/assets/4ae0d87b-3bf1-4703-ae4c-64cea9567ace)

## Inline Help

![image](https://github.com/user-attachments/assets/66b183bc-b3f7-439a-849f-b8fb31d5b4bb)


## Controls

* <kbd>S</kbd> - Save File
* <kbd>L</kbd> - Load File
* <kbd>Space</kbd> - Play/Stop
* <kbd>Esc</kbd> - Quit
* <kbd>H</kbd> - Inline Help
* <kbd>P</kbd> - Pattern View
* <kbd>A</kbd> - Arrangement View

### All Views

* <kbd>Up</kbd>/<kbd>Down</kbd> - Next Step
* <kbd>Home</kbd>/<kbd>End</kbd> - First/Last Step
* <kbd>Left</kbd>/<kbd>Right</kbd> - Adjust Step
* <kbd>PgUp</kbd>/<kbd>PgDown</kbd> - Adjust Step Plus
* <kbd>Ins</kbd> - Duplicate Last Step
* <kbd>Del</kbd> - Delete Step
* <kbd>Tab</kbd> - Next Cell

### Arrangement View

* <kbd>R</kbd>,<kbd>T</kbd> - Tempo -/+

### Pattern View

* <kbd>1</kbd>-<kbd>4</kbd> - Voice 1-4
* <kbd>[</kbd>/<kbd>]</kbd> - Prev/Next Pattern
* <kbd>N</kbd> - New Pattern

## FX Codes

### Arrangement FX

* [X] 0x00-0x1F - goto arrangement
* [X] 0x20-0x3F - tempo ((value - 0x20 + 1 * 4))
* [ ] 0xD0-0xDC - transpose by (value - 0xD0) steps

### Pattern FX

* [ ] 0x00 - note off
* [ ] 0x01 - note on
* [X] 0x10 - Square
* [X] 0x11 - Triangle
* [X] 0x12 - Saw
* [X] 0x13 - Noise
* [X] 0xC0-0xC8 - Countdown (value - 0xC0)
* [X] 0xD0-0xEF - Goto Step (value - 0xD0)
* [X] 0xF0-0xF8 - Volume (0, 12.5, 25, 37.5, 50, 62.5, 75, 87.5, 100)
