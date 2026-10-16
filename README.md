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

* <kbd>F2</kbd> - Save File
* <kbd>L</kbd> - Load File
* <kbd>Space</kbd> - Play/Stop (when not recording)
* <kbd>Q</kbd> - Quit
* <kbd>F1</kbd> - Inline Help
* <kbd>P</kbd> - Pattern View
* <kbd>A</kbd> - Arrangement View (when not recording)

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
* <kbd>F5</kbd> - New Pattern
* <kbd>F6</kbd> - Clear Pattern (confirmation required)
* <kbd>F7</kbd> - Delete Pattern (confirmation required)
* <kbd>R</kbd> - Toggle recording while playback is stopped
* <kbd>A</kbd> - Write OFF to the selected voice while recording
* <kbd>F</kbd> - Write OFF to all four voices while recording
* <kbd>-</kbd>/<kbd>=</kbd> - Lower/raise the piano input octave
* <kbd>Space</kbd> - Advance one row while recording (wraps after the last row)

### Piano input

In Pattern view, use the bottom QWERTY row as piano keys:

```text
    S  D     G  H  J
    C# D#    F# G# A#
  Z  X  C  V  B  N  M
  C  D  E  F  G  A  B
```

The default octave is 4 (Z plays C4). The header shows AUDITION, RECORD, or
PLAY, the active octave (0–8), and the number of sounding piano keys held.

With recording off, notes only audition and do not change the track. Press R
while stopped to enable recording; each note press writes only the note into
the current row. Waveform and effect fields are preserved. Release stops the
audition without writing a note-off. Press A while recording to write OFF into
the selected voice's current row and stop its held audition note, preserving
waveform and effects. Press F while recording to write OFF into all four voices
on the current row and stop all held audition notes, preserving waveform and
effects in each voice. Turn recording off before using A to switch to Arrangement
view. Rows do not advance automatically: use
Space or the navigation keys. Advancing while holding a chord does not copy it
into the next row; release and press the notes again to enter another chord.

Hold multiple keys to audition or record up to four notes together. The first
note uses the selected voice, and additional held notes use the next free
voice columns, wrapping from voice 4 to voice 1. A fifth held note is ignored;
release and press it again once a voice is free. Key repeats do not enter extra
notes. The keyboard must support the chosen key combination (keyboard rollover).

Each voice auditions with its most recent waveform and volume settings from
the start of the current pattern through the selected row, defaulting to square
wave at 75% volume. Auditioning does not execute jumps or countdowns.

Voice selection with 1–4 and waveform/effect editing work in record mode.
Recording and playback are mutually exclusive: stop playback before pressing R,
and turn recording off before pressing Space to play. Leaving Pattern view or
loading a track turns recording off; dialogs preserve record mode and stop held
audition notes. The octave resets to 4 when loading a track.

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
* [X] 0xF0-0xF4 - Volume Voice (0, 25, 50, 75, 100)
* [ ] 0xF5-0xF9 - Volume All (0, 25, 50, 75, 100)
* [ ] 0xF0-0xF4 - Pan (Center, Left, Right)

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

