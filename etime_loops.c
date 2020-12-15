/*
 * etime_loops.c: Original work Copyright (C) 2020 by Doug Blewett

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

 *  enc code uses file length to read input files - when they are specified.
 *  this is done to avoid using feof().  You can run this code to compare
 *  both methods.  length is 10 times faster than using feof() in our tests.
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

int main(int argc, char const *argv[])
{
    off_t size_input;
    FILE *fp_input;
    bool redirect_stdin = true;
    off_t i;

    /*
     * parse the arguments to the program
     */
    if (argc != 3)
    {
	fprintf(stderr, "%s: run the program with two arguments\n", argv[0]);
	fprintf(stderr, "  %s readable-filename 1\n\t this times length technique\n\n", argv[0]);
	fprintf(stderr, "  %s readable-filename 2\n\t this times feof() as in code\n\n", argv[0]);
	fprintf(stderr, "  %s readable-filename 1\n\t this times feof() as loop test\n\n", argv[0]);
	exit(1);
    }
    fp_input = fopen(argv[1], "r");

    if (fp_input == NULL)
    {
	fprintf(stderr, "%s: cannot open the input file: %s\n",
		argv[0], argv[1]);
	exit(1);
    }

    // Get the size of the input file
    size_input = (off_t) 1000000000;

    i = 0;
    if (*argv[2] == '1')
    {
	printf("index loop\n");
	for (off_t c = 0; c < size_input; c++)
	{
	    if (i > size_input)
		break;		/* never happens */

	    if (redirect_stdin == false) /* always fail option */
	    {
		if (feof(fp_input))
		    break;
	    } else
		i++;
	}
    }

    if (*argv[2] == '2')
    {
	printf("feof as in code\n");
	for (off_t c = 0; c < size_input; c++)
	{
	    if (redirect_stdin) /* always succeed option */
	    {

		if (feof(fp_input))
		    break;
	    } else
		i++;
	}
    }

    if (*argv[2] == '3')
    {
	printf("feof as loop test\n");
	for (off_t c = 0; c < size_input; c++)
	{
	    if (feof(fp_input))
		break;
	}
    }

    return 0;
}
