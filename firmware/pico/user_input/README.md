# User Input Module

Handles local controls connected to the Pico.

Current pins:
- GP4: rotary encoder A
- GP5: rotary encoder B
- GP16: encoder push/select
- GP17: back button

Current support:
- uT-Kernel User Input Task
- rotary direction detection
- encoder push/select detection
- back button detection
- basic button debouncing

Serial output:
- `[input] next`
- `[input] previous`
- `[input] select`
- `[input] back`

The inputs are detected here first. Menu actions will be connected in the next
stage.
