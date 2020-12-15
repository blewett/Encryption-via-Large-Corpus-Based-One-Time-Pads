all: ecorpus emap eunmap etally

CFLAGS=-O -std=gnu99 -Wall
# CFLAGS=-O -std=c99 -Wall

ecorpus: ecorpus.c
	gcc ${CFLAGS} -o ecorpus ecorpus.c -lm

emap: emap.c ecorpus_tokens.c
	gcc ${CFLAGS} -o emap emap.c ecorpus_tokens.c

eunmap: eunmap.c ecorpus_tokens.c
	gcc ${CFLAGS} -o eunmap eunmap.c ecorpus_tokens.c

etally: etally.c
	gcc ${CFLAGS} -o etally etally.c -lm

tar:
	tar cvf encrypt.tar *.c Makefile *.doc emap.1
	ls -l *.tar

clean:
	rm -f ecorpus enatcorpus emap eunmap etally etime_loops corpus* *.txt *.tally


tests: testa testb testc testd teste testf testg testh testi testj

testa: ecorpus emap eunmap etally
	@echo "#"
	@echo "# testa: sample encryption with file size reduction"
	@echo "#"

	@echo
	@echo "# get the list of bytes in input_bytes.txt"
	@echo "#"
	cat e*.c > input_bytes.txt
	./etally -print_bytes input_bytes.txt

	@echo
	@echo "# create a corpus file which reduces the size of the encrypted output"
	@echo "#  uniform random blocks of random bytes and"
	@echo "#  bytes limited to those found in the document"
	@echo "#"
	./ecorpus -uniform -corpus corpus -corpus_size 1000000 -byte_list input_bytes.txt.tally

	@echo
	@echo "# encrypt and decrypt"
	@echo "#"
	./emap corpus input_bytes.txt encrypted.txt
	./eunmap corpus encrypted.txt unencrypted.txt

	@echo
	@echo "# compare the files"
	@echo "#"
	diff input_bytes.txt unencrypted.txt
	ls -l  input_bytes.txt unencrypted.txt encrypted.txt
	@echo

testb: ecorpus emap eunmap
	@echo "#"
	@echo "# testb: sample encrypt and decrypt"
	@echo "#"
	@echo
	@echo "# create a simple coprus file"
	@echo "#"
	./ecorpus -corpus corpus -corpus_size 1000000

	@echo
	@echo "# encrypt and decrypt eunmap.c"
	@echo "#"
	./emap corpus eunmap.c encrypted.txt
	./eunmap corpus encrypted.txt unencrypted.txt

	@echo
	@echo "# compare the eunmap.c source to the encrypted and decrypted"
	@echo "#"
	diff eunmap.c unencrypted.txt
	ls -l encrypted.txt unencrypted.txt eunmap.c
	@echo

testc: ecorpus emap eunmap
	@echo "#"
	@echo "# testc: pipe in and out of encryption - gzip is helpful"
	@echo "#"

	@echo
	@echo "# create a simple corpus file"
	@echo "#"
	./ecorpus -corpus corpus -corpus_size 1000000

	@echo
	@echo "# encrypt and decrypt thru a pipe line"
	@echo "#"
	cat eunmap.c | gzip | ./emap corpus - - | ./eunmap corpus - - | gunzip > unencrypted.txt

	@echo
	@echo "# compare the source to the output"
	@echo "#"
	diff eunmap.c unencrypted.txt
	ls -l unencrypted.txt eunmap.c
	@echo

testd: ecorpus emap eunmap
	@echo "#"
	@echo "# testd: reduce the encrypted file size with a corpus of specifc bytes"
	@echo "#"

	@echo
	@echo "# create a simple corpus file"
	@echo "#"
	./ecorpus -corpus corpus -corpus_size 1000000

	@echo
	@echo "# encrypt and compare the of the source and encrypted files"
	@echo "#  compare the size of the source and the encrypted files"
	@echo "#"
	./emap corpus eunmap.c encrypted.txt
	ls -l encrypted.txt eunmap.c

	@echo
	@echo "# create a corpus file which contains only bytes from the source"
	@echo "#"
	./ecorpus -corpus corpus -corpus_size 1000000 -byte_list eunmap.c

	@echo
	@echo "# encrypt and compare the of the source and encrypted files"
	@echo "#  compare the size of the source and the encrypted files"
	@echo "#"
	./emap corpus eunmap.c encrypted.txt
	ls -l encrypted.txt eunmap.c
	@echo

teste: ecorpus emap eunmap
	@echo "#"
	@echo "# teste: observe encryption of repeated phrase"
	@echo "#"

	@echo
	@echo "# create two corpus files with uniform blocks of random bytes"
	@echo "#"
	./ecorpus -uniform -corpus corpus1 -corpus_size 1000000
	./ecorpus -uniform -corpus corpus2 -corpus_size 1000000

	@echo
	@echo "# observe encryption output of repeated phrases from two corpuses"
	@echo "#"
	@echo
	echo "Bell Bell Bell" | ./emap corpus1 - - | od -c -
	@echo
	echo "Bell Bell Bell" | ./emap corpus2 - - | od -c -
	@echo

testf: ecorpus emap eunmap
	@echo "#"
	@echo "# testf: at different locations produce the same corpus by using the -key option"
	@echo "#"

	@echo
	@echo "# create the two corpus files - could be on separate machines"
	@echo "# lets say one in the circus in London"
	@echo "#"
	./ecorpus -key 734329821 -uniform -corpus corpus1 -corpus_size 1000000

	@echo
	@echo "# and lets say another with Smiley's friend in Berlin"
	@echo "#"
	./ecorpus -key 734329821 -uniform -corpus corpus2 -corpus_size 1000000

	@echo
	@echo "# compare the two corpus files"
	@echo "#"
	cmp corpus1 corpus2
	ls -l corpus1 corpus2
	@echo

testg: ecorpus emap eunmap
	@echo "#"
	@echo "# testg: pipe in and out of encryption - gzip is helpful"
	@echo "#"

	@echo
	@echo "# create three corpus files"
	@echo "#"
	./ecorpus -uniform -corpus corpus1 -corpus_size 1000000
	./ecorpus -uniform -corpus corpus2 -corpus_size 1000000
	./ecorpus -uniform -corpus corpus3 -corpus_size 1000000

	@echo
	@echo "# encrypt using the three corpus files"
	@echo "#"
	gzip < eunmap.c | ./emap corpus1 - - | ./emap corpus2 - - | ./emap corpus3 - encrypted1.txt
	ls -l encrypted1.txt

	@echo
	@echo "# encrypt and decrypt through three corpus files"
	@echo "#"
	./emap corpus1 eunmap.c - | ./emap corpus2 - - | ./emap corpus3 - - | \
	  ./eunmap corpus3 - - | ./eunmap corpus2 - - | ./eunmap corpus1 - unencrypted1.txt

	@echo
	@echo "# compare the output of the encrypt/decrypt pipe line"
	@echo "#"
	diff eunmap.c unencrypted1.txt
	ls -l eunmap.c unencrypted1.txt

	@echo
	@echo "# decrypt the previous pipe lined encryption"
	@echo "#"
	./eunmap corpus3 encrypted1.txt - | ./eunmap corpus2 - - | ./eunmap corpus1 - - | gunzip > unencrypted2.txt

	@echo
	@echo "# compare the source and decrypted output - and file sizes"
	@echo "#"
	diff eunmap.c unencrypted2.txt
	ls -l encrypted1.txt eunmap.c unencrypted1.txt unencrypted2.txt
	@echo

testh: emap eunmap
	@echo "#"
	@echo "# testh: encrypt using a local binary as the corpus"
	@echo "#"

	@echo
	@echo "# use the binary vim.tiny as a corpus file for encryption"
	@echo "#"
	cat /usr/bin/vim.tiny > corpus.vim
	./emap corpus.vim eunmap.c encrypted.txt
	./eunmap corpus.vim encrypted.txt unencrypted.txt

	@echo
	@echo "# compare the source and processed files"
	@echo "#"
	diff unencrypted.txt eunmap.c
	ls -l encrypted.txt unencrypted.txt eunmap.c
	@echo

testi: ecorpus emap eunmap etally
	@echo "#"
	@echo "# testi: uniform values in corpus"
	@echo "#"

	@echo
	@echo "# create a larger source file and tally the bytes in it"
	@echo "#"
	cat e*.c > input_bytes.txt
	./etally -print_bytes input_bytes.txt
	ls -l input_bytes.txt.tally

	@echo
	@echo "# create a corpus file with specified bytes"
	@echo "#"
	./ecorpus -key 912345 -corpus corpus1 -corpus_size 1000000 -byte_list input_bytes.txt.tally

	@echo
	@echo "# encrypt using the corpus"
	@echo "#"
	cat input_bytes.txt | ./emap corpus1 - encrypted1.txt
	ls -l encrypted1.txt input_bytes.txt

	@echo
	@echo "# create a corpus file with specified bytes - and uniform blocks"
	@echo "#"
	./ecorpus -key 912345 -uniform -corpus corpus2 -corpus_size 1000000 -byte_list input_bytes.txt.tally

	@echo
	@echo "# encrypt using the uniform corpus"
	@echo "#"
	cat input_bytes.txt | ./emap corpus2 - encrypted2.txt

	@echo
	@echo "# decrypt using uniform corpus and compare files"
	@echo "#"
	./eunmap corpus2 encrypted2.txt unencrypted.txt
	diff unencrypted.txt input_bytes.txt

	@echo
	@echo "# encrypted2 with uniform blocks is smaller than encrypted1"
	@echo "#"
	ls -l encrypted1.txt encrypted2.txt unencrypted.txt input_bytes.txt
	@echo

testj: ecorpus emap eunmap
	@echo "#"
	@echo "# testj: corpus flags"
	@echo "#"

	time -p ./ecorpus -key 4787 -skip_random -start_skip 123 -skip 3 -uniform -corpus corpus1 -corpus_size 1000000
	time -p ./ecorpus -key 4787 -skip_random -start_skip 123 -skip 3 -uniform -corpus corpus2 -corpus_size 1000000
	@echo
	@echo "# test that the corpus files match"
	@echo "#"
	cmp corpus1 corpus2
	@echo
	time -p ./ecorpus -key 4787 -start_skip 123 -skip 3 -uniform -corpus corpus2 -corpus_size 1000000
	@echo
	time -p ./ecorpus -key 4787 -skip_random -uniform -corpus corpus1 -corpus_size 1000000
	time -p ./ecorpus -key 4787 -skip_random -uniform -corpus corpus2 -corpus_size 1000000
	cmp corpus1 corpus2
	@echo
	time -p ./ecorpus -key 4787 -skip_random -corpus corpus1 -corpus_size 10000000

STORAGE=/tmp/enc.txt
testcreate: ecorpus emap ecorpus
	@echo "#"
	@echo "# testcreate: encrypt gpg | emap | gpg"
	@echo "#"

	./ecorpus -key 2742 -uniform -corpus /tmp/c -corpus_size 10000000
	tar c .  | gpg --symmetric | ./emap /tmp/c - - \
		| (sleep 10; gpg --symmetric) > ${STORAGE}

testdisplay: ecorpus eunmap ecorpus
	@echo "#"
	@echo "# testdisplay: decrypt gpg | eunmap | gpg"
	@echo "#"

	./ecorpus -key 2742 -uniform -corpus /tmp/c -corpus_size 10000000
	cat ${STORAGE} | gpg --decrypt | ./eunmap /tmp/c - - | (sleep 10; gpg --decrypt) | tar tvf -

etime_loops: etime_loops.c
	gcc ${CFLAGS} -o etime_loops etime_loops.c

test_etime_loops: etime_loops
	@echo "#"
	@echo "# test_etime_loops"
	@echo "#"

	@echo
	@echo "# 1"
	@echo "#"
	time -p ./etime_loops etime_loops.c 1
	@echo
	@echo "# 2"
	@echo "#"
	time -p ./etime_loops etime_loops.c 2
	@echo
	@echo "# 3"
	@echo "#"
	time -p ./etime_loops etime_loops.c 3

linecount:
	cat emap.c eunmap.c | grep ";" | wc -l
	cat ecorpus.c | grep ";" | wc -l
	cat etally.c | grep ";" | wc -l
