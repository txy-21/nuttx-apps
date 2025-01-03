/****************************************************************************
 * apps/examples/pwlines/pwlines_update.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <debug.h>
#include <fixedmath.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>

#include "pwlines_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_NX_ANTIALIASING
  /* If anti-aliasing is enabled, then we must clear a slightly
   * larger region to prevent weird edge effects.
   */

#  define CLEAR_WIDTH (CONFIG_EXAMPLES_PWLINES_LINEWIDTH + 2)
#else
#  define CLEAR_WIDTH CONFIG_EXAMPLES_PWLINES_LINEWIDTH
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwlines_circle
 *
 * Description:
 *   Draw a circle in each window.
 *
 ****************************************************************************/

void pwlines_circle(FAR struct pwlines_state_s *st)
{
  FAR struct pwlines_window_s *wndo;
  nxgl_coord_t maxradius;
  nxgl_coord_t radius;
  int wndx;
  int ret;

  for (wndx = 0; wndx < 3; wndx++)
    {
      wndo = &st->wndo[wndx];

      /* Get the maximum radius and center of the circle */

      maxradius       = MIN(wndo->size.h, wndo->size.h) >> 1;
      wndo->center.x  = wndo->size.w >> 1;
      wndo->center.y  = wndo->size.h >> 1;

      /* Draw a circular background */

      radius = maxradius - ((CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 1) / 2);
      ret = nxtk_fillcirclewindow(wndo->hwnd, &wndo->center, radius,
                                  st->facecolor);
      if (ret < 0)
        {
          printf("pwlines_update: ERROR: nxtk_fillcirclewindow failed: %d\n",
                 ret);
        }

      /* Draw the circular border */

      ret = nxtk_drawcirclewindow(wndo->hwnd, &wndo->center, radius,
                                  CONFIG_EXAMPLES_PWLINES_BORDERWIDTH,
                                  st->bordercolor);
      if (ret < 0)
        {
          printf("pwlines_update: ERROR: nxtk_drawcirclewindow failed: %d\n",
                 ret);
        }

      /* Back off the radius to account for the thickness of border line
       * and with a big fudge factor that will (hopefully) prevent the
       * corners of the lines from overwriting the border.  This is overly
       * complicated here because we don't assume anything about the screen
       * resolution or the borderwidth or the line thickness (and there are
       * certainly some smarter ways to do this).
       */

      if (maxradius > (CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 80))
        {
          wndo->radius = maxradius -
                         (CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 40);
        }
      else if (maxradius > (CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 60))
        {
          wndo->radius = maxradius -
                         (CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 30);
        }
      else if (maxradius > (CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 40))
        {
          wndo->radius = maxradius -
                         (CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 20);
        }
      else if (maxradius > (CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 20))
        {
          wndo->radius = maxradius -
                         (CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 10);
        }
      else if (maxradius > (CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 10))
        {
          wndo->radius = maxradius -
                         (CONFIG_EXAMPLES_PWLINES_BORDERWIDTH + 5);
        }
      else
        {
          wndo->radius = maxradius - CONFIG_EXAMPLES_PWLINES_BORDERWIDTH;
        }

      wndo->angle = 0;
      wndo->previous.pt1.x = wndo->center.x;
      wndo->previous.pt1.y = wndo->center.y;
      wndo->previous.pt2.x = wndo->center.x;
      wndo->previous.pt2.y = wndo->center.y;
    }
}

/****************************************************************************
 * Name: pwlines_update
 *
 * Description:
 *   Update line motion.
 *
 ****************************************************************************/

void pwlines_update(FAR struct pwlines_state_s *st)
{
  FAR struct pwlines_window_s *wndo;
  struct nxgl_vector_s vector;
  nxgl_coord_t halfx;
  nxgl_coord_t halfy;
  b16_t sinangle;
  b16_t cosangle;
  int wndx;
  int ret;

  /* Update each window */

  for (wndx = 0; wndx < 3; wndx++)
    {
      wndo = &st->wndo[wndx];

      /* Determine the position of the line on this pass */

      sinangle = b16sin(wndo->angle);
      halfx = b16toi(b16muli(sinangle, wndo->radius));

      cosangle = b16cos(wndo->angle);
      halfy = b16toi(b16muli(cosangle, wndo->radius));

      vector.pt1.x = wndo->center.x + halfx;
      vector.pt1.y = wndo->center.y + halfy;
      vector.pt2.x = wndo->center.x - halfx;
      vector.pt2.y = wndo->center.y - halfy;

      printf("Angle: %08" PRIx32 " vector: (%d,%d)->(%d,%d)\n",
             (uint32_t)wndo->angle, vector.pt1.x, vector.pt1.y,
             vector.pt2.x, vector.pt2.y);

      /* Clear the previous line by overwriting it with the circle face
       * color
       */

      ret = nx_drawline(wndo->hwnd, &wndo->previous, CLEAR_WIDTH,
                        st->facecolor, NX_LINECAP_NONE);
      if (ret < 0)
        {
          printf("pwlines_update: nx_drawline failed clearing: %d\n", ret);
        }

      /* Draw the new line */

      ret = nx_drawline(wndo->hwnd, &vector,
                        CONFIG_EXAMPLES_PWLINES_LINEWIDTH, st->linecolor,
                        NX_LINECAP_NONE);
      if (ret < 0)
        {
          printf("pwlines_update: nx_drawline failed clearing: %d\n", ret);
        }

#ifdef CONFIG_NX_ANTIALIASING
      /* If anti-aliasing is enabled, then we must clear a slightly
       * larger region to prevent weird edge effects.
       */

      halfx = b16toi(b16muli(sinangle, wndo->radius + 1));
      halfy = b16toi(b16muli(cosangle, wndo->radius + 1));

      wndo->previous.pt1.x = wndo->center.x + halfx;
      wndo->previous.pt1.y = wndo->center.y + halfy;
      wndo->previous.pt2.x = wndo->center.x - halfx;
      wndo->previous.pt2.y = wndo->center.y - halfy;
#else
      memcpy(&wndo->previous, &vector, sizeof(struct nxgl_vector_s));
#endif

      /* Set up for the next time through the loop. */

      wndo->angle += b16PI / 16;  /* 32 angular positions in full circle */

      /* Check if we have gone all the way around */

      if (wndo->angle > (31 *  (2 * b16PI) / 32))
        {
          /* Wrap back to zero and continue with the test */

          wndo->angle = 0;
        }
    }
}
