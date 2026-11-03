# Menu Controller

Handles the current menu selection and screen navigation.

Input events:
- NEXT
- PREVIOUS
- SELECT
- BACK

Behaviour:
- NEXT/PREVIOUS moves the highlighted main-menu item.
- SELECT opens the highlighted screen.
- BACK returns from a feature screen to the main menu.
- Navigation wraps from the last menu item to the first and vice versa.

The Menu Controller does not access the LCD directly. It sends display events
to the Display Manager, so the Display Task remains the only LCD owner.
