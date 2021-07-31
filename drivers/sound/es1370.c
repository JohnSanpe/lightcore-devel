/* SPDX-License-Identifier: GPL-2.0-or-later */

#define DRIVER_NAME "es1370"
#define pr_fmt(fmt) DRIVER_NAME ": " fmt

#include <mm.h>
#include <kernel/irq.h>
#include <init/initcall.h>
#include <driver/pci.h>
#include <driver/sound.h>
#include <driver/sound/es1370.h>
#include <driver/sound/codec-ak4531.h>

#include <asm/io.h>

struct ensoniq_device {
    struct snd_device device;
    int irq;
    uint16_t base;
};

static __always_inline uint8_t 
es1370_inb(struct ensoniq_device *ensoniq, uint16_t reg)
{
    return inb(ensoniq->base + reg);
}

static __always_inline void
es1370_outb(struct ensoniq_device *ensoniq, uint16_t reg, uint8_t val)
{
    outb(ensoniq->base + reg, val);
}

static enum irq_return es1370_handle(irqnr_t vector, void *data)
{


    return IRQ_RET_HANDLED;
}

// static state snd_playback1_open(struct snd_pcm_stream *stream)
// {

//     return -ENOERR;
// }

// static state snd_playback1_close(struct snd_pcm_stream *stream)
// {

//     return -ENOERR;
// }

// static state snd_playback1_trigger(struct snd_pcm_stream *stream)
// {

//     return -ENOERR;
// }

// static struct snd_pcm_ops snd_playback1_ops = {
//     .open = snd_playback1_open,
//     .close = snd_playback1_close,
//     .trigger = snd_playback1_trigger,
// };

static void es1370_hwinit(struct ensoniq_device *ensoniq)
{
    uint32_t val;

    val = ES_1370_CDC_EN | ES_1370_PCLKDIVO(ES_1370_SRTODIV(8000));
    outl(ensoniq->base + ES_REG_CONTROL, val);
    outl(ensoniq->base + ES_REG_SERIAL, 0);
    outl(ensoniq->base + ES_REG_MEM_PAGE, ES_MEM_PAGEO(0));
    outl(ensoniq->base + ES_REG_PHANTOM_FRAME, ES_MEM_PAGEO(0));
    outl(ensoniq->base + ES_REG_PHANTOM_COUNT, 0);

}

static const struct pci_device_id es1370_pci_tbl[] = {
    {
        .vendor = PCI_VENDOR_ID_ENSONIQ,
        .device = PCI_DEVICE_ID_ENSONIQ_ES1370,
        .subvendor = PCI_ANY_ID,
        .subdevice = PCI_ANY_ID,
    },
    { }
};

static state es1370_probe(struct pci_device *pdev, int pdata)
{
    struct ensoniq_device *ensoniq;

    ensoniq = kzalloc(sizeof(*ensoniq), GFP_KERNEL);
    if(!ensoniq)
        return -ENOMEM;
    pci_set_devdata(pdev, ensoniq);

    irq_request(pdev->irq, IRQ_FLAG_SHARED, es1370_handle, 
                ensoniq, DRIVER_NAME);

    ensoniq->base = pci_resource_start(pdev, 0);
    es1370_hwinit(ensoniq);

    return -ENOERR;
}

static state es1370_remove(struct pci_device *pdev)
{
    return -ENOERR;
}

static struct pci_driver es1370_pci_driver = {
    .driver = {
        .name = DRIVER_NAME,
    },
    .id_table = es1370_pci_tbl,
    .probe = es1370_probe,
    .remove = es1370_remove,
};

static state es1370_init(void)
{
    return pci_driver_register(&es1370_pci_driver);
}
driver_initcall(es1370_init);
