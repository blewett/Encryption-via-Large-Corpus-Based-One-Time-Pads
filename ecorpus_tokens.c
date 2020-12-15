/*
 * ecorpus_tokens.c: Original work Copyright (C) 2020 by Doug Blewett

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

 *  generate corpus stream for encryption
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

static long int prandom(void)
{
#ifdef __STRICT_ANSI__
    return rand();
#else
    return random();
#endif
}

static void psrandom(unsigned int key)
{
#ifdef __STRICT_ANSI__
    srand(key)
#else
    srandom(key);
#endif
}

static void
sub_fail(char *argv0)
{
    fprintf(stderr, "%s: corpus options are as follows:\n\n", argv0);
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

static int bytes[256];
static int bytes_count;
static int uniform_byte_counts[256];
static int uniform_byte_count;
// options
static bool uniform = false;
static time_t key = 0;
static unsigned long start_skip = 0;
static FILE *fp_byte_list = NULL;
static unsigned long skip = 0;
static bool skip_random = false;
static unsigned char skip_random_mask = 0377;
static FILE *fp_filter = NULL;
static unsigned char filter_mask = 0377;
static unsigned long filter_skip = 0;

void ecorpus_tokens_init(char *argv0, char *stream_file)
{
    FILE *fp_stream = NULL;
    char buf[1024];

    unsigned int rvalue;
    unsigned long scan_token;
    unsigned char c;

    char *ptr = strchr(stream_file, ':');

    if (ptr == NULL)
    {
	fprintf(stderr, "%s: badly formed stream file: %s\n",
		argv0, stream_file);
	sub_fail(argv0);
    }
    ptr++;

    fp_stream = fopen(ptr, "r");

    if (fp_stream == NULL)
    {
	fprintf(stderr, "%s: cannot open the stream file: %s\n",
		argv0, ptr);
	sub_fail(argv0);
    }

    while (fgets(buf, 1024, fp_stream) != NULL)
    {
	char *argv1 = buf;
	char *argv2 = "";
	char *nptr;
	int len = strlen(buf);

	// ignore comments
	if (*buf == '#')
	    continue;

	// find and trim the options and arguments - one per line : -skip 23
	while (isspace(buf[len - 1]))
	{
	    len--;
	    buf[len] = '\0';
	}

	// trim leading spaces
	while (isspace(*argv1)) argv1++;
	if (argv1 - buf >= len)
	    continue;

	// find the end of the option token
	nptr = argv1 + 1;
	while (!isspace(*nptr))
	{
	    if (nptr - buf >= len)
		break;
	    nptr++;
	}
	if (isspace(*nptr))
	{
	    *nptr = '\0';
	    nptr++;
	}

	// argument trim leading spaces
	argv2 = nptr;
	if (argv2 - buf < len)
	{
	    while (isspace(*argv2))
	    {
		argv2++;
		if (argv2 - buf >= len)
		    break;
	    }
	}

	/*
	 * parse the options to the program
	 */
	if (strcmp(argv1, "-uniform") == 0)
	{
	    uniform = true;
	    continue;
	}

	if (strcmp(argv1, "-key") == 0)
	{
	    if  (*argv2 == '\0')
	    {
		fprintf(stderr, "%s: no -key value given\n", argv0);
		sub_fail(argv0);
	    }

	    rvalue = sscanf(argv2, "%lu", &scan_token);
	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX)
	    {
		fprintf(stderr, "%s: -key value (%s) is not an integer in the range of 0 to %u\n",
			argv0, argv2, UINT_MAX);
		sub_fail(argv0);
	    }

	    key = scan_token;
	    continue;
	}

	if (strcmp(argv1, "-start_skip") == 0)
	{
	    if  (*argv2 == '\0')
	    {
		fprintf(stderr, "%s: no -start_skip value given\n", argv0);
		sub_fail(argv0);
	    }

	    rvalue = sscanf(argv2, "%lu", &scan_token);
	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX)
	    {
		fprintf(stderr, "%s: -start_skip value (%s) is not an integer in the range of 0 to %u\n",
			argv0, argv2, UINT_MAX);
		sub_fail(argv0);
	    }

	    start_skip = scan_token;
	    continue;
	}

	if (strcmp(argv1, "-byte_list") == 0)
	{
	    if  (*argv2 == '\0')
	    {
		fprintf(stderr, "%s: no -byte_liste filename given\n", argv0);
		sub_fail(argv0);
	    }

	    fp_byte_list = fopen(argv2, "r");
	    if (fp_byte_list == NULL)
	    {
		fprintf(stderr, "%s: cannot open the byte_list: %s\n",
			argv0, argv2);
		sub_fail(argv0);
	    }
	    continue;
	}

	if (strcmp(argv1, "-skip") == 0)
	{
	    if  (*argv2 == '\0')
	    {
		fprintf(stderr, "%s: no -skip value given\n", argv0);
		sub_fail(argv0);
	    }

	    rvalue = sscanf(argv2, "%lu", &scan_token);
	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX)
	    {
		fprintf(stderr, "%s: -skip value (%s) is not an integer in the range of 0 to %u\n",
			argv0, argv2, UINT_MAX);
		sub_fail(argv0);
	    }

	    skip = scan_token;
	    continue;
	}

	if (strcmp(argv1, "-skip_random") == 0)
	{
	    skip_random = true;
	    continue;
	}


	if (strcmp(argv1, "-skip_random_mask") == 0)
	{
	    int octal_int = 0;

	    if  (*argv2 == '\0')
	    {
		fprintf(stderr, "%s: no -skip_random_mask value given\n", argv0);
		sub_fail(argv0);
	    }

	    if (*argv2 == '0' || *argv2 == 'o')
	    {
		if (*argv2 == 'o')
		    ++argv2;
		rvalue = sscanf(argv2, "%o", &octal_int);
	    }
	    else
		rvalue = sscanf(argv2, "%i", &octal_int);

	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX ||
		octal_int < 0 || octal_int > 255)
	    {
		fprintf(stderr, "%s: -skip_random_mask value (%s) is not an integer in the range of 0 to 255 (0377)\n",
			argv0, argv2);
		sub_fail(argv0);
	    }

	    skip_random_mask = octal_int;
	    continue;
	}

	if (strcmp(argv1, "-filter_file") == 0)
	{
	    if  (*argv2 == '\0')
	    {
		fprintf(stderr, "%s: no -filter_file filename given\n", argv0);
		sub_fail(argv0);
	    }

	    fp_filter = fopen(argv2, "r");
	    if (fp_filter == NULL)
	    {
		fprintf(stderr, "%s: cannot open the filter file: %s\n",
			argv0, argv2);
		sub_fail(argv0);
	    }
	    continue;
	}

	if (strcmp(argv1, "-filter_skip") == 0)
	{
	    if  (*argv2 == '\0')
	    {
		fprintf(stderr, "%s: no -filter_skip value given\n", argv0);
		sub_fail(argv0);
	    }

	    rvalue = sscanf(argv2, "%lu", &scan_token);
	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX)
	    {
		fprintf(stderr, "%s: -filter_skip value (%s) is not an integer in the range of 0 to %u\n",
			argv0, argv2, UINT_MAX);
		sub_fail(argv0);
	    }

	    filter_skip = scan_token;
	    continue;
	}

	if (strcmp(argv1, "-filter_mask") == 0)
	{
	    int octal_int = 0;

	    if  (*argv2 == '\0')
	    {
		fprintf(stderr, "%s: no -filter_mask value given\n", argv0);
		sub_fail(argv0);
	    }

	    if (*argv2 == '0' || *argv2 == 'o')
	    {
		if (*argv2 == 'o')
		    ++argv2;
		rvalue = sscanf(argv2, "%o", &octal_int);
	    }
	    else
		rvalue = sscanf(argv2, "%i", &octal_int);

	    if (rvalue == 0 || rvalue == EOF || scan_token > UINT_MAX ||
		octal_int < 0 || octal_int > 255)
	    {
		fprintf(stderr, "%s: -filter_mask value (%s) is not an integer in the range of 0 to 255 (0377)\n",
			argv0, argv2);
		sub_fail(argv0);
	    }

	    filter_mask = octal_int;
	    continue;
	}
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
		fprintf(stderr, "%s: The filter_file is smaller than the filter_skip count\n", argv0);
		sub_fail(argv0);
		break;
	    }
	}
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
	uniform_byte_counts[i] = 0;

    uniform_byte_count = 0;

    /*
     * generate - all of the stuff above is fluff
     */
    if (skip_random)
	start_skip += prandom() & skip_random_mask;

    for (unsigned int j = 0; j < start_skip; j++)
	prandom();

    fclose(fp_stream);
}


unsigned char ecorpus_next_token ()
{
    unsigned char token;

    while (true)
    {
	unsigned long skipr = 0;
	unsigned long skipf = 0;

	if (fp_filter != NULL)
	{
	    fgetc(fp_filter);
	    if (feof(fp_filter))  // loop back around
	    {
		rewind(fp_filter);
		for (unsigned int j = filter_skip; j > 0; j--)
		    fgetc(fp_filter);
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

	return token;
    }
    // never here
    return token;
}
