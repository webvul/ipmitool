/*
 * Copyright (c) 2003 Sun Microsystems, Inc.  All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistribution of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * Redistribution in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of Sun Microsystems, Inc. or the names of
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * This software is provided "AS IS," without a warranty of any kind.
 * ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED.
 * SUN MICROSYSTEMS, INC. ("SUN") AND ITS LICENSORS SHALL NOT BE LIABLE
 * FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING
 * OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.  IN NO EVENT WILL
 * SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA,
 * OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
 * PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY OF
 * LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * 
 * You acknowledge that this software is not designed or intended for use
 * in the design, construction, operation or maintenance of any nuclear
 * facility.
 */

#ifndef _IPMI_SDR_H
#define _IPMI_SDR_H

#include <math.h>
#include <byteswap.h>
#include <ipmi.h>

int ipmi_sdr_main(struct ipmi_intf *, int, char **);
int utos(unsigned val, unsigned bits);

#define __TO_TOL(mtol)     (bswap_16(mtol) & 0x3f)
#define __TO_M(mtol)       (utos((((bswap_16(mtol) & 0xff00) >> 8) | ((bswap_16(mtol) & 0xc0) << 2)), 10))
#define __TO_B(bacc)       (utos((((bswap_32(bacc) & 0xff000000) >> 24) | \
                           ((bswap_32(bacc) & 0xc00000) >> 14)), 10))
#define __TO_ACC(bacc)     (((bswap_32(bacc) & 0x3f0000) >> 16) | ((bswap_32(bacc) & 0xf000) >> 6))
#define __TO_ACC_EXP(bacc) ((bswap_32(bacc) & 0xc00) >> 10)
#define __TO_R_EXP(bacc)   (utos(((bswap_32(bacc) & 0xf0) >> 4), 4))
#define __TO_B_EXP(bacc)   (utos((bswap_32(bacc) & 0xf), 4))

#define CONVERT_RAW(val, m, b, k1, k2)	(float)(((m * val) + (b * pow(10, k1))) * pow(10, k2))
#define CONVERT_TOL(val, m, k2)		(float)(((m * val) / 2) * pow(10, k2))

#define CONVERT_SENSOR_RAW(sensor, val) (float)(((__TO_M((sensor)->mtol) * val) + (__TO_B((sensor)->bacc) * pow(10, __TO_B_EXP((sensor)->bacc)))) * pow(10, __TO_R_EXP((sensor)->bacc)))
#define CONVERT_SENSOR_TOL(sensor)	(float)((((__TO_M((sensor)->mtol) * __TO_TOL((sensor)->mtol)) / 2) * pow(10, __TO_R_EXP((sensor)->bacc))))

#define GET_SDR_REPO_INFO	0x20
#define GET_SDR_ALLOC_INFO	0x21

#define SDR_SENSOR_STAT_LO_NC	(1<<0)
#define SDR_SENSOR_STAT_LO_CR	(1<<1)
#define SDR_SENSOR_STAT_LO_NR	(1<<2)
#define SDR_SENSOR_STAT_HI_NC	(1<<3)
#define SDR_SENSOR_STAT_HI_CR	(1<<4)
#define SDR_SENSOR_STAT_HI_NR	(1<<5)

struct sdr_repo_info_rs {
	unsigned char	version;	/* SDR version (51h) */
	unsigned short	count;		/* number of records */
	unsigned short	free;		/* free space in SDR */
	unsigned long	add_stamp;	/* last add timestamp */
	unsigned long	erase_stamp;	/* last del timestamp */
	unsigned char	op_support;	/* supported operations */
} __attribute__ ((packed));

#define GET_SDR_RESERVE_REPO	0x22
struct sdr_reserve_repo_rs {
	unsigned short	reserve_id;	/* reservation ID */
} __attribute__ ((packed));

#define GET_SDR		0x23
struct sdr_get_rq {
	unsigned short	reserve_id;	/* reservation ID */
	unsigned short	id;		/* record ID */
	unsigned char	offset;		/* offset into SDR */
#define GET_SDR_ENTIRE_RECORD	0xff
#define GET_SDR_MAX_LEN		30
	unsigned char	length;		/* length to read */
} __attribute__ ((packed));

struct sdr_get_rs {
	unsigned short	next;		/* next record id */
	unsigned short	id;		/* record ID */
	unsigned char	version;	/* SDR version (51h) */
#define SDR_RECORD_TYPE_FULL_SENSOR	0x01
#define SDR_RECORD_TYPE_COMPACT_SENSOR	0x02
	unsigned char	type;		/* record type */
	unsigned char	length;		/* remaining record bytes */
} __attribute__ ((packed));

struct sdr_record_compact_sensor {
	struct {
		unsigned char	owner_id;
		unsigned char	lun         : 2,	/* sensor owner lun */
				__reserved  : 2,
				channel     : 4;	/* channel number */
		unsigned char	sensor_num;		/* unique sensor number */
	} keys;

	struct {
		unsigned char	id;			/* physical entity id */
		unsigned char	instance    : 7,	/* instance number */
				logical     : 1;	/* physical/logical */
	} entity;
	
	struct {
		struct {
			unsigned char	sensor_scan  : 1,
					event_gen    : 1,
					type         : 1,
					hysteresis   : 1,
					thresholds   : 1,
					events       : 1,
					scanning     : 1,
				        __reserved   : 1;
		} init;
		struct {
			unsigned char	event_msg    : 2,
					threshold    : 2,
					hysteresis   : 2,
					rearm        : 1,
					ignore       : 1;
		} capabilities;
		unsigned char	type; /* sensor type */
	} sensor;

	unsigned char	event_type; /* event/reading type code */

	union {
		struct {
			unsigned short	assert_event;	/* assertion event mask */
			unsigned short	deassert_event;	/* de-assertion event mask */
			unsigned short	read;		/* discrete reaading mask */
		} discrete;
		struct {
			unsigned short	lower;		/* lower threshold reading mask */
			unsigned short	upper;		/* upper threshold reading mask */
			unsigned char	set;		/* settable threshold mask */
			unsigned char	read;		/* readable threshold mask */
		} threshold;
	} mask;

	struct {
		unsigned char		pct           : 1,
				modifier      : 2,
				rate          : 3,
				analog        : 2;
		struct {
			unsigned char	base;
			unsigned char	modifier;
		} type;
	} unit;

	struct {
		unsigned char	count       : 4,
				mod_type    : 2,
				__reserved  : 2;
		unsigned char	mod_offset  : 7,
				entity_inst : 1;
	} share;

	struct {
		struct {
			unsigned char	positive;
			unsigned char	negative;
		} hysteresis;
	} threshold;

	unsigned char	__reserved[3];
	unsigned char	oem;		/* reserved for OEM use */
	unsigned char	id_code;	/* sensor ID string type/length code */
	unsigned char	id_string[16];	/* sensor ID string bytes, only if id_code != 0 */

} __attribute__ ((packed));

struct sdr_record_full_sensor {
	struct {
		unsigned char	owner_id;
		unsigned char	lun         : 2,	/* sensor owner lun */
				__reserved  : 2,
				channel     : 4;	/* channel number */
		unsigned char	sensor_num;		/* unique sensor number */
	} keys;

	struct {
		unsigned char	id;			/* physical entity id */
		unsigned char	instance    : 7,	/* instance number */
				logical     : 1;	/* physical/logical */
	} entity;

	struct {
		struct {
			unsigned char	sensor_scan  : 1,
					event_gen    : 1,
					type         : 1,
					hysteresis   : 1,
					thresholds   : 1,
					events       : 1,
					scanning     : 1,
				        __reserved   : 1;
		} init;
		struct {
			unsigned char	event_msg    : 2,
					threshold    : 2,
					hysteresis   : 2,
					rearm        : 1,
					ignore       : 1;
		} capabilities;
		unsigned char	type;
	} sensor;

	unsigned char	event_type;			/* event/reading type code */

	union {
		struct {
			unsigned short	assert_event;	/* assertion event mask */
			unsigned short	deassert_event;	/* de-assertion event mask */
			unsigned short	read;		/* discrete reaading mask */
		} discrete;
		struct {
			unsigned short	lower;		/* lower threshold reading mask */
			unsigned short	upper;		/* upper threshold reading mask */
			unsigned char	set;		/* settable threshold mask */
			unsigned char	read;		/* readable threshold mask */
		} threshold;
	} mask;

	struct {
		unsigned char	pct           : 1,
				modifier      : 2,
				rate          : 3,
				analog        : 2;
		struct {
			unsigned char	base;
			unsigned char	modifier;
		} type;
	} unit;

	unsigned char	linearization;	/* 70h=non linear, 71h-7Fh=non linear, OEM */

	unsigned short	mtol;		/* M, tolerance */

	unsigned long	bacc;		/* accuracy, B, Bexp, Rexp */

	struct {
		unsigned char	nominal_read  : 1,	/* nominal reading field specified */
				normal_max    : 1,	/* normal max field specified */
				normal_min    : 1,	/* normal min field specified */
				__reserved    : 5;
	} analog_flag;

	unsigned char	nominal_read;	/* nominal reading, raw value */
	unsigned char	normal_max;	/* normal maximum, raw value */
	unsigned char	normal_min;	/* normal minimum, raw value */
	unsigned char	sensor_max;	/* sensor maximum, raw value */
	unsigned char	sensor_min;	/* sensor minimum, raw value */

	struct {
		struct {
			unsigned char	non_recover;
			unsigned char	critical;
			unsigned char	non_critical;
		} upper;
		struct {
			unsigned char	non_recover;
			unsigned char	critical;
			unsigned char	non_critical;
		} lower;
		struct {
			unsigned char	positive;
			unsigned char	negative;
		} hysteresis;
	} threshold;
	unsigned char	__reserved[2];
	unsigned char	oem;		/* reserved for OEM use */
	unsigned char	id_code;	/* sensor ID string type/length code */
	unsigned char	id_string[16];	/* sensor ID string bytes, only if id_code != 0 */
} __attribute__ ((packed));

/* unit description codes (IPMI v1.5 section 37.16) */
#define UNIT_MAX	0x90
static const char * unit_desc[] __attribute__((unused)) = {
	"unspecified",
	"degrees C", "degrees F", "degrees K",
	"Volts", "Amps", "Watts", "Joules",
	"Coulombs", "VA", "Nits",
	"lumen", "lux", "Candela",
	"kPa", "PSI", "Newton",
	"CFM", "RPM", "Hz",
	"microsecond", "millisecond", "second", "minute", "hour", "day", "week",
	"mil", "inches", "feet", "cu in", "cu feet", "mm", "cm", "m", "cu cm", "cu m",
	"liters", "fluid ounce",
	"radians", "steradians", "revolutions", "cycles", "gravities",
	"ounce", "pound", "ft-lb", "oz-in",
	"gauss", "gilberts", "henry", "millihenry",
	"farad", "microfarad", "ohms", "siemens", "mole", "becquerel",
	"PPM", "reserved",
	"Decibels", "DbA", "DbC",
	"gray", "sievert", "color temp deg K",
	"bit", "kilobit", "megabit", "gigabit",
	"byte", "kilobyte", "megabyte", "gigabyte",
	"word", "dword", "qword", "line",
	"hit", "miss", "retry", "reset",
	"overflow", "underrun",
	"collision", "packets",
	"messages", "characters",
	"error", "correctable error", "uncorrectable error",
};

#endif  /* _IPMI_SDR_H */

