/* This test program is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

const char            laconic = 'A';
const char           *const lewd=&laconic;

/* volatile variables */

volatile char vox = 'B';
volatile unsigned char victuals = 'C';
volatile short vixen = 200;
volatile unsigned short vitriol = 300;
volatile long vellum = 1000;
volatile unsigned long valve = 2000;
volatile float vacuity = 3.0;
volatile double vertigo = 10;

/* pointers to volatile variables */

volatile char           * vampire = &vox;
volatile unsigned char  * viper  = &victuals;
volatile short          * vigour = &vixen;
volatile unsigned short * vapour = &vitriol;
volatile long           * ventricle = &vellum;
volatile unsigned long  * vigintillion = &valve;
volatile float          * vocation = &vacuity;
volatile double         * veracity = &vertigo;

/* volatile pointers to volatile variables */

volatile char           * volatile vapidity = &vox;
volatile unsigned char  * volatile velocity = &victuals;
volatile short          * volatile veneer = &vixen;
volatile unsigned short * volatile video = &vitriol;
volatile long           * volatile vacuum = &vellum;
volatile unsigned long  * volatile veniality = &valve;
volatile float          * volatile vitality = &vacuity;
volatile double         * volatile voracity = &vertigo;

/* volatile arrays */

volatile char violent[2];
volatile unsigned char violet[2];
volatile short vips[2];
volatile unsigned short virgen[2];
volatile long vulgar[2];
volatile unsigned long vulture[2];
volatile float vilify[2];
volatile double villar[2];

/* const volatile vars */

const volatile char           victor = 'Y';

/* pointers to const volatiles */

const volatile char              * victory = &victor;

/* const pointers to const volatile vars */

const volatile char              * const cavern = &victor;

/* volatile pointers to const vars */

const char                       * volatile caveat = &laconic;
const unsigned char              * volatile covenant;

/* volatile pointers to const volatile vars */

const volatile char              * volatile vizier = &victor;
const volatile unsigned char     * volatile vanadium;

/* const volatile pointers */

char                             * const volatile vane;
unsigned char                    * const volatile veldt;

/* const volatile pointers to const vars */

const char                       * const volatile cove;
const unsigned char              * const volatile cavity;
 
/* const volatile pointers to volatile vars */

volatile char                    * const volatile vagus;
volatile unsigned char           * const volatile vagrancy;
 
/* const volatile pointers to const volatile */

const volatile char              * const volatile vagary;
const volatile unsigned char     * const volatile vendor;

/* const volatile arrays */

const volatile char vindictive[2];
const volatile unsigned char vegetation[2];

int
main (void)
{
  return 0;
}
