# Menu Controller

Handles the current menu selection and screen navigation.

Input events:
- NEXT
- PREVIOUS
- ACTION

Behaviour:
- NEXT/PREVIOUS moves the highlighted main-menu item.
- ACTION on the main menu opens the highlighted screen.
- ACTION inside a feature screen returns to the main menu.
- Navigation wraps from the last menu item to the first and vice versa.

The ACTION event comes from the physical KO button on GP17. This matches the
working prototype where the rotary knob itself does not provide a usable press
switch.

The Menu Controller does not access the LCD directly. It sends display events
to the Display Manager, so the Display Task remains the only LCD owner.
