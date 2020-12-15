/*
 * eunmap.c: Original work Copyright (C) 2020 by Doug Blewett

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

 *  unmap encrypted file through a corpus file for purposes of decrypting
 */
#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdbool.h>

extern void ecorpus_tokens_init(char *argv0, char *stream_file);
extern unsigned char ecorpus_next_token ();

void
fail(char *argv0)
{
    fprintf(stderr, "%s: run the program with three arguments\n", argv0);
    fprintf(stderr, "  %s corpusfilename inputfilename outputfilename\n",
	    argv0);
    fprintf(stderr, "  %s: use \"-start N\" to start reading after the first N bytes of the corpus file\n", argv0);
    exit(1);
}

int main(int argc, char *argv[])
{
    unsigned char *corpus;

    int fd_corpus = 0;
    struct stat s;
    off_t size_corpus = UINT_MAX;
    off_t size_input;
    unsigned long start = 0;
    bool streaming_corpus = false;

    bool redirect_stdin = false;
    FILE *fp_input;
    FILE *fp_output;

    unsigned char c;
    off_t index_corpus;
    off_t distance;
    off_t distance2;

    char *args[4] = { "", "", "", ""};
    int argsc = 1;

    /*
     * parse the arguments to the program
     */
    args[0] = argv[0];
    for (unsigned int i = 1; i < argc; i++)
    {
	if (argsc >= 4)
	    fail(args[0]);

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
	args[argsc] = argv[i];
	argsc++;
    }

    if (argsc != 4)
	fail(args[0]);

    /*
     * map the corpus file
     */
    if (strncmp("stream:", args[1], 7) == 0)
    {
	ecorpus_tokens_init(args[0], args[1]);
	streaming_corpus = true;
    }
    else
    {
	fd_corpus = open (args[1], O_RDONLY);
	if (fd_corpus == -1)
	{
	    fprintf(stderr, "%s: cannot read the corpus file: %s\n",
		    args[0], args[1]);
	    exit(1);
	}

	// Get the size of the corpus file
	fstat (fd_corpus, &s);
	size_corpus = s.st_size;

	corpus = (unsigned char *) mmap (0, size_corpus, PROT_READ, MAP_PRIVATE,
					 fd_corpus, 0);
    }

    /*
     * map the input file
     */
    if (strcmp("-", args[2]) == 0)
    {
	redirect_stdin = true;
	fp_input = stdin;
	size_input = INT_MAX;
    }
    else
    {
	fp_input = fopen(args[2], "r");

	if (fp_input == NULL)
	{
	    fprintf(stderr, "%s: cannot open the input file: %s\n",
		    args[0], args[3]);
	    exit(1);
	}

	// Get the size of the input file
	fstat (fileno(fp_input), &s);
	size_input = s.st_size;
    }

    /*
     * open the output file
     */
    if (strcmp("-", args[3]) == 0)
	fp_output = stdout;
    else
	fp_output = fopen(args[3], "wb");

    if (fp_output == NULL)
    {
	fprintf(stderr, "%s: cannot create the outout file: %s\n",
		args[0], args[3]);
	exit(1);
    }

    /*
     * adjust the start - if it is set
     */
    index_corpus = start;
    if (streaming_corpus)
    {
	for (int i = 0; i <= start; i++)
	    ecorpus_next_token();
    }

    /*
     * loop through the input finding corpus token distances
     */
    for (off_t i = 0; i < size_input; )
    {
	distance = fgetc(fp_input) & 0377;
	if (redirect_stdin)
	{
	    if(feof(stdin))
		break;
	} else
	    i++;

	/*
	 * zero distances mark wrap or counts larger than 255
	 */
	if (distance == 0)
	{
	    distance2 = fgetc(fp_input) & 0377;
	    if (redirect_stdin)
	    {
		if (feof(stdin))
		    break;
	    } else
		i++;

	    /*
	     * none found - wrap around the corpus
	     */
	    if (distance2 == 0)
	    {
		index_corpus = start;
		continue;
	    }

	    /*
	     * decode counts of greater than 255
	     *
	     * distances greater than 255 are encoded as counts of 255
	     */
	    while (distance2 != 0)
	    {
		distance = distance + (255 * distance2);
		distance2 = fgetc(fp_input) & 0377;
		if (redirect_stdin)
		{
		    if (feof(stdin))
			break;
		} else
		    i++;
	    }

	    distance2 = fgetc(fp_input) & 0377;
	    if (redirect_stdin)
	    {
		if (feof(stdin))
		    break;
	    } else
		i++;
	    distance += distance2;
	}

	/*
	 * advance the index into the corpus to the target byte
	 */
	index_corpus += distance;

	if (streaming_corpus)
	{
	    c = ecorpus_next_token();
	    for (int i = 1; i < distance; i++)
		c = ecorpus_next_token();
	}
	else
	    c = corpus[index_corpus];

	fputc(c, fp_output);
    }

    fclose(fp_output);
    fclose(fp_input);
    close(fd_corpus);

    return 0;
}
