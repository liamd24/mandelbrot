#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <omp.h>
#include <time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

/* ==============================================
 *  Program: Mandelbrot Set Generation
 *  Written By: Liam DSouza - 0808199
 *  Date: 24 Mar 2026
 *	
 *  Project
 *	
 *  Purpose:
 *		Create a png image of the Mandelbrot set,
 *		optionally centered on a user-specified
 *		point in the complex plane with <zoom>.
 *												
 *  Description:
 *		Renders the Mandelbrot set using OpenMP.
 *		The user can supply a zoom level and a
 *		center coordinate (real, imag) to focus
 *		on any region of the set. Coloring is 
 *		based on escape iteration count.
 *  
 *	Libraries Required:
 *		stdio.h, stdlib.h, stdint.h, omp.h, time.h
 *      stb_image_write.h
 * 
 *  Compile and Usage:
 *      gcc -fopenmp -O3 -o mandelbrot mandelbrot.c
 *      ./mandelbrot <output>.png <zoom> <center_real> <center_imag>
 *      ./mandelbrot <output>.png <zoom>
 ============================================== */

// Define base values for image
#define WIDTH    1920   // Image width  (pixels)
#define HEIGHT   1080   // Image height (pixels)
#define CHANNELS 3      // Colour channels (r, g, b)

#define MAX_ITER 1000   // Max iterations before a point is considered inside the set

// Define the default complex plane bounds for the full Mandelbrot view
#define RS -2.0     // Real axis start
#define RE  1.0     // Real axis end
#define IS -1.25    // Imaginary axis start
#define IE  1.25    // Imaginary axis end

// Default center of the full view
#define DEFAULT_CENTER_REAL ((RS + RE) / 2.0)   // = -0.5
#define DEFAULT_CENTER_IMAG ((IS + IE) / 2.0)   // =  0.0

// side length (in pixels) of each boundary-following sample cell.
#define GRID_SIZE 16

void mandelbrot(uint8_t *pixels, double zoom, double center_real, double center_imag);
int calculate_fractal(double cr, double ci);
static void color_pixel(uint8_t *pixels, int x, int y, int iter);
int save_to_image(char *image_name, uint8_t *pixels);

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 5) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s <output>.png <zoom>\n", argv[0]);
        fprintf(stderr, "  %s <output>.png <zoom> <center_real> <center_imag>\n", argv[0]);
        exit(1);
    }

    // Required args
    char *filename = argv[1];
    double zoom = atof(argv[2]);

    if (zoom < 1.0) {
        zoom = 1.0;
        fprintf(stderr, "Zoom level invalid. Reset to minimum of 1.0\n");
    }

    // Optional center coordinate args
    double center_real = DEFAULT_CENTER_REAL;
    double center_imag = DEFAULT_CENTER_IMAG;
    if (argc == 5) {
        center_real = atof(argv[3]);
        center_imag = atof(argv[4]);
    }

    printf("----- RENDERING -----\n");
    printf("width:   %d\n", WIDTH);
    printf("height:  %d\n", HEIGHT);
    printf("zoom:    %.4f\n", zoom);
    printf("center:  %.10f, %.10f\n", center_real, center_imag);
    printf("threads: %d\n\n", omp_get_max_threads());

    // Allocate pixel buffer (r, g, b per pixel)
    uint8_t *pixels = malloc(sizeof(uint8_t) * WIDTH * HEIGHT * CHANNELS);
    if (!pixels) {
        fprintf(stderr, "Failed to allocate memory for pixels.");
        exit(1);
    }

    printf("Starting.\n");

    // Calculate runtime
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    mandelbrot(pixels, zoom, center_real, center_imag);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long long time_diff_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
    double time_diff_sec   = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // Save to image; exit code reflects success or failure
    int return_status = save_to_image(filename, pixels);

    printf("Mandelbrot Calculation Time:\n");
    printf("    %lld\tns\n", time_diff_ns);
    printf("    %.6f\tsec\n", time_diff_sec);
    printf("----- END -----\n");

    free(pixels);
    exit(return_status);
}

/*
 * color_pixel
 *
 * Writes the RGB colour for a given iteration count into the pixel buffer.
 *
 * Params:
 * (uint8_t *) pixels -> pixel buffer (WIDTH * HEIGHT * CHANNELS bytes)
 * (int) x, y         -> pixel coordinates
 * (int) iter         -> iteration count returned by calculate_fractal()
 */
static void color_pixel(uint8_t *pixels, int x, int y, int iter) {
    int index = (y * WIDTH + x) * CHANNELS;
    if (iter == MAX_ITER) {
        // Point inside the set -> paint black
        pixels[index + 0] = 0;
        pixels[index + 1] = 0;
        pixels[index + 2] = 0;
    } else {
        // Point escaped the set -> colour gradient based on iteration count
        pixels[index + 0] = (iter % 8)  * 32;   // Red
        pixels[index + 1] = (iter % 16) * 16;   // Green
        pixels[index + 2] = (iter % 32) * 8;    // Blue
    }
}

/*
 * mandelbrot
 *
 * Renders the Mandelbrot set into pixels using a two-pass boundary-following 
 * algorithm parallelised with OpenMP.
 * 
 * Pass 1: Computes and caches iteration counts for a grid of corner pixels.
 * Pass 2: Evaluates each cell. If all 4 corners match, the cell is uniformly
 *         filled. Otherwise, every pixel inside is calculated.
 * 
 * 
 * Params:
 * (uint8_t *) pixels      -> pixel buffer (WIDTH * HEIGHT * CHANNELS bytes)
 * (double)    zoom        -> zoom multiplier; higher = more zoomed in
 * (double)    center_real -> real component of the zoom target point
 * (double)    center_imag -> imaginary component of the zoom target point
 */
void mandelbrot(uint8_t *pixels, double zoom, double center_real, double center_imag) {
    // Shrink the visible range by zoom factor
    double range_real = (RE - RS) / zoom;
    double range_imag = (IE - IS) / zoom;

    // Compute the top-left corner of the visible window so that
    // (center_real, center_imag) lands exactly in the middle of the image
    double start_real = center_real - (range_real / 2.0);
    double start_imag = center_imag - (range_imag / 2.0);

    // Compute grid dimensions, ensuring image edges are covered
    int gcols = (WIDTH  - 1) / GRID_SIZE + 2;
    int grows = (HEIGHT - 1) / GRID_SIZE + 2;

    // Buffer to cache iteration counts for grid corners
    int *grid_buffer = malloc(sizeof(int) * gcols * grows);
    if (!grid_buffer) {
        fprintf(stderr, "Failed to allocate boundary following grid buffer.\n");
        exit(1);
    }

    // --- Pass 1: Sample grid-corner pixels ---
    #pragma omp parallel for collapse(2) schedule(static)
    for (int giy = 0; giy < grows; giy++) {
        for (int gix = 0; gix < gcols; gix++) {
            // Clamp grid index to valid pixel coordinates
            int px = gix * GRID_SIZE; if (px >= WIDTH)  px = WIDTH  - 1;
            int py = giy * GRID_SIZE; if (py >= HEIGHT) py = HEIGHT - 1;

            // Map clamped pixel to its point on the complex plane
            double cr = start_real + (double)px / WIDTH  * range_real;
            double ci = start_imag + (double)py / HEIGHT * range_imag;

            // Store the result for Pass 2; do not colour the pixel yet
            grid_buffer[giy * gcols + gix] = calculate_fractal(cr, ci);
        }
    }

    // --- Pass 2: Boundary following cell fill ---
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int giy = 0; giy < grows - 1; giy++) {
        for (int gix = 0; gix < gcols - 1; gix++) {
            // Half-open pixel range owned exclusively by this cell
            int x0 = gix * GRID_SIZE;
            int y0 = giy * GRID_SIZE;
            int x1 = (gix + 1) * GRID_SIZE; if (x1 > WIDTH)  x1 = WIDTH;
            int y1 = (giy + 1) * GRID_SIZE; if (y1 > HEIGHT) y1 = HEIGHT;

            // Retrieve the four pre-computed corner iteration counts
            int ctl = grid_buffer[giy * gcols + gix];               // top-left
            int ctr = grid_buffer[giy * gcols + (gix + 1)];         // top-right
            int cbl = grid_buffer[(giy + 1) * gcols + gix];         // bottom-left
            int cbr = grid_buffer[(giy + 1) * gcols + (gix + 1)];   // bottom-right

            if (ctl == ctr && ctr == cbl && cbl == cbr) {
                // Uniform cell: Fill with shared colour
                for (int y = y0; y < y1; y++) {
                    for (int x = x0; x < x1; x++) {
                        color_pixel(pixels, x, y, ctl);
                    }
                }
            } else {
                // Boundary cell: Compute iteration count for every pixel
                for (int y = y0; y < y1; y++) {
                    for (int x = x0; x < x1; x++) {
                        double cr = start_real + (double)x / WIDTH  * range_real;
                        double ci = start_imag + (double)y / HEIGHT * range_imag;
                        color_pixel(pixels, x, y, calculate_fractal(cr, ci));
                    }
                }
            }
        }
    }

    free(grid_buffer);
}

/*
 * calculate_fractal
 *
 * Tests whether the point c = (cr + ci*i) on the complex plane belongs
 * to the Mandelbrot set by iterating z = z^2 + c until either the
 * magnitude of z exceeds the escape radius (2.0) or MAX_ITER is reached.
 *
 * Uses a cardioid / period-2 bulb pre-check to immediately return MAX_ITER
 * for points that are mathematically guaranteed to be inside the set,
 * skipping the full iteration loop for a significant speedup in those regions.
 *
 * Params:
 * (double) cr -> real      component of the point c
 * (double) ci -> imaginary component of the point c
 *
 * Returns:
 * (int) iter -> iterations taken to escape, or MAX_ITER if inside the set
 */
int calculate_fractal(double cr, double ci) {
    // --- Cardioid check ---
    // Points inside the main cardioid satisfy:
    // q*(q + (cr-0.25)) <= 0.25*ci^2
    // where q = (cr-0.25)^2 + ci^2
    double cardioid = cr - 0.25;
    double q = cardioid * cardioid + ci * ci;
    if ((q * (q + cardioid) <= 0.25 * ci * ci) ||
        // --- Period-2 bulb check ---
        // Points inside the period-2 bulb (left circle) satisfy:
        // (cr+1)^2 + ci^2 <= 0.0625  (radius 0.25 centered at -1+0i)
        ((cr + 1.0) * (cr + 1.0) + ci * ci <= 0.0625)) {
        return MAX_ITER;
    }

    double zr  = 0.0, zi  = 0.0;
    double zr2 = 0.0;   // zr^2
    double zi2 = 0.0;   // zi^2
    int iter = 0;

    // Iterate z = z^2 + c in component form:
    //   z^2 = (zr + zi*i)^2 = (zr^2 - zi^2) + (2*zr*zi)*i
    // so:
    //   new_zr = zr^2 - zi^2 + cr
    //   new_zi = 2*zr*zi + ci
    // zi must be updated before zr because the new zi depends on the old zr.
    while (zr2 + zi2 <= 4.0 && iter < MAX_ITER) {
        zi  = 2.0 * zr * zi + ci;
        zr  = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;
        iter++;
    }

    return iter;
}

/*
 * save_to_image
 *
 * Writes the pixel buffer to a PNG file inside the "images/" folder
 * using the stb_image_write library.
 *
 * Params:
 * (char *)    image_name -> filename to save as
 * (uint8_t *) pixels     -> pixel buffer; each pixel is CHANNELS bytes
 *
 * Returns:
 * (int) 0 on SUCCESS, 1 on FAILURE
 */
int save_to_image(char *image_name, uint8_t *pixels) {
    // Build full relative path: images/<image_name>
    char *folder = "images";
    char file[64];
    snprintf(file, sizeof(file), "%s/%s", folder, image_name);

    // Write the PNG; stbi_write_png returns 0 on failure, non-zero on success
    int success = stbi_write_png(file, WIDTH, HEIGHT, CHANNELS, pixels, WIDTH * CHANNELS);
    if (!success) {
        fprintf(stderr, "Failed to write png %s\n", image_name);
        return 1;
    }

    printf("Successfully saved %s to %s\n", image_name, file);
    return 0;
}