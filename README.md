# ecorpus

Corpus Based Encryption Tools
=============================

The programs in this group include:

  emap for encrypting a source file,
  eunmap for decrypting an encrypted file,
  ecorpus for creating corpus files, and
  etally for generating statistics from the data in files.

This is a collection of simple software tools that can be used to perform corpus based encryption.  Each byte of an inputfile is converted to the relative index of the matching byte in another file.  The file containing the matching bytes we call the corpus file.  Corpus based encryption and one time pad based encryption have been shown to be very strong encryption techniques.

https://en.wikipedia.org/wiki/One-time_pad

Here is a couple line description of Corpus based encryption.  Source file bytes are converted into relative indices of a corpus file.  For example, the source letter "A" may be converted the number 23 if the next "A" in the corpus files is 23 bytes from the current location.  A subsequent source "A" might be converted to 180 if the next A is 180 bytes from the current location in the corpus file.

Corpus files can be existing files or files generated specifically for encryption.  Naturally occurring corpus files can be the texts of books or binaries, including compiled programs.  A corpus file should be thought of as a bag of bytes in something approximating a random order.  Corpus based encryption has been employed for centuries using religious texts.  Ideal corpus files should be huge, random, and kept private.  They should also not be reused - create or select a new corpus for each encryption task.  Another less secure method is to change the starting index into the corpus.  One should always change the starting index of corpus files which are compiled programs.


Build
-----

This is very simple C code that can be picked up and understood by beginning CS students or business majors.  Makefile is a simple make control file setup for options one might commonly find on a linux machine.  The targets of interest are:

  make
   or
  make all

    all: ecorpus emap eunmap etally

  make clean

  make tests


Manual: man page
----------------

The traditional Unix man page is in emap.1.  Process with the usual:

  man ./emap.1


Document
--------

This document describes the project rationale - other than we are all locked up in our residences and had nothing else to do.  View the document with the usual:

  libreoffice Corpus-based-encryption.doc

You are on your own - but you knew that.  This is covered under the MIT license.

doug.blewett@gmail.com
