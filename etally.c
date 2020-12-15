/*
 *
 * etalley.c: Original work Copyright (C) 2020 by Blewett

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

 *  talley byte code values from a file - all: corpus file or regular input
 */
#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>

void
fail(char *argv0)
{
    fprintf(stderr, "%s: run the program with file to tally\n", argv0);
    fprintf(stderr, "  %s file-to-talley-bytes\n   or\n", argv0);
    fprintf(stderr, "  %s file-to-talley-bytes [ OPTIONS ]\n", argv0);
    fprintf(stderr, "  %s: use \"-start N\" start reading after the first N bytes in the file\n", argv0);
    fprintf(stderr, "  %s: use \"-print_outliers\" to print the list of outliers\n", argv0);
    fprintf(stderr, "  %s: use \"-stop_on_256\" to exit when 256 values are found\n", argv0);
    fprintf(stderr, "  %s: use \"-print_bytes\" to print the bytes found\n", argv0);
    fprintf(stderr, "  %s:  \"-print_bytes\" can be used for creating byte lists for ecorpus\n", argv0);
    exit(1);
}

int main(int argc, char **argv)
{
    FILE *fp_bytes;
    int bytes[256];
    int bytes_count;
    double mean = 0;
    double std_dev;
    int outliers = 0;

    unsigned char c;

    bool print_outliers = false;
    bool stop_on_256 = false;
    bool print_bytes = false;
    unsigned long start = 0;

    char *argv1 = NULL;
    int total_256 = 0;

    /*
     * parse the arguments to the program
     */

    if (argc < 2)
	fail(argv[0]);

    for (unsigned int i = 1; i < argc; i++)
    {
	if (strcmp(argv[i], "-print_outliers") == 0)
	{
	    print_outliers = true;
	    continue;
	}

	if (strcmp(argv[i], "-print_bytes") == 0)
	{
	    print_bytes = true;
	    continue;
	}

	if (strcmp(argv[i], "-stop_on_256") == 0)
	{
	    stop_on_256 = true;
	    continue;
	}

	if (strcmp(argv[i], "-start") == 0)
	{
	    unsigned int rvalue;
	    unsigned long scan_token;

	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -start value given\n", argv[0]);
		fail(argv[0]);
	    }

	    rvalue = sscanf(argv[i + 1], "%lu", &scan_token);
	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX)
	    {
		fprintf(stderr, "%s: -start value (%s) is not an integer in the range of 0 to %u\n",
			argv[0], argv[i + 1], UINT_MAX);
		fail(argv[0]);
	    }

	    start = scan_token;
	    i++;
	    continue;
	}

	if (argv1 != NULL || *argv[i] == '-')
	{
	    fprintf(stderr, "%s: argument needs fixing: \"%s\"\n", argv[0], argv[i]);
	    fail(argv[0]);
	}
	argv1 = argv[i];
    }

    /*
     * total the bytes in the file
     */
    fp_bytes = fopen(argv1, "r");
    if (fp_bytes == NULL)
    {
	fprintf(stderr, "%s: cannot open the byte list file: %s\n",
		argv[0], argv1);
	exit(1);
    }

    for (unsigned int i = 0; i < start; i++)
    {
	c = fgetc(fp_bytes) & 0377;
	if (feof(fp_bytes))
	{
	    fprintf(stderr, "%s: byte list file: %s smaller than start: %ld\n",
		    argv[0], argv1, start);
	    fail(argv[0]);
	}
    }

    bytes_count = 0;
    for (unsigned int i = 0; i < 256; i++)
	bytes[i] = 0;

    while (true)
    {
	c = fgetc(fp_bytes) & 0377;
	if (feof(fp_bytes))
	    break;

	bytes[c]++;
	if (stop_on_256 && bytes[c] == 1)
	{
	    total_256++;
	    if (total_256 == 256)
	    {
		if (print_bytes)
		    break;
		fprintf(stdout, "Found 256 values in %s\n", argv1);
		exit(0);
	    }
	}
    }

    fclose(fp_bytes);

    /*
     * print the list of bytes for subsequent corpus generating
     */
    if (print_bytes)
    {
	FILE *fp;
	char byte_file[1024];
	unsigned char token;
	int bcount;

	if (strlen(argv1) > 1012)
	{
	    fprintf(stderr, "%s: filename is too long to create tally file\n",
		    argv1);
	    exit(1);
	}

	sprintf(byte_file, "%s.tally", argv1);
	fp = fopen(byte_file, "wb");
	if (fp == NULL)
	{
	    fprintf(stderr, "%s: cannot open the output tally file: %s\n",
		    argv[0], byte_file);
	    exit(1);
	}

	bcount = 0;
	for (unsigned int i = 0; i < 256; i++)
	{
	    if (bytes[i] == 0)
		continue;
	    token = i & 0377;
	    fputc(token, fp);
	    bcount++;
	}

	fclose(fp);

	fprintf(stdout, "%s: wrote %d bytes to %s\n", argv[0], bcount, byte_file);

	exit(0);
    }

    /*
     * find the mean, std, and outliers
     */
    for (unsigned int i = 0; i < 256; i++)
    {
	if (bytes[i] == 0)
	    continue;

	mean += bytes[i];
	bytes_count++;
    }

    mean = mean / bytes_count;
    fprintf(stdout, "mean count per byte value = %.2f\n", mean);

    {
	double sum_of_squares = 0.0;
	double distance;

	for(int i = 0; i < 256; i++)
	{
	    if (bytes[i] == 0)
		continue;

	    distance = bytes[i] - mean;
	    sum_of_squares = sum_of_squares + distance * distance;
	}

	std_dev = sqrt(sum_of_squares / bytes_count);
	fprintf(stdout, "standard deviation: %.2f\n", std_dev);
	fprintf(stdout, "unique byte count = %d\n", bytes_count);
    }

    if (print_outliers == true && std_dev > 1)
    {
	double sd2 = 2 * std_dev;
	bool printing = false;
	char *heading = "two standard deviations outliers:";

	for (int i = 0; i < 256; i++)
	{
	    /*
	     * zero counts drop out
	     */
	    if (bytes[i] == 0)
		continue;

	    if (bytes[i] < (mean - sd2))
	    {
		if (printing == false)
		{
		    fprintf(stdout, "\n%s\n", heading);
		    printing = true;
		}
		fprintf(stdout, " byte value = %d  count = %d\n", i, bytes[i]);
		outliers++;
	    }
	    else if (bytes[i] > (mean + sd2))
	    {
		if (printing == false)
		{
		    fprintf(stdout, "\n%s\n", heading);
		    printing = true;
		}
		fprintf(stdout, " byte value = %d  count = %d\n", i, bytes[i]);
		outliers++;
	    }
	}

	if (printing == true)
	    fprintf(stdout, "total outlier count: %d\n", outliers);
    }
}
