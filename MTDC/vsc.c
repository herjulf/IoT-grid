/*
 * vsc.c an implementation a of Voltage Source Controller (VSC) for the DC-grid
 *
 * When load increases the current/load is shared among the available terminals
 * according according to their capacity. The higher capacity the higher load.
 * This a part of the basic droop control proposed for various grid. 
 * The parameters used in the work below is illustrates a small DC "pico" grid.
 *
 *  Robert Olsson  <robert@roolss.kth.se>  
 *                 <robert@radio-sensors.com>
 *
 *  This work is cofunded by Radio Sensors AB
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *  Member KTH IoT grid team
 *  Project site:
 *  https://github.com/herjulf/IoT-grid.git
 */

#include <stdio.h>
#include <string.h>

#define MTDC_MAX 2     /* Max MTDC terminals */

/* 
   DCGRID_MAX to DCGRID_MIN defines the operational voltage interval 
   for load sharing according to droop control algorithm.
*/


#define DCGRID_MAX 15  /* Max Grid Voltage droop */
#define DCGRID_MIN 12  /* Min Grid Voltage droop */

/* Terminal/Converter parameter settings */

struct vsc
{
  double c1;       /* Linear coeff 1 */
  double c2;       /* Linear coeff 2*/
  double p;        /* Current Power */
  double p_max;
  double u;        /* Terminal Voltage */
  double u_max;
  double i;        /* Terminal Current */
  double i_min;
};

void vsc_droop_init(struct vsc *v, int id)
{
  if(id == 0) {
    v->c1 = DCGRID_MAX;
    v->c2 = -0.07;
    v->p_max = 60;
  }

  if(id == 1) {
    v->c1 = DCGRID_MAX;
    v->c2 = -0.03;
    v->p_max = 100;
  }

  v->i_min = 0.1;
  v->u_max = v->c1;
  v->u = v->u_max;
}

void vsc_droop(struct vsc *v, double i)
{

  if(i < v->i_min) {
    v->u = v->u_max;
    return;
  }

  v->u = v->c1 / (1 - i * v->c2);
  v->p = v->u * i;

  if(v->p >= v->p_max) {
    v->p = v->p_max;
    v->u = v->p_max/i;
    return;
  }
}

/* A simple test program */

int main()
{
  struct vsc t[MTDC_MAX];
  double i;
  int p1, p2; 

  memset(t, 0, sizeof(t));
  i = 0;

  for(p1=0; p1 < MTDC_MAX; p1++)
    vsc_droop_init(&t[p1], p1);

  for(p1=0; p1 < DCGRID_MAX; p1++) {

    printf("i=%-5.2f ", i);
    for(p2=0; p2 < MTDC_MAX; p2++) {
      vsc_droop(&t[p2], i);
      if(0)
	printf("p[%-d]=%-5.2f u[%-d]=%-5.2f ", p2, t[p2].p, p2, t[p2].u);
      else
	printf("%-5.2f %-5.2f ", t[p2].p, t[p2].u);
    }
    printf("\n");
    i = i + 0.7;
  }
  return 0;
}
