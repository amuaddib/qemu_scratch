convert bus into IRQ lines

databus
  callback - when registering callback so whether it only gets called during a change...
  bits
  buffer - should persist data - only trigger irqs if change.
  irqs - bits may be mapped to irq's

struct Databus {
    Connector conn;  // Should we make this an object??? Object obj;

      //when registering callback or irqs, indicate whether it gets called during a change...
    int num_bits;
    uint8_t *value; // only trigger irqs if change.
    qemu_irq in_irq, out_irq;
};
