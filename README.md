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

For conveience, the step indcators on the left are listed as D0-EF to
correspond with the Goto FX.  FX E0 will goto Step E0 (ie; Step 16).

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
* [ ] 0x02 - 1/32th
* [ ] 0x03 - 1/64th
* [X] 0x10 - Square
* [X] 0x11 - Triangle
* [X] 0x12 - Saw
* [X] 0x13 - Noise
* [X] 0xC0-0xC8 - Countdown (value - 0xC0)
* [X] 0xD0-0xEF - Goto Step (value - 0xD0)
* [X] 0xF0-0xF8 - Volume (0, 12.5, 25, 37.5, 50, 62.5, 75, 87.5, 100)

#### Notes

F1 is processed before the steps note and wave data are updated.
F2 is processed after the steps note and wave data are updated.

Countdown only works in F1 Cell.  When first entering the step the countdown is set to the value (value - 0xC0), each return to this step
will decrement the value by 1.  F2 will not be processed while F1's
counter is > 0.  This allows you to set an FX in F2 that is only
processed every "Countdown" repeats.

Example:
```
D0  C-4 0 C1 12 ; switch to Saw, every other time
D1  --- - -- --
D2  OFF - -- --
D3  --- - -- --
D4  C-4 - -- --
D5  --- - -- --
D6  OFF - -- --
D7  --- - C1 D0 ; goto D0 every other time
D8  C-4 - -- --
D9  --- - -- --
DA  OFF - -- --
DB  --- - -- --
DC  C-4 - -- --
DD  --- - -- --
DE  OFF - -- --
DF  --- - -- D0 ; always goto D0
```

Every time D0 is played, the waveform will switch to Square.
Every other time, the waveform will temporarily be switched to Saw.
Every other time D7 plays, it will jump to D0.
Every time DF plays, it will jump to D0.

