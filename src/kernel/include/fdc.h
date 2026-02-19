#ifndef FDC_H
#define FDC_H

#include <stdint.h>

/* Floppy Disk Controller I/O Ports */
#define FDC_DOR         0x3F2  /* Digital Output Register */
#define FDC_MSR         0x3F4  /* Main Status Register */
#define FDC_FIFO        0x3F5  /* Data FIFO */
#define FDC_CCR         0x3F7  /* Configuration Control Register */

/* DOR (Digital Output Register) bits */
#define DOR_DRIVE_A     0x00
#define DOR_MOTOR_A     (1 << 4)
#define DOR_MOTOR_B     (1 << 5)
#define DOR_IRQ_DMA     (1 << 3)
#define DOR_RESET       (1 << 2)
#define DOR_NOT_RESET   (1 << 2)

/* MSR (Main Status Register) bits */
#define MSR_BUSY        (1 << 4)
#define MSR_DIRECTION   (1 << 6)  /* 1 = FDC to CPU (read) */
#define MSR_DATA_READY  (1 << 7)  /* 1 = data register ready */

/* FDC Commands */
#define CMD_SPECIFY     0x03
#define CMD_RECALIBRATE 0x07
#define CMD_SEEK        0x0F
#define CMD_READ_DATA   0x06
#define CMD_WRITE_DATA  0x05
#define CMD_READ_ID     0x0A
#define CMD_FORMAT_TRACK 0x0D
#define CMD_SENSE_STAT  0x04

/* Default values for 1.44MB floppy */
#define FDC_TRACKS      80      /* Cylinders */
#define FDC_HEADS       2       /* Sides */
#define FDC_SECTORS     18      /* Sectors per track */
#define FDC_SECTOR_SIZE 512     /* Bytes per sector */

/* FDC States */
typedef enum {
    FDC_SUCCESS = 0,
    FDC_ERROR_TIMEOUT = -1,
    FDC_ERROR_NOT_READY = -2,
    FDC_ERROR_SEEK = -3,
    FDC_ERROR_IO = -4,
} FdcError;

/* Function prototypes */
int fdc_init(void);
int fdc_read_sector(uint32_t lba, uint8_t *buffer);
int fdc_write_sector(uint32_t lba, const uint8_t *buffer);
int fdc_motor_on(void);
int fdc_motor_off(void);

#endif /* FDC_H */
