/* Name: main.c
 * Project: USBaspLoader
 * Author: Christian Starkjohann
 * Creation Date: 2007-12-08
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt)
 * This Revision: $Id: main.c 786 2010-05-30 20:41:40Z cs $
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
//#include <string.h>

static void leaveBootloader() __attribute__((__noreturn__));

#include "bootloaderconfig.h"
#include "usbdrv/usbdrv.c"

/* ------------------------------------------------------------------------ */

/* Request constants used by USBasp */
#define USBASP_FUNC_CONNECT         1
#define USBASP_FUNC_DISCONNECT      2
#define USBASP_FUNC_TRANSMIT        3
#define USBASP_FUNC_READFLASH       4
#define USBASP_FUNC_ENABLEPROG      5
#define USBASP_FUNC_WRITEFLASH      6
#define USBASP_FUNC_READEEPROM      7
#define USBASP_FUNC_WRITEEEPROM     8
#define USBASP_FUNC_SETLONGADDRESS  9

/* ------------------------------------------------------------------------ */

#ifndef ulong
#   define ulong    unsigned long
#endif
#ifndef uint
#   define uint     unsigned int
#endif

/* defaults if not in config file: */
#ifndef HAVE_EEPROM_PAGED_ACCESS
#   define HAVE_EEPROM_PAGED_ACCESS 0
#endif
#ifndef HAVE_EEPROM_BYTE_ACCESS
#   define HAVE_EEPROM_BYTE_ACCESS  0
#endif
#ifndef BOOTLOADER_CAN_EXIT
#   define  BOOTLOADER_CAN_EXIT     0
#endif

/* allow compatibility with avrusbboot's bootloaderconfig.h: */
#ifdef BOOTLOADER_INIT
#   define bootLoaderInit()         BOOTLOADER_INIT
#   define bootLoaderExit()
#endif
#ifdef BOOTLOADER_CONDITION
#   define bootLoaderCondition()    BOOTLOADER_CONDITION
#endif

/* device compatibility: */
#ifndef GICR    /* ATMega*8 don't have GICR, use MCUCR instead */
#   define GICR     MCUCR
#endif

/* ------------------------------------------------------------------------ */

#if (FLASHEND) > 0xffff /* we need long addressing */
#   define CURRENT_ADDRESS  currentAddress.l
#   define addr_t           ulong
#else
#   define CURRENT_ADDRESS  currentAddress.w[0]
#   define addr_t           uint
#endif

typedef union longConverter {
	addr_t l;
	uint w[sizeof(addr_t) / 2];
	uchar b[sizeof(addr_t)];
} longConverter_t;

static uchar requestBootLoaderExit;
static longConverter_t currentAddress; /* in bytes */
static uchar bytesRemaining;
static uchar isLastPage;
#if HAVE_EEPROM_PAGED_ACCESS
static uchar currentRequest;
#else
static const uchar currentRequest = 0;
#endif

static const uchar signatureBytes[4] = {
#ifdef SIGNATURE_BYTES
		SIGNATURE_BYTES
#elif defined (__AVR_ATmega8__) || defined (__AVR_ATmega8HVA__)
		0x1e, 0x93, 0x07, 0
#elif defined (__AVR_ATmega48__) || defined (__AVR_ATmega48P__)
		0x1e, 0x92, 0x05, 0
#elif defined (__AVR_ATmega88__) || defined (__AVR_ATmega88P__)
		0x1e, 0x93, 0x0a, 0
#elif defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__)
		0x1e, 0x94, 0x06, 0
#elif defined (__AVR_ATmega328P__)
		0x1e, 0x95, 0x0f, 0
#else
#   error "Device signature is not known, please edit main.c!"
#endif
		};

/* ------------------------------------------------------------------------ */

static void (*nullVector)(void) __attribute__((__noreturn__));

static void leaveBootloader() {
	cli();
	usbDeviceDisconnect(); /* do this while interrupts are disabled */
	bootLoaderExit();
//	USB_INTR_ENABLE = 0;
//	USB_INTR_CFG = 0; /* also reset config bits */
	GICR = (1 << IVCE); /* enable change of interrupt vectors */
	GICR = (0 << IVSEL); /* move interrupts to application flash section */
	/* We must go through a global function pointer variable instead of writing
	 *  ((void (*)(void))0)();
	 * because the compiler optimizes a constant 0 to "rcall 0" which is not
	 * handled correctly by the assembler.
	 */
	nullVector();
}

/* ------------------------------------------------------------------------ */

uchar usbFunctionSetup(uchar data[8]) {
	usbRequest_t *rq = (void *) data;
	uchar len = 0;
	static uchar replyBuffer[4];

	usbMsgPtr = replyBuffer;

	usbWord_t address;


	switch (rq->bRequest) {
	case USBASP_FUNC_TRANSMIT: /* emulate parts of ISP protocol */
		address.bytes[1] = rq->wValue.bytes[1];
		address.bytes[0] = rq->wIndex.bytes[0];

		if (rq->wValue.bytes[0] == 0x30) { /* read signature */
			// reuse len, its going to be overwritten anyway
			len = rq->wIndex.bytes[0] & 3;
			replyBuffer[3] = signatureBytes[len];
		} else if (rq->wValue.bytes[0] == 0xa0) { /* read EEPROM byte */
			replyBuffer[3] = eeprom_read_byte((void *) address.word);
		} else if (rq->wValue.bytes[0] == 0xc0) { /* write EEPROM byte */
			eeprom_write_byte((void *) address.word, rq->wIndex.bytes[1]);
		}

		len = 4;
		break;
	case USBASP_FUNC_ENABLEPROG:
		/* replyBuffer[0] = 0; is never touched and thus always 0 which means success */
		len = 1;
		break;
	case USBASP_FUNC_DISCONNECT:
		requestBootLoaderExit = 1;
		break;
	default: // rq->bRequest >= USBASP_FUNC_READFLASH && rq->bRequest <= USBASP_FUNC_SETLONGADDRESS
		currentAddress.w[0] = rq->wValue.word;
		bytesRemaining = rq->wLength.bytes[0];
		isLastPage = rq->wIndex.bytes[1] & 0x02;
#if HAVE_EEPROM_PAGED_ACCESS
		currentRequest = rq->bRequest;
#endif
		len = 0xff; /* hand over to usbFunctionRead() / usbFunctionWrite() */
	}
	return len;
}

uchar usbFunctionWrite(uchar *data, uchar len) {
	uchar isLast;
	uint8_t i;

	timeout = TIMEOUT_DISABLED;

	if (len > bytesRemaining) {
		len = bytesRemaining;
	}
	bytesRemaining -= len;
	isLast = bytesRemaining == 0;
	if (currentRequest == USBASP_FUNC_WRITEEEPROM) {
		for (i = 0; i < len; i++) {
			eeprom_write_byte((void *) (currentAddress.w[0]++), *data++);
		}
	} else {
		for (i = 0; i < len;) {
			LED_PIN |= _BV(LED_BIT);
#if !HAVE_CHIP_ERASE
			if ((currentAddress.w[0] & (SPM_PAGESIZE - 1)) == 0) { /* if page start: erase */
#   ifndef NO_FLASH_WRITE
//				cli();
				boot_page_erase(CURRENT_ADDRESS); /* erase page */
//				sei();
				boot_spm_busy_wait(); /* wait until page is erased */
#   endif
			}
#endif
			i += 2;
			cli();
			boot_page_fill(CURRENT_ADDRESS, *(short *)data);
			sei();
			CURRENT_ADDRESS += 2;
			data += 2;
			/* write page when we cross page boundary or we have the last partial page */
			if ((currentAddress.w[0] & (SPM_PAGESIZE - 1)) == 0 || (isLast && i >= len && isLastPage)) {
#ifndef NO_FLASH_WRITE
				cli();
				boot_page_write(CURRENT_ADDRESS - 2);
				sei();
				boot_spm_busy_wait();
				cli();
				boot_rww_enable();
				sei();
#endif
			}
		}
	}
	return isLast;
}

uchar usbFunctionRead(uchar *data, uchar len) {
	uchar i;

	timeout = TIMEOUT_DISABLED;

	if (len > bytesRemaining)
		len = bytesRemaining;
	bytesRemaining -= len;
	for (i = 0; i < len; i++) {
		LED_PIN |= _BV(LED_BIT);
		if (currentRequest >= USBASP_FUNC_READEEPROM) {
			*data = eeprom_read_byte((void *) currentAddress.w[0]);
		} else {
			*data = pgm_read_byte((void *)CURRENT_ADDRESS);
		}
		data++;
		CURRENT_ADDRESS++;
	}
	return len;
}

/* ------------------------------------------------------------------------ */

static inline void initForUsbConnectivity(void) {
	usbInit();

	/* Wait for the forced line (D-) to be released from ground */
	while ((PIND & (1 << JUMPER_BIT)) == 0) {}

	/* enforce USB re-enumerate: */
	usbDeviceDisconnect(); /* do this while interrupts are disabled */
	USBOUT &= ~_BV(JUMPER_BIT);

	/* fake USB disconnect for > 250 ms */
	uint8_t i = 6;
	while (i--) {
		uint8_t ticksPer100ms = 10;
		while (--ticksPer100ms) {
			while (TIMERVAL < 195) {}
			TIMERVAL = 0;
		}
		LED_PIN |= _BV(LED_BIT);
	}

	usbDeviceConnect();
	sei();
}

int __attribute__((noreturn)) main(void) {
	/* initialize  */
	bootLoaderInit();

#ifndef NO_FLASH_WRITE
	GICR = (1 << IVCE); /* enable change of interrupt vectors */
	GICR = (1 << IVSEL); /* move interrupts to boot flash section */
#endif
	if (bootLoaderCondition()) {
		initForUsbConnectivity();

		uint8_t exitFlag = 0;
		for (;;) { // main loop
			usbPoll();
#if BOOTLOADER_CAN_EXIT
			if (TIMERVAL >= 195) { /* 1/100th of a second has elapsed */
				TIMERVAL = 0;

				if (!ticks--) {
					if (TIMEOUT_DISABLED != timeout) {
						ticks = 100;
						timeout--;
					}
				}
			}
			if ((0 == timeout) || requestBootLoaderExit) {
				if (!--exitFlag) {
					break;
				}
			}
#endif
		} /* main event loop */
	}
	leaveBootloader();
}

/* ------------------------------------------------------------------------ */
