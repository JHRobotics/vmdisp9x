/* gtf.c  Generate mode timings using the GTF Timing Standard
 *
 * gcc gtf.c -o gtf -lm -Wall
 *
 * Copyright (c) 2001, Andy Ritger  aritger@nvidia.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * o Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer
 *   in the documentation and/or other materials provided with the
 *   distribution.
 * o Neither the name of NVIDIA nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 * This program is based on the Generalized Timing Formula(GTF TM)
 * Standard Version: 1.0, Revision: 1.0
 *
 * The GTF Document contains the following Copyright information:
 *
 * Copyright (c) 1994, 1995, 1996 - Video Electronics Standards
 * Association. Duplication of this document within VESA member
 * companies for review purposes is permitted. All other rights
 * reserved.
 *
 * While every precaution has been taken in the preparation
 * of this standard, the Video Electronics Standards Association and
 * its contributors assume no responsibility for errors or omissions,
 * and make no warranties, expressed or implied, of functionality
 * of suitability for any purpose. The sample code contained within
 * this standard may be used without restriction.
 *
 *
 *
 * The GTF EXCEL(TM) SPREADSHEET, a sample (and the definitive)
 * implementation of the GTF Timing Standard, is available at:
 *
 * ftp://ftp.vesa.org/pub/GTF/GTF_V1R1.xls
 *
 *
 *
 * This program takes a desired resolution and vertical refresh rate,
 * and computes mode timings according to the GTF Timing Standard.
 * These mode timings can then be formatted as an XServer modeline
 * or a mode description for use by fbset(8).
 *
 *
 *
 * NOTES:
 *
 * The GTF allows for computation of "margins" (the visible border
 * surrounding the addressable video); on most non-overscan type
 * systems, the margin period is zero.  I've implemented the margin
 * computations but not enabled it because 1) I don't really have
 * any experience with this, and 2) neither XServer modelines nor
 * fbset fb.modes provide an obvious way for margin timings to be
 * included in their mode descriptions (needs more investigation).
 *
 * The GTF provides for computation of interlaced mode timings;
 * I've implemented the computations but not enabled them, yet.
 * I should probably enable and test this at some point.
 *
 *
 *
 * TODO:
 *
 * o Add support for interlaced modes.
 *
 * o Implement the other portions of the GTF: compute mode timings
 *   given either the desired pixel clock or the desired horizontal
 *   frequency.
 *
 * o It would be nice if this were more general purpose to do things
 *   outside the scope of the GTF: like generate double scan mode
 *   timings, for example.
 *
 * o Printing digits to the right of the decimal point when the
 *   digits are 0 annoys me.
 *
 * o Error checking.
 *
 *
 *
 * Integer version by Jaroslav Hensl, 2026
 *
 */
#include "winhack.h"
#include "vmm.h"
#include "vxd.h"

#include "vxd_lib.h"
#include "3d_accel.h"

#include "vesa.h"
#include "mtrr.h"

#include "code32.h"

#define MARGIN_PERMILLE   18    /* % of active vertical image                */
#define CELL_GRAN         8    /* assumed character cell granularity        */
#define MIN_PORCH         1     /* minimum front porch                       */
#define V_SYNC_RQD        3     /* width of vsync in lines                   */
#define H_SYNC_PERCENT    8     /* width of hsync as % of total line         */
#define MIN_VSYNC_PLUS_BP 550  /* min time of vsync + back porch (microsec) */
#define M                 600  /* blanking formula gradient                 */
#define C                 40   /* blanking formula offset                   */
#define K                 128  /* blanking formula scaling factor           */
#define J                 20   /* blanking formula scaling factor           */

/* C' and M' are part of the Blanking Duty Cycle computation */

#define C_PRIME           30 /* (((C - J) * K/256.0) + J) */
#define M_PRIME           300 /*  (K/256.0 * M) */

/* struct definitions */

typedef struct __mode
{
    int hr, hss, hse, hfl;
    int vr, vss, vse, vfl;
    long pclk, h_freq, v_freq;
} mode;

#define print_value(n, name, val)

/*
 * vert_refresh() - as defined by the GTF Timing Standard, compute the
 * Stage 1 Parameters using the vertical refresh frequency.  In other
 * words: input a desired resolution and desired refresh rate, and
 * output the GTF mode timings.
 *
 * XXX All the code is in place to compute interlaced modes, but I don't
 * feel like testing it right now.
 *
 * XXX margin computations are implemented but not tested (nor used by
 * XFree86 of fbset mode descriptions, from what I can tell).
 */

void vert_refresh (int h_pixels, int v_lines, long freq,
                    int interlaced, int margins, mode *m)
{
    long h_pixels_rnd;
    long v_lines_rnd;
    long v_field_rate_rqd;
    long top_margin;
    long bottom_margin;
    long interlace;
    long h_period_est_k;
    long vsync_plus_bp;
    long vsync_plus_bp_k;
    long v_back_porch;
    long total_v_lines;
    long v_field_rate_est_k;
    long h_period_k;
    long v_field_rate;
    long v_frame_rate;
    long left_margin;
    long right_margin;
    long total_active_pixels;
    long ideal_duty_cycle_k;
    long h_blank;
    long total_pixels;
    long pixel_freq_k;
    long h_freq_k;

    long h_sync;
    long h_front_porch;
    long v_odd_front_porch_lines;

    /*  1. In order to give correct results, the number of horizontal
     *  pixels requested is first processed to ensure that it is divisible
     *  by the character size, by rounding it to the nearest character
     *  cell boundary:
     *
     *  [H PIXELS RND] = ((ROUND([H PIXELS]/[CELL GRAN RND],0))*[CELLGRAN RND])
     */

    h_pixels_rnd = (h_pixels + 4) & 0xFFFFFFF8;

    print_value(1, "[H PIXELS RND]", h_pixels_rnd);

    /*  2. If interlace is requested, the number of vertical lines assumed
     *  by the calculation must be halved, as the computation calculates
     *  the number of vertical lines per field. In either case, the
     *  number of lines is rounded to the nearest integer.
     *
     *  [V LINES RND] = IF([INT RQD?]="y", ROUND([V LINES]/2,0),
     *                                     ROUND([V LINES],0))
     */

    v_lines_rnd = interlaced ? v_lines / 2 : v_lines;

    print_value(2, "[V LINES RND]", v_lines_rnd);

    /*  3. Find the frame rate required:
     *
     *  [V FIELD RATE RQD] = IF([INT RQD?]="y", [I/P FREQ RQD]*2,
     *                                          [I/P FREQ RQD])
     */

    v_field_rate_rqd = interlaced ? freq * 2 : (freq);

    print_value(3, "[V FIELD RATE RQD]", v_field_rate_rqd);

    /*  4. Find number of lines in Top margin:
     *
     *  [TOP MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
     *          ROUND(([MARGIN%]/100*[V LINES RND]),0),
     *          0)
     */

    top_margin = margins ? (v_lines_rnd * MARGIN_PERMILLE)/1000 : 0;

    print_value(4, "[TOP MARGIN (LINES)]", top_margin);

    /*  5. Find number of lines in Bottom margin:
     *
     *  [BOT MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
     *          ROUND(([MARGIN%]/100*[V LINES RND]),0),
     *          0)
     */

    bottom_margin =
        margins ? (v_lines_rnd * MARGIN_PERMILLE)/1000 : 0;

    print_value(5, "[BOT MARGIN (LINES)]", bottom_margin);

    /*  6. If interlace is required, then set variable [INTERLACE]=0.5:
     *
     *  [INTERLACE]=(IF([INT RQD?]="y",0.5,0))
     */

    // FIXME: interlace is not supported yet
    interlace = 0;

    print_value(6, "[INTERLACE]", interlace);

    /*  7. Estimate the Horizontal period
     *
     *  [H PERIOD EST] = ((1/[V FIELD RATE RQD]) - [MIN VSYNC+BP]/1000000) /
     *                    ([V LINES RND] + (2*[TOP MARGIN (LINES)]) +
     *                     [MIN PORCH RND]+[INTERLACE]) * 1000000
     */

    h_period_est_k = ((1000000000 / v_field_rate_rqd) - (MIN_VSYNC_PLUS_BP*1000)) / (v_lines_rnd + (2 * top_margin) + MIN_PORCH + interlace);

    print_value(7, "[H PERIOD EST]", h_period_est_k);

    /*  8. Find the number of lines in V sync + back porch:
     *
     *  [V SYNC+BP] = ROUND(([MIN VSYNC+BP]/[H PERIOD EST]),0)
     */

    vsync_plus_bp_k = (MIN_VSYNC_PLUS_BP*1000000) / h_period_est_k;
    vsync_plus_bp = (vsync_plus_bp_k + 500)/1000;

    print_value(8, "[V SYNC+BP]", vsync_plus_bp);

    /*  9. Find the number of lines in V back porch alone:
     *
     *  [V BACK PORCH] = [V SYNC+BP] - [V SYNC RND]
     *
     *  XXX is "[V SYNC RND]" a typo? should be [V SYNC RQD]?
     */

    v_back_porch = vsync_plus_bp - V_SYNC_RQD;

    print_value(9, "[V BACK PORCH]", v_back_porch);

    /*  10. Find the total number of lines in Vertical field period:
     *
     *  [TOTAL V LINES] = [V LINES RND] + [TOP MARGIN (LINES)] +
     *                    [BOT MARGIN (LINES)] + [V SYNC+BP] + [INTERLACE] +
     *                    [MIN PORCH RND]
     */

    total_v_lines = v_lines_rnd + top_margin + bottom_margin + vsync_plus_bp +
        interlace + MIN_PORCH;

    print_value(10, "[TOTAL V LINES]", total_v_lines);

    /*  11. Estimate the Vertical field frequency:
     *
     *  [V FIELD RATE EST] = 1 / [H PERIOD EST] / [TOTAL V LINES] * 1000000
     */

    v_field_rate_est_k = 1000000000 / ((h_period_est_k * total_v_lines)/1000);

    print_value(11, "[V FIELD RATE EST]", v_field_rate_est_k);

    /*  12. Find the actual horizontal period:
     *
     *  [H PERIOD] = [H PERIOD EST] / ([V FIELD RATE RQD] / [V FIELD RATE EST])
     */

    h_period_k = (h_period_est_k*v_field_rate_est_k) / (v_field_rate_rqd*1000);

    print_value(12, "[H PERIOD]", h_period_k);

    /*  13. Find the actual Vertical field frequency:
     *
     *  [V FIELD RATE] = 1 / [H PERIOD] / [TOTAL V LINES] * 1000000
     */

    v_field_rate = (1000000000 / h_period_k) / total_v_lines;

    print_value(13, "[V FIELD RATE]", v_field_rate);

    /*  14. Find the Vertical frame frequency:
     *
     *  [V FRAME RATE] = (IF([INT RQD?]="y", [V FIELD RATE]/2, [V FIELD RATE]))
     */

    v_frame_rate = interlaced ? v_field_rate / 2 : v_field_rate;

    print_value(14, "[V FRAME RATE]", v_frame_rate);

    /*  15. Find number of pixels in left margin:
     *
     *  [LEFT MARGIN (PIXELS)] = (IF( [MARGINS RQD?]="Y",
     *          (ROUND( ([H PIXELS RND] * [MARGIN%] / 100 /
     *                   [CELL GRAN RND]),0)) * [CELL GRAN RND],
     *          0))
     */

    left_margin = margins ?
        ((h_pixels_rnd * MARGIN_PERMILLE / CELL_GRAN) * CELL_GRAN) / 1000 : 0;

    print_value(15, "[LEFT MARGIN (PIXELS)]", left_margin);

    /*  16. Find number of pixels in right margin:
     *
     *  [RIGHT MARGIN (PIXELS)] = (IF( [MARGINS RQD?]="Y",
     *          (ROUND( ([H PIXELS RND] * [MARGIN%] / 100 /
     *                   [CELL GRAN RND]),0)) * [CELL GRAN RND],
     *          0))
     */

    right_margin = margins ?
        (h_pixels_rnd * MARGIN_PERMILLE / CELL_GRAN) * CELL_GRAN / 1000 : 0;

    print_value(16, "[RIGHT MARGIN (PIXELS)]", right_margin);

    /*  17. Find total number of active pixels in image and left and right
     *  margins:
     *
     *  [TOTAL ACTIVE PIXELS] = [H PIXELS RND] + [LEFT MARGIN (PIXELS)] +
     *                          [RIGHT MARGIN (PIXELS)]
     */

    total_active_pixels = h_pixels_rnd + left_margin + right_margin;

    print_value(17, "[TOTAL ACTIVE PIXELS]", total_active_pixels);

    /*  18. Find the ideal blanking duty cycle from the blanking duty cycle
     *  equation:
     *
     *  [IDEAL DUTY CYCLE] = [C'] - ([M']*[H PERIOD]/1000)
     */

    ideal_duty_cycle_k = (((C_PRIME*1000000) - (M_PRIME * h_period_k)))/1000;

    print_value(18, "[IDEAL DUTY CYCLE]", ideal_duty_cycle_k);

    /*  19. Find the number of pixels in the blanking time to the nearest
     *  double character cell:
     *
     *  [H BLANK (PIXELS)] = (ROUND(([TOTAL ACTIVE PIXELS] *
     *                               [IDEAL DUTY CYCLE] /
     *                               (100-[IDEAL DUTY CYCLE]) /
     *                               (2*[CELL GRAN RND])), 0))
     *                       * (2*[CELL GRAN RND])
     */

    h_blank = (((total_active_pixels * ideal_duty_cycle_k)
    	/ (100000 - ideal_duty_cycle_k)) + 8) & 0xFFFFFFF0;
    /* round (!) to 16 */

    print_value(19, "[H BLANK (PIXELS)]", h_blank);

    /*  20. Find total number of pixels:
     *
     *  [TOTAL PIXELS] = [TOTAL ACTIVE PIXELS] + [H BLANK (PIXELS)]
     */

    total_pixels = total_active_pixels + h_blank;

    print_value(20, "[TOTAL PIXELS]", total_pixels);

    /*  21. Find pixel clock frequency:
     *
     *  [PIXEL FREQ] = [TOTAL PIXELS] / [H PERIOD]
     */

    pixel_freq_k = (total_pixels * 100000) / (h_period_k/10);

    print_value(21, "[PIXEL FREQ]", pixel_freq_k);

    /*  22. Find horizontal frequency:
     *
     *  [H FREQ] = 1000 / [H PERIOD]
     */

    h_freq_k = 1000000000 / h_period_k;

    print_value(22, "[H FREQ]", h_freq_k);

    /* Stage 1 computations are now complete; I should really pass
       the results to another function and do the Stage 2
       computations, but I only need a few more values so I'll just
       append the computations here for now */

    /*  17. Find the number of pixels in the horizontal sync period:
     *
     *  [H SYNC (PIXELS)] =(ROUND(([H SYNC%] / 100 * [TOTAL PIXELS] /
     *                             [CELL GRAN RND]),0))*[CELL GRAN RND]
     */

    h_sync = (((H_SYNC_PERCENT * total_pixels)/100)+4) & 0xFFFFFFF8;

    print_value(17, "[H SYNC (PIXELS)]", h_sync);

    /*  18. Find the number of pixels in the horizontal front porch period:
     *
     *  [H FRONT PORCH (PIXELS)] = ([H BLANK (PIXELS)]/2)-[H SYNC (PIXELS)]
     */

    h_front_porch = (h_blank / 2) - h_sync;

    print_value(18, "[H FRONT PORCH (PIXELS)]", h_front_porch);

    /*  36. Find the number of lines in the odd front porch period:
     *
     *  [V ODD FRONT PORCH(LINES)]=([MIN PORCH RND]+[INTERLACE])
     */

    v_odd_front_porch_lines = MIN_PORCH + interlace;

    print_value(36, "[V ODD FRONT PORCH(LINES)]", v_odd_front_porch_lines);

    /* finally, pack the results in the mode struct */

    m->hr =  h_pixels_rnd;
    m->hss = h_pixels_rnd + h_front_porch;
    m->hse = h_pixels_rnd + h_front_porch + h_sync;
    m->hfl = total_pixels;

    m->vr = v_lines_rnd;
    m->vss = v_lines_rnd + v_odd_front_porch_lines;
    m->vse = v_lines_rnd + v_odd_front_porch_lines + V_SYNC_RQD;
    m->vfl = total_v_lines;

    m->pclk = pixel_freq_k * 1000;
    m->h_freq = h_freq_k;
    m->v_freq = freq;
} // vert_refresh()

void gtf_calc(DWORD w, DWORD h, DWORD freq, vesa_crtc_info_t *crtc_info)
{
	mode m;
	vert_refresh(w, h, freq, 0, 0, &m);
	memset(crtc_info, 0, sizeof(vesa_crtc_info_t));
	
	crtc_info->HorizontalTotal     = m.hfl;
	crtc_info->HorizontalSyncStart = m.hss;
	crtc_info->HorizontalSyncEnd   = m.hse;
	crtc_info->VerticalTotal       = m.vfl;
	crtc_info->VerticalSyncStart   = m.vss;
	crtc_info->VerticalSyncEnd     = m.vse;
	crtc_info->PixelClock          = m.pclk;
	crtc_info->RefreshRate         = m.v_freq*10;

	crtc_info->Flags               = VESA_CRTC_FLAG_HSYC_NEG | VESA_CRTC_FLAG_VSYC_POS; 
}

void gtf_update_pclk(vesa_crtc_info_t *crtc_info, long pclk)
{
	crtc_info->PixelClock  = pclk;
	crtc_info->RefreshRate = (pclk*10) / (crtc_info->HorizontalTotal*crtc_info->VerticalTotal);
}
