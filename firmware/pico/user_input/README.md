# User Input Module

Handles local controls connected to the Pico.

Current controls:
- GP4: rotary encoder A
- GP5: rotary encoder B
- GP17: KO action button
- GP16: not used for the current physical knob

Current support:
- uT-Kernel User Input Task
- rotary direction detection
- KO button detection
- basic button debouncing
- sends input events to the Menu Controller

Serial output:
- `[input] next`
- `[input] previous`
- `[input] ko`

The physical knob used in the current prototype does not have a usable push
switch. The KO button is therefore context-sensitive:
- on the main menu: select/open the highlighted item
- inside a feature screen: return to the main menu

The User Input Task only detects hardware input. Menu behaviour is decided by
the Menu Controller.
