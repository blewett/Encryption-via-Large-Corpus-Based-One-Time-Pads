/*
 * ecorpus.c: Original work Copyright (C) 2020 by Doug Blewett

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

 *  generate corpus files for encryption
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>

long int prandom(void)
{
#ifdef __STRICT_ANSI__
    return rand();
#else
    return random();
#endif
}

void psrandom(unsigned int key)
{
#ifdef __STRICT_ANSI__
    srand(key)
#else
    srandom(key);
#endif
}

void
fail(char *argv0)
{
    fprintf(stderr, "%s: run the program as follows:\n\n", argv0);
    fprintf(stderr, "  %s -corpus filename -corpus_size size\n", argv0);
    fprintf(stderr, "\n  options:\n");
    fprintf(stderr, "  -corpus set the output filename: -corpus filename\n");
    fprintf(stderr, "  -corpus_size set the size of the file to be created: -corpus_size number\n");
    fprintf(stderr, "  -uniform sets byte values per block to be uniform and random\n");
    fprintf(stderr, "  -key sets the randomizing seed: -key number\n");
    fprintf(stderr, "  -byte_list file containing byte values for the corpus: -byte_list file\n");
    fprintf(stderr, "  -start_skip skip the first N random numbers: -start_skip number\n");
    fprintf(stderr, "  -skip skip N numbers on each call to random: -skip number\n");
    fprintf(stderr, "  -skip_random skip randomly at each call to random()\n");
    fprintf(stderr, "  -skip_random_mask mask for -skip_random: -skip_random_mask number\n");
    fprintf(stderr, "  -filter_file file of skip N numbers: -filter_file filename\n");
    fprintf(stderr, "  -filter_skip bytes to skip in the filter file: -filter_skip number\n");
    fprintf(stderr, "  -filter_mask mask to be used for skip numbers: -filter_mask number\n");
    fprintf(stderr, "\n");
    exit(1);
}

static void coverage(char *args0, int bytes_count, int *bytes, int *counts, int count);

int main(int argc, char **argv)
{
    unsigned int rvalue;
    FILE *fp_corpus = NULL;
    unsigned int corpus_size = 0;
    unsigned long scan_token;
    int counts[256];
    int bytes[256];
    int bytes_count;
    int uniform_byte_counts[256];
    int uniform_byte_count;
    unsigned char token;
    unsigned char c;
    // options
    bool uniform = false;
    time_t key = 0;
    unsigned long start_skip = 0;
    FILE *fp_byte_list = NULL;
    unsigned long skip = 0;
    bool skip_random = false;
    unsigned char skip_random_mask = 0377;
    FILE *fp_filter = NULL;
    unsigned long filter_skip = 0;
    unsigned char filter_mask = 0377;

    /*
     * parse the options to the program
     */
    for (int i = 1; i < argc; i++)
    {
	if (strcmp(argv[i], "-corpus") == 0)
	{
	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -corpus filename given\n", argv[0]);
		fail(argv[0]);
	    }

	    fp_corpus = fopen(argv[i + 1], "w");
	    if (fp_corpus == NULL)
	    {
		fprintf(stderr, "%s: cannot open the corpus: %s\n",
			argv[0], argv[i + 1]);
		fail(argv[0]);
	    }
	    i++;
	    continue;
	}

	if (strcmp(argv[i], "-corpus_size") == 0)
	{
	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -corpus_size value given\n", argv[0]);
		fail(argv[0]);
	    }

	    rvalue = sscanf(argv[i + 1], "%lu", &scan_token);
	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX)
	    {
		fprintf(stderr, "%s: -corpus_size value (%s) is not an integer in the range of 0 to %u\n",
			argv[0], argv[i + 1], UINT_MAX);
		fail(argv[0]);
	    }

	    corpus_size = scan_token;
	    i++;
	    continue;
	}

	if (strcmp(argv[i], "-uniform") == 0)
	{
	    uniform = true;
	    continue;
	}

	if (strcmp(argv[i], "-key") == 0)
	{
	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -key value given\n", argv[0]);
		fail(argv[0]);
	    }

	    rvalue = sscanf(argv[i + 1], "%lu", &scan_token);
	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX)
	    {
		fprintf(stderr, "%s: -key value (%s) is not an integer in the range of 0 to %u\n",
			argv[0], argv[i + 1], UINT_MAX);
		fail(argv[0]);
	    }

	    key = scan_token;
	    i++;
	    continue;
	}

	if (strcmp(argv[i], "-start_skip") == 0)
	{
	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -start_skip value given\n", argv[0]);
		fail(argv[0]);
	    }

	    rvalue = sscanf(argv[i + 1], "%lu", &scan_token);
	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX)
	    {
		fprintf(stderr, "%s: -start_skip value (%s) is not an integer in the range of 0 to %u\n",
			argv[0], argv[i + 1], UINT_MAX);
		fail(argv[0]);
	    }

	    start_skip = scan_token;
	    i++;
	    continue;
	}

	if (strcmp(argv[i], "-byte_list") == 0)
	{
	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -byte_liste filename given\n", argv[0]);
		fail(argv[0]);
	    }

	    fp_byte_list = fopen(argv[i + 1], "r");
	    if (fp_byte_list == NULL)
	    {
		fprintf(stderr, "%s: cannot open the byte_list: %s\n",
			argv[0], argv[i + 1]);
		fail(argv[0]);
	    }
	    i++;
	    continue;
	}

	if (strcmp(argv[i], "-skip") == 0)
	{
	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -skip value given\n", argv[0]);
		fail(argv[0]);
	    }

	    rvalue = sscanf(argv[i + 1], "%lu", &scan_token);
	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX)
	    {
		fprintf(stderr, "%s: -skip value (%s) is not an integer in the range of 0 to %u\n",
			argv[0], argv[i + 1], UINT_MAX);
		fail(argv[0]);
	    }

	    skip = scan_token;
	    i++;
	    continue;
	}

	if (strcmp(argv[i], "-skip_random") == 0)
	{
	    skip_random = true;
	    continue;
	}

	if (strcmp(argv[i], "-skip_random_mask") == 0)
	{
	    int octal_int = 0;

	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -skip_random_mask value given\n", argv[0]);
		fail(argv[0]);
	    }

	    if (*argv[i + 1] == '0' || *argv[i + 1] == 'o')
	    {
		if (*argv[i + 1] == 'o')
		    ++argv[i + 1];
		rvalue = sscanf(argv[i + 1], "%o", &octal_int);
	    }
	    else
		rvalue = sscanf(argv[i + 1], "%i", &octal_int);

	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX ||
		octal_int < 0 || octal_int > 255)
	    {
		fprintf(stderr, "%s: -skip_random_mask value (%s) is not an integer in the range of 0 to 255 (0377)\n",
			argv[0], argv[i + 1]);
		fail(argv[0]);
	    }

	    skip_random_mask = octal_int;
	    i++;
	    continue;
	}

	if (strcmp(argv[i], "-filter_file") == 0)
	{
	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -filter_file filename given\n", argv[0]);
		fail(argv[0]);
	    }

	    fp_filter = fopen(argv[i + 1], "r");
	    if (fp_filter == NULL)
	    {
		fprintf(stderr, "%s: cannot open the filter file: %s\n",
			argv[0], argv[i + 1]);
		fail(argv[0]);
	    }
	    i++;
	    continue;
	}

	if (strcmp(argv[i], "-filter_skip") == 0)
	{
	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -filter_skip value given\n", argv[0]);
		fail(argv[0]);
	    }

	    rvalue = sscanf(argv[i + 1], "%lu", &scan_token);
	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX)
	    {
		fprintf(stderr, "%s: -filter_skip value (%s) is not an integer in the range of 0 to %u\n",
			argv[0], argv[i + 1], UINT_MAX);
		fail(argv[0]);
	    }

	    filter_skip = scan_token;
	    i++;
	    continue;
	}

	if (strcmp(argv[i], "-filter_mask") == 0)
	{
	    int octal_int = 0;

	    if  (argc <= i + 1)
	    {
		fprintf(stderr, "%s: no -filter_mask value given\n", argv[0]);
		fail(argv[0]);
	    }

	    if (*argv[i + 1] == '0' || *argv[i + 1] == 'o')
	    {
		if (*argv[i + 1] == 'o')
		    ++argv[i + 1];
		rvalue = sscanf(argv[i + 1], "%o", &octal_int);
	    }
	    else
		rvalue = sscanf(argv[i + 1], "%i", &octal_int);

	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX ||
		octal_int < 0 || octal_int > 255)
	    {
		fprintf(stderr, "%s: -filter_mask value (%s) is not an integer in the range of 0 to 255 (0377)\n",
			argv[0], argv[i + 1]);
		fail(argv[0]);
	    }

	    filter_mask = octal_int;
	    i++;
	    continue;
	}

	fprintf(stderr, "%s: argument needs fixing: \"%s\"\n", argv[0], argv[i]);
	fail(argv[0]);
    }

    if (uniform == true)
	fprintf(stdout, "uniform blocks enabled\n");

    if (key != 0)
	fprintf(stdout, "key provided\n");
	    
    if (fp_byte_list != NULL)
	fprintf(stdout, "byte_list provided\n");

    if (start_skip != 0)
	fprintf(stdout, "start_skip provided\n");

    if (skip != 0)
	fprintf(stdout, "skip provided\n");
	    
    if (skip_random == true)
	fprintf(stdout, "skip_random enabled\n");

    if(fp_filter != NULL)
	fprintf(stdout, "filter_file provided\n");

    /*
     * open the byte_list file - create the corpus file with specific bytes.
     *   this makes the encrypted files smaller
     */
    if (fp_byte_list != NULL)
    {
	bytes_count = 0;
	for (int i = 0; i < 256; i++)
	    bytes[i] = 0;

	while (true)
	{
	    c = fgetc(fp_byte_list) & 0377;
	    if (feof(fp_byte_list))
		break;

	    bytes[c]++;

	    /*
	     * optional speed up - remove if balancing byte proportions
	     */
	    if(bytes[c] == 1)
		bytes_count++;
	    if (bytes_count == 256)
		break;
	}

	fclose(fp_byte_list);

	fprintf(stdout, "unique bytes count = %d\n", bytes_count);
    }
    else
    {
	for (int i = 0; i < 256; i++)
	    bytes[i] = 1;

	bytes_count = 256;
    }

    /*
     * advance the filter_file the filter_skip byte count
     */
    if (fp_filter != NULL && filter_skip > 0)
    {
	for (unsigned int i = filter_skip; i > 0; i--)
	{
	    c = fgetc(fp_filter);
	    if (feof(fp_filter))
	    {
		fclose(fp_filter);
		fp_filter = NULL;
		fprintf(stderr, "%s: The filter_file is smaller than the filter_skip count\n", argv[0]);
		fail(argv[0]);
		break;
	    }
	}
    }

    if (fp_corpus == NULL || corpus_size == 0)
    {
	fprintf(stderr, "%s: -corpus and -corpus_size must both be set\n", argv[0]);
	fail(argv[0]);
    }
    
    /*
     * seed the random number generator
     */
    if (key == 0)
    {
	sleep(1);  // make back to back corpuses differ
	key = time(NULL);
    }
    psrandom((unsigned int) key);

    /*
     * clear the tabulation arrays
     */
    for (int i = 0; i < 256; i++)
    {
	counts[i] = 0;
	uniform_byte_counts[i] = 0;
    }
    uniform_byte_count = 0;

    /*
     * generate - all of the stuff above is fluff
     */
    if (skip_random)
	start_skip += prandom() & skip_random_mask;

    for (unsigned int j = 0; j < start_skip; j++)
	token = prandom();

    for (unsigned int i = 0; i < corpus_size; )
    {
	unsigned long skipr = 0;
	unsigned long skipf = 0;

	if (fp_filter != NULL)
	{
	    c = fgetc(fp_filter);
	    if (feof(fp_filter))  // loop back around
	    {
		rewind(fp_filter);
		for (unsigned int j = filter_skip; j > 0; j--)
		    c = fgetc(fp_filter);
	    }

	    skipf = fgetc(fp_filter) & filter_mask;
	}


	if (skip_random)
	    skipr = prandom() & skip_random_mask;

	skipr = skipr + skipf + skip;

	for (unsigned int j = 0; j < skipr; j++)
	    token = prandom();

	token = prandom() & 0377;

	if (bytes[token] == 0)
	    continue;

	if(uniform && uniform_byte_counts[token] != 0)
	    continue;

	fputc(token, fp_corpus);
	i++;
	counts[token]++;

	if(uniform)
	{
	    uniform_byte_counts[token] = 1;
	    uniform_byte_count++;

	    if (uniform_byte_count == bytes_count)
	    {
		uniform_byte_count = 0;
		for (int j = 0; j < 256; j++)
		    uniform_byte_counts[j] = 0;
	    }
	}
    }

    fclose(fp_corpus);

    coverage(argv[0], bytes_count, bytes, counts, corpus_size);

    return 0;
}

static void
coverage(char *args0, int bytes_count, int *bytes, int *counts, int count)
{
    /*
     * check the data coverage and uniformity
     */
    double mean = 0;
    double std_dev;
    int outliers = 0;

    /*
     * check the data coverage
     */
    for (int i = 0; i < 256; i++)
    {
	if (bytes[i] && counts[i] == 0)
	{
	    fprintf(stderr, "%s: byte (%d) not covered\n", args0, i);
	    fprintf(stderr, "%s: try increasing the corpus_size from %d\n",
		    args0, count);
	    exit(1);
	}
    }

    /*
     * check the data uniformity
     */
    for (int i = 0; i < 256; i++)
    {
	if (bytes[i] == 0)
	    continue;

	mean += counts[i];
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

	    distance = counts[i] - mean;
	    sum_of_squares = sum_of_squares + distance * distance;
	}

	std_dev = sqrt(sum_of_squares / bytes_count);
	fprintf(stdout, "standard deviation: %.2f\n", std_dev);
    }

    if (std_dev > 1)  // may be half of a uniform group
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

	    if (counts[i] < (mean - sd2))
	    {
		if (printing == false)
		{
		    fprintf(stdout, "\n%s\n", heading);
		    printing = true;
		}
		fprintf(stdout, " byte value = %d  count = %d\n", i, counts[i]);
		outliers++;
	    }
	    else if (counts[i] > (mean + sd2))
	    {
		if (printing == false)
		{
		    fprintf(stdout, "\n%s\n", heading);
		    printing = true;
		}
		fprintf(stdout, " byte value = %d  count = %d\n", i, counts[i]);
		outliers++;
	    }
	}

	if (printing == true)
	    fprintf(stdout, "total outlier count: %d\n", outliers);
    }

    return;
}
