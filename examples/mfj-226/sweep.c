/*
 * Read scattering parameter data from MFJ-226 over a given frequency range.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <meas/meas.h>

#define Z0 50	/* system impedence */

#define S1P_FILE_COMMENT "! Captured with MFJ-226 and jmeloranta/libmeas\n"

enum modes {
	MODE_S11, MODE_SWR, MODE_Z,
} mode;

int main(int argc, char **argv) {
	double freq, end, step;
	char *mode_str = argv[1];

	if (argc != 5) {
		fprintf(stderr, "Usage: %s <mode> <begin> <end> <step>\n", argv[0]);
		fprintf(stderr, "Output mode may be SWR, S11 or Z. Sweep parameters are given in Hz.\n");
		fprintf(stderr, "E.g. %s swr 1e6 30e6 1e5\n", argv[0]);
		exit(0);
	}

	if (!strcmp(mode_str, "s11") || !strcmp(mode_str, "S11")) {
		mode = MODE_S11;
	} else if (!strcmp(mode_str, "swr") || !strcmp(mode_str, "SWR")) {
		mode = MODE_SWR;
	} else if (!strcmp(mode_str, "z") || !strcmp(mode_str, "Z")) {
		mode = MODE_Z;
	} else {
		exit(255);
	}

	meas_mfj226_open(0, "/dev/ttyUSB0");

	if (mode == MODE_S11) {
		/* Touchstone .S1P format */
		printf(S1P_FILE_COMMENT);
		printf("# Hz S MA R %d\n", Z0);
	} else if (mode == MODE_Z) {
		/* Touchstone .S1P format */
		printf(S1P_FILE_COMMENT);
		printf("# Hz Z RI R %d\n", Z0);
	} else if (mode == MODE_SWR) {
		/* .CSV format */
		printf("\"%s\",\"%s\",\"%s\",\"%s\"\r\n",
		       "frequency", "Re(Z)", "Im(Z)", "SWR");
	}

	freq = atof(argv[2]);
	end = atof(argv[3]);
	step = atof(argv[4]);

	while (freq <= end) {
		double mag, phase;

		meas_mfj226_write(0, freq);
		meas_mfj226_read(0, &mag, &phase);

		if (mode == MODE_S11) {
			printf("%.0f %.6f %.6f\n", freq, mag, phase);
		} else {
			double phi = M_PI * phase / 180.0;
			double complex gamma = mag * (cos(phi) + I * sin(phi));
			double complex z_norm = (1.0 + gamma) / (1.0 - gamma);

			if (mode == MODE_Z) {
				printf("%.0f %.6f %.6f\n",
				       freq, creal(z_norm), cimag(z_norm));
			} else if (mode == MODE_SWR) {
				double swr = fabs((1.0 + mag) / (1.0 - mag));
				double complex z = Z0 * z_norm;

				printf("%.0f,%.6f,%.6f,%.6f\r\n",
				       freq, creal(z), cimag(z), swr);
			}
		}
		freq += step;
	}

	meas_mfj226_close(0);
}
