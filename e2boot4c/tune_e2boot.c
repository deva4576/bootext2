/* Copyright 1998 John F. Reiser.  All rights reserved.
   This file is part of bootext2 (boot block for ext2 filesystem)

   bootext2 is free software; you can redistribute it and/or modify it under the
   terms of the GNU General Public License as published by the Free Software 
   Foundation; either version 2, or (at your option) any later version.  

   bootext2 is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more 
   details.  

   You should have received a copy of the GNU General Public License along with 
   bootext2; see the file COPYING.  If not, write to the Free Software Foundation, 
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static void
uread(int fd, void *buf, int len, char const *msg)
{
	int const bytes = read(fd, buf, len);
	if (bytes!=len) {
		fprintf(stderr,"%s: read only %d of %d bytes\n",
			msg, bytes, len);
		exit(1);
	}
}

static void
uwrite(int fd, void *buf, int len, char const *msg)
{
	int const bytes = write(fd, buf, len);
	if (bytes!=len) {
		fprintf(stderr,"%s: wrote only %d of %d bytes\n",
			msg, bytes, len);
		exit(1);
	}
}

static void
usage()
{
	fprintf(stderr,"tune_e2boot <.bin >.br -wait=n -boot=\"<kernel>[\\|<initial_ramdisk>] args\"\n");
	fprintf(stderr,"example: tune_e2boot <e2boot.bin >e2boot.br -wait=20"
	" -boot=\"/boot/vmlinuz|initrd root=/dev/hda11 mem=96M\"\n");
}

unsigned char bootrec[1024];

void
main(int argc, char **argv)
{
	unsigned long countdown = 20;
	char const *bootline = "/boot/vmlinuz";

	char const *dev_name;
	int fd_dev;

	if (argc<2) {
		usage();
		exit(1);
	}
	uread(0, &bootrec, 5, ".bin hdr");  /* discard header */
	uread(0, &bootrec, sizeof(bootrec), ".bin text");

	for (	(argv+=1), (argc-=1);
		argc > 0;
		(++argv), (--argc)
	) if ('-'==**argv) {
		char *value = strchr(*argv, '=');
		if (0!=value) {
			*value++ = 0;
		}
		if (0==strcmp("-wait", *argv)) {
			if (0!=value) {
				countdown = atoi(value);
				if (1 < countdown && countdown < 4000) {
					countdown *= 1000000;
				}
			}
		}
		else if (0==strcmp("-boot", *argv)) {
			if (0!=value) {
				bootline = value;
			}
		}
		else {
			fprintf(stderr,"ignoring option %s\n", *argv);
		}
	}
	else {
		break;
	}

	/* Somebody please tell me how to read an .obj symbol table
	   so I can do these magic offsets symbolically!
	*/
#define USE_MBR 1
#define USE_PTAB 0
#define USE_LABEL 0

#if USE_MBR
#define COUNT 0x353
#define BOOTLINE 0x3b0
#endif

#if USE_PTAB
#define COUNT 0x36a
#define BOOTLINE 0x3c2
#endif

#if USE_LABEL
#define COUNT 0x3a3
#define BOOTLINE 0x3f2
#endif

	memcpy(&bootrec[COUNT -4], &countdown, sizeof(countdown));
	if (0==countdown) {/* wait forever: no count at all */
		bootrec[COUNT -6] = 0xeb;  /* JMP rel8 */
		bootrec[COUNT -5] = 0x0d;  /* skip to getcWait */
	}
    {
	int const deficit = (BOOTLINE + 1+strlen(bootline)) - (1024 -0);
	if (0 < deficit) {
		fprintf(stderr,"-boot is %d bytes too long\n", deficit);
		exit(1);
	}
    }
	strcpy(&bootrec[BOOTLINE], bootline);
	uwrite(1, &bootrec, sizeof(bootrec), "stdout");
	close(1);

	dup(2);  /* put following diagnostic output onto stderr */
    {
	char *param;
	param = strchr(bootline,' ');
	if (0!=param) {  /* cut off parameters */
		*param = '\0';
	}
	param = strchr(bootline,'|');
	if (0!=param) {  /* cut off ramdisk */
		*param = '\0';
	}
    }
    {
	char *option="srRv";
	char cmd[1024];
	char c;
	sprintf(cmd,"/usr/sbin/rdev %s", bootline);
	system(cmd);
	while (0!=(c= *option++)) {
		sprintf(cmd,"/usr/sbin/rdev -%c %s", c, bootline);
		system(cmd);
	}
    }

	exit(0);
}
