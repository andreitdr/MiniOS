#include "fdc.h"
#include "io.h"
#include "debug.h"
#include <stddef.h>

/* FDC state */
static int fdc_ready = 0;
static int fdc_motor_running = 0;

/* Timeout constants (in microseconds) */
#define FDC_TIMEOUT 1000000  /* 1 second */
#define FDC_MOTOR_DELAY 500000 /* 500ms for motor spin-up */

/* Wait for FDC to be ready for command */
static int fdc_wait_input(int timeout) {
    for (int i = 0; i < timeout; i++) {
        uint8_t msr = inb(FDC_MSR);
        if ((msr & MSR_DATA_READY) && !(msr & MSR_BUSY)) {
            return 0;
        }
        io_wait();
    }
    return -1;
}

/* Wait for FDC to have output data ready */
static int fdc_wait_output(int timeout) {
    for (int i = 0; i < timeout; i++) {
        uint8_t msr = inb(FDC_MSR);
        if ((msr & MSR_DATA_READY) && (msr & MSR_DIRECTION)) {
            return 0;
        }
        io_wait();
    }
    return -1;
}

/* Wait for FDC to be ready for command - short timeout */
static int fdc_wait_input_short(int timeout) {
    for (int i = 0; i < timeout; i++) {
        uint8_t msr = inb(FDC_MSR);
        if ((msr & MSR_DATA_READY) && !(msr & MSR_BUSY)) {
            return 0;
        }
    }
    return -1;
}

/* Read result from FDC */
static uint8_t fdc_read_byte(void) {
    return inb(FDC_FIFO);
}

/* Write command/data to FDC */
static void fdc_write_byte(uint8_t byte) {
    outb(FDC_FIFO, byte);
}

/* Read result phase (7 bytes for read/write commands) */
static int fdc_read_result(uint8_t *result, int count) {
    for (int i = 0; i < count; i++) {
        if (fdc_wait_output(FDC_TIMEOUT) < 0) {
            return -1;
        }
        result[i] = fdc_read_byte();
    }
    return 0;
}

/* Issue command and wait for completion */
static int fdc_issue_command(const uint8_t *cmd, int cmd_len, uint8_t *result, int result_len) {
    /* Wait for input ready */
    if (fdc_wait_input(FDC_TIMEOUT) < 0) {
        ERROR("FDC not ready for command");
        return -1;
    }
    
    /* Send command bytes */
    for (int i = 0; i < cmd_len; i++) {
        if (fdc_wait_input(FDC_TIMEOUT) < 0) {
            return -1;
        }
        fdc_write_byte(cmd[i]);
    }
    
    /* Read result bytes if expected */
    if (result_len > 0) {
        if (fdc_read_result(result, result_len) < 0) {
            return -1;
        }
    }
    
    return 0;
}

/* Reset FDC - clears internal state and prepares for use */
static int fdc_reset(void) {
    DEBUG("FDC Reset");
    
    /* Clear all pending interrupts by reading MSR multiple times */
    for (int i = 0; i < 10; i++) {
        volatile uint8_t msr = inb(FDC_MSR);
        (void)msr;
    }
    
    /* Pulse reset low then high */
    outb(FDC_DOR, 0x00);
    for (int i = 0; i < 1000; i++) io_wait();
    
    outb(FDC_DOR, DOR_IRQ_DMA);
    for (int i = 0; i < 1000; i++) io_wait();
    
    /* Wait for ready and clear result queue */
    for (int i = 0; i < 100000; i++) {
        uint8_t msr = inb(FDC_MSR);
        if ((msr & MSR_DATA_READY) && !(msr & MSR_BUSY)) {
            /* Clear result queue if any */
            for (int j = 0; j < 10 && (inb(FDC_MSR) & MSR_DATA_READY); j++) {
                volatile uint8_t dummy = inb(FDC_FIFO);
                (void)dummy;
            }
            DEBUG("FDC Reset complete");
            return 0;
        }
        io_wait();
    }
    
    return 0;
}

/* Initialize FDC */
int fdc_init(void) {
    INFO("FDC Init");
    
    /* Minimal initialization - just set state flags */
    /* In QEMU, the FDC is usually already initialized by BIOS */
    /* We'll attempt to wake it up by setting DOR register */
    
    uint8_t dor = inb(FDC_DOR);
    DEBUG("FDC DOR initial");
    
    /* Set IRQ_DMA and normal operation bits */
    dor |= DOR_IRQ_DMA;  /* Enable IRQ/DMA */
    dor |= DOR_MOTOR_A;  /* Turn on motor A */
    outb(FDC_DOR, dor);
    
    /* Wait for motor spin-up (about 500ms) */
    for (int i = 0; i < 500000; i++) {
        io_wait();
    }
    
    fdc_ready = 1;
    fdc_motor_running = 1;
    
    INFO("FDC initialized");
    
    return 0;
}

/* Turn motor on and wait for stabilization */
int fdc_motor_on(void) {
    if (fdc_motor_running) {
        return 0;
    }
    
    INFO("FDC motor on");
    
    /* Set motor on bit in DOR */
    uint8_t dor = inb(FDC_DOR);
    dor |= DOR_MOTOR_A;
    outb(FDC_DOR, dor);
    
    /* Wait for motor to spin up */
    for (int i = 0; i < FDC_MOTOR_DELAY; i++) {
        io_wait();
    }
    
    fdc_motor_running = 1;
    return 0;
}

/* Turn motor off */
int fdc_motor_off(void) {
    if (!fdc_motor_running) {
        return 0;
    }
    
    INFO("FDC motor off");
    
    /* Clear motor on bit in DOR */
    uint8_t dor = inb(FDC_DOR);
    dor &= ~DOR_MOTOR_A;
    outb(FDC_DOR, dor);
    
    fdc_motor_running = 0;
    return 0;
}

/* Recalibrate - seek to cylinder 0 */
static int fdc_recalibrate(void) {
    DEBUG("Recalibrate");
    
    /* Send recalibrate command directly with minimal waiting */
    outb(FDC_FIFO, CMD_RECALIBRATE);
    for (int i = 0; i < 100000; i++) io_wait();
    
    outb(FDC_FIFO, 0);  /* Drive A */
    for (int i = 0; i < 100000; i++) io_wait();
    
    /* Wait for completion */
    for (int i = 0; i < 5000000; i++) io_wait();
    
    /* Drain any result bytes */
    for (int i = 0; i < 10; i++) {
        if (inb(FDC_MSR) & MSR_DATA_READY) {
            volatile uint8_t dummy = inb(FDC_FIFO);
            (void)dummy;
        }
    }
    
    DEBUG("Recalibrate complete");
    return 0;
}

/* Seek to cylinder */
static int fdc_seek(uint8_t cylinder) {
    DEBUG("Seek");
    
    outb(FDC_FIFO, CMD_SEEK);
    for (int i = 0; i < 100000; i++) io_wait();
    
    outb(FDC_FIFO, 0);  /* Head 0 */
    for (int i = 0; i < 100000; i++) io_wait();
    
    outb(FDC_FIFO, cylinder);
    for (int i = 0; i < 100000; i++) io_wait();
    
    /* Wait for seek completion */
    for (int i = 0; i < 5000000; i++) io_wait();
    
    /* Drain any result bytes */
    for (int i = 0; i < 10; i++) {
        if (inb(FDC_MSR) & MSR_DATA_READY) {
            volatile uint8_t dummy = inb(FDC_FIFO);
            (void)dummy;
        }
    }
    
    DEBUG("Seek complete");
    return 0;
}

/* Read sector using LBA (Logical Block Address) */
int fdc_read_sector(uint32_t lba, uint8_t *buffer) {
    if (!fdc_ready) {
        ERROR("FDC not ready");
        return -1;
    }
    if (!buffer) {
        ERROR("Buffer is null");
        return -1;
    }
    
    /* Ensure motor is running */
    if (!fdc_motor_running) {
        if (fdc_motor_on() < 0) {
            return -1;
        }
    }
    
    /* Convert LBA to CHS */
    uint8_t cylinder = (lba / FDC_SECTORS) / FDC_HEADS;
    uint8_t head = (lba / FDC_SECTORS) % FDC_HEADS;
    uint8_t sector = (lba % FDC_SECTORS) + 1;  /* Sectors are 1-indexed */
    
    /* Recalibrate */
    if (fdc_recalibrate() < 0) {
        ERROR("Recalibrate failed");
        return -1;
    }
    
    /* Seek to cylinder */
    if (fdc_seek(cylinder) < 0) {
        ERROR("Seek failed");
        return -1;
    }
    
    /* Send READ_DATA command - write all bytes directly */
    outb(FDC_FIFO, CMD_READ_DATA);
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, head);  /* Head */
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, cylinder);
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, head);
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, sector);
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, 2);  /* Sector size: 2 = 512 bytes */
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, FDC_SECTORS);  /* Sectors per track */
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, 0x1B);  /* Gap3 */
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, 0xFF);  /* Data length */
    for (int i = 0; i < 50000; i++) io_wait();
    
    /* Wait for data phase */
    for (int i = 0; i < 10000000; i++) io_wait();
    
    /* Read sector data */
    for (uint32_t i = 0; i < FDC_SECTOR_SIZE; i++) {
        int timeout = 1000000;
        while (timeout-- > 0 && !(inb(FDC_MSR) & MSR_DATA_READY)) {
            io_wait();
        }
        if (timeout < 0) {
            ERROR("Read data timeout at byte");
            return -1;
        }
        buffer[i] = inb(FDC_FIFO);
    }
    
    /* Read result bytes (7 bytes) - just drain them */
    for (int i = 0; i < 7; i++) {
        int timeout = 100000;
        while (timeout-- > 0 && !(inb(FDC_MSR) & MSR_DATA_READY)) {
            io_wait();
        }
        if (timeout >= 0) {
            volatile uint8_t result = inb(FDC_FIFO);
            (void)result;
        }
    }
    
    INFO("Sector read ok");
    return 0;
}

/* Write sector */
int fdc_write_sector(uint32_t lba, const uint8_t *buffer) {
    if (!fdc_ready || !buffer) {
        return -1;
    }
    
    /* Ensure motor is running */
    if (!fdc_motor_running) {
        if (fdc_motor_on() < 0) {
            return -1;
        }
    }
    
    /* Convert LBA to CHS */
    uint8_t cylinder = (lba / FDC_SECTORS) / FDC_HEADS;
    uint8_t head = (lba / FDC_SECTORS) % FDC_HEADS;
    uint8_t sector = (lba % FDC_SECTORS) + 1;
    
    /* Recalibrate */
    if (fdc_recalibrate() < 0) {
        ERROR("Write recalibrate failed");
        return -1;
    }
    
    /* Seek to cylinder */
    if (fdc_seek(cylinder) < 0) {
        ERROR("Write seek failed");
        return -1;
    }
    
    /* Send WRITE_DATA command - write all bytes directly */
    outb(FDC_FIFO, CMD_WRITE_DATA);
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, head);  /* Head */
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, cylinder);
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, head);
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, sector);
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, 2);  /* Sector size */
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, FDC_SECTORS);
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, 0x1B);  /* Gap3 */
    for (int i = 0; i < 50000; i++) io_wait();
    
    outb(FDC_FIFO, 0xFF);  /* Data length */
    for (int i = 0; i < 50000; i++) io_wait();
    
    /* Wait for write ready */
    for (int i = 0; i < 10000000; i++) io_wait();
    
    /* Write sector data */
    for (uint32_t i = 0; i < FDC_SECTOR_SIZE; i++) {
        int timeout = 1000000;
        while (timeout-- > 0 && !(inb(FDC_MSR) & MSR_DATA_READY)) {
            io_wait();
        }
        if (timeout < 0) {
            ERROR("Write data timeout");
            return -1;
        }
        outb(FDC_FIFO, buffer[i]);
    }
    
    /* Read result bytes - just drain them */
    for (int i = 0; i < 7; i++) {
        int timeout = 100000;
        while (timeout-- > 0 && !(inb(FDC_MSR) & MSR_DATA_READY)) {
            io_wait();
        }
        if (timeout >= 0) {
            volatile uint8_t result = inb(FDC_FIFO);
            (void)result;
        }
    }
    
    INFO("Sector write ok");
    return 0;
}
