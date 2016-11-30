/*
 * drivers/uio/uio_pdrv_genirq.c
 *
 * Userspace I/O platform driver with generic IRQ handling code.
 *
 * Copyright (C) 2008 Magnus Damm
 *
 * Based on uio_pdrv.c by Uwe Kleine-Koenig,
 * Copyright (C) 2008 by Digi International Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>

#include <asm/io.h>
#include "uioDrv.h"

#define MAXIRQ 10000
static struct uio_info *uioinfo;
static unsigned int timerVal[MAXIRQ];
static int irqc;

/*
 * Called when associated /dev/uioX device is opened from userspace
 *
 * Does nothing. All required initalization is done in user space part
 */
static int xilAxiTimer_open(struct uio_info *info, struct inode *inode)
{
        return 0;
}

/*
 * Called when associated /dev/uioX device is closed from userspace
 *
 * Disable timer.
 */
static int xilAxiTimer_release(struct uio_info *info, struct inode *inode)
{
        volatile unsigned char *iomem = uioinfo->mem[0].internal_addr;

        iowrite32(0, iomem + TC0_CS_REG);
        return 0;
}

/*
 * Interrupt service routine:
 * Interrupt is cleared in IP core and the timer value is saved(, though
 * I don't know how to make that value accessible from userspace).
 * Any further handling is done in user space part of driver.
 */
static irqreturn_t xilAxiTimer_isr(int irq, struct uio_info *dev_info)
{
        unsigned int tmp;
        volatile unsigned char *iomem = uioinfo->mem[0].internal_addr;
        /* Just disable the interrupt in the interrupt controller, and
         * remember the state so we can allow user space to enable it later.
         */

//      if (!test_and_set_bit(0, &uioinfo->irq_flags))
//              disable_irq_nosync(irq);

        // save the timer value as soon as we get here
        if (irqc < MAXIRQ)
                timerVal[irqc++] = ioread32(iomem + TC0_TC_REG);
        // clear the interrupt in ipcore
        tmp = ioread32(iomem + TC0_CS_REG);
        tmp |= TC_T0INT;
        iowrite32(tmp, iomem + TC0_CS_REG);

        return IRQ_HANDLED;
}

/*
 * Allow user space to enable and disable the interrupt
 * in the interrupt controller, but keep track of the
 * state to prevent per-irq depth damage.
 *
 * Serialize this operation to support multiple tasks.
 * Writing 1/0 to the associated /dev/uioX enables/disables the interrupt.
 */
static int xilAxiTimer_irqcontrol(struct uio_info *dev_info, s32 irq_on)
{
        unsigned int tmp;
        volatile unsigned char *iomem = uioinfo->mem[0].internal_addr;

        tmp = ioread32(iomem + TC0_CS_REG);
        if(irq_on)
                tmp |= TC_ENIT0;
        else
                tmp &= ~TC_ENIT0;
        iowrite32(tmp, iomem + TC0_CS_REG);

        return 0;
}

static int xilAxiTimer_probe(struct platform_device *pdev)
{
        int ret = -EINVAL;

        if (request_mem_region(TIMERBAR, IOREGIONSIZE, DEVICENAME) == NULL) {
                printk(KERN_WARNING "Memory region already in use.\n");
                return -EBUSY;
        }

        uioinfo = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
        if (uioinfo == NULL) {
                printk(KERN_WARNING "Unable to allocate memory.\n");
                release_mem_region(TIMERBAR, IOREGIONSIZE);
                return -ENOMEM;
        }

        uioinfo->name = DEVICENAME;
        uioinfo->version = DRVVERSION;
        uioinfo->priv = NULL;
        uioinfo->open = xilAxiTimer_open;
        uioinfo->release = xilAxiTimer_release;
        uioinfo->handler = xilAxiTimer_isr;
        uioinfo->irqcontrol = xilAxiTimer_irqcontrol;
        uioinfo->irq = 61;
        uioinfo->irq_flags = IRQF_SHARED;
        uioinfo->mem[0].name = "Registers";
        uioinfo->mem[0].addr = TIMERBAR;
        uioinfo->mem[0].size = IOREGIONSIZE;
        uioinfo->mem[0].memtype = UIO_MEM_PHYS;
        uioinfo->mem[0].internal_addr = ioremap(TIMERBAR, IOREGIONSIZE);

        if (uioinfo->mem[0].internal_addr == NULL) {
                printk(KERN_WARNING "Unable to remap memory.\n");
                release_mem_region(TIMERBAR, IOREGIONSIZE);
                return -ENOMEM;
        }

        /* Enable Runtime PM for this device:
         * The device starts in suspended state to allow the hardware to be
         * turned off by default. The Runtime PM bus code should power on the
         * hardware and enable clocks at open().
         */
//      pm_runtime_enable(&pdev->dev);

        ret = uio_register_device(&pdev->dev, uioinfo);
        if (ret) {
                dev_err(&pdev->dev, "unable to register uio device\n");
                goto bad1;
        }

        return 0;

      bad1:
//      pm_runtime_disable(&pdev->dev);
        iounmap(uioinfo->mem[0].internal_addr);
        kfree(uioinfo);
        release_mem_region(TIMERBAR, IOREGIONSIZE);
        ret = -ENODEV;
        return ret;
}

static int xilAxiTimer_remove(struct platform_device *pdev)
{
        int i;
//      pm_runtime_disable(&pdev->dev);
        uio_unregister_device(uioinfo);
        iounmap(uioinfo->mem[0].internal_addr);
        kfree(uioinfo);
        release_mem_region(TIMERBAR, IOREGIONSIZE);
        for (i = 0; i < MAXIRQ; i++)
                printk(KERN_INFO "IRQ latency kernel(%d): %u\n", i, 0xfffff - timerVal[i]);
        return 0;
}

static int uio_pdrv_genirq_runtime_nop(struct device *dev)
{
        /* Runtime PM callback shared between ->runtime_suspend()
         * and ->runtime_resume(). Simply returns success.
         *
         * In this driver pm_runtime_get_sync() and pm_runtime_put_sync()
         * are used at open() and release() time. This allows the
         * Runtime PM code to turn off power to the device while the
         * device is unused, ie before open() and after release().
         *
         * This Runtime PM callback does not need to save or restore
         * any registers since user space is responsbile for hardware
         * register reinitialization after open().
         */
        return 0;
}

static const struct dev_pm_ops uio_pdrv_genirq_dev_pm_ops = {
        .runtime_suspend = uio_pdrv_genirq_runtime_nop,
        .runtime_resume = uio_pdrv_genirq_runtime_nop
};

static struct platform_driver xilAxiTimerPdrv = {
        .probe = xilAxiTimer_probe,
        .remove = xilAxiTimer_remove,
        .driver = {
                   .name = "FABRICPARENT",
                   .owner = THIS_MODULE,
                   .pm = &uio_pdrv_genirq_dev_pm_ops,
                   }
};

static int __init xilAxiTimer_init(void)
{
        return platform_driver_register(&xilAxiTimerPdrv);
}

static void __exit xilAxiTimer_exit(void)
{
        platform_driver_unregister(&xilAxiTimerPdrv);
}



module_init(xilAxiTimer_init);
module_exit(xilAxiTimer_exit);

MODULE_AUTHOR("Xilinx");
MODULE_DESCRIPTION("Userspace I/O driver for the Xilinx AXI Timer IP core.");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("uio:xilaxitimer");
