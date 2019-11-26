```c
void __init init_IRQ(void)
{
    init_irq_stacks();
    irqchip_init();
    if (!handle_arch_irq)
        panic("No interrupt controller found.");
}
```

