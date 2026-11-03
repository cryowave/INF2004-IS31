#include <tk/tkernel.h>
#include <tk/syslib.h>
#include <tm/tmonitor.h>
#include <sys/sysdef.h>
#include <bsp/libbsp.h>

#if TK_SUPPORT_SMP
#include <tk/smp.h>
#endif

#include "user_input.h"

#define ENCODER_PIN_A          4U
#define ENCODER_PIN_B          5U
#define ENCODER_BUTTON_PIN     16U
#define BACK_BUTTON_PIN        17U

#define INPUT_STACK_SIZE       2048
#define INPUT_POLL_MS          2
#define BUTTON_DEBOUNCE_COUNT  5U

/* used to work out the direction from the previous and current A/B state */
static const B encoder_table[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

typedef struct {
    UINT stable;
    UINT last_read;
    UB count;
} ButtonState;

static void setup_input_pin(UINT pin)
{
    gpio_set_pin(pin, GPIO_MODE_IN);

    /* keep the input high until the switch pulls it low */
    set_w(GPIO(pin), GPIO_PUE);
    clr_w(GPIO(pin), GPIO_PDE);
}

static void button_init(ButtonState *button, UINT pin)
{
    UINT value = gpio_get_val(pin);

    button->stable = value;
    button->last_read = value;
    button->count = 0U;
}

static BOOL button_pressed(ButtonState *button, UINT pin)
{
    UINT value = gpio_get_val(pin);

    if (value == button->last_read) {
        if (button->count < BUTTON_DEBOUNCE_COUNT) {
            button->count++;
        }
    } else {
        button->last_read = value;
        button->count = 0U;
    }

    /* only accept the change after it stays the same for a few reads */
    if (button->count >= BUTTON_DEBOUNCE_COUNT &&
        button->stable != button->last_read) {

        button->stable = button->last_read;
        button->count = 0U;

        if (button->stable == 0U) {
            return TRUE;
        }
    }

    return FALSE;
}

static void input_task(INT stacd, void *exinf)
{
    UINT previous_state;
    UINT current_state;
    INT encoder_count = 0;
    INT change;

    ButtonState select_button;
    ButtonState back_button;

    (void)stacd;
    (void)exinf;

    setup_input_pin(ENCODER_PIN_A);
    setup_input_pin(ENCODER_PIN_B);
    setup_input_pin(ENCODER_BUTTON_PIN);
    setup_input_pin(BACK_BUTTON_PIN);

    button_init(&select_button, ENCODER_BUTTON_PIN);
    button_init(&back_button, BACK_BUTTON_PIN);

    previous_state =
        (gpio_get_val(ENCODER_PIN_A) << 1) |
         gpio_get_val(ENCODER_PIN_B);

    tm_printf((UB *)"[input] controls ready\n");

    while (1) {
        current_state =
            (gpio_get_val(ENCODER_PIN_A) << 1) |
             gpio_get_val(ENCODER_PIN_B);

        if (current_state != previous_state) {
            change = encoder_table[(previous_state << 2) | current_state];
            encoder_count += change;
            previous_state = current_state;

            if (encoder_count >= 4) {
                tm_printf((UB *)"[input] next\n");
                encoder_count = 0;
            } else if (encoder_count <= -4) {
                tm_printf((UB *)"[input] previous\n");
                encoder_count = 0;
            }
        }

        if (button_pressed(&select_button, ENCODER_BUTTON_PIN)) {
            tm_printf((UB *)"[input] select\n");
        }

        if (button_pressed(&back_button, BACK_BUTTON_PIN)) {
            tm_printf((UB *)"[input] back\n");
        }

        tk_dly_tsk(INPUT_POLL_MS);
    }
}

static T_CTSK input_task_config = {
    .itskpri = 6,
    .stksz = INPUT_STACK_SIZE,
    .task = input_task,
#if TK_SUPPORT_SMP
    .tskatr = TA_HLNG | TA_RNG3 | TA_ASSPRC,
    .assprc = TP_PRC1,
#else
    .tskatr = TA_HLNG | TA_RNG3,
#endif
};

ER user_input_init(void)
{
    ID task_id;

    task_id = tk_cre_tsk(&input_task_config);

    if (task_id <= E_OK) {
        return (ER)task_id;
    }

    return tk_sta_tsk(task_id, 0);
}
