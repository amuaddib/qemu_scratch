#include "hw/core/register.h"
#include "hw/core/queue.h"

/*
register
  address
  num bytes
  reset-value
  field array
    startbit / endbit or bit???
    databus or irq
    type (see pg. 46 if RM 00008) - maybe use callback/function pointer to handle each field rather than branch logic - some of the special cases maybe should use custom callback:
      read/write
      read-only
      write-only
      read_clear_with_1
      read_clear_with_0
      read_auto-clear
      reserved-must-be-reset-value
      toggle_with_1
      etc., etc.
    or callback??? maybe have a type which says to do callback.
    cache value for field?
  or cache value for whole register?
  when init register probably make read mask and write mask
    this handles read/write, read-only, write-only
    then have callback list for special fields?
    and update list for writes???
    do callback for some reads rather than ???
            use linked lists rather than arrays???

registerfile - use device bus for this instead???
  startaddress
  endaddress
  registerarray

Look at virtio-balloon
*/

struct BitField {

};

/* Or these together to get the deisred acceess size */
#define ALLOWED_ACCESS_SIZE_BYTE 1
#define ALLOWED_ACCESS_SIZE_HWORD 2
#define ALLOWED_ACCESS_SIZE_WORD 4

struct Register {
   int regiser_byte_size;

   //can use this like a mask against word size
   uint8_t allowed_access_sizes;

   uint32_t reset_value;
   uint32_t value; // or use array?

   uint32_t write_mask, read_mask;

   QTAILQ_HEAD(, BitField) bit_fields;

   void (*write_callback)(Register register, uint32_t value);
};

struct RegisterFile {
    /* Inherited */
    DeviceState parent_obj

    int register_byte_size;
    int boundary_size;
    int num_registers;
    Register *registers; // Array of registers

    /* Properties */
    stm32_periph_t periph;
    void *stm32_rcc_prop;

    /* Private */
    MemoryRegion iomem;

    Stm32Rcc *stm32_rcc;


};

typedef struct RegisterFileClass {
    /* This is what a VirtioDevice must implement */
    DeviceClass parent;
    int (*init)(RegisterFileDevice *rfdev);
    uint32_t (*get_features)(VirtIODevice *vdev, uint32_t requested_features);
    uint32_t (*bad_features)(VirtIODevice *vdev);
    void (*set_features)(VirtIODevice *vdev, uint32_t val);
    void (*get_config)(VirtIODevice *vdev, uint8_t *config);
    void (*set_config)(VirtIODevice *vdev, const uint8_t *config);
    void (*reset)(VirtIODevice *vdev);
    void (*set_status)(VirtIODevice *vdev, uint8_t val);
    /* Test and clear event pending status.
     * Should be called after unmask to avoid losing events.
     * If backend does not support masking,
     * must check in frontend instead.
     */
    bool (*guest_notifier_pending)(VirtIODevice *vdev, int n);
    /* Mask/unmask events from this vq. Any events reported
     * while masked will become pending.
     * If backend does not support masking,
     * must mask in frontend instead.
     */
    void (*guest_notifier_mask)(VirtIODevice *vdev, int n, bool mask);
} RegisterFileClass;


static int register_file_device_init(RegisterFile *rfdev)
{


    DeviceState *dev = DEVICE(rfdev);
    VirtIOBalloon *s = VIRTIO_BALLOON(vdev);
    int ret;

    virtio_init(vdev, "virtio-balloon", VIRTIO_ID_BALLOON, 8);

    ret = qemu_add_balloon_handler(virtio_balloon_to_target,
                                   virtio_balloon_stat, s);

    if (ret < 0) {
        virtio_cleanup(VIRTIO_DEVICE(s));
        return -1;
    }

    s->ivq = virtio_add_queue(vdev, 128, virtio_balloon_handle_output);
    s->dvq = virtio_add_queue(vdev, 128, virtio_balloon_handle_output);
    s->svq = virtio_add_queue(vdev, 128, virtio_balloon_receive_stats);

    register_savevm(qdev, "virtio-balloon", -1, 1,
                    virtio_balloon_save, virtio_balloon_load, s);

    object_property_add(OBJECT(qdev), "guest-stats", "guest statistics",
                        balloon_stats_get_all, NULL, NULL, s, NULL);

    object_property_add(OBJECT(qdev), "guest-stats-polling-interval", "int",
                        balloon_stats_get_poll_interval,
                        balloon_stats_set_poll_interval,
                        NULL, s, NULL);
    return 0;
}

static Property register_file_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static void register_file_realize(DeviceState *dev, Error **errp)
{
    RegisterFile *rfdev = REGISTER_FILE(dev);

    /* Use object property for size??? */
    memory_region_init_io(&rfdev->iomem, &stm32_gpio_ops, s,
                          "gpio", 0x03ff);

    static int index;
    ISADevice *isadev = ISA_DEVICE(dev);
    ISASerialState *isa = ISA_SERIAL(dev);
    SerialState *s = &isa->state;

    if (isa->index == -1) {
        isa->index = index;
    }
    if (isa->index >= MAX_SERIAL_PORTS) {
        error_setg(errp, "Max. supported number of ISA serial ports is %d.",
                   MAX_SERIAL_PORTS);
        return;
    }
    if (isa->iobase == -1) {
        isa->iobase = isa_serial_io[isa->index];
    }
    if (isa->isairq == -1) {
        isa->isairq = isa_serial_irq[isa->index];
    }
    index++;

    s->baudbase = 115200;
    isa_init_irq(isadev, &s->irq, isa->isairq);
    serial_realize_core(s, errp);
    qdev_set_legacy_instance_id(dev, isa->iobase, 3);

    memory_region_init_io(&s->io, &serial_io_ops, s, "serial", 8);
    isa_register_ioport(isadev, &s->io, isa->iobase);
}

static void register_file_reset(DeviceState *dev)
{
    RegisterFile *rf = REGISTER_FILE(dev);

    rf->value = rf->reset_value;
    // Do we need to trigger actions?  Still not sure how to handle this.
    // Custom reset callback???
}

static void register_file_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    RegisterFileClass *rfc = REGISTER_FILE_CLASS(klass);
    //dc->exit = virtio_balloon_device_exit;
    dc->realize = register_file_realize;
    dc->props = register_file_properties;
    dc->reset = register_file_reset;
    // Shouldn't we be using dc->init? rfc->init = register_file_device_init;
    //rfc->get_config = virtio_balloon_get_config;
    //rfc->set_config = virtio_balloon_set_config;
    //rfc->get_features = virtio_balloon_get_features;
}

static const TypeInfo register_file_info = {
    .name = TYPE_REGISTER_FILE,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(RegisterFile),
    .class_init = register_file_class_init,
};

static void register_file_types(void)
{
    type_register_static(&register_file_info);
}

type_init(register_file_types)
