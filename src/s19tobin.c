/*
 *      s19tobin --- convert Motorola S-records to a binary memory image
 *
 *      usage: s19tobin [-f XX] [-s START] [-e END] in.s19 [out.bin]
 *
 *      Options may come anywhere, as "-f FF" or "-fFF".  Without out.bin
 *      the output is named after the input: prog.s19 -> prog.bin.
 *      Byte n of out.bin belongs at address START+n.  Without -s/-e the
 *      image runs from the lowest to the highest address in the file.
 *      Gaps are filled with XX (default FF, an erased EPROM).
 *
 *      Errors (no output file is written):
 *        - a line that is not a valid S-record, or a bad checksum;
 *        - an address that is written twice;
 *        - data outside the -s/-e range.
 *
 *      Numbers are hex, with or without $ or 0x: -s E000, -s $E000.
 *      S1, S2 and S3 data records are read; S0, S5 and S7-S9 are
 *      checked and otherwise ignored.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXLINE 600             /* longest S-record is 4+2*255 chars   */
#define MAXIMAGE 0x1000000L     /* refuse images over 16 MiB            */

struct rec {
	unsigned long addr;
	int     len;
	int     line;
	unsigned char data[255];
};

static struct rec *recs = NULL;
static int nrecs = 0;
static const char *inname = "";

static void usage(void)
{
	fprintf(stderr,
	    "usage: s19tobin [-f XX] [-s START] [-e END] in.s19 [out.bin]\n"
	    "  without out.bin: prog.s19 -> prog.bin\n"
	    "  -f XX     fill byte for gaps (hex, default FF)\n"
	    "  -s START  first address of the image (default: lowest in file)\n"
	    "  -e END    last address of the image (default: highest in file)\n");
	exit(2);
}

static void die(int line, const char *msg, unsigned long addr)
{
	if( line > 0 )
		fprintf(stderr,"s19tobin: %s:%d: ",inname,line);
	else
		fprintf(stderr,"s19tobin: %s: ",inname);
	fprintf(stderr,msg,addr);
	fprintf(stderr,"\n");
	exit(1);
}

/* parse a hex number, optionally prefixed by $ or 0x */
static unsigned long hexarg(const char *s)
{
	char    *end;
	unsigned long v;

	if( *s == '$' )
		s++;
	else if( s[0] == '0' && (s[1] == 'x' || s[1] == 'X') )
		s += 2;
	if( !isxdigit((unsigned char)*s) )
		usage();
	v = strtoul(s,&end,16);
	if( *end != '\0' )
		usage();
	return v;
}

static int hexbyte(const char *p)
{
	int     i, v = 0;

	for(i=0;i<2;i++){
		int c = (unsigned char)p[i];
		if( !isxdigit(c) )
			return -1;
		v = v*16 + (isdigit(c) ? c-'0' : tolower(c)-'a'+10);
	}
	return v;
}

static void readfile(FILE *f)
{
	char    buf[MAXLINE];
	int     line = 0;
	int     cap = 0;

	while( fgets(buf,sizeof buf,f) != NULL ){
		size_t  n;
		int     type, count, alen, i, b, sum;
		unsigned long addr;
		struct rec *r;

		line++;
		n = strlen(buf);
		if( n == sizeof buf - 1 && buf[n-1] != '\n' )
			die(line,"line too long",0);
		while( n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r'
				|| buf[n-1] == ' ' || buf[n-1] == '\t') )
			buf[--n] = '\0';
		if( n == 0 )
			continue;               /* blank line */
		if( buf[0] != 'S' || !isdigit((unsigned char)buf[1]) || n < 4 )
			die(line,"not an S-record",0);
		type = buf[1]-'0';
		count = hexbyte(buf+2);
		if( count < 0 || n != (size_t)(4 + 2*count) )
			die(line,"bad record length",0);
		sum = count;
		for(i=0;i<count;i++){
			if( (b = hexbyte(buf+4+2*i)) < 0 )
				die(line,"bad hex digit",0);
			sum += b;
		}
		if( (sum & 0xFF) != 0xFF )
			die(line,"bad checksum",0);

		switch(type){
		case 1: alen = 2; break;
		case 2: alen = 3; break;
		case 3: alen = 4; break;
		case 0: case 5: case 6: case 7: case 8: case 9:
			continue;               /* header, count, start address */
		default:
			die(line,"unknown record type S%lu",(unsigned long)type);
			return;
		}
		if( count < alen+1 )
			die(line,"bad record length",0);
		addr = 0;
		for(i=0;i<alen;i++)
			addr = addr*256 + (unsigned long)hexbyte(buf+4+2*i);

		if( nrecs == cap ){
			cap = cap ? 2*cap : 256;
			recs = realloc(recs,cap*sizeof *recs);
			if( recs == NULL )
				die(0,"out of memory",0);
		}
		r = &recs[nrecs++];
		r->addr = addr;
		r->line = line;
		r->len  = count - alen - 1;
		for(i=0;i<r->len;i++)
			r->data[i] = (unsigned char)hexbyte(buf+4+2*(alen+i));
	}
}

int main(int argc, char *argv[])
{
	int     fill = 0xFF;
	int     have_s = 0, have_e = 0;
	unsigned long start = 0, end = 0, size, a;
	unsigned char *image, *used;
	const char *outname;
	const char *files[2];
	char    *madename = NULL;     /* default output name, if made */
	int     nfiles = 0;
	FILE    *f;
	int     i, j;

	/* options and file names in any order; "-f FF" or "-fFF" */
	for(i=1; i<argc; i++){
		const char *arg = argv[i];
		const char *val;
		unsigned long v;

		if( arg[0] != '-' || arg[1] == '\0' ){
			if( nfiles == 2 )
				usage();
			files[nfiles++] = arg;
			continue;
		}
		if( strchr("fse",arg[1]) == NULL )
			usage();
		if( arg[2] != '\0' )
			val = arg+2;            /* -fFF */
		else if( i+1 < argc )
			val = argv[++i];        /* -f FF */
		else
			usage();
		v = hexarg(val);
		switch(arg[1]){
		case 'f':
			if( v > 0xFF )
				usage();
			fill = (int)v;
			break;
		case 's':
			start = v; have_s = 1;
			break;
		case 'e':
			end = v; have_e = 1;
			break;
		}
	}
	if( nfiles == 0 )
		usage();
	inname  = files[0];
	if( nfiles == 2 )
		outname = files[1];
	else{                           /* prog.s19 -> prog.bin */
		const char *slash = strrchr(inname,'/');
		const char *dot   = strrchr(inname,'.');
		size_t  len = strlen(inname);
		char    *name;

		if( dot != NULL && (slash == NULL || dot > slash+1) )
			len = (size_t)(dot - inname);
		if( (name = malloc(len+5)) == NULL )
			die(0,"out of memory",0);
		memcpy(name,inname,len);
		strcpy(name+len,".bin");
		if( strcmp(name,inname) == 0 )
			die(0,"input is already called .bin; give an output name",0);
		outname = madename = name;
	}

	if( (f = fopen(inname,"r")) == NULL ){
		perror(inname);
		return 1;
	}
	readfile(f);
	fclose(f);

	/* range of the image */
	if( !have_s || !have_e ){
		unsigned long lo = 0, hi = 0;
		int     any = 0;
		for(j=0;j<nrecs;j++){
			if( recs[j].len == 0 )
				continue;
			if( !any || recs[j].addr < lo )
				lo = recs[j].addr;
			if( !any || recs[j].addr + recs[j].len - 1 > hi )
				hi = recs[j].addr + recs[j].len - 1;
			any = 1;
		}
		if( !any )
			die(0,"no data records",0);
		if( !have_s ) start = lo;
		if( !have_e ) end = hi;
	}
	if( end < start )
		die(0,"end address below start address",0);
	size = end - start + 1;
	if( size > MAXIMAGE )
		die(0,"image larger than 16 MiB ($%lx bytes)",size);

	image = malloc(size);
	used  = calloc(size,1);
	if( image == NULL || used == NULL )
		die(0,"out of memory",0);
	memset(image,fill,size);

	for(j=0;j<nrecs;j++){
		for(i=0;i<recs[j].len;i++){
			a = recs[j].addr + i;
			if( a < start || a > end )
				die(recs[j].line,"data at $%04lx outside the -s/-e range",a);
			if( used[a-start] )
				die(recs[j].line,"address $%04lx written twice",a);
			used[a-start] = 1;
			image[a-start] = recs[j].data[i];
		}
	}

	if( (f = fopen(outname,"wb")) == NULL ){
		perror(outname);
		return 1;
	}
	if( fwrite(image,1,size,f) != size || fclose(f) != 0 ){
		perror(outname);
		remove(outname);
		return 1;
	}
	printf("%s: $%04lx-$%04lx, %lu bytes\n",outname,start,end,size);
	free(image);
	free(used);
	free(recs);
	free(madename);
	return 0;
}
