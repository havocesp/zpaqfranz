///////// EXPERIMENTAL BUILD
///////// The source is a mess
///////// Strongly in development
/////////
///////// https://github.com/fcorbelli/zpaqfranz
///////// https://sourceforge.net/projects/zpaqfranz/

///////// I had to reluctantly move parts of the comments and some times even delete them, 
///////// because github doesn't like files >1MB. I apologize with the authors,
///////// it's not a foolish attempt to take over their jobs
///////// https://github.com/fcorbelli/zpaqfranz/blob/main/notes.txt

#define ZPAQ_VERSION "51.40-experimental"
#define FRANZOFFSET 	50
#define FRANZMAXPATH 	240

/*
                         __                     
  _____ __   __ _  __ _ / _|_ __ __ _ _ __  ____
 |_  / '_ \ / _` |/ _` | |_| '__/ _` | '_ \|_  /
  / /| |_) | (_| | (_| |  _| | | (_| | | | |/ / 
 /___| .__/ \__,_|\__, |_| |_|  \__,_|_| |_/___|
     |_|             |_|                        
                                                                    

## This is zpaqfranz, a patched  but (maybe) compatible fork of ZPAQ version 7.15 
(http://mattmahoney.net/dc/zpaq.html)

Ancient version is in FreeBSD /ports/archivers/paq (v 6.57 of 2014)

From branch 51 all source code merged in one zpaqfranz.cpp
aiming to make it as easy as possible to compile on "strange" systems (NAS, vSphere etc).

So be patient if the source is not linear, 
updating and compilation are now trivial.

The source is composed of the fusion of different software 
from different authors. 

Therefore there is no uniform style of programming. 

I have made a number of efforts to maintain compatibility 
with unmodified version (7.15), even at the cost 
of expensive on inelegant workarounds.

So don't be surprised if it looks like what in Italy 
we call "zibaldone" or in Emilia-Romagna "mappazzone".
Jun 2021: starting to fix the mess (refactoring). Work in progress.

Windows binary builds (32 and 64 bit) on github/sourceforge

## **Provided as-is, with no warranty whatsoever, by Franco Corbelli, franco@francocorbelli.com**


####      Key differences against 7.15 by zpaqfranz -diff or here 
####  https://github.com/fcorbelli/zpaqfranz/blob/main/differences715.txt 

Portions of software by other authors, mentioned later, are included.
As far as I know this is allowed by the licenses. 

**I apologize if I have unintentionally violated any rule.**
**Report it and I will fix as soon as possible.**

- Include mod by data man and reg2s patch from encode.su forum 
- Crc32.h Copyright (c) 2011-2019 Stephan Brumme 
- Slicing-by-16 contributed by Bulat Ziganshin 
- xxHash Extremely Fast Hash algorithm, Copyright (C) 2012-2020 Yann Collet 
- crc32c.c Copyright (C) 2013 Mark Adler



FreeBSD port, quick and dirty to get a /usr/local/bin/zpaqfranz
```
mkdir /tmp/testme
cd /tmp/testme
wget http://www.francocorbelli.it/zpaqfranz/zpaqfranz-51.30.tar.gz
tar -xvf zpaqfranz-51.30.tar.gz
make install clean
```

NOTE1: -, not -- (into switch)
NOTE2: switches ARE case sensitive.   -maxsize <> -MAXSIZE


# How to build
My main development platforms are Windows and FreeBSD. 
I rarely use Linux or MacOS, so changes may be needed.

As explained the program is single file, 
be careful to link the pthread library.

Targets
```
Windows 64 (g++ 7.3.0)
g++ -O3  zpaqfranz.cpp -o zpaqfranz 

Windows 32 (g++ 7.3.0 64 bit)
c:\mingw32\bin\g++ -m32 -O3 zpaqfranz.cpp -o zpaqfranz32 -pthread -static

FreeBSD (11.x) gcc 7
gcc7 -O3 -march=native -Dunix zpaqfranz.cpp -static -lstdc++ -pthread -o zpaqfranz -static -lm

FreeBSD (12.1) gcc 9.3.0
g++ -O3 -march=native -Dunix zpaqfranz.cpp  -pthread -o zpaqfranz -static-libstdc++ -static-libgcc

FreeBSD (11.4) gcc 10.2.0
g++ -O3 -march=native -Dunix zpaqfranz.cpp  -pthread -o zpaqfranz -static-libstdc++ -static-libgcc -Wno-stringop-overflow

FreeBSD (11.3) clang 6.0.0
clang++ -march=native -Dunix zpaqfranz.cpp  -pthread -o zpaqfranz -static

Debian Linux (10/11) gcc 8.3.0
ubuntu 21.04 desktop-amd64 gcc  10.3.0
manjaro 21.07 gcc 11.1.0
g++ -O3 -Dunix zpaqfranz.cpp  -pthread -o zpaqfranz -static

QNAP NAS TS-431P3 (Annapurna AL314) gcc 7.4.0
g++ -Dunix zpaqfranz.cpp  -pthread -o zpaqfranz -Wno-psabi
```

*/


#define _FILE_OFFSET_BITS 64  // In Linux make sizeof(off_t) == 8

#ifndef UNICODE
	#define UNICODE  // For Windows
#endif

#ifndef DEBUG
	#define NDEBUG 1
#endif

#ifdef _OPENMP
	#include <omp.h>
#endif

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	#ifndef unix
		#define unix 1
	#endif
#endif

#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstddef>

#ifdef unix
	#ifndef NOJIT
		#include <sys/mman.h>
	#endif

	#define PTHREAD 1
	#include <termios.h>
	#include <sys/param.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/time.h>
	#include <dirent.h>
	#include <utime.h>

	#ifdef BSD
		#include <sys/sysctl.h>
		#include <sys/mount.h>
	#endif

#else  // Assume Windows
	
///	#include <wincrypt.h>
	#include <conio.h>
	#include <windows.h>
	#include <io.h>
	#include <sys/stat.h>
	using namespace std;

#endif

// For testing -Dunix in Windows
#ifdef unixtest
	#define lstat(a,b) stat(a,b)
	#define mkdir(a,b) mkdir(a)
	#ifndef fseeko
		#define fseeko(a,b,c) fseeko64(a,b,c)
	#endif
	#ifndef ftello
		#define ftello(a) ftello64(a)
	#endif
#endif


namespace libzpaq {

// 1, 2, 4, 8 byte unsigned integers
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

// Tables for parsing ZPAQL source code
extern const char* compname[256];    // list of ZPAQL component types
extern const int compsize[256];      // number of bytes to encode a component
extern const char* opcodelist[272];  // list of ZPAQL instructions

// Callback for error handling
extern void error(const char* msg);

// Virtual base classes for input and output
// get() and put() must be overridden to read or write 1 byte.
// read() and write() may be overridden to read or write n bytes more
// efficiently than calling get() or put() n times.
class Reader {
public:
  virtual int get() = 0;  // should return 0..255, or -1 at EOF
  virtual int read(char* buf, int n); // read to buf[n], return no. read
  virtual ~Reader() {}
};

class Writer {
public:
  virtual void put(int c) = 0;  // should output low 8 bits of c
  virtual void write(const char* buf, int n);  // write buf[n]
  virtual ~Writer() {}
};

// Read 16 bit little-endian number
int toU16(const char* p);

// An Array of T is cleared and aligned on a 64 byte address
//   with no constructors called. No copy or assignment.
// Array<T> a(n, ex=0);  - creates n<<ex elements of type T
// a[i] - index
// a(i) - index mod n, n must be a power of 2
// a.size() - gets n
template <typename T>
class Array {
  T *data;     // user location of [0] on a 64 byte boundary
  size_t n;    // user size
  int offset;  // distance back in bytes to start of actual allocation
  void operator=(const Array&);  // no assignment
  Array(const Array&);  // no copy
public:
  Array(size_t sz=0, int ex=0): data(0), n(0), offset(0) {
    resize(sz, ex);} // [0..sz-1] = 0
  void resize(size_t sz, int ex=0); // change size, erase content to zeros
  ~Array() {resize(0);}  // free memory
  size_t size() const {return n;}  // get size
  int isize() const {return int(n);}  // get size as an int
  T& operator[](size_t i) {assert(n>0 && i<n); return data[i];}
  T& operator()(size_t i) {assert(n>0 && (n&(n-1))==0); return data[i&(n-1)];}
};

// Change size to sz<<ex elements of 0
template<typename T>
void Array<T>::resize(size_t sz, int ex) {
  assert(size_t(-1)>0);  // unsigned type?
  while (ex>0) {
    if (sz>sz*2) error("Array too big");
    sz*=2, --ex;
  }
  if (n>0) {
    assert(offset>0 && offset<=64);
    assert((char*)data-offset);
    ::free((char*)data-offset);
  }
  n=0;
  offset=0;
  if (sz==0) return;
  n=sz;
  const size_t nb=128+n*sizeof(T);  // test for overflow
  if (nb<=128 || (nb-128)/sizeof(T)!=n) n=0, error("Array too big");
  data=(T*)::calloc(nb, 1);
  if (!data) n=0, error("Out of memory");
  offset=64-(((char*)data-(char*)0)&63);
  assert(offset>0 && offset<=64);
  data=(T*)((char*)data+offset);
}






//////////////////////////// SHA1 ////////////////////////////

// For computing SHA-1 checksums
class SHA1 {
public:
  void put(int c) {  // hash 1 byte
    U32& r=w[U32(len)>>5&15];
    r=(r<<8)|(c&255);
    len+=8;
    if ((U32(len)&511)==0) process();
  }
  void write(const char* buf, int64_t n); // hash buf[0..n-1]
  double size() const {return len/8;}     // size in bytes
  uint64_t usize() const {return len/8;}  // size in bytes
  const char* result();  // get hash and reset
  SHA1() {init();}
private:
  void init();      // reset, but don't clear hbuf
  U64 len;          // length in bits
  U32 h[5];         // hash state
  U32 w[16];        // input buffer
  char hbuf[20];    // result
  void process();   // hash 1 block
  
	///uint64_t _wyp[4];

};

//////////////////////////// SHA256 //////////////////////////

// For computing SHA-256 checksums
// http://en.wikipedia.org/wiki/SHA-2
class SHA256 {
public:
  void put(int c) {  // hash 1 byte
    unsigned& r=w[len0>>5&15];
    r=(r<<8)|(c&255);
    if (!(len0+=8)) ++len1;
    if ((len0&511)==0) process();
  }
  double size() const {return len0/8+len1*536870912.0;} // size in bytes
  uint64_t usize() const {return len0/8+(U64(len1)<<29);} //size in bytes
  const char* result();  // get hash and reset
  SHA256() {init();}
private:
  void init();           // reset, but don't clear hbuf
  unsigned len0, len1;   // length in bits (low, high)
  unsigned s[8];         // hash state
  unsigned w[16];        // input buffer
  char hbuf[32];         // result
  void process();        // hash 1 block
};

//////////////////////////// AES /////////////////////////////

// For encrypting with AES in CTR mode.
// The i'th 16 byte block is encrypted by XOR with AES(i)
// (i is big endian or MSB first, starting with 0).
class AES_CTR {
  U32 Te0[256], Te1[256], Te2[256], Te3[256], Te4[256]; // encryption tables
  U32 ek[60];  // round key
  int Nr;  // number of rounds (10, 12, 14 for AES 128, 192, 256)
  U32 iv0, iv1;  // first 8 bytes in CTR mode
public:
  AES_CTR(const char* key, int keylen, const char* iv=0);
    // Schedule: keylen is 16, 24, or 32, iv is 8 bytes or NULL
  void encrypt(U32 s0, U32 s1, U32 s2, U32 s3, unsigned char* ct);
  void encrypt(char* buf, int n, U64 offset);  // encrypt n bytes of buf
};

//////////////////////////// stretchKey //////////////////////

// Strengthen password pw[0..pwlen-1] and salt[0..saltlen-1]
// to produce key buf[0..buflen-1]. Uses O(n*r*p) time and 128*r*n bytes
// of memory. n must be a power of 2 and r <= 8.
void scrypt(const char* pw, int pwlen,
            const char* salt, int saltlen,
            int n, int r, int p, char* buf, int buflen);

// Generate a strong key out[0..31] key[0..31] and salt[0..31].
// Calls scrypt(key, 32, salt, 32, 16384, 8, 1, out, 32);
void stretchKey(char* out, const char* key, const char* salt);

//////////////////////////// random //////////////////////////

// Fill buf[0..n-1] with n cryptographic random bytes. The first
// byte is never '7' or 'z'.
void random(char* buf, int n);

//////////////////////////// ZPAQL ///////////////////////////

// Symbolic constants, instruction size, and names
typedef enum {NONE,CONS,CM,ICM,MATCH,AVG,MIX2,MIX,ISSE,SSE} CompType;
extern const int compsize[256];
class Decoder;  // forward

// A ZPAQL machine COMP+HCOMP or PCOMP.
class ZPAQL {
public:
  ZPAQL();
  ~ZPAQL();
  void clear();           // Free memory, erase program, reset machine state
  void inith();           // Initialize as HCOMP to run
  void initp();           // Initialize as PCOMP to run
  double memory();        // Return memory requirement in bytes
  void run(U32 input);    // Execute with input
  int read(Reader* in2);  // Read header
  bool write(Writer* out2, bool pp); // If pp write PCOMP else HCOMP header
  int step(U32 input, int mode);  // Trace execution (defined externally)

  Writer* output;         // Destination for OUT instruction, or 0 to suppress
  SHA1* sha1;             // Points to checksum computer
  U32 H(int i) {return h(i);}  // get element of h

  void flush();           // write outbuf[0..bufptr-1] to output and sha1
  void outc(int ch) {     // output byte ch (0..255) or -1 at EOS
    if (ch<0 || (outbuf[bufptr]=ch, ++bufptr==outbuf.isize())) flush();
  }

  // ZPAQ1 block header
  Array<U8> header;   // hsize[2] hh hm ph pm n COMP (guard) HCOMP (guard)
  int cend;           // COMP in header[7...cend-1]
  int hbegin, hend;   // HCOMP/PCOMP in header[hbegin...hend-1]

private:
  // Machine state for executing HCOMP
  Array<U8> m;        // memory array M for HCOMP
  Array<U32> h;       // hash array H for HCOMP
  Array<U32> r;       // 256 element register array
  Array<char> outbuf; // output buffer
  int bufptr;         // number of bytes in outbuf
  U32 a, b, c, d;     // machine registers
  int f;              // condition flag
  int pc;             // program counter
  int rcode_size;     // length of rcode
  U8* rcode;          // JIT code for run()

  // Support code
  int assemble();  // put JIT code in rcode
  void init(int hbits, int mbits);  // initialize H and M sizes
  int execute();  // interpret 1 instruction, return 0 after HALT, else 1
  void run0(U32 input);  // default run() if not JIT
  void div(U32 x) {if (x) a/=x; else a=0;}
  void mod(U32 x) {if (x) a%=x; else a=0;}
  void swap(U32& x) {a^=x; x^=a; a^=x;}
  void swap(U8& x)  {a^=x; x^=a; a^=x;}
  void err();  // exit with run time error
};

///////////////////////// Component //////////////////////////

// A Component is a context model, indirect context model, match model,
// fixed weight mixer, adaptive 2 input mixer without or with current
// partial byte as context, adaptive m input mixer (without or with),
// or SSE (without or with).

struct Component {
  size_t limit;   // max count for cm
  size_t cxt;     // saved context
  size_t a, b, c; // multi-purpose variables
  Array<U32> cm;  // cm[cxt] -> p in bits 31..10, n in 9..0; MATCH index
  Array<U8> ht;   // ICM/ISSE hash table[0..size1][0..15] and MATCH buf
  Array<U16> a16; // MIX weights
  void init();    // initialize to all 0
  Component() {init();}
};

////////////////////////// StateTable ////////////////////////

// Next state table
class StateTable {
public:
  U8 ns[1024]; // state*4 -> next state if 0, if 1, n0, n1
  int next(int state, int y) {  // next state for bit y
    assert(state>=0 && state<256);
    assert(y>=0 && y<4);
    return ns[state*4+y];
  }
  int cminit(int state) {  // initial probability of 1 * 2^23
    assert(state>=0 && state<256);
    return ((ns[state*4+3]*2+1)<<22)/(ns[state*4+2]+ns[state*4+3]+1);
  }
  StateTable();
};

///////////////////////// Predictor //////////////////////////

// A predictor guesses the next bit
class Predictor {
public:
  Predictor(ZPAQL&);
  ~Predictor();
  void init();          // build model
  int predict();        // probability that next bit is a 1 (0..4095)
  void update(int y);   // train on bit y (0..1)
  int stat(int);        // Defined externally
  bool isModeled() {    // n>0 components?
    assert(z.header.isize()>6);
    return z.header[6]!=0;
  }
private:

  // Predictor state
  int c8;               // last 0...7 bits.
  int hmap4;            // c8 split into nibbles
  int p[256];           // predictions
  U32 h[256];           // unrolled copy of z.h
  ZPAQL& z;             // VM to compute context hashes, includes H, n
  Component comp[256];  // the model, includes P
  bool initTables;      // are tables initialized?

  // Modeling support functions
  int predict0();       // default
  void update0(int y);  // default
  int dt2k[256];        // division table for match: dt2k[i] = 2^12/i
  int dt[1024];         // division table for cm: dt[i] = 2^16/(i+1.5)
  U16 squasht[4096];    // squash() lookup table
  short stretcht[32768];// stretch() lookup table
  StateTable st;        // next, cminit functions
  U8* pcode;            // JIT code for predict() and update()
  int pcode_size;       // length of pcode

  // reduce prediction error in cr.cm
  void train(Component& cr, int y) {
    assert(y==0 || y==1);
    U32& pn=cr.cm(cr.cxt);
    U32 count=pn&0x3ff;
    int error=y*32767-(cr.cm(cr.cxt)>>17);
    pn+=(error*dt[count]&-1024)+(count<cr.limit);
  }

  // x -> floor(32768/(1+exp(-x/64)))
  int squash(int x) {
    assert(initTables);
    assert(x>=-2048 && x<=2047);
    return squasht[x+2048];
  }

  // x -> round(64*log((x+0.5)/(32767.5-x))), approx inverse of squash
  int stretch(int x) {
    assert(initTables);
    assert(x>=0 && x<=32767);
    return stretcht[x];
  }

  // bound x to a 12 bit signed int
  int clamp2k(int x) {
    if (x<-2048) return -2048;
    else if (x>2047) return 2047;
    else return x;
  }

  // bound x to a 20 bit signed int
  int clamp512k(int x) {
    if (x<-(1<<19)) return -(1<<19);
    else if (x>=(1<<19)) return (1<<19)-1;
    else return x;
  }

  // Get cxt in ht, creating a new row if needed
  size_t find(Array<U8>& ht, int sizebits, U32 cxt);

  // Put JIT code in pcode
  int assemble_p();
};

//////////////////////////// Decoder /////////////////////////

// Decoder decompresses using an arithmetic code
class Decoder: public Reader {
public:
  Reader* in;        // destination
  Decoder(ZPAQL& z);
  int decompress();  // return a byte or EOF
  int skip();        // skip to the end of the segment, return next byte
  void init();       // initialize at start of block
  int stat(int x) {return pr.stat(x);}
  int get() {        // return 1 byte of buffered input or EOF
    if (rpos==wpos) {
      rpos=0;
      wpos=in ? in->read(&buf[0], BUFSIZE) : 0;
      assert(wpos<=BUFSIZE);
    }
    return rpos<wpos ? U8(buf[rpos++]) : -1;
  }
  int buffered() {return wpos-rpos;}  // how far read ahead?
private:
  U32 low, high;     // range
  U32 curr;          // last 4 bytes of archive or remaining bytes in subblock
  U32 rpos, wpos;    // read, write position in buf
  Predictor pr;      // to get p
  enum {BUFSIZE=1<<16};
  Array<char> buf;   // input buffer of size BUFSIZE bytes
  int decode(int p); // return decoded bit (0..1) with prob. p (0..65535)
};

/////////////////////////// PostProcessor ////////////////////

class PostProcessor {
  int state;   // input parse state: 0=INIT, 1=PASS, 2..4=loading, 5=POST
  int hsize;   // header size
  int ph, pm;  // sizes of H and M in z
public:
  ZPAQL z;     // holds PCOMP
  PostProcessor(): state(0), hsize(0), ph(0), pm(0) {}
  void init(int h, int m);  // ph, pm sizes of H and M
  int write(int c);  // Input a byte, return state
  int getState() const {return state;}
  void setOutput(Writer* out) {z.output=out;}
  void setSHA1(SHA1* sha1ptr) {z.sha1=sha1ptr;}
};

//////////////////////// Decompresser ////////////////////////

// For decompression and listing archive contents
class Decompresser {
public:
  Decompresser(): z(), dec(z), pp(), state(BLOCK), decode_state(FIRSTSEG) {}
  void setInput(Reader* in) {dec.in=in;}
  bool findBlock(double* memptr = 0);
  void hcomp(Writer* out2) {z.write(out2, false);}
  bool findFilename(Writer* = 0);
  void readComment(Writer* = 0);
  void setOutput(Writer* out) {pp.setOutput(out);}
  void setSHA1(SHA1* sha1ptr) {pp.setSHA1(sha1ptr);}
  bool decompress(int n = -1);  // n bytes, -1=all, return true until done
  bool pcomp(Writer* out2) {return pp.z.write(out2, true);}
  void readSegmentEnd(char* sha1string = 0);
  int stat(int x) {return dec.stat(x);}
  int buffered() {return dec.buffered();}
private:
  ZPAQL z;
  Decoder dec;
  PostProcessor pp;
  enum {BLOCK, FILENAME, COMMENT, DATA, SEGEND} state;  // expected next
  enum {FIRSTSEG, SEG, SKIP} decode_state;  // which segment in block?
};

/////////////////////////// decompress() /////////////////////

void decompress(Reader* in, Writer* out);

//////////////////////////// Encoder /////////////////////////

// Encoder compresses using an arithmetic code
class Encoder {
public:
  Encoder(ZPAQL& z, int size=0):
    out(0), low(1), high(0xFFFFFFFF), pr(z) {}
  void init();
  void compress(int c);  // c is 0..255 or EOF
  int stat(int x) {return pr.stat(x);}
  Writer* out;  // destination
private:
  U32 low, high; // range
  Predictor pr;  // to get p
  Array<char> buf; // unmodeled input
  void encode(int y, int p); // encode bit y (0..1) with prob. p (0..65535)
};

//////////////////////////// Compiler ////////////////////////

// Input ZPAQL source code with args and store the compiled code
// in hz and pz and write pcomp_cmd to out2.

class Compiler {
public:
  Compiler(const char* in, int* args, ZPAQL& hz, ZPAQL& pz, Writer* out2);
private:
  const char* in;  // ZPAQL source code
  int* args;       // Array of up to 9 args, default NULL = all 0
  ZPAQL& hz;       // Output of COMP and HCOMP sections
  ZPAQL& pz;       // Output of PCOMP section
  Writer* out2;    // Output ... of "PCOMP ... ;"
  int line;        // Input line number for reporting errors
  int state;       // parse state: 0=space -1=word >0 (nest level)

  // Symbolic constants
  typedef enum {NONE,CONS,CM,ICM,MATCH,AVG,MIX2,MIX,ISSE,SSE,
    JT=39,JF=47,JMP=63,LJ=255,
    POST=256,PCOMP,END,IF,IFNOT,ELSE,ENDIF,DO,
    WHILE,UNTIL,FOREVER,IFL,IFNOTL,ELSEL,SEMICOLON} CompType;

  void syntaxError(const char* msg, const char* expected=0); // error()
  void next();                     // advance in to next token
  bool matchToken(const char* tok);// in==token?
  int rtoken(int low, int high);   // return token which must be in range
  int rtoken(const char* list[]);  // return token by position in list
  void rtoken(const char* s);      // return token which must be s
  int compile_comp(ZPAQL& z);      // compile either HCOMP or PCOMP

  // Stack of n elements
  class Stack {
    libzpaq::Array<U16> s;
    size_t top;
  public:
    Stack(int n): s(n), top(0) {}
    void push(const U16& x) {
      if (top>=s.size()) error("IF or DO nested too deep");
      s[top++]=x;
    }
    U16 pop() {
      if (top<=0) error("unmatched IF or DO");
      return s[--top];
    }
  };

  Stack if_stack, do_stack;
};

//////////////////////// Compressor //////////////////////////

class Compressor {
public:
  Compressor(): enc(z), in(0), state(INIT), verify(false) {}
  void setOutput(Writer* out) {enc.out=out;}
  void writeTag();
  void startBlock(int level);  // level=1,2,3
  void startBlock(const char* hcomp);     // ZPAQL byte code
  void startBlock(const char* config,     // ZPAQL source code
                  int* args,              // NULL or int[9] arguments
                  Writer* pcomp_cmd = 0); // retrieve preprocessor command
  void setVerify(bool v) {verify = v;}    // check postprocessing?
  void hcomp(Writer* out2) {z.write(out2, false);}
  bool pcomp(Writer* out2) {return pz.write(out2, true);}
  void startSegment(const char* filename = 0, const char* comment = 0);
  void setInput(Reader* i) {in=i;}
  void postProcess(const char* pcomp = 0, int len = 0);  // byte code
  bool compress(int n = -1);  // n bytes, -1=all, return true until done
  void endSegment(const char* sha1string = 0);
  char* endSegmentChecksum(int64_t* size = 0, bool dosha1=true);
  int64_t getSize() {return sha1.usize();}
  const char* getChecksum() {return sha1.result();}
  void endBlock();
  int stat(int x) {return enc.stat(x);}
private:
  ZPAQL z, pz;  // model and test postprocessor
  Encoder enc;  // arithmetic encoder containing predictor
  Reader* in;   // input source
  SHA1 sha1;    // to test pz output
  char sha1result[20];  // sha1 output
  enum {INIT, BLOCK1, SEG1, BLOCK2, SEG2} state;
  bool verify;  // if true then test by postprocessing
};

/////////////////////////// StringBuffer /////////////////////

// For (de)compressing to/from a string. Writing appends bytes
// which can be later read.
class StringBuffer: public libzpaq::Reader, public libzpaq::Writer {
  unsigned char* p;  // allocated memory, not NUL terminated, may be NULL
  size_t al;         // number of bytes allocated, 0 iff p is NULL
  size_t wpos;       // index of next byte to write, wpos <= al
  size_t rpos;       // index of next byte to read, rpos < wpos or return EOF.
  size_t limit;      // max size, default = -1
  const size_t init; // initial size on first use after reset

  // Increase capacity to a without changing size
  void reserve(size_t a) {
    assert(!al==!p);
    if (a<=al) return;
    unsigned char* q=0;
    if (a>0) q=(unsigned char*)(p ? realloc(p, a) : malloc(a));
    if (a>0 && !q) error("Out of memory");
    p=q;
    al=a;
  }

  // Enlarge al to make room to write at least n bytes.
  void lengthen(size_t n) {
    assert(wpos<=al);
    if (wpos+n>limit || wpos+n<wpos) error("StringBuffer overflow");
    if (wpos+n<=al) return;
    size_t a=al;
    while (wpos+n>=a) a=a*2+init;
    reserve(a);
  }

  // No assignment or copy
  void operator=(const StringBuffer&);
  StringBuffer(const StringBuffer&);

public:

  // Direct access to data
  unsigned char* data() {assert(p || wpos==0); return p;}

  // Allocate no memory initially
  StringBuffer(size_t n=0):
      p(0), al(0), wpos(0), rpos(0), limit(size_t(-1)), init(n>128?n:128) {}

  // Set output limit
  void setLimit(size_t n) {limit=n;}

  // Free memory
  ~StringBuffer() {if (p) free(p);}

  // Return number of bytes written.
  size_t size() const {return wpos;}

  // Return number of bytes left to read
  size_t remaining() const {return wpos-rpos;}

  // Reset size to 0 and free memory.
  void reset() {
    if (p) free(p);
    p=0;
    al=rpos=wpos=0;
  }

  // Write a single byte.
  void put(int c) {  // write 1 byte
    lengthen(1);
    assert(p);
    assert(wpos<al);
    p[wpos++]=c;
    assert(wpos<=al);
  }

  // Write buf[0..n-1]. If buf is NULL then advance write pointer only.
  void write(const char* buf, int n) {
    if (n<1) return;
    lengthen(n);
    assert(p);
    assert(wpos+n<=al);
    if (buf) memcpy(p+wpos, buf, n);
    wpos+=n;
  }

  // Read a single byte. Return EOF (-1) at end.
  int get() {
    assert(rpos<=wpos);
    assert(rpos==wpos || p);
    return rpos<wpos ? p[rpos++] : -1;
  }

  // Read up to n bytes into buf[0..] or fewer if EOF is first.
  // Return the number of bytes actually read.
  // If buf is NULL then advance read pointer without reading.
  int read(char* buf, int n) {
    assert(rpos<=wpos);
    assert(wpos<=al);
    assert(!al==!p);
    if (rpos+n>wpos) n=wpos-rpos;
    if (n>0 && buf) memcpy(buf, p+rpos, n);
    rpos+=n;
    return n;
  }

  // Return the entire string as a read-only array.
  const char* c_str() const {return (const char*)p;}

  // Truncate the string to size i.
  void resize(size_t i) {
    wpos=i;
    if (rpos>wpos) rpos=wpos;
  }

  // Swap efficiently (init is not swapped)
  void swap(StringBuffer& s) {
    std::swap(p, s.p);
    std::swap(al, s.al);
    std::swap(wpos, s.wpos);
    std::swap(rpos, s.rpos);
    std::swap(limit, s.limit);
  }
};

/////////////////////////// compress() ///////////////////////

// Compress in to out in multiple blocks. Default method is "14,128,0"
// Default filename is "". Comment is appended to input size.
// dosha1 means save the SHA-1 checksum.
void compress(Reader* in, Writer* out, const char* method,
     const char* filename=0, const char* comment=0, bool dosha1=true);

// Same as compress() but output is 1 block, ignoring block size parameter.
void compressBlock(StringBuffer* in, Writer* out, const char* method,
     const char* filename=0, const char* comment=0, bool dosha1=true);

}  // namespace libzpaq





namespace libzpaq {

// Read 16 bit little-endian number
int toU16(const char* p) {
  return (p[0]&255)+256*(p[1]&255);
}

// Default read() and write()
int Reader::read(char* buf, int n) {
  int i=0, c;
  while (i<n && (c=get())>=0)
    buf[i++]=c;
  return i;
}

void Writer::write(const char* buf, int n) {
  for (int i=0; i<n; ++i)
    put(U8(buf[i]));
}

///////////////////////// allocx //////////////////////

// Allocate newsize > 0 bytes of executable memory and update
// p to point to it and newsize = n. Free any previously
// allocated memory first. If newsize is 0 then free only.
// Call error in case of failure. If NOJIT, ignore newsize
// and set p=0, n=0 without allocating memory.
void allocx(U8* &p, int &n, int newsize) {
#ifdef NOJIT
  p=0;
  n=0;
#else
  if (p || n) {
    if (p)
#ifdef unix
      munmap(p, n);
#else // Windows
      VirtualFree(p, 0, MEM_RELEASE);
#endif
    p=0;
    n=0;
  }
  if (newsize>0) {
#ifdef unix
    p=(U8*)mmap(0, newsize, PROT_READ|PROT_WRITE|PROT_EXEC,
                MAP_PRIVATE|MAP_ANON, -1, 0);
    if ((void*)p==MAP_FAILED) p=0;
#else
    p=(U8*)VirtualAlloc(0, newsize, MEM_RESERVE|MEM_COMMIT,
                        PAGE_EXECUTE_READWRITE);
#endif
    if (p)
      n=newsize;
    else {
      n=0;
      error("allocx failed");
    }
  }
#endif
}

//////////////////////////// SHA1 ////////////////////////////

// SHA1 code, see http://en.wikipedia.org/wiki/SHA-1

// Start a new hash
void SHA1::init() {
  len=0;
  h[0]=0x67452301;
  h[1]=0xEFCDAB89;
  h[2]=0x98BADCFE;
  h[3]=0x10325476;
  h[4]=0xC3D2E1F0;
  memset(w, 0, sizeof(w));
  
///	make_secret(345,_wyp);
}

// Return old result and start a new hash
const char* SHA1::result() {

  // pad and append length
  const U64 s=len;
  put(0x80);
  while ((len&511)!=448)
    put(0);
  put(s>>56);
  put(s>>48);
  put(s>>40);
  put(s>>32);
  put(s>>24);
  put(s>>16);
  put(s>>8);
  put(s);

  // copy h to hbuf
  for (unsigned int i=0; i<5; ++i) {
    hbuf[4*i]=h[i]>>24;
    hbuf[4*i+1]=h[i]>>16;
    hbuf[4*i+2]=h[i]>>8;
    hbuf[4*i+3]=h[i];
  }

  // return hash prior to clearing state
  init();
  return hbuf;
}

// Hash buf[0..n-1]
void SHA1::write(const char* buf, int64_t n) {
  const unsigned char* p=(const unsigned char*) buf;
  for (; n>0 && (U32(len)&511)!=0; --n) put(*p++);
  for (; n>=64; n-=64) {
    for (unsigned int i=0; i<16; ++i)
      w[i]=p[0]<<24|p[1]<<16|p[2]<<8|p[3], p+=4;
    len+=512;
    process();
  }
  for (; n>0; --n) put(*p++);
}

// Hash 1 block of 64 bytes
void SHA1::process() {
	

  U32 a=h[0], b=h[1], c=h[2], d=h[3], e=h[4];
  static const U32 k[4]={0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6};
  #define f(a,b,c,d,e,i) \
    if (i>=16) \
      w[(i)&15]^=w[(i-3)&15]^w[(i-8)&15]^w[(i-14)&15], \
      w[(i)&15]=w[(i)&15]<<1|w[(i)&15]>>31; \
    e+=(a<<5|a>>27)+k[(i)/20]+w[(i)&15] \
      +((i)%40>=20 ? b^c^d : i>=40 ? (b&c)|(d&(b|c)) : d^(b&(c^d))); \
    b=b<<30|b>>2;
  #define r(i) f(a,b,c,d,e,i) f(e,a,b,c,d,i+1) f(d,e,a,b,c,i+2) \
               f(c,d,e,a,b,i+3) f(b,c,d,e,a,i+4)
  r(0)  r(5)  r(10) r(15) r(20) r(25) r(30) r(35)
  r(40) r(45) r(50) r(55) r(60) r(65) r(70) r(75)
  #undef f
  #undef r
  h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e;
}

//////////////////////////// SHA256 //////////////////////////

void SHA256::init() {
  len0=len1=0;
  s[0]=0x6a09e667;
  s[1]=0xbb67ae85;
  s[2]=0x3c6ef372;
  s[3]=0xa54ff53a;
  s[4]=0x510e527f;
  s[5]=0x9b05688c;
  s[6]=0x1f83d9ab;
  s[7]=0x5be0cd19;
  memset(w, 0, sizeof(w));
}

void SHA256::process() {

  #define ror(a,b) ((a)>>(b)|(a<<(32-(b))))

  #define m(i) \
     w[(i)&15]+=w[(i-7)&15] \
       +(ror(w[(i-15)&15],7)^ror(w[(i-15)&15],18)^(w[(i-15)&15]>>3)) \
       +(ror(w[(i-2)&15],17)^ror(w[(i-2)&15],19)^(w[(i-2)&15]>>10))

  #define r(a,b,c,d,e,f,g,h,i) { \
    unsigned t1=ror(e,14)^e; \
    t1=ror(t1,5)^e; \
    h+=ror(t1,6)+((e&f)^(~e&g))+k[i]+w[(i)&15]; } \
    d+=h; \
    {unsigned t1=ror(a,9)^a; \
    t1=ror(t1,11)^a; \
    h+=ror(t1,2)+((a&b)^(c&(a^b))); }

  #define mr(a,b,c,d,e,f,g,h,i) m(i); r(a,b,c,d,e,f,g,h,i);

  #define r8(i) \
    r(a,b,c,d,e,f,g,h,i);   \
    r(h,a,b,c,d,e,f,g,i+1); \
    r(g,h,a,b,c,d,e,f,i+2); \
    r(f,g,h,a,b,c,d,e,i+3); \
    r(e,f,g,h,a,b,c,d,i+4); \
    r(d,e,f,g,h,a,b,c,i+5); \
    r(c,d,e,f,g,h,a,b,i+6); \
    r(b,c,d,e,f,g,h,a,i+7);

  #define mr8(i) \
    mr(a,b,c,d,e,f,g,h,i);   \
    mr(h,a,b,c,d,e,f,g,i+1); \
    mr(g,h,a,b,c,d,e,f,i+2); \
    mr(f,g,h,a,b,c,d,e,i+3); \
    mr(e,f,g,h,a,b,c,d,i+4); \
    mr(d,e,f,g,h,a,b,c,i+5); \
    mr(c,d,e,f,g,h,a,b,i+6); \
    mr(b,c,d,e,f,g,h,a,i+7);

  static const unsigned k[64]={
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

  unsigned a=s[0];
  unsigned b=s[1];
  unsigned c=s[2];
  unsigned d=s[3];
  unsigned e=s[4];
  unsigned f=s[5];
  unsigned g=s[6];
  unsigned h=s[7];

  r8(0);
  r8(8);
  mr8(16);
  mr8(24);
  mr8(32);
  mr8(40);
  mr8(48);
  mr8(56);

  s[0]+=a;
  s[1]+=b;
  s[2]+=c;
  s[3]+=d;
  s[4]+=e;
  s[5]+=f;
  s[6]+=g;
  s[7]+=h;

  #undef mr8
  #undef r8
  #undef mr
  #undef r
  #undef m
  #undef ror
}

// Return old result and start a new hash
const char* SHA256::result() {

  // pad and append length
  const unsigned s1=len1, s0=len0;
  put(0x80);
  while ((len0&511)!=448) put(0);
  put(s1>>24);
  put(s1>>16);
  put(s1>>8);
  put(s1);
  put(s0>>24);
  put(s0>>16);
  put(s0>>8);
  put(s0);

  // copy s to hbuf
  for (unsigned int i=0; i<8; ++i) {
    hbuf[4*i]=s[i]>>24;
    hbuf[4*i+1]=s[i]>>16;
    hbuf[4*i+2]=s[i]>>8;
    hbuf[4*i+3]=s[i];
  }

  // return hash prior to clearing state
  init();
  return hbuf;
}

//////////////////////////// AES /////////////////////////////

// Some AES code is derived from libtomcrypt 1.17 (public domain).

#define Te4_0 0x000000FF & Te4
#define Te4_1 0x0000FF00 & Te4
#define Te4_2 0x00FF0000 & Te4
#define Te4_3 0xFF000000 & Te4

// Extract byte n of x
static inline unsigned byte(unsigned x, unsigned n) {return (x>>(8*n))&255;}

// x = y[0..3] MSB first
static inline void LOAD32H(U32& x, const char* y) {
  const unsigned char* u=(const unsigned char*)y;
  x=u[0]<<24|u[1]<<16|u[2]<<8|u[3];
}

// y[0..3] = x MSB first
static inline void STORE32H(U32& x, unsigned char* y) {
  y[0]=x>>24;
  y[1]=x>>16;
  y[2]=x>>8;
  y[3]=x;
}

#define setup_mix(temp) \
  ((Te4_3[byte(temp, 2)]) ^ (Te4_2[byte(temp, 1)]) ^ \
   (Te4_1[byte(temp, 0)]) ^ (Te4_0[byte(temp, 3)]))

// Initialize encryption tables and round key. keylen is 16, 24, or 32.
AES_CTR::AES_CTR(const char* key, int keylen, const char* iv) {
  assert(key  != NULL);
  assert(keylen==16 || keylen==24 || keylen==32);

  // Initialize IV (default 0)
  iv0=iv1=0;
  if (iv) {
    LOAD32H(iv0, iv);
    LOAD32H(iv1, iv+4);
  }

  // Initialize encryption tables
  for (unsigned int i=0; i<256; ++i) {
    unsigned s1=
    "\x63\x7c\x77\x7b\xf2\x6b\x6f\xc5\x30\x01\x67\x2b\xfe\xd7\xab\x76"
    "\xca\x82\xc9\x7d\xfa\x59\x47\xf0\xad\xd4\xa2\xaf\x9c\xa4\x72\xc0"
    "\xb7\xfd\x93\x26\x36\x3f\xf7\xcc\x34\xa5\xe5\xf1\x71\xd8\x31\x15"
    "\x04\xc7\x23\xc3\x18\x96\x05\x9a\x07\x12\x80\xe2\xeb\x27\xb2\x75"
    "\x09\x83\x2c\x1a\x1b\x6e\x5a\xa0\x52\x3b\xd6\xb3\x29\xe3\x2f\x84"
    "\x53\xd1\x00\xed\x20\xfc\xb1\x5b\x6a\xcb\xbe\x39\x4a\x4c\x58\xcf"
    "\xd0\xef\xaa\xfb\x43\x4d\x33\x85\x45\xf9\x02\x7f\x50\x3c\x9f\xa8"
    "\x51\xa3\x40\x8f\x92\x9d\x38\xf5\xbc\xb6\xda\x21\x10\xff\xf3\xd2"
    "\xcd\x0c\x13\xec\x5f\x97\x44\x17\xc4\xa7\x7e\x3d\x64\x5d\x19\x73"
    "\x60\x81\x4f\xdc\x22\x2a\x90\x88\x46\xee\xb8\x14\xde\x5e\x0b\xdb"
    "\xe0\x32\x3a\x0a\x49\x06\x24\x5c\xc2\xd3\xac\x62\x91\x95\xe4\x79"
    "\xe7\xc8\x37\x6d\x8d\xd5\x4e\xa9\x6c\x56\xf4\xea\x65\x7a\xae\x08"
    "\xba\x78\x25\x2e\x1c\xa6\xb4\xc6\xe8\xdd\x74\x1f\x4b\xbd\x8b\x8a"
    "\x70\x3e\xb5\x66\x48\x03\xf6\x0e\x61\x35\x57\xb9\x86\xc1\x1d\x9e"
    "\xe1\xf8\x98\x11\x69\xd9\x8e\x94\x9b\x1e\x87\xe9\xce\x55\x28\xdf"
    "\x8c\xa1\x89\x0d\xbf\xe6\x42\x68\x41\x99\x2d\x0f\xb0\x54\xbb\x16"
    [i]&255;
    unsigned s2=s1<<1;
    if (s2>=0x100) s2^=0x11b;
    unsigned s3=s1^s2;
    Te0[i]=s2<<24|s1<<16|s1<<8|s3;
    Te1[i]=s3<<24|s2<<16|s1<<8|s1;
    Te2[i]=s1<<24|s3<<16|s2<<8|s1;
    Te3[i]=s1<<24|s1<<16|s3<<8|s2;
    Te4[i]=s1<<24|s1<<16|s1<<8|s1;
  }

  // setup the forward key
  Nr = 10 + ((keylen/8)-2)*2;  // 10, 12, or 14 rounds
  int i = 0;
  U32* rk = &ek[0];
  U32 temp;
  static const U32 rcon[10] = {
    0x01000000UL, 0x02000000UL, 0x04000000UL, 0x08000000UL,
    0x10000000UL, 0x20000000UL, 0x40000000UL, 0x80000000UL,
    0x1B000000UL, 0x36000000UL};  // round constants

  LOAD32H(rk[0], key   );
  LOAD32H(rk[1], key +  4);
  LOAD32H(rk[2], key +  8);
  LOAD32H(rk[3], key + 12);
  if (keylen == 16) {
    for (;;) {
      temp  = rk[3];
      rk[4] = rk[0] ^ setup_mix(temp) ^ rcon[i];
      rk[5] = rk[1] ^ rk[4];
      rk[6] = rk[2] ^ rk[5];
      rk[7] = rk[3] ^ rk[6];
      if (++i == 10) {
         break;
      }
      rk += 4;
    }
  }
  else if (keylen == 24) {
    LOAD32H(rk[4], key + 16);
    LOAD32H(rk[5], key + 20);
    for (;;) {
      temp = rk[5];
      rk[ 6] = rk[ 0] ^ setup_mix(temp) ^ rcon[i];
      rk[ 7] = rk[ 1] ^ rk[ 6];
      rk[ 8] = rk[ 2] ^ rk[ 7];
      rk[ 9] = rk[ 3] ^ rk[ 8];
      if (++i == 8) {
        break;
      }
      rk[10] = rk[ 4] ^ rk[ 9];
      rk[11] = rk[ 5] ^ rk[10];
      rk += 6;
    }
  }
  else if (keylen == 32) {
    LOAD32H(rk[4], key + 16);
    LOAD32H(rk[5], key + 20);
    LOAD32H(rk[6], key + 24);
    LOAD32H(rk[7], key + 28);
    for (;;) {
      temp = rk[7];
      rk[ 8] = rk[ 0] ^ setup_mix(temp) ^ rcon[i];
      rk[ 9] = rk[ 1] ^ rk[ 8];
      rk[10] = rk[ 2] ^ rk[ 9];
      rk[11] = rk[ 3] ^ rk[10];
      if (++i == 7) {
        break;
      }
      temp = rk[11];
      rk[12] = rk[ 4] ^ setup_mix(temp<<24|temp>>8);
      rk[13] = rk[ 5] ^ rk[12];
      rk[14] = rk[ 6] ^ rk[13];
      rk[15] = rk[ 7] ^ rk[14];
      rk += 8;
    }
  }
}

// Encrypt to ct[16]
void AES_CTR::encrypt(U32 s0, U32 s1, U32 s2, U32 s3, unsigned char* ct) {
  int r = Nr >> 1;
  U32 *rk = &ek[0];
  U32 t0=0, t1=0, t2=0, t3=0;
  s0 ^= rk[0];
  s1 ^= rk[1];
  s2 ^= rk[2];
  s3 ^= rk[3];
  for (;;) {
    t0 =
      Te0[byte(s0, 3)] ^
      Te1[byte(s1, 2)] ^
      Te2[byte(s2, 1)] ^
      Te3[byte(s3, 0)] ^
      rk[4];
    t1 =
      Te0[byte(s1, 3)] ^
      Te1[byte(s2, 2)] ^
      Te2[byte(s3, 1)] ^
      Te3[byte(s0, 0)] ^
      rk[5];
    t2 =
      Te0[byte(s2, 3)] ^
      Te1[byte(s3, 2)] ^
      Te2[byte(s0, 1)] ^
      Te3[byte(s1, 0)] ^
      rk[6];
    t3 =
      Te0[byte(s3, 3)] ^
      Te1[byte(s0, 2)] ^
      Te2[byte(s1, 1)] ^
      Te3[byte(s2, 0)] ^
      rk[7];

    rk += 8;
    if (--r == 0) {
      break;
    }

    s0 =
      Te0[byte(t0, 3)] ^
      Te1[byte(t1, 2)] ^
      Te2[byte(t2, 1)] ^
      Te3[byte(t3, 0)] ^
      rk[0];
    s1 =
      Te0[byte(t1, 3)] ^
      Te1[byte(t2, 2)] ^
      Te2[byte(t3, 1)] ^
      Te3[byte(t0, 0)] ^
      rk[1];
    s2 =
      Te0[byte(t2, 3)] ^
      Te1[byte(t3, 2)] ^
      Te2[byte(t0, 1)] ^
      Te3[byte(t1, 0)] ^
      rk[2];
    s3 =
      Te0[byte(t3, 3)] ^
      Te1[byte(t0, 2)] ^
      Te2[byte(t1, 1)] ^
      Te3[byte(t2, 0)] ^
      rk[3];
  }

  // apply last round and map cipher state to byte array block:
  s0 =
    (Te4_3[byte(t0, 3)]) ^
    (Te4_2[byte(t1, 2)]) ^
    (Te4_1[byte(t2, 1)]) ^
    (Te4_0[byte(t3, 0)]) ^
    rk[0];
  STORE32H(s0, ct);
  s1 =
    (Te4_3[byte(t1, 3)]) ^
    (Te4_2[byte(t2, 2)]) ^
    (Te4_1[byte(t3, 1)]) ^
    (Te4_0[byte(t0, 0)]) ^
    rk[1];
  STORE32H(s1, ct+4);
  s2 =
    (Te4_3[byte(t2, 3)]) ^
    (Te4_2[byte(t3, 2)]) ^
    (Te4_1[byte(t0, 1)]) ^
    (Te4_0[byte(t1, 0)]) ^
    rk[2];
  STORE32H(s2, ct+8);
  s3 =
    (Te4_3[byte(t3, 3)]) ^
    (Te4_2[byte(t0, 2)]) ^
    (Te4_1[byte(t1, 1)]) ^
    (Te4_0[byte(t2, 0)]) ^ 
    rk[3];
  STORE32H(s3, ct+12);
}

// Encrypt or decrypt slice buf[0..n-1] at offset by XOR with AES(i) where
// i is the 128 bit big-endian distance from the start in 16 byte blocks.
void AES_CTR::encrypt(char* buf, int n, U64 offset) {
  for (U64 i=offset/16; i<=(offset+n)/16; ++i) {
    unsigned char ct[16];
    encrypt(iv0, iv1, i>>32, i, ct);
    for (int j=0; j<16; ++j) {
      const int k=i*16-offset+j;
      if (k>=0 && k<n)
        buf[k]^=ct[j];
    }
  }
}

#undef setup_mix
#undef Te4_3
#undef Te4_2
#undef Te4_1
#undef Te4_0

//////////////////////////// stretchKey //////////////////////

// PBKDF2(pw[0..pwlen], salt[0..saltlen], c) to buf[0..dkLen-1]
// using HMAC-SHA256, for the special case of c = 1 iterations
// output size dkLen a multiple of 32, and pwLen <= 64.
static void pbkdf2(const char* pw, int pwLen, const char* salt, int saltLen,
                   int c, char* buf, int dkLen) {
  assert(c==1);
  assert(dkLen%32==0);
  assert(pwLen<=64);

  libzpaq::SHA256 sha256;
  char b[32];
  for (int i=1; i*32<=dkLen; ++i) {
    for (int j=0; j<pwLen; ++j) sha256.put(pw[j]^0x36);
    for (int j=pwLen; j<64; ++j) sha256.put(0x36);
    for (int j=0; j<saltLen; ++j) sha256.put(salt[j]);
    for (int j=24; j>=0; j-=8) sha256.put(i>>j);
    memcpy(b, sha256.result(), 32);
    for (int j=0; j<pwLen; ++j) sha256.put(pw[j]^0x5c);
    for (int j=pwLen; j<64; ++j) sha256.put(0x5c);
    for (int j=0; j<32; ++j) sha256.put(b[j]);
    memcpy(buf+i*32-32, sha256.result(), 32);
  }
}

// Hash b[0..15] using 8 rounds of salsa20
// Modified from http://cr.yp.to/salsa20.html (public domain) to 8 rounds
static void salsa8(U32* b) {
  unsigned x[16]={0};
  memcpy(x, b, 64);
  for (unsigned int i=0; i<4; ++i) {
    #define R(a,b) (((a)<<(b))+((a)>>(32-b)))
    x[ 4] ^= R(x[ 0]+x[12], 7);  x[ 8] ^= R(x[ 4]+x[ 0], 9);
    x[12] ^= R(x[ 8]+x[ 4],13);  x[ 0] ^= R(x[12]+x[ 8],18);
    x[ 9] ^= R(x[ 5]+x[ 1], 7);  x[13] ^= R(x[ 9]+x[ 5], 9);
    x[ 1] ^= R(x[13]+x[ 9],13);  x[ 5] ^= R(x[ 1]+x[13],18);
    x[14] ^= R(x[10]+x[ 6], 7);  x[ 2] ^= R(x[14]+x[10], 9);
    x[ 6] ^= R(x[ 2]+x[14],13);  x[10] ^= R(x[ 6]+x[ 2],18);
    x[ 3] ^= R(x[15]+x[11], 7);  x[ 7] ^= R(x[ 3]+x[15], 9);
    x[11] ^= R(x[ 7]+x[ 3],13);  x[15] ^= R(x[11]+x[ 7],18);
    x[ 1] ^= R(x[ 0]+x[ 3], 7);  x[ 2] ^= R(x[ 1]+x[ 0], 9);
    x[ 3] ^= R(x[ 2]+x[ 1],13);  x[ 0] ^= R(x[ 3]+x[ 2],18);
    x[ 6] ^= R(x[ 5]+x[ 4], 7);  x[ 7] ^= R(x[ 6]+x[ 5], 9);
    x[ 4] ^= R(x[ 7]+x[ 6],13);  x[ 5] ^= R(x[ 4]+x[ 7],18);
    x[11] ^= R(x[10]+x[ 9], 7);  x[ 8] ^= R(x[11]+x[10], 9);
    x[ 9] ^= R(x[ 8]+x[11],13);  x[10] ^= R(x[ 9]+x[ 8],18);
    x[12] ^= R(x[15]+x[14], 7);  x[13] ^= R(x[12]+x[15], 9);
    x[14] ^= R(x[13]+x[12],13);  x[15] ^= R(x[14]+x[13],18);
    #undef R
  }
  for (unsigned int i=0; i<16; ++i) b[i]+=x[i];
}

// BlockMix_{Salsa20/8, r} on b[0..128*r-1]
static void blockmix(U32* b, int r) {
  assert(r<=8);
  U32 x[16];
  U32 y[256];
  memcpy(x, b+32*r-16, 64);
  for (int i=0; i<2*r; ++i) {
    for (int j=0; j<16; ++j) x[j]^=b[i*16+j];
    salsa8(x);
    memcpy(&y[i*16], x, 64);
  }
  for (int i=0; i<r; ++i) memcpy(b+i*16, &y[i*32], 64);
  for (int i=0; i<r; ++i) memcpy(b+(i+r)*16, &y[i*32+16], 64);
}

// Mix b[0..128*r-1]. Uses 128*r*n bytes of memory and O(r*n) time
static void smix(char* b, int r, int n) {
  libzpaq::Array<U32> x(32*r), v(32*r*n);
  for (int i=0; i<r*128; ++i) x[i/4]+=(b[i]&255)<<i%4*8;
  for (int i=0; i<n; ++i) {
    memcpy(&v[i*r*32], &x[0], r*128);
    blockmix(&x[0], r);
  }
  for (int i=0; i<n; ++i) {
    U32 j=x[(2*r-1)*16]&(n-1);
    for (int k=0; k<r*32; ++k) x[k]^=v[j*r*32+k];
    blockmix(&x[0], r);
  }
  for (int i=0; i<r*128; ++i) b[i]=x[i/4]>>(i%4*8);
}

// Strengthen password pw[0..pwlen-1] and salt[0..saltlen-1]
// to produce key buf[0..buflen-1]. Uses O(n*r*p) time and 128*r*n bytes
// of memory. n must be a power of 2 and r <= 8.
void scrypt(const char* pw, int pwlen,
            const char* salt, int saltlen,
            int n, int r, int p, char* buf, int buflen) {
  assert(r<=8);
  assert(n>0 && (n&(n-1))==0);  // power of 2?
  libzpaq::Array<char> b(p*r*128);
  pbkdf2(pw, pwlen, salt, saltlen, 1, &b[0], p*r*128);
  for (int i=0; i<p; ++i) smix(&b[i*r*128], r, n);
  pbkdf2(pw, pwlen, &b[0], p*r*128, 1, buf, buflen);
}

// Stretch key in[0..31], assumed to be SHA256(password), with
// NUL terminate salt to produce new key out[0..31]
void stretchKey(char* out, const char* in, const char* salt) {
  scrypt(in, 32, salt, 32, 1<<14, 8, 1, out, 32);
}

//////////////////////////// random //////////////////////////

// Put n cryptographic random bytes in buf[0..n-1].
// The first byte will not be 'z' or '7' (start of a ZPAQ archive).
// For a pure random number, discard the first byte.
// In VC++, must link to advapi32.lib.

void random(char* buf, int n) {
#ifdef unix
  FILE* in=fopen("/dev/urandom", "rb");
  if (in && int(fread(buf, 1, n, in))==n)
    fclose(in);
  else {
    error("key generation failed");
  }
#else
  HCRYPTPROV h;
  if (CryptAcquireContext(&h, NULL, NULL, PROV_RSA_FULL,
      CRYPT_VERIFYCONTEXT) && CryptGenRandom(h, n, (BYTE*)buf))
    CryptReleaseContext(h, 0);
  else {
    fprintf(stderr, "CryptGenRandom: error %d\n", int(GetLastError()));
    error("key generation failed");
  }
#endif
  if (n>=1 && (buf[0]=='z' || buf[0]=='7'))
    buf[0]^=0x80;
}

//////////////////////////// Component ///////////////////////

// A Component is a context model, indirect context model, match model,
// fixed weight mixer, adaptive 2 input mixer without or with current
// partial byte as context, adaptive m input mixer (without or with),
// or SSE (without or with).

const int compsize[256]={0,2,3,2,3,4,6,6,3,5};

void Component::init() {
  limit=cxt=a=b=c=0;
  cm.resize(0);
  ht.resize(0);
  a16.resize(0);
}

////////////////////////// StateTable ////////////////////////

// sns[i*4] -> next state if 0, next state if 1, n0, n1
static const U8 sns[1024]={
     1,     2,     0,     0,     3,     5,     1,     0,
     4,     6,     0,     1,     7,     9,     2,     0,
     8,    11,     1,     1,     8,    11,     1,     1,
    10,    12,     0,     2,    13,    15,     3,     0,
    14,    17,     2,     1,    14,    17,     2,     1,
    16,    19,     1,     2,    16,    19,     1,     2,
    18,    20,     0,     3,    21,    23,     4,     0,
    22,    25,     3,     1,    22,    25,     3,     1,
    24,    27,     2,     2,    24,    27,     2,     2,
    26,    29,     1,     3,    26,    29,     1,     3,
    28,    30,     0,     4,    31,    33,     5,     0,
    32,    35,     4,     1,    32,    35,     4,     1,
    34,    37,     3,     2,    34,    37,     3,     2,
    36,    39,     2,     3,    36,    39,     2,     3,
    38,    41,     1,     4,    38,    41,     1,     4,
    40,    42,     0,     5,    43,    33,     6,     0,
    44,    47,     5,     1,    44,    47,     5,     1,
    46,    49,     4,     2,    46,    49,     4,     2,
    48,    51,     3,     3,    48,    51,     3,     3,
    50,    53,     2,     4,    50,    53,     2,     4,
    52,    55,     1,     5,    52,    55,     1,     5,
    40,    56,     0,     6,    57,    45,     7,     0,
    58,    47,     6,     1,    58,    47,     6,     1,
    60,    63,     5,     2,    60,    63,     5,     2,
    62,    65,     4,     3,    62,    65,     4,     3,
    64,    67,     3,     4,    64,    67,     3,     4,
    66,    69,     2,     5,    66,    69,     2,     5,
    52,    71,     1,     6,    52,    71,     1,     6,
    54,    72,     0,     7,    73,    59,     8,     0,
    74,    61,     7,     1,    74,    61,     7,     1,
    76,    63,     6,     2,    76,    63,     6,     2,
    78,    81,     5,     3,    78,    81,     5,     3,
    80,    83,     4,     4,    80,    83,     4,     4,
    82,    85,     3,     5,    82,    85,     3,     5,
    66,    87,     2,     6,    66,    87,     2,     6,
    68,    89,     1,     7,    68,    89,     1,     7,
    70,    90,     0,     8,    91,    59,     9,     0,
    92,    77,     8,     1,    92,    77,     8,     1,
    94,    79,     7,     2,    94,    79,     7,     2,
    96,    81,     6,     3,    96,    81,     6,     3,
    98,   101,     5,     4,    98,   101,     5,     4,
   100,   103,     4,     5,   100,   103,     4,     5,
    82,   105,     3,     6,    82,   105,     3,     6,
    84,   107,     2,     7,    84,   107,     2,     7,
    86,   109,     1,     8,    86,   109,     1,     8,
    70,   110,     0,     9,   111,    59,    10,     0,
   112,    77,     9,     1,   112,    77,     9,     1,
   114,    97,     8,     2,   114,    97,     8,     2,
   116,    99,     7,     3,   116,    99,     7,     3,
    62,   101,     6,     4,    62,   101,     6,     4,
    80,    83,     5,     5,    80,    83,     5,     5,
   100,    67,     4,     6,   100,    67,     4,     6,
   102,   119,     3,     7,   102,   119,     3,     7,
   104,   121,     2,     8,   104,   121,     2,     8,
    86,   123,     1,     9,    86,   123,     1,     9,
    70,   124,     0,    10,   125,    59,    11,     0,
   126,    77,    10,     1,   126,    77,    10,     1,
   128,    97,     9,     2,   128,    97,     9,     2,
    60,    63,     8,     3,    60,    63,     8,     3,
    66,    69,     3,     8,    66,    69,     3,     8,
   104,   131,     2,     9,   104,   131,     2,     9,
    86,   133,     1,    10,    86,   133,     1,    10,
    70,   134,     0,    11,   135,    59,    12,     0,
   136,    77,    11,     1,   136,    77,    11,     1,
   138,    97,    10,     2,   138,    97,    10,     2,
   104,   141,     2,    10,   104,   141,     2,    10,
    86,   143,     1,    11,    86,   143,     1,    11,
    70,   144,     0,    12,   145,    59,    13,     0,
   146,    77,    12,     1,   146,    77,    12,     1,
   148,    97,    11,     2,   148,    97,    11,     2,
   104,   151,     2,    11,   104,   151,     2,    11,
    86,   153,     1,    12,    86,   153,     1,    12,
    70,   154,     0,    13,   155,    59,    14,     0,
   156,    77,    13,     1,   156,    77,    13,     1,
   158,    97,    12,     2,   158,    97,    12,     2,
   104,   161,     2,    12,   104,   161,     2,    12,
    86,   163,     1,    13,    86,   163,     1,    13,
    70,   164,     0,    14,   165,    59,    15,     0,
   166,    77,    14,     1,   166,    77,    14,     1,
   168,    97,    13,     2,   168,    97,    13,     2,
   104,   171,     2,    13,   104,   171,     2,    13,
    86,   173,     1,    14,    86,   173,     1,    14,
    70,   174,     0,    15,   175,    59,    16,     0,
   176,    77,    15,     1,   176,    77,    15,     1,
   178,    97,    14,     2,   178,    97,    14,     2,
   104,   181,     2,    14,   104,   181,     2,    14,
    86,   183,     1,    15,    86,   183,     1,    15,
    70,   184,     0,    16,   185,    59,    17,     0,
   186,    77,    16,     1,   186,    77,    16,     1,
    74,    97,    15,     2,    74,    97,    15,     2,
   104,    89,     2,    15,   104,    89,     2,    15,
    86,   187,     1,    16,    86,   187,     1,    16,
    70,   188,     0,    17,   189,    59,    18,     0,
   190,    77,    17,     1,    86,   191,     1,    17,
    70,   192,     0,    18,   193,    59,    19,     0,
   194,    77,    18,     1,    86,   195,     1,    18,
    70,   196,     0,    19,   193,    59,    20,     0,
   197,    77,    19,     1,    86,   198,     1,    19,
    70,   196,     0,    20,   199,    77,    20,     1,
    86,   200,     1,    20,   201,    77,    21,     1,
    86,   202,     1,    21,   203,    77,    22,     1,
    86,   204,     1,    22,   205,    77,    23,     1,
    86,   206,     1,    23,   207,    77,    24,     1,
    86,   208,     1,    24,   209,    77,    25,     1,
    86,   210,     1,    25,   211,    77,    26,     1,
    86,   212,     1,    26,   213,    77,    27,     1,
    86,   214,     1,    27,   215,    77,    28,     1,
    86,   216,     1,    28,   217,    77,    29,     1,
    86,   218,     1,    29,   219,    77,    30,     1,
    86,   220,     1,    30,   221,    77,    31,     1,
    86,   222,     1,    31,   223,    77,    32,     1,
    86,   224,     1,    32,   225,    77,    33,     1,
    86,   226,     1,    33,   227,    77,    34,     1,
    86,   228,     1,    34,   229,    77,    35,     1,
    86,   230,     1,    35,   231,    77,    36,     1,
    86,   232,     1,    36,   233,    77,    37,     1,
    86,   234,     1,    37,   235,    77,    38,     1,
    86,   236,     1,    38,   237,    77,    39,     1,
    86,   238,     1,    39,   239,    77,    40,     1,
    86,   240,     1,    40,   241,    77,    41,     1,
    86,   242,     1,    41,   243,    77,    42,     1,
    86,   244,     1,    42,   245,    77,    43,     1,
    86,   246,     1,    43,   247,    77,    44,     1,
    86,   248,     1,    44,   249,    77,    45,     1,
    86,   250,     1,    45,   251,    77,    46,     1,
    86,   252,     1,    46,   253,    77,    47,     1,
    86,   254,     1,    47,   253,    77,    48,     1,
    86,   254,     1,    48,     0,     0,     0,     0
};

// Initialize next state table ns[state*4] -> next if 0, next if 1, n0, n1
StateTable::StateTable() {
  memcpy(ns, sns, sizeof(ns));
}

/////////////////////////// ZPAQL //////////////////////////

// Write header to out2, return true if HCOMP/PCOMP section is present.
// If pp is true, then write only the postprocessor code.
bool ZPAQL::write(Writer* out2, bool pp) {
  if (header.size()<=6) return false;
  assert(header[0]+256*header[1]==cend-2+hend-hbegin);
  assert(cend>=7);
  assert(hbegin>=cend);
  assert(hend>=hbegin);
  assert(out2);
  if (!pp) {  // if not a postprocessor then write COMP
    for (int i=0; i<cend; ++i)
      out2->put(header[i]);
  }
  else {  // write PCOMP size only
    out2->put((hend-hbegin)&255);
    out2->put((hend-hbegin)>>8);
  }
  for (int i=hbegin; i<hend; ++i)
    out2->put(header[i]);
  return true;
}

// Read header from in2
int ZPAQL::read(Reader* in2) {

  // Get header size and allocate
  int hsize=in2->get();
  hsize+=in2->get()*256;
  header.resize(hsize+300);
  cend=hbegin=hend=0;
  header[cend++]=hsize&255;
  header[cend++]=hsize>>8;
  while (cend<7) header[cend++]=in2->get(); // hh hm ph pm n

  // Read COMP
  int n=header[cend-1];
  for (int i=0; i<n; ++i) {
    int type=in2->get();  // component type
    if (type<0 || type>255) error("unexpected end of file");
    header[cend++]=type;  // component type
    int size=compsize[type];
    if (size<1) error("Invalid component type");
    if (cend+size>hsize) error("COMP overflows header");
    for (int j=1; j<size; ++j)
      header[cend++]=in2->get();
  }
  if ((header[cend++]=in2->get())!=0) error("missing COMP END");

  // Insert a guard gap and read HCOMP
  hbegin=hend=cend+128;
  if (hend>hsize+129) error("missing HCOMP");
  while (hend<hsize+129) {
    assert(hend<header.isize()-8);
    int op=in2->get();
    if (op==-1) error("unexpected end of file");
    header[hend++]=op;
  }
  if ((header[hend++]=in2->get())!=0) error("missing HCOMP END");
  assert(cend>=7 && cend<header.isize());
  assert(hbegin==cend+128 && hbegin<header.isize());
  assert(hend>hbegin && hend<header.isize());
  assert(hsize==header[0]+256*header[1]);
  assert(hsize==cend-2+hend-hbegin);
  allocx(rcode, rcode_size, 0);  // clear JIT code
  return cend+hend-hbegin;
}

// Free memory, but preserve output, sha1 pointers
void ZPAQL::clear() {
  cend=hbegin=hend=0;  // COMP and HCOMP locations
  a=b=c=d=f=pc=0;      // machine state
  header.resize(0);
  h.resize(0);
  m.resize(0);
  r.resize(0);
  allocx(rcode, rcode_size, 0);
}

// Constructor
ZPAQL::ZPAQL() {
  output=0;
  sha1=0;
  rcode=0;
  rcode_size=0;
  clear();
  outbuf.resize(1<<14);
  bufptr=0;
}

ZPAQL::~ZPAQL() {
  allocx(rcode, rcode_size, 0);
}

// Initialize machine state as HCOMP
void ZPAQL::inith() {
  assert(header.isize()>6);
  assert(output==0);
  assert(sha1==0);
  init(header[2], header[3]); // hh, hm
}

// Initialize machine state as PCOMP
void ZPAQL::initp() {
  assert(header.isize()>6);
  init(header[4], header[5]); // ph, pm
}

// Flush pending output
void ZPAQL::flush() {
  if (output) output->write(&outbuf[0], bufptr);
  if (sha1) sha1->write(&outbuf[0], bufptr);
  bufptr=0;
}

// pow(2, x)
static double pow2(int x) {
  double r=1;
  for (; x>0; x--) r+=r;
  return r;
}

// Return memory requirement in bytes
double ZPAQL::memory() {
  double mem=pow2(header[2]+2)+pow2(header[3])  // hh hm
            +pow2(header[4]+2)+pow2(header[5])  // ph pm
            +header.size();
  int cp=7;  // start of comp list
  for (unsigned int i=0; i<header[6]; ++i) {  // n
    assert(cp<cend);
    double size=pow2(header[cp+1]); // sizebits
    switch(header[cp]) {
      case CM: mem+=4*size; break;
      case ICM: mem+=64*size+1024; break;
      case MATCH: mem+=4*size+pow2(header[cp+2]); break; // bufbits
      case MIX2: mem+=2*size; break;
      case MIX: mem+=4*size*header[cp+3]; break; // m
      case ISSE: mem+=64*size+2048; break;
      case SSE: mem+=128*size; break;
    }
    cp+=compsize[header[cp]];
  }
  return mem;
}

// Initialize machine state to run a program.
void ZPAQL::init(int hbits, int mbits) {
  assert(header.isize()>0);
  assert(cend>=7);
  assert(hbegin>=cend+128);
  assert(hend>=hbegin);
  assert(hend<header.isize()-130);
  assert(header[0]+256*header[1]==cend-2+hend-hbegin);
  assert(bufptr==0);
  assert(outbuf.isize()>0);
  if (hbits>32) error("H too big");
  if (mbits>32) error("M too big");
  h.resize(1, hbits);
  m.resize(1, mbits);
  r.resize(256);
  a=b=c=d=pc=f=0;
}

// Run program on input by interpreting header
void ZPAQL::run0(U32 input) {
  assert(cend>6);
  assert(hbegin>=cend+128);
  assert(hend>=hbegin);
  assert(hend<header.isize()-130);
  assert(m.size()>0);
  assert(h.size()>0);
  assert(header[0]+256*header[1]==cend+hend-hbegin-2);
  pc=hbegin;
  a=input;
  while (execute()) ;
}

// Execute one instruction, return 0 after HALT else 1
int ZPAQL::execute() {
  switch(header[pc++]) {
    case 0: err(); break; // ERROR
    case 1: ++a; break; // A++
    case 2: --a; break; // A--
    case 3: a = ~a; break; // A!
    case 4: a = 0; break; // A=0
    case 7: a = r[header[pc++]]; break; // A=R N
    case 8: swap(b); break; // B<>A
    case 9: ++b; break; // B++
    case 10: --b; break; // B--
    case 11: b = ~b; break; // B!
    case 12: b = 0; break; // B=0
    case 15: b = r[header[pc++]]; break; // B=R N
    case 16: swap(c); break; // C<>A
    case 17: ++c; break; // C++
    case 18: --c; break; // C--
    case 19: c = ~c; break; // C!
    case 20: c = 0; break; // C=0
    case 23: c = r[header[pc++]]; break; // C=R N
    case 24: swap(d); break; // D<>A
    case 25: ++d; break; // D++
    case 26: --d; break; // D--
    case 27: d = ~d; break; // D!
    case 28: d = 0; break; // D=0
    case 31: d = r[header[pc++]]; break; // D=R N
    case 32: swap(m(b)); break; // *B<>A
    case 33: ++m(b); break; // *B++
    case 34: --m(b); break; // *B--
    case 35: m(b) = ~m(b); break; // *B!
    case 36: m(b) = 0; break; // *B=0
    case 39: if (f) pc+=((header[pc]+128)&255)-127; else ++pc; break; // JT N
    case 40: swap(m(c)); break; // *C<>A
    case 41: ++m(c); break; // *C++
    case 42: --m(c); break; // *C--
    case 43: m(c) = ~m(c); break; // *C!
    case 44: m(c) = 0; break; // *C=0
    case 47: if (!f) pc+=((header[pc]+128)&255)-127; else ++pc; break; // JF N
    case 48: swap(h(d)); break; // *D<>A
    case 49: ++h(d); break; // *D++
    case 50: --h(d); break; // *D--
    case 51: h(d) = ~h(d); break; // *D!
    case 52: h(d) = 0; break; // *D=0
    case 55: r[header[pc++]] = a; break; // R=A N
    case 56: return 0  ; // HALT
    case 57: outc(a&255); break; // OUT
    case 59: a = (a+m(b)+512)*773; break; // HASH
    case 60: h(d) = (h(d)+a+512)*773; break; // HASHD
    case 63: pc+=((header[pc]+128)&255)-127; break; // JMP N
    case 64: break; // A=A
    case 65: a = b; break; // A=B
    case 66: a = c; break; // A=C
    case 67: a = d; break; // A=D
    case 68: a = m(b); break; // A=*B
    case 69: a = m(c); break; // A=*C
    case 70: a = h(d); break; // A=*D
    case 71: a = header[pc++]; break; // A= N
    case 72: b = a; break; // B=A
    case 73: break; // B=B
    case 74: b = c; break; // B=C
    case 75: b = d; break; // B=D
    case 76: b = m(b); break; // B=*B
    case 77: b = m(c); break; // B=*C
    case 78: b = h(d); break; // B=*D
    case 79: b = header[pc++]; break; // B= N
    case 80: c = a; break; // C=A
    case 81: c = b; break; // C=B
    case 82: break; // C=C
    case 83: c = d; break; // C=D
    case 84: c = m(b); break; // C=*B
    case 85: c = m(c); break; // C=*C
    case 86: c = h(d); break; // C=*D
    case 87: c = header[pc++]; break; // C= N
    case 88: d = a; break; // D=A
    case 89: d = b; break; // D=B
    case 90: d = c; break; // D=C
    case 91: break; // D=D
    case 92: d = m(b); break; // D=*B
    case 93: d = m(c); break; // D=*C
    case 94: d = h(d); break; // D=*D
    case 95: d = header[pc++]; break; // D= N
    case 96: m(b) = a; break; // *B=A
    case 97: m(b) = b; break; // *B=B
    case 98: m(b) = c; break; // *B=C
    case 99: m(b) = d; break; // *B=D
    case 100: break; // *B=*B
    case 101: m(b) = m(c); break; // *B=*C
    case 102: m(b) = h(d); break; // *B=*D
    case 103: m(b) = header[pc++]; break; // *B= N
    case 104: m(c) = a; break; // *C=A
    case 105: m(c) = b; break; // *C=B
    case 106: m(c) = c; break; // *C=C
    case 107: m(c) = d; break; // *C=D
    case 108: m(c) = m(b); break; // *C=*B
    case 109: break; // *C=*C
    case 110: m(c) = h(d); break; // *C=*D
    case 111: m(c) = header[pc++]; break; // *C= N
    case 112: h(d) = a; break; // *D=A
    case 113: h(d) = b; break; // *D=B
    case 114: h(d) = c; break; // *D=C
    case 115: h(d) = d; break; // *D=D
    case 116: h(d) = m(b); break; // *D=*B
    case 117: h(d) = m(c); break; // *D=*C
    case 118: break; // *D=*D
    case 119: h(d) = header[pc++]; break; // *D= N
    case 128: a += a; break; // A+=A
    case 129: a += b; break; // A+=B
    case 130: a += c; break; // A+=C
    case 131: a += d; break; // A+=D
    case 132: a += m(b); break; // A+=*B
    case 133: a += m(c); break; // A+=*C
    case 134: a += h(d); break; // A+=*D
    case 135: a += header[pc++]; break; // A+= N
    case 136: a -= a; break; // A-=A
    case 137: a -= b; break; // A-=B
    case 138: a -= c; break; // A-=C
    case 139: a -= d; break; // A-=D
    case 140: a -= m(b); break; // A-=*B
    case 141: a -= m(c); break; // A-=*C
    case 142: a -= h(d); break; // A-=*D
    case 143: a -= header[pc++]; break; // A-= N
    case 144: a *= a; break; // A*=A
    case 145: a *= b; break; // A*=B
    case 146: a *= c; break; // A*=C
    case 147: a *= d; break; // A*=D
    case 148: a *= m(b); break; // A*=*B
    case 149: a *= m(c); break; // A*=*C
    case 150: a *= h(d); break; // A*=*D
    case 151: a *= header[pc++]; break; // A*= N
    case 152: div(a); break; // A/=A
    case 153: div(b); break; // A/=B
    case 154: div(c); break; // A/=C
    case 155: div(d); break; // A/=D
    case 156: div(m(b)); break; // A/=*B
    case 157: div(m(c)); break; // A/=*C
    case 158: div(h(d)); break; // A/=*D
    case 159: div(header[pc++]); break; // A/= N
    case 160: mod(a); break; // A%=A
    case 161: mod(b); break; // A%=B
    case 162: mod(c); break; // A%=C
    case 163: mod(d); break; // A%=D
    case 164: mod(m(b)); break; // A%=*B
    case 165: mod(m(c)); break; // A%=*C
    case 166: mod(h(d)); break; // A%=*D
    case 167: mod(header[pc++]); break; // A%= N
    case 168: a &= a; break; // A&=A
    case 169: a &= b; break; // A&=B
    case 170: a &= c; break; // A&=C
    case 171: a &= d; break; // A&=D
    case 172: a &= m(b); break; // A&=*B
    case 173: a &= m(c); break; // A&=*C
    case 174: a &= h(d); break; // A&=*D
    case 175: a &= header[pc++]; break; // A&= N
    case 176: a &= ~ a; break; // A&~A
    case 177: a &= ~ b; break; // A&~B
    case 178: a &= ~ c; break; // A&~C
    case 179: a &= ~ d; break; // A&~D
    case 180: a &= ~ m(b); break; // A&~*B
    case 181: a &= ~ m(c); break; // A&~*C
    case 182: a &= ~ h(d); break; // A&~*D
    case 183: a &= ~ header[pc++]; break; // A&~ N
    case 184: a |= a; break; // A|=A
    case 185: a |= b; break; // A|=B
    case 186: a |= c; break; // A|=C
    case 187: a |= d; break; // A|=D
    case 188: a |= m(b); break; // A|=*B
    case 189: a |= m(c); break; // A|=*C
    case 190: a |= h(d); break; // A|=*D
    case 191: a |= header[pc++]; break; // A|= N
    case 192: a ^= a; break; // A^=A
    case 193: a ^= b; break; // A^=B
    case 194: a ^= c; break; // A^=C
    case 195: a ^= d; break; // A^=D
    case 196: a ^= m(b); break; // A^=*B
    case 197: a ^= m(c); break; // A^=*C
    case 198: a ^= h(d); break; // A^=*D
    case 199: a ^= header[pc++]; break; // A^= N
    case 200: a <<= (a&31); break; // A<<=A
    case 201: a <<= (b&31); break; // A<<=B
    case 202: a <<= (c&31); break; // A<<=C
    case 203: a <<= (d&31); break; // A<<=D
    case 204: a <<= (m(b)&31); break; // A<<=*B
    case 205: a <<= (m(c)&31); break; // A<<=*C
    case 206: a <<= (h(d)&31); break; // A<<=*D
    case 207: a <<= (header[pc++]&31); break; // A<<= N
    case 208: a >>= (a&31); break; // A>>=A
    case 209: a >>= (b&31); break; // A>>=B
    case 210: a >>= (c&31); break; // A>>=C
    case 211: a >>= (d&31); break; // A>>=D
    case 212: a >>= (m(b)&31); break; // A>>=*B
    case 213: a >>= (m(c)&31); break; // A>>=*C
    case 214: a >>= (h(d)&31); break; // A>>=*D
    case 215: a >>= (header[pc++]&31); break; // A>>= N
    case 216: f = 1; break; // A==A
    case 217: f = (a == b); break; // A==B
    case 218: f = (a == c); break; // A==C
    case 219: f = (a == d); break; // A==D
    case 220: f = (a == U32(m(b))); break; // A==*B
    case 221: f = (a == U32(m(c))); break; // A==*C
    case 222: f = (a == h(d)); break; // A==*D
    case 223: f = (a == U32(header[pc++])); break; // A== N
    case 224: f = 0; break; // A<A
    case 225: f = (a < b); break; // A<B
    case 226: f = (a < c); break; // A<C
    case 227: f = (a < d); break; // A<D
    case 228: f = (a < U32(m(b))); break; // A<*B
    case 229: f = (a < U32(m(c))); break; // A<*C
    case 230: f = (a < h(d)); break; // A<*D
    case 231: f = (a < U32(header[pc++])); break; // A< N
    case 232: f = 0; break; // A>A
    case 233: f = (a > b); break; // A>B
    case 234: f = (a > c); break; // A>C
    case 235: f = (a > d); break; // A>D
    case 236: f = (a > U32(m(b))); break; // A>*B
    case 237: f = (a > U32(m(c))); break; // A>*C
    case 238: f = (a > h(d)); break; // A>*D
    case 239: f = (a > U32(header[pc++])); break; // A> N
    case 255: if((pc=hbegin+header[pc]+256*header[pc+1])>=hend)err();break;//LJ
    default: err();
  }
  return 1;
}

// Print illegal instruction error message and exit
void ZPAQL::err() {
  error("ZPAQL execution error");
}

///////////////////////// Predictor /////////////////////////

// sdt2k[i]=2048/i;
static const int sdt2k[256]={
     0,  2048,  1024,   682,   512,   409,   341,   292,
   256,   227,   204,   186,   170,   157,   146,   136,
   128,   120,   113,   107,   102,    97,    93,    89,
    85,    81,    78,    75,    73,    70,    68,    66,
    64,    62,    60,    58,    56,    55,    53,    52,
    51,    49,    48,    47,    46,    45,    44,    43,
    42,    41,    40,    40,    39,    38,    37,    37,
    36,    35,    35,    34,    34,    33,    33,    32,
    32,    31,    31,    30,    30,    29,    29,    28,
    28,    28,    27,    27,    26,    26,    26,    25,
    25,    25,    24,    24,    24,    24,    23,    23,
    23,    23,    22,    22,    22,    22,    21,    21,
    21,    21,    20,    20,    20,    20,    20,    19,
    19,    19,    19,    19,    18,    18,    18,    18,
    18,    18,    17,    17,    17,    17,    17,    17,
    17,    16,    16,    16,    16,    16,    16,    16,
    16,    15,    15,    15,    15,    15,    15,    15,
    15,    14,    14,    14,    14,    14,    14,    14,
    14,    14,    14,    13,    13,    13,    13,    13,
    13,    13,    13,    13,    13,    13,    12,    12,
    12,    12,    12,    12,    12,    12,    12,    12,
    12,    12,    12,    11,    11,    11,    11,    11,
    11,    11,    11,    11,    11,    11,    11,    11,
    11,    11,    11,    10,    10,    10,    10,    10,
    10,    10,    10,    10,    10,    10,    10,    10,
    10,    10,    10,    10,    10,     9,     9,     9,
     9,     9,     9,     9,     9,     9,     9,     9,
     9,     9,     9,     9,     9,     9,     9,     9,
     9,     9,     9,     9,     8,     8,     8,     8,
     8,     8,     8,     8,     8,     8,     8,     8,
     8,     8,     8,     8,     8,     8,     8,     8,
     8,     8,     8,     8,     8,     8,     8,     8
};

// sdt[i]=(1<<17)/(i*2+3)*2;
static const int sdt[1024]={
 87380, 52428, 37448, 29126, 23830, 20164, 17476, 15420,
 13796, 12482, 11396, 10484,  9708,  9038,  8456,  7942,
  7488,  7084,  6720,  6392,  6096,  5824,  5576,  5348,
  5140,  4946,  4766,  4598,  4442,  4296,  4160,  4032,
  3912,  3798,  3692,  3590,  3494,  3404,  3318,  3236,
  3158,  3084,  3012,  2944,  2880,  2818,  2758,  2702,
  2646,  2594,  2544,  2496,  2448,  2404,  2360,  2318,
  2278,  2240,  2202,  2166,  2130,  2096,  2064,  2032,
  2000,  1970,  1940,  1912,  1884,  1858,  1832,  1806,
  1782,  1758,  1736,  1712,  1690,  1668,  1648,  1628,
  1608,  1588,  1568,  1550,  1532,  1514,  1496,  1480,
  1464,  1448,  1432,  1416,  1400,  1386,  1372,  1358,
  1344,  1330,  1316,  1304,  1290,  1278,  1266,  1254,
  1242,  1230,  1218,  1208,  1196,  1186,  1174,  1164,
  1154,  1144,  1134,  1124,  1114,  1106,  1096,  1086,
  1078,  1068,  1060,  1052,  1044,  1036,  1028,  1020,
  1012,  1004,   996,   988,   980,   974,   966,   960,
   952,   946,   938,   932,   926,   918,   912,   906,
   900,   894,   888,   882,   876,   870,   864,   858,
   852,   848,   842,   836,   832,   826,   820,   816,
   810,   806,   800,   796,   790,   786,   782,   776,
   772,   768,   764,   758,   754,   750,   746,   742,
   738,   734,   730,   726,   722,   718,   714,   710,
   706,   702,   698,   694,   690,   688,   684,   680,
   676,   672,   670,   666,   662,   660,   656,   652,
   650,   646,   644,   640,   636,   634,   630,   628,
   624,   622,   618,   616,   612,   610,   608,   604,
   602,   598,   596,   594,   590,   588,   586,   582,
   580,   578,   576,   572,   570,   568,   566,   562,
   560,   558,   556,   554,   550,   548,   546,   544,
   542,   540,   538,   536,   532,   530,   528,   526,
   524,   522,   520,   518,   516,   514,   512,   510,
   508,   506,   504,   502,   500,   498,   496,   494,
   492,   490,   488,   488,   486,   484,   482,   480,
   478,   476,   474,   474,   472,   470,   468,   466,
   464,   462,   462,   460,   458,   456,   454,   454,
   452,   450,   448,   448,   446,   444,   442,   442,
   440,   438,   436,   436,   434,   432,   430,   430,
   428,   426,   426,   424,   422,   422,   420,   418,
   418,   416,   414,   414,   412,   410,   410,   408,
   406,   406,   404,   402,   402,   400,   400,   398,
   396,   396,   394,   394,   392,   390,   390,   388,
   388,   386,   386,   384,   382,   382,   380,   380,
   378,   378,   376,   376,   374,   372,   372,   370,
   370,   368,   368,   366,   366,   364,   364,   362,
   362,   360,   360,   358,   358,   356,   356,   354,
   354,   352,   352,   350,   350,   348,   348,   348,
   346,   346,   344,   344,   342,   342,   340,   340,
   340,   338,   338,   336,   336,   334,   334,   332,
   332,   332,   330,   330,   328,   328,   328,   326,
   326,   324,   324,   324,   322,   322,   320,   320,
   320,   318,   318,   316,   316,   316,   314,   314,
   312,   312,   312,   310,   310,   310,   308,   308,
   308,   306,   306,   304,   304,   304,   302,   302,
   302,   300,   300,   300,   298,   298,   298,   296,
   296,   296,   294,   294,   294,   292,   292,   292,
   290,   290,   290,   288,   288,   288,   286,   286,
   286,   284,   284,   284,   284,   282,   282,   282,
   280,   280,   280,   278,   278,   278,   276,   276,
   276,   276,   274,   274,   274,   272,   272,   272,
   272,   270,   270,   270,   268,   268,   268,   268,
   266,   266,   266,   266,   264,   264,   264,   262,
   262,   262,   262,   260,   260,   260,   260,   258,
   258,   258,   258,   256,   256,   256,   256,   254,
   254,   254,   254,   252,   252,   252,   252,   250,
   250,   250,   250,   248,   248,   248,   248,   248,
   246,   246,   246,   246,   244,   244,   244,   244,
   242,   242,   242,   242,   242,   240,   240,   240,
   240,   238,   238,   238,   238,   238,   236,   236,
   236,   236,   234,   234,   234,   234,   234,   232,
   232,   232,   232,   232,   230,   230,   230,   230,
   230,   228,   228,   228,   228,   228,   226,   226,
   226,   226,   226,   224,   224,   224,   224,   224,
   222,   222,   222,   222,   222,   220,   220,   220,
   220,   220,   220,   218,   218,   218,   218,   218,
   216,   216,   216,   216,   216,   216,   214,   214,
   214,   214,   214,   212,   212,   212,   212,   212,
   212,   210,   210,   210,   210,   210,   210,   208,
   208,   208,   208,   208,   208,   206,   206,   206,
   206,   206,   206,   204,   204,   204,   204,   204,
   204,   204,   202,   202,   202,   202,   202,   202,
   200,   200,   200,   200,   200,   200,   198,   198,
   198,   198,   198,   198,   198,   196,   196,   196,
   196,   196,   196,   196,   194,   194,   194,   194,
   194,   194,   194,   192,   192,   192,   192,   192,
   192,   192,   190,   190,   190,   190,   190,   190,
   190,   188,   188,   188,   188,   188,   188,   188,
   186,   186,   186,   186,   186,   186,   186,   186,
   184,   184,   184,   184,   184,   184,   184,   182,
   182,   182,   182,   182,   182,   182,   182,   180,
   180,   180,   180,   180,   180,   180,   180,   178,
   178,   178,   178,   178,   178,   178,   178,   176,
   176,   176,   176,   176,   176,   176,   176,   176,
   174,   174,   174,   174,   174,   174,   174,   174,
   172,   172,   172,   172,   172,   172,   172,   172,
   172,   170,   170,   170,   170,   170,   170,   170,
   170,   170,   168,   168,   168,   168,   168,   168,
   168,   168,   168,   166,   166,   166,   166,   166,
   166,   166,   166,   166,   166,   164,   164,   164,
   164,   164,   164,   164,   164,   164,   162,   162,
   162,   162,   162,   162,   162,   162,   162,   162,
   160,   160,   160,   160,   160,   160,   160,   160,
   160,   160,   158,   158,   158,   158,   158,   158,
   158,   158,   158,   158,   158,   156,   156,   156,
   156,   156,   156,   156,   156,   156,   156,   154,
   154,   154,   154,   154,   154,   154,   154,   154,
   154,   154,   152,   152,   152,   152,   152,   152,
   152,   152,   152,   152,   152,   150,   150,   150,
   150,   150,   150,   150,   150,   150,   150,   150,
   150,   148,   148,   148,   148,   148,   148,   148,
   148,   148,   148,   148,   148,   146,   146,   146,
   146,   146,   146,   146,   146,   146,   146,   146,
   146,   144,   144,   144,   144,   144,   144,   144,
   144,   144,   144,   144,   144,   142,   142,   142,
   142,   142,   142,   142,   142,   142,   142,   142,
   142,   142,   140,   140,   140,   140,   140,   140,
   140,   140,   140,   140,   140,   140,   140,   138,
   138,   138,   138,   138,   138,   138,   138,   138,
   138,   138,   138,   138,   138,   136,   136,   136,
   136,   136,   136,   136,   136,   136,   136,   136,
   136,   136,   136,   134,   134,   134,   134,   134,
   134,   134,   134,   134,   134,   134,   134,   134,
   134,   132,   132,   132,   132,   132,   132,   132,
   132,   132,   132,   132,   132,   132,   132,   132,
   130,   130,   130,   130,   130,   130,   130,   130,
   130,   130,   130,   130,   130,   130,   130,   128,
   128,   128,   128,   128,   128,   128,   128,   128,
   128,   128,   128,   128,   128,   128,   128,   126
};

// ssquasht[i]=int(32768.0/(1+exp((i-2048)*(-1.0/64))));
// Middle 1344 of 4096 entries only.
static const U16 ssquasht[1344]={
     0,     0,     0,     0,     0,     0,     0,     1,
     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,
     4,     4,     4,     4,     4,     4,     4,     4,
     4,     4,     4,     4,     4,     4,     5,     5,
     5,     5,     5,     5,     5,     5,     5,     5,
     5,     5,     6,     6,     6,     6,     6,     6,
     6,     6,     6,     6,     7,     7,     7,     7,
     7,     7,     7,     7,     8,     8,     8,     8,
     8,     8,     8,     8,     9,     9,     9,     9,
     9,     9,    10,    10,    10,    10,    10,    10,
    10,    11,    11,    11,    11,    11,    12,    12,
    12,    12,    12,    13,    13,    13,    13,    13,
    14,    14,    14,    14,    15,    15,    15,    15,
    15,    16,    16,    16,    17,    17,    17,    17,
    18,    18,    18,    18,    19,    19,    19,    20,
    20,    20,    21,    21,    21,    22,    22,    22,
    23,    23,    23,    24,    24,    25,    25,    25,
    26,    26,    27,    27,    28,    28,    28,    29,
    29,    30,    30,    31,    31,    32,    32,    33,
    33,    34,    34,    35,    36,    36,    37,    37,
    38,    38,    39,    40,    40,    41,    42,    42,
    43,    44,    44,    45,    46,    46,    47,    48,
    49,    49,    50,    51,    52,    53,    54,    54,
    55,    56,    57,    58,    59,    60,    61,    62,
    63,    64,    65,    66,    67,    68,    69,    70,
    71,    72,    73,    74,    76,    77,    78,    79,
    81,    82,    83,    84,    86,    87,    88,    90,
    91,    93,    94,    96,    97,    99,   100,   102,
   103,   105,   107,   108,   110,   112,   114,   115,
   117,   119,   121,   123,   125,   127,   129,   131,
   133,   135,   137,   139,   141,   144,   146,   148,
   151,   153,   155,   158,   160,   163,   165,   168,
   171,   173,   176,   179,   182,   184,   187,   190,
   193,   196,   199,   202,   206,   209,   212,   215,
   219,   222,   226,   229,   233,   237,   240,   244,
   248,   252,   256,   260,   264,   268,   272,   276,
   281,   285,   289,   294,   299,   303,   308,   313,
   318,   323,   328,   333,   338,   343,   349,   354,
   360,   365,   371,   377,   382,   388,   394,   401,
   407,   413,   420,   426,   433,   440,   446,   453,
   460,   467,   475,   482,   490,   497,   505,   513,
   521,   529,   537,   545,   554,   562,   571,   580,
   589,   598,   607,   617,   626,   636,   646,   656,
   666,   676,   686,   697,   708,   719,   730,   741,
   752,   764,   776,   788,   800,   812,   825,   837,
   850,   863,   876,   890,   903,   917,   931,   946,
   960,   975,   990,  1005,  1020,  1036,  1051,  1067,
  1084,  1100,  1117,  1134,  1151,  1169,  1186,  1204,
  1223,  1241,  1260,  1279,  1298,  1318,  1338,  1358,
  1379,  1399,  1421,  1442,  1464,  1486,  1508,  1531,
  1554,  1577,  1600,  1624,  1649,  1673,  1698,  1724,
  1749,  1775,  1802,  1829,  1856,  1883,  1911,  1940,
  1968,  1998,  2027,  2057,  2087,  2118,  2149,  2181,
  2213,  2245,  2278,  2312,  2345,  2380,  2414,  2450,
  2485,  2521,  2558,  2595,  2633,  2671,  2709,  2748,
  2788,  2828,  2869,  2910,  2952,  2994,  3037,  3080,
  3124,  3168,  3213,  3259,  3305,  3352,  3399,  3447,
  3496,  3545,  3594,  3645,  3696,  3747,  3799,  3852,
  3906,  3960,  4014,  4070,  4126,  4182,  4240,  4298,
  4356,  4416,  4476,  4537,  4598,  4660,  4723,  4786,
  4851,  4916,  4981,  5048,  5115,  5183,  5251,  5320,
  5390,  5461,  5533,  5605,  5678,  5752,  5826,  5901,
  5977,  6054,  6131,  6210,  6289,  6369,  6449,  6530,
  6613,  6695,  6779,  6863,  6949,  7035,  7121,  7209,
  7297,  7386,  7476,  7566,  7658,  7750,  7842,  7936,
  8030,  8126,  8221,  8318,  8415,  8513,  8612,  8712,
  8812,  8913,  9015,  9117,  9221,  9324,  9429,  9534,
  9640,  9747,  9854,  9962, 10071, 10180, 10290, 10401,
 10512, 10624, 10737, 10850, 10963, 11078, 11192, 11308,
 11424, 11540, 11658, 11775, 11893, 12012, 12131, 12251,
 12371, 12491, 12612, 12734, 12856, 12978, 13101, 13224,
 13347, 13471, 13595, 13719, 13844, 13969, 14095, 14220,
 14346, 14472, 14599, 14725, 14852, 14979, 15106, 15233,
 15361, 15488, 15616, 15744, 15872, 16000, 16128, 16256,
 16384, 16511, 16639, 16767, 16895, 17023, 17151, 17279,
 17406, 17534, 17661, 17788, 17915, 18042, 18168, 18295,
 18421, 18547, 18672, 18798, 18923, 19048, 19172, 19296,
 19420, 19543, 19666, 19789, 19911, 20033, 20155, 20276,
 20396, 20516, 20636, 20755, 20874, 20992, 21109, 21227,
 21343, 21459, 21575, 21689, 21804, 21917, 22030, 22143,
 22255, 22366, 22477, 22587, 22696, 22805, 22913, 23020,
 23127, 23233, 23338, 23443, 23546, 23650, 23752, 23854,
 23955, 24055, 24155, 24254, 24352, 24449, 24546, 24641,
 24737, 24831, 24925, 25017, 25109, 25201, 25291, 25381,
 25470, 25558, 25646, 25732, 25818, 25904, 25988, 26072,
 26154, 26237, 26318, 26398, 26478, 26557, 26636, 26713,
 26790, 26866, 26941, 27015, 27089, 27162, 27234, 27306,
 27377, 27447, 27516, 27584, 27652, 27719, 27786, 27851,
 27916, 27981, 28044, 28107, 28169, 28230, 28291, 28351,
 28411, 28469, 28527, 28585, 28641, 28697, 28753, 28807,
 28861, 28915, 28968, 29020, 29071, 29122, 29173, 29222,
 29271, 29320, 29368, 29415, 29462, 29508, 29554, 29599,
 29643, 29687, 29730, 29773, 29815, 29857, 29898, 29939,
 29979, 30019, 30058, 30096, 30134, 30172, 30209, 30246,
 30282, 30317, 30353, 30387, 30422, 30455, 30489, 30522,
 30554, 30586, 30618, 30649, 30680, 30710, 30740, 30769,
 30799, 30827, 30856, 30884, 30911, 30938, 30965, 30992,
 31018, 31043, 31069, 31094, 31118, 31143, 31167, 31190,
 31213, 31236, 31259, 31281, 31303, 31325, 31346, 31368,
 31388, 31409, 31429, 31449, 31469, 31488, 31507, 31526,
 31544, 31563, 31581, 31598, 31616, 31633, 31650, 31667,
 31683, 31700, 31716, 31731, 31747, 31762, 31777, 31792,
 31807, 31821, 31836, 31850, 31864, 31877, 31891, 31904,
 31917, 31930, 31942, 31955, 31967, 31979, 31991, 32003,
 32015, 32026, 32037, 32048, 32059, 32070, 32081, 32091,
 32101, 32111, 32121, 32131, 32141, 32150, 32160, 32169,
 32178, 32187, 32196, 32205, 32213, 32222, 32230, 32238,
 32246, 32254, 32262, 32270, 32277, 32285, 32292, 32300,
 32307, 32314, 32321, 32327, 32334, 32341, 32347, 32354,
 32360, 32366, 32373, 32379, 32385, 32390, 32396, 32402,
 32407, 32413, 32418, 32424, 32429, 32434, 32439, 32444,
 32449, 32454, 32459, 32464, 32468, 32473, 32478, 32482,
 32486, 32491, 32495, 32499, 32503, 32507, 32511, 32515,
 32519, 32523, 32527, 32530, 32534, 32538, 32541, 32545,
 32548, 32552, 32555, 32558, 32561, 32565, 32568, 32571,
 32574, 32577, 32580, 32583, 32585, 32588, 32591, 32594,
 32596, 32599, 32602, 32604, 32607, 32609, 32612, 32614,
 32616, 32619, 32621, 32623, 32626, 32628, 32630, 32632,
 32634, 32636, 32638, 32640, 32642, 32644, 32646, 32648,
 32650, 32652, 32653, 32655, 32657, 32659, 32660, 32662,
 32664, 32665, 32667, 32668, 32670, 32671, 32673, 32674,
 32676, 32677, 32679, 32680, 32681, 32683, 32684, 32685,
 32686, 32688, 32689, 32690, 32691, 32693, 32694, 32695,
 32696, 32697, 32698, 32699, 32700, 32701, 32702, 32703,
 32704, 32705, 32706, 32707, 32708, 32709, 32710, 32711,
 32712, 32713, 32713, 32714, 32715, 32716, 32717, 32718,
 32718, 32719, 32720, 32721, 32721, 32722, 32723, 32723,
 32724, 32725, 32725, 32726, 32727, 32727, 32728, 32729,
 32729, 32730, 32730, 32731, 32731, 32732, 32733, 32733,
 32734, 32734, 32735, 32735, 32736, 32736, 32737, 32737,
 32738, 32738, 32739, 32739, 32739, 32740, 32740, 32741,
 32741, 32742, 32742, 32742, 32743, 32743, 32744, 32744,
 32744, 32745, 32745, 32745, 32746, 32746, 32746, 32747,
 32747, 32747, 32748, 32748, 32748, 32749, 32749, 32749,
 32749, 32750, 32750, 32750, 32750, 32751, 32751, 32751,
 32752, 32752, 32752, 32752, 32752, 32753, 32753, 32753,
 32753, 32754, 32754, 32754, 32754, 32754, 32755, 32755,
 32755, 32755, 32755, 32756, 32756, 32756, 32756, 32756,
 32757, 32757, 32757, 32757, 32757, 32757, 32757, 32758,
 32758, 32758, 32758, 32758, 32758, 32759, 32759, 32759,
 32759, 32759, 32759, 32759, 32759, 32760, 32760, 32760,
 32760, 32760, 32760, 32760, 32760, 32761, 32761, 32761,
 32761, 32761, 32761, 32761, 32761, 32761, 32761, 32762,
 32762, 32762, 32762, 32762, 32762, 32762, 32762, 32762,
 32762, 32762, 32762, 32763, 32763, 32763, 32763, 32763,
 32763, 32763, 32763, 32763, 32763, 32763, 32763, 32763,
 32763, 32764, 32764, 32764, 32764, 32764, 32764, 32764,
 32764, 32764, 32764, 32764, 32764, 32764, 32764, 32764,
 32764, 32764, 32764, 32764, 32765, 32765, 32765, 32765,
 32765, 32765, 32765, 32765, 32765, 32765, 32765, 32765,
 32765, 32765, 32765, 32765, 32765, 32765, 32765, 32765,
 32765, 32765, 32765, 32765, 32765, 32765, 32766, 32766,
 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
 32766, 32766, 32766, 32766, 32766, 32766, 32766, 32766,
 32766, 32766, 32767, 32767, 32767, 32767, 32767, 32767
};

// stdt[i]=count of -i or i in botton or top of stretcht[]
static const U8 stdt[712]={
    64,   128,   128,   128,   128,   128,   127,   128,
   127,   128,   127,   127,   127,   127,   126,   126,
   126,   126,   126,   125,   125,   124,   125,   124,
   123,   123,   123,   123,   122,   122,   121,   121,
   120,   120,   119,   119,   118,   118,   118,   116,
   117,   115,   116,   114,   114,   113,   113,   112,
   112,   111,   110,   110,   109,   108,   108,   107,
   106,   106,   105,   104,   104,   102,   103,   101,
   101,   100,    99,    98,    98,    97,    96,    96,
    94,    94,    94,    92,    92,    91,    90,    89,
    89,    88,    87,    86,    86,    84,    84,    84,
    82,    82,    81,    80,    79,    79,    78,    77,
    76,    76,    75,    74,    73,    73,    72,    71,
    70,    70,    69,    68,    67,    67,    66,    65,
    65,    64,    63,    62,    62,    61,    61,    59,
    59,    59,    57,    58,    56,    56,    55,    54,
    54,    53,    52,    52,    51,    51,    50,    49,
    49,    48,    48,    47,    47,    45,    46,    44,
    45,    43,    43,    43,    42,    41,    41,    40,
    40,    40,    39,    38,    38,    37,    37,    36,
    36,    36,    35,    34,    34,    34,    33,    32,
    33,    32,    31,    31,    30,    31,    29,    30,
    28,    29,    28,    28,    27,    27,    27,    26,
    26,    25,    26,    24,    25,    24,    24,    23,
    23,    23,    23,    22,    22,    21,    22,    21,
    20,    21,    20,    19,    20,    19,    19,    19,
    18,    18,    18,    18,    17,    17,    17,    17,
    16,    16,    16,    16,    15,    15,    15,    15,
    15,    14,    14,    14,    14,    13,    14,    13,
    13,    13,    12,    13,    12,    12,    12,    11,
    12,    11,    11,    11,    11,    11,    10,    11,
    10,    10,    10,    10,     9,    10,     9,     9,
     9,     9,     9,     8,     9,     8,     9,     8,
     8,     8,     7,     8,     8,     7,     7,     8,
     7,     7,     7,     6,     7,     7,     6,     6,
     7,     6,     6,     6,     6,     6,     6,     5,
     6,     5,     6,     5,     5,     5,     5,     5,
     5,     5,     5,     5,     4,     5,     4,     5,
     4,     4,     5,     4,     4,     4,     4,     4,
     4,     3,     4,     4,     3,     4,     4,     3,
     3,     4,     3,     3,     3,     4,     3,     3,
     3,     3,     3,     3,     2,     3,     3,     3,
     2,     3,     2,     3,     3,     2,     2,     3,
     2,     2,     3,     2,     2,     2,     2,     3,
     2,     2,     2,     2,     2,     2,     1,     2,
     2,     2,     2,     1,     2,     2,     2,     1,
     2,     1,     2,     2,     1,     2,     1,     2,
     1,     1,     2,     1,     1,     2,     1,     1,
     2,     1,     1,     1,     1,     2,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     0,     1,     1,     1,     1,     0,
     1,     1,     1,     0,     1,     1,     1,     0,
     1,     1,     0,     1,     1,     0,     1,     0,
     1,     1,     0,     1,     0,     1,     0,     1,
     0,     1,     0,     1,     0,     1,     0,     1,
     0,     1,     0,     1,     0,     1,     0,     0,
     1,     0,     1,     0,     0,     1,     0,     1,
     0,     0,     1,     0,     0,     1,     0,     0,
     1,     0,     0,     1,     0,     0,     0,     1,
     0,     0,     1,     0,     0,     0,     1,     0,
     0,     0,     1,     0,     0,     0,     1,     0,
     0,     0,     0,     1,     0,     0,     0,     0,
     1,     0,     0,     0,     0,     1,     0,     0,
     0,     0,     0,     1,     0,     0,     0,     0,
     0,     1,     0,     0,     0,     0,     0,     0,
     1,     0,     0,     0,     0,     0,     0,     0,
     1,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     1,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     1,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     1,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     1,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     1,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     1,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     1,     0
};

Predictor::Predictor(ZPAQL& zr):
    c8(1), hmap4(1), z(zr) {
  assert(sizeof(U8)==1);
  assert(sizeof(U16)==2);
  assert(sizeof(U32)==4);
  assert(sizeof(U64)==8);
  assert(sizeof(short)==2);
  assert(sizeof(int)==4);
  pcode=0;
  pcode_size=0;
  initTables=false;
}

Predictor::~Predictor() {
  allocx(pcode, pcode_size, 0);  // free executable memory
}

// Initialize the predictor with a new model in z
void Predictor::init() {

  // Clear old JIT code if any
  allocx(pcode, pcode_size, 0);

  // Initialize context hash function
  z.inith();

  // Initialize model independent tables
  if (!initTables && isModeled()) {
    initTables=true;
    memcpy(dt2k, sdt2k, sizeof(dt2k));
    memcpy(dt, sdt, sizeof(dt));

    // ssquasht[i]=int(32768.0/(1+exp((i-2048)*(-1.0/64))));
    // Copy middle 1344 of 4096 entries.
    memset(squasht, 0, 1376*2);
    memcpy(squasht+1376, ssquasht, 1344*2);
    for (int i=2720; i<4096; ++i) squasht[i]=32767;

    // sstretcht[i]=int(log((i+0.5)/(32767.5-i))*64+0.5+100000)-100000;
    int k=16384;
    for (unsigned int i=0; i<712; ++i)
      for (int j=stdt[i]; j>0; --j)
        stretcht[k++]=i;
    assert(k==32768);
    for (unsigned int i=0; i<16384; ++i)
      stretcht[i]=-stretcht[32767-i];

#ifndef NDEBUG
    // Verify floating point math for squash() and stretch()
    U32 sqsum=0, stsum=0;
    for (int i=32767; i>=0; --i)
      stsum=stsum*3+stretch(i);
    for (int i=4095; i>=0; --i)
      sqsum=sqsum*3+squash(i-2048);
    assert(stsum==3887533746u);
    assert(sqsum==2278286169u);
#endif
  }

  // Initialize predictions
  for (unsigned int i=0; i<256; ++i) h[i]=p[i]=0;

  // Initialize components
  for (unsigned int i=0; i<256; ++i)  // clear old model
    comp[i].init();
  int n=z.header[6]; // hsize[0..1] hh hm ph pm n (comp)[n] END 0[128] (hcomp) END
  const U8* cp=&z.header[7];  // start of component list
  for (int i=0; i<n; ++i) {
    assert(cp<&z.header[z.cend]);
    assert(cp>&z.header[0] && cp<&z.header[z.header.isize()-8]);
    Component& cr=comp[i];
    switch(cp[0]) {
      case CONS:  // c
        p[i]=(cp[1]-128)*4;
        break;
      case CM: // sizebits limit
        if (cp[1]>32) error("max size for CM is 32");
        cr.cm.resize(1, cp[1]);  // packed CM (22 bits) + CMCOUNT (10 bits)
        cr.limit=cp[2]*4;
        for (size_t j=0; j<cr.cm.size(); ++j)
          cr.cm[j]=0x80000000;
        break;
      case ICM: // sizebits
        if (cp[1]>26) error("max size for ICM is 26");
        cr.limit=1023;
        cr.cm.resize(256);
        cr.ht.resize(64, cp[1]);
        for (size_t j=0; j<cr.cm.size(); ++j)
          cr.cm[j]=st.cminit(j);
        break;
      case MATCH:  // sizebits
        if (cp[1]>32 || cp[2]>32) error("max size for MATCH is 32 32");
        cr.cm.resize(1, cp[1]);  // index
        cr.ht.resize(1, cp[2]);  // buf
        cr.ht(0)=1;
        break;
      case AVG: // j k wt
        if (cp[1]>=i) error("AVG j >= i");
        if (cp[2]>=i) error("AVG k >= i");
        break;
      case MIX2:  // sizebits j k rate mask
        if (cp[1]>32) error("max size for MIX2 is 32");
        if (cp[3]>=i) error("MIX2 k >= i");
        if (cp[2]>=i) error("MIX2 j >= i");
        cr.c=(size_t(1)<<cp[1]); // size (number of contexts)
        cr.a16.resize(1, cp[1]);  // wt[size][m]
        for (size_t j=0; j<cr.a16.size(); ++j)
          cr.a16[j]=32768;
        break;
      case MIX: {  // sizebits j m rate mask
        if (cp[1]>32) error("max size for MIX is 32");
        if (cp[2]>=i) error("MIX j >= i");
        if (cp[3]<1 || cp[3]>i-cp[2]) error("MIX m not in 1..i-j");
        int m=cp[3];  // number of inputs
        assert(m>=1);
        cr.c=(size_t(1)<<cp[1]); // size (number of contexts)
        cr.cm.resize(m, cp[1]);  // wt[size][m]
        for (size_t j=0; j<cr.cm.size(); ++j)
          cr.cm[j]=65536/m;
        break;
      }
      case ISSE:  // sizebits j
        if (cp[1]>32) error("max size for ISSE is 32");
        if (cp[2]>=i) error("ISSE j >= i");
        cr.ht.resize(64, cp[1]);
        cr.cm.resize(512);
        for (int j=0; j<256; ++j) {
          cr.cm[j*2]=1<<15;
          cr.cm[j*2+1]=clamp512k(stretch(st.cminit(j)>>8)*1024);
        }
        break;
      case SSE: // sizebits j start limit
        if (cp[1]>32) error("max size for SSE is 32");
        if (cp[2]>=i) error("SSE j >= i");
        if (cp[3]>cp[4]*4) error("SSE start > limit*4");
        cr.cm.resize(32, cp[1]);
        cr.limit=cp[4]*4;
        for (size_t j=0; j<cr.cm.size(); ++j)
          cr.cm[j]=squash((j&31)*64-992)<<17|cp[3];
        break;
      default: error("unknown component type");
    }
    assert(compsize[*cp]>0);
    cp+=compsize[*cp];
    assert(cp>=&z.header[7] && cp<&z.header[z.cend]);
  }
}

// Return next bit prediction using interpreted COMP code
int Predictor::predict0() {
  assert(initTables);
  assert(c8>=1 && c8<=255);

  // Predict next bit
  int n=z.header[6];
  assert(n>0 && n<=255);
  const U8* cp=&z.header[7];
  assert(cp[-1]==n);
  for (int i=0; i<n; ++i) {
    assert(cp>&z.header[0] && cp<&z.header[z.header.isize()-8]);
    Component& cr=comp[i];
    switch(cp[0]) {
      case CONS:  // c
        break;
      case CM:  // sizebits limit
        cr.cxt=h[i]^hmap4;
        p[i]=stretch(cr.cm(cr.cxt)>>17);
        break;
      case ICM: // sizebits
        assert((hmap4&15)>0);
        if (c8==1 || (c8&0xf0)==16) cr.c=find(cr.ht, cp[1]+2, h[i]+16*c8);
        cr.cxt=cr.ht[cr.c+(hmap4&15)];
        p[i]=stretch(cr.cm(cr.cxt)>>8);
        break;
      case MATCH: // sizebits bufbits: a=len, b=offset, c=bit, cxt=bitpos,
                  //                   ht=buf, limit=pos
        assert(cr.cm.size()==(size_t(1)<<cp[1]));
        assert(cr.ht.size()==(size_t(1)<<cp[2]));
        assert(cr.a<=255);
        assert(cr.c==0 || cr.c==1);
        assert(cr.cxt<8);
        assert(cr.limit<cr.ht.size());
        if (cr.a==0) p[i]=0;
        else {
          cr.c=(cr.ht(cr.limit-cr.b)>>(7-cr.cxt))&1; // predicted bit
          p[i]=stretch(dt2k[cr.a]*(cr.c*-2+1)&32767);
        }
        break;
      case AVG: // j k wt
        p[i]=(p[cp[1]]*cp[3]+p[cp[2]]*(256-cp[3]))>>8;
        break;
      case MIX2: { // sizebits j k rate mask
                   // c=size cm=wt[size] cxt=input
        cr.cxt=((h[i]+(c8&cp[5]))&(cr.c-1));
        assert(cr.cxt<cr.a16.size());
        int w=cr.a16[cr.cxt];
        assert(w>=0 && w<65536);
        p[i]=(w*p[cp[2]]+(65536-w)*p[cp[3]])>>16;
        assert(p[i]>=-2048 && p[i]<2048);
      }
        break;
      case MIX: {  // sizebits j m rate mask
                   // c=size cm=wt[size][m] cxt=index of wt in cm
        int m=cp[3];
        assert(m>=1 && m<=i);
        cr.cxt=h[i]+(c8&cp[5]);
        cr.cxt=(cr.cxt&(cr.c-1))*m; // pointer to row of weights
        assert(cr.cxt<=cr.cm.size()-m);
        int* wt=(int*)&cr.cm[cr.cxt];
        p[i]=0;
        for (int j=0; j<m; ++j)
          p[i]+=(wt[j]>>8)*p[cp[2]+j];
        p[i]=clamp2k(p[i]>>8);
      }
        break;
      case ISSE: { // sizebits j -- c=hi, cxt=bh
        assert((hmap4&15)>0);
        if (c8==1 || (c8&0xf0)==16)
          cr.c=find(cr.ht, cp[1]+2, h[i]+16*c8);
        cr.cxt=cr.ht[cr.c+(hmap4&15)];  // bit history
        int *wt=(int*)&cr.cm[cr.cxt*2];
        p[i]=clamp2k((wt[0]*p[cp[2]]+wt[1]*64)>>16);
      }
        break;
      case SSE: { // sizebits j start limit
        cr.cxt=(h[i]+c8)*32;
        int pq=p[cp[2]]+992;
        if (pq<0) pq=0;
        if (pq>1983) pq=1983;
        int wt=pq&63;
        pq>>=6;
        assert(pq>=0 && pq<=30);
        cr.cxt+=pq;
        p[i]=stretch(((cr.cm(cr.cxt)>>10)*(64-wt)+(cr.cm(cr.cxt+1)>>10)*wt)>>13);
        cr.cxt+=wt>>5;
      }
        break;
      default:
        error("component predict not implemented");
    }
    cp+=compsize[cp[0]];
    assert(cp<&z.header[z.cend]);
    assert(p[i]>=-2048 && p[i]<2048);
  }
  assert(cp[0]==NONE);
  return squash(p[n-1]);
}

// Update model with decoded bit y (0...1)
void Predictor::update0(int y) {
  assert(initTables);
  assert(y==0 || y==1);
  assert(c8>=1 && c8<=255);
  assert(hmap4>=1 && hmap4<=511);

  // Update components
  const U8* cp=&z.header[7];
  int n=z.header[6];
  assert(n>=1 && n<=255);
  assert(cp[-1]==n);
  for (int i=0; i<n; ++i) {
    Component& cr=comp[i];
    switch(cp[0]) {
      case CONS:  // c
        break;
      case CM:  // sizebits limit
        train(cr, y);
        break;
      case ICM: { // sizebits: cxt=ht[b]=bh, ht[c][0..15]=bh row, cxt=bh
        cr.ht[cr.c+(hmap4&15)]=st.next(cr.ht[cr.c+(hmap4&15)], y);
        U32& pn=cr.cm(cr.cxt);
        pn+=int(y*32767-(pn>>8))>>2;
      }
        break;
      case MATCH: // sizebits bufbits:
                  //   a=len, b=offset, c=bit, cm=index, cxt=bitpos
                  //   ht=buf, limit=pos
      {
        assert(cr.a<=255);
        assert(cr.c==0 || cr.c==1);
        assert(cr.cxt<8);
        assert(cr.cm.size()==(size_t(1)<<cp[1]));
        assert(cr.ht.size()==(size_t(1)<<cp[2]));
        assert(cr.limit<cr.ht.size());
        if (int(cr.c)!=y) cr.a=0;  // mismatch?
        cr.ht(cr.limit)+=cr.ht(cr.limit)+y;
        if (++cr.cxt==8) {
          cr.cxt=0;
          ++cr.limit;
          cr.limit&=(1<<cp[2])-1;
          if (cr.a==0) {  // look for a match
            cr.b=cr.limit-cr.cm(h[i]);
            if (cr.b&(cr.ht.size()-1))
              while (cr.a<255
                     && cr.ht(cr.limit-cr.a-1)==cr.ht(cr.limit-cr.a-cr.b-1))
                ++cr.a;
          }
          else cr.a+=cr.a<255;
          cr.cm(h[i])=cr.limit;
        }
      }
        break;
      case AVG:  // j k wt
        break;
      case MIX2: { // sizebits j k rate mask
                   // cm=wt[size], cxt=input
        assert(cr.a16.size()==cr.c);
        assert(cr.cxt<cr.a16.size());
        int err=(y*32767-squash(p[i]))*cp[4]>>5;
        int w=cr.a16[cr.cxt];
        w+=(err*(p[cp[2]]-p[cp[3]])+(1<<12))>>13;
        if (w<0) w=0;
        if (w>65535) w=65535;
        cr.a16[cr.cxt]=w;
      }
        break;
      case MIX: {   // sizebits j m rate mask
                    // cm=wt[size][m], cxt=input
        int m=cp[3];
        assert(m>0 && m<=i);
        assert(cr.cm.size()==m*cr.c);
        assert(cr.cxt+m<=cr.cm.size());
        int err=(y*32767-squash(p[i]))*cp[4]>>4;
        int* wt=(int*)&cr.cm[cr.cxt];
        for (int j=0; j<m; ++j)
          wt[j]=clamp512k(wt[j]+((err*p[cp[2]+j]+(1<<12))>>13));
      }
        break;
      case ISSE: { // sizebits j  -- c=hi, cxt=bh
        assert(cr.cxt==cr.ht[cr.c+(hmap4&15)]);
        int err=y*32767-squash(p[i]);
        int *wt=(int*)&cr.cm[cr.cxt*2];
        wt[0]=clamp512k(wt[0]+((err*p[cp[2]]+(1<<12))>>13));
        wt[1]=clamp512k(wt[1]+((err+16)>>5));
        cr.ht[cr.c+(hmap4&15)]=st.next(cr.cxt, y);
      }
        break;
      case SSE:  // sizebits j start limit
        train(cr, y);
        break;
      default:
        assert(0);
    }
    cp+=compsize[cp[0]];
    assert(cp>=&z.header[7] && cp<&z.header[z.cend] 
           && cp<&z.header[z.header.isize()-8]);
  }
  assert(cp[0]==NONE);

  // Save bit y in c8, hmap4
  c8+=c8+y;
  if (c8>=256) {
    z.run(c8-256);
    hmap4=1;
    c8=1;
    for (int i=0; i<n; ++i) h[i]=z.H(i);
  }
  else if (c8>=16 && c8<32)
    hmap4=(hmap4&0xf)<<5|y<<4|1;
  else
    hmap4=(hmap4&0x1f0)|(((hmap4&0xf)*2+y)&0xf);
}

// Find cxt row in hash table ht. ht has rows of 16 indexed by the
// low sizebits of cxt with element 0 having the next higher 8 bits for
// collision detection. If not found after 3 adjacent tries, replace the
// row with lowest element 1 as priority. Return index of row.
size_t Predictor::find(Array<U8>& ht, int sizebits, U32 cxt) {
  assert(initTables);
  assert(ht.size()==size_t(16)<<sizebits);
  int chk=cxt>>sizebits&255;
  size_t h0=(cxt*16)&(ht.size()-16);
  if (ht[h0]==chk) return h0;
  size_t h1=h0^16;
  if (ht[h1]==chk) return h1;
  size_t h2=h0^32;
  if (ht[h2]==chk) return h2;
  if (ht[h0+1]<=ht[h1+1] && ht[h0+1]<=ht[h2+1])
    return memset(&ht[h0], 0, 16), ht[h0]=chk, h0;
  else if (ht[h1+1]<ht[h2+1])
    return memset(&ht[h1], 0, 16), ht[h1]=chk, h1;
  else
    return memset(&ht[h2], 0, 16), ht[h2]=chk, h2;
}

/////////////////////// Decoder ///////////////////////

Decoder::Decoder(ZPAQL& z):
    in(0), low(1), high(0xFFFFFFFF), curr(0), rpos(0), wpos(0),
    pr(z), buf(BUFSIZE) {
}

void Decoder::init() {
  pr.init();
  if (pr.isModeled()) low=1, high=0xFFFFFFFF, curr=0;
  else low=high=curr=0;
}

// Return next bit of decoded input, which has 16 bit probability p of being 1
int Decoder::decode(int p) {
  assert(pr.isModeled());
  assert(p>=0 && p<65536);
  assert(high>low && low>0);
  if (curr<low || curr>high) error("archive corrupted");
  assert(curr>=low && curr<=high);
  U32 mid=low+U32(((high-low)*U64(U32(p)))>>16);  // split range
  assert(high>mid && mid>=low);
  int y;
  if (curr<=mid) y=1, high=mid;  // pick half
  else y=0, low=mid+1;
  while ((high^low)<0x1000000) { // shift out identical leading bytes
    high=high<<8|255;
    low=low<<8;
    low+=(low==0);
    int c=get();
    if (c<0) error("unexpected end of file");
    curr=curr<<8|c;
  }
  return y;
}

// Decompress 1 byte or -1 at end of input
int Decoder::decompress() {
  if (pr.isModeled()) {  // n>0 components?
    if (curr==0) {  // segment initialization
      for (int i=0; i<4; ++i)
        curr=curr<<8|get();
    }
    if (decode(0)) {
      if (curr!=0) error("decoding end of stream");
      return -1;
    }
    else {
      int c=1;
      while (c<256) {  // get 8 bits
        int p=pr.predict()*2+1;
        c+=c+decode(p);
        pr.update(c&1);
      }
      return c-256;
    }
  }
  else {
    if (curr==0) {
      for (int i=0; i<4; ++i) curr=curr<<8|get();
      if (curr==0) return -1;
    }
    --curr;
    return get();
  }
}

// Find end of compressed data and return next byte
int Decoder::skip() {
  int c=-1;
  if (pr.isModeled()) {
    while (curr==0)  // at start?
      curr=get();
    while (curr && (c=get())>=0)  // find 4 zeros
      curr=curr<<8|c;
    while ((c=get())==0) ;  // might be more than 4
    return c;
  }
  else {
    if (curr==0)  // at start?
      for (int i=0; i<4 && (c=get())>=0; ++i) curr=curr<<8|c;
    while (curr>0) {
      while (curr>0) {
        --curr;
        if (get()<0) return error("skipped to EOF"), -1;
      }
      for (int i=0; i<4 && (c=get())>=0; ++i) curr=curr<<8|c;
    }
    if (c>=0) c=get();
    return c;
  }
}

////////////////////// PostProcessor //////////////////////

// Copy ph, pm from block header
void PostProcessor::init(int h, int m) {
  state=hsize=0;
  ph=h;
  pm=m;
  z.clear();
}

// (PASS=0 | PROG=1 psize[0..1] pcomp[0..psize-1]) data... EOB=-1
// Return state: 1=PASS, 2..4=loading PROG, 5=PROG loaded
int PostProcessor::write(int c) {
  assert(c>=-1 && c<=255);
  switch (state) {
    case 0:  // initial state
      if (c<0) error("Unexpected EOS");
      state=c+1;  // 1=PASS, 2=PROG
      if (state>2) error("unknown post processing type");
      if (state==1) z.clear();
      break;
    case 1:  // PASS
      z.outc(c);
      break;
    case 2: // PROG
      if (c<0) error("Unexpected EOS");
      hsize=c;  // low byte of size
      state=3;
      break;
    case 3:  // PROG psize[0]
      if (c<0) error("Unexpected EOS");
      hsize+=c*256;  // high byte of psize
      if (hsize<1) error("Empty PCOMP");
      z.header.resize(hsize+300);
      z.cend=8;
      z.hbegin=z.hend=z.cend+128;
      z.header[4]=ph;
      z.header[5]=pm;
      state=4;
      break;
    case 4:  // PROG psize[0..1] pcomp[0...]
      if (c<0) error("Unexpected EOS");
      assert(z.hend<z.header.isize());
      z.header[z.hend++]=c;  // one byte of pcomp
      if (z.hend-z.hbegin==hsize) {  // last byte of pcomp?
        hsize=z.cend-2+z.hend-z.hbegin;
        z.header[0]=hsize&255;  // header size with empty COMP
        z.header[1]=hsize>>8;
        z.initp();
        state=5;
      }
      break;
    case 5:  // PROG ... data
      z.run(c);
      if (c<0) z.flush();
      break;
  }
  return state;
}

/////////////////////// Decompresser /////////////////////

// Find the start of a block and return true if found. Set memptr
// to memory used.
bool Decompresser::findBlock(double* memptr) {
  assert(state==BLOCK);

  // Find start of block
  U32 h1=0x3D49B113, h2=0x29EB7F93, h3=0x2614BE13, h4=0x3828EB13;
  // Rolling hashes initialized to hash of first 13 bytes
  int c;
  while ((c=dec.get())!=-1) {
    h1=h1*12+c;
    h2=h2*20+c;
    h3=h3*28+c;
    h4=h4*44+c;
    if (h1==0xB16B88F1 && h2==0xFF5376F1 && h3==0x72AC5BF1 && h4==0x2F909AF1)
      break;  // hash of 16 byte string
  }
  if (c==-1) return false;

  // Read header
  if ((c=dec.get())!=1 && c!=2) error("unsupported ZPAQ level");
  if (dec.get()!=1) error("unsupported ZPAQL type");
  z.read(&dec);
  if (c==1 && z.header.isize()>6 && z.header[6]==0)
    error("ZPAQ level 1 requires at least 1 component");
  if (memptr) *memptr=z.memory();
  state=FILENAME;
  decode_state=FIRSTSEG;
  return true;
}

// Read the start of a segment (1) or end of block code (255).
// If a segment is found, write the filename and return true, else false.
bool Decompresser::findFilename(Writer* filename) {
  assert(state==FILENAME);
  int c=dec.get();
  if (c==1) {  // segment found
    while (true) {
      c=dec.get();
      if (c==-1) error("unexpected EOF");
      if (c==0) {
        state=COMMENT;
        return true;
      }
      if (filename) filename->put(c);
    }
  }
  else if (c==255) {  // end of block found
    state=BLOCK;
    return false;
  }
  else
    error("missing segment or end of block");
  return false;
}

// Read the comment from the segment header
void Decompresser::readComment(Writer* comment) {
  assert(state==COMMENT);
  state=DATA;
  while (true) {
    int c=dec.get();
    if (c==-1) error("unexpected EOF");
    if (c==0) break;
    if (comment) comment->put(c);
  }
  if (dec.get()!=0) error("missing reserved byte");
}

// Decompress n bytes, or all if n < 0. Return false if done
bool Decompresser::decompress(int n) {
  assert(state==DATA);
  if (decode_state==SKIP) error("decompression after skipped segment");
  assert(decode_state!=SKIP);

  // Initialize models to start decompressing block
  if (decode_state==FIRSTSEG) {
    dec.init();
    assert(z.header.size()>5);
    pp.init(z.header[4], z.header[5]);
    decode_state=SEG;
  }

  // Decompress and load PCOMP into postprocessor
  while ((pp.getState()&3)!=1)
    pp.write(dec.decompress());

  // Decompress n bytes, or all if n < 0
  while (n) {
    int c=dec.decompress();
    pp.write(c);
    if (c==-1) {
      state=SEGEND;
      return false;
    }
    if (n>0) --n;
  }
  return true;
}

// Read end of block. If a SHA1 checksum is present, write 1 and the
// 20 byte checksum into sha1string, else write 0 in first byte.
// If sha1string is 0 then discard it.
void Decompresser::readSegmentEnd(char* sha1string) {
  assert(state==DATA || state==SEGEND);

  // Skip remaining data if any and get next byte
  int c=0;
  if (state==DATA) {
    c=dec.skip();
    decode_state=SKIP;
  }
  else if (state==SEGEND)
    c=dec.get();
  state=FILENAME;

  // Read checksum
  if (c==254) {
    if (sha1string) sha1string[0]=0;  // no checksum
  }
  else if (c==253) {
    if (sha1string) sha1string[0]=1;
    for (int i=1; i<=20; ++i) {
      c=dec.get();
      if (sha1string) sha1string[i]=c;
    }
  }
  else
    error("missing end of segment marker");
}

/////////////////////////// decompress() //////////////////////

void decompress(Reader* in, Writer* out) {
  Decompresser d;
  d.setInput(in);
  d.setOutput(out);
  while (d.findBlock()) {       // don't calculate memory
    while (d.findFilename()) {  // discard filename
      d.readComment();          // discard comment
      d.decompress();           // to end of segment
      d.readSegmentEnd();       // discard sha1string
    }
  }
}

/////////////////////////// Encoder ///////////////////////////

// Initialize for start of block
void Encoder::init() {
  low=1;
  high=0xFFFFFFFF;
  pr.init();
  if (!pr.isModeled()) low=0, buf.resize(1<<16);
}

// compress bit y having probability p/64K
void Encoder::encode(int y, int p) {
  assert(out);
  assert(p>=0 && p<65536);
  assert(y==0 || y==1);
  assert(high>low && low>0);
  U32 mid=low+U32(((high-low)*U64(U32(p)))>>16);  // split range
  assert(high>mid && mid>=low);
  if (y) high=mid; else low=mid+1; // pick half
  while ((high^low)<0x1000000) { // write identical leading bytes
    out->put(high>>24);  // same as low>>24
    high=high<<8|255;
    low=low<<8;
    low+=(low==0); // so we don't code 4 0 bytes in a row
  }
}

// compress byte c (0..255 or -1=EOS)
void Encoder::compress(int c) {
  assert(out);
  if (pr.isModeled()) {
    if (c==-1)
      encode(1, 0);
    else {
      assert(c>=0 && c<=255);
      encode(0, 0);
      for (int i=7; i>=0; --i) {
        int p=pr.predict()*2+1;
        assert(p>0 && p<65536);
        int y=c>>i&1;
        encode(y, p);
        pr.update(y);
      }
    }
  }
  else {
    if (low && (c<0 || low==buf.size())) {
      out->put((low>>24)&255);
      out->put((low>>16)&255);
      out->put((low>>8)&255);
      out->put(low&255);
      out->write(&buf[0], low);
      low=0;
    }
    if (c>=0) buf[low++]=c;
  }
}

//////////////////////////// Compiler /////////////////////////

// Component names
const char* compname[256]=
  {"","const","cm","icm","match","avg","mix2","mix","isse","sse",0};

// Opcodes
const char* opcodelist[272]={
"error","a++",  "a--",  "a!",   "a=0",  "",     "",     "a=r",
"b<>a", "b++",  "b--",  "b!",   "b=0",  "",     "",     "b=r",
"c<>a", "c++",  "c--",  "c!",   "c=0",  "",     "",     "c=r",
"d<>a", "d++",  "d--",  "d!",   "d=0",  "",     "",     "d=r",
"*b<>a","*b++", "*b--", "*b!",  "*b=0", "",     "",     "jt",
"*c<>a","*c++", "*c--", "*c!",  "*c=0", "",     "",     "jf",
"*d<>a","*d++", "*d--", "*d!",  "*d=0", "",     "",     "r=a",
"halt", "out",  "",     "hash", "hashd","",     "",     "jmp",
"a=a",  "a=b",  "a=c",  "a=d",  "a=*b", "a=*c", "a=*d", "a=",
"b=a",  "b=b",  "b=c",  "b=d",  "b=*b", "b=*c", "b=*d", "b=",
"c=a",  "c=b",  "c=c",  "c=d",  "c=*b", "c=*c", "c=*d", "c=",
"d=a",  "d=b",  "d=c",  "d=d",  "d=*b", "d=*c", "d=*d", "d=",
"*b=a", "*b=b", "*b=c", "*b=d", "*b=*b","*b=*c","*b=*d","*b=",
"*c=a", "*c=b", "*c=c", "*c=d", "*c=*b","*c=*c","*c=*d","*c=",
"*d=a", "*d=b", "*d=c", "*d=d", "*d=*b","*d=*c","*d=*d","*d=",
"",     "",     "",     "",     "",     "",     "",     "",
"a+=a", "a+=b", "a+=c", "a+=d", "a+=*b","a+=*c","a+=*d","a+=",
"a-=a", "a-=b", "a-=c", "a-=d", "a-=*b","a-=*c","a-=*d","a-=",
"a*=a", "a*=b", "a*=c", "a*=d", "a*=*b","a*=*c","a*=*d","a*=",
"a/=a", "a/=b", "a/=c", "a/=d", "a/=*b","a/=*c","a/=*d","a/=",
"a%=a", "a%=b", "a%=c", "a%=d", "a%=*b","a%=*c","a%=*d","a%=",
"a&=a", "a&=b", "a&=c", "a&=d", "a&=*b","a&=*c","a&=*d","a&=",
"a&~a", "a&~b", "a&~c", "a&~d", "a&~*b","a&~*c","a&~*d","a&~",
"a|=a", "a|=b", "a|=c", "a|=d", "a|=*b","a|=*c","a|=*d","a|=",
"a^=a", "a^=b", "a^=c", "a^=d", "a^=*b","a^=*c","a^=*d","a^=",
"a<<=a","a<<=b","a<<=c","a<<=d","a<<=*b","a<<=*c","a<<=*d","a<<=",
"a>>=a","a>>=b","a>>=c","a>>=d","a>>=*b","a>>=*c","a>>=*d","a>>=",
"a==a", "a==b", "a==c", "a==d", "a==*b","a==*c","a==*d","a==",
"a<a",  "a<b",  "a<c",  "a<d",  "a<*b", "a<*c", "a<*d", "a<",
"a>a",  "a>b",  "a>c",  "a>d",  "a>*b", "a>*c", "a>*d", "a>",
"",     "",     "",     "",     "",     "",     "",     "",
"",     "",     "",     "",     "",     "",     "",     "lj",
"post", "pcomp","end",  "if",   "ifnot","else", "endif","do",
"while","until","forever","ifl","ifnotl","elsel",";",    0};

// Advance in to start of next token. Tokens are delimited by white
// space. Comments inclosed in ((nested) parenthsis) are skipped.
void Compiler::next() {
  assert(in);
  for (; *in; ++in) {
    if (*in=='\n') ++line;
    if (*in=='(') state+=1+(state<0);
    else if (state>0 && *in==')') --state;
    else if (state<0 && *in<=' ') state=0;
    else if (state==0 && *in>' ') {state=-1; break;}
  }
  if (!*in) error("unexpected end of config");
}

// convert to lower case
int tolower(int c) {return (c>='A' && c<='Z') ? c+'a'-'A' : c;}

// return true if in==word up to white space or '(', case insensitive
bool Compiler::matchToken(const char* word) {
  const char* a=in;
  for (; (*a>' ' && *a!='(' && *word); ++a, ++word)
    if (tolower(*a)!=tolower(*word)) return false;
  return !*word && (*a<=' ' || *a=='(');
}

// Print error message and exit
void Compiler::syntaxError(const char* msg, const char* expected) {
  Array<char> sbuf(128);  // error message to report
  char* s=&sbuf[0];
  strcat(s, "Config line ");
  for (int i=strlen(s), r=1000000; r; r/=10)  // append line number
    if (line/r) s[i++]='0'+line/r%10;
  strcat(s, " at ");
  for (int i=strlen(s); i<40 && *in>' '; ++i)  // append token found
    s[i]=*in++;
  strcat(s, ": ");
  strncat(s, msg, 40);  // append message
  if (expected) {
    strcat(s, ", expected: ");
    strncat(s, expected, 20);  // append expected token if any
  }
  error(s);
}

// Read a token, which must be in the NULL terminated list or else
// exit with an error. If found, return its index.
int Compiler::rtoken(const char* list[]) {
  assert(in);
  assert(list);
  next();
  for (int i=0; list[i]; ++i)
    if (matchToken(list[i]))
      return i;
  syntaxError("unexpected");
  assert(0);
  return -1; // not reached
}

// Read a token which must be the specified value s
void Compiler::rtoken(const char* s) {
  assert(s);
  next();
  if (!matchToken(s)) syntaxError("expected", s);
}

// Read a number in (low...high) or exit with an error
// For numbers like $N+M, return arg[N-1]+M
int Compiler::rtoken(int low, int high) {
  next();
  int r=0;
  if (in[0]=='$' && in[1]>='1' && in[1]<='9') {
    if (in[2]=='+') r=atoi(in+3);
    if (args) r+=args[in[1]-'1'];
  }
  else if (in[0]=='-' || (in[0]>='0' && in[0]<='9')) r=atoi(in);
  else syntaxError("expected a number");
  if (r<low) syntaxError("number too low");
  if (r>high) syntaxError("number too high");
  return r;
}

// Compile HCOMP or PCOMP code. Exit on error. Return
// code for end token (POST, PCOMP, END)
int Compiler::compile_comp(ZPAQL& z) {
  int op=0;
  const int comp_begin=z.hend;
  while (true) {
    op=rtoken(opcodelist);
    if (op==POST || op==PCOMP || op==END) break;
    int operand=-1; // 0...255 if 2 bytes
    int operand2=-1;  // 0...255 if 3 bytes
    if (op==IF) {
      op=JF;
      operand=0; // set later
      if_stack.push(z.hend+1); // save jump target location
    }
    else if (op==IFNOT) {
      op=JT;
      operand=0;
      if_stack.push(z.hend+1); // save jump target location
    }
    else if (op==IFL || op==IFNOTL) {  // long if
      if (op==IFL) z.header[z.hend++]=(JT);
      if (op==IFNOTL) z.header[z.hend++]=(JF);
      z.header[z.hend++]=(3);
      op=LJ;
      operand=operand2=0;
      if_stack.push(z.hend+1);
    }
    else if (op==ELSE || op==ELSEL) {
      if (op==ELSE) op=JMP, operand=0;
      if (op==ELSEL) op=LJ, operand=operand2=0;
      int a=if_stack.pop();  // conditional jump target location
      assert(a>comp_begin && a<int(z.hend));
      if (z.header[a-1]!=LJ) {  // IF, IFNOT
        assert(z.header[a-1]==JT || z.header[a-1]==JF || z.header[a-1]==JMP);
        int j=z.hend-a+1+(op==LJ); // offset at IF
        assert(j>=0);
        if (j>127) syntaxError("IF too big, try IFL, IFNOTL");
        z.header[a]=j;
      }
      else {  // IFL, IFNOTL
        int j=z.hend-comp_begin+2+(op==LJ);
        assert(j>=0);
        z.header[a]=j&255;
        z.header[a+1]=(j>>8)&255;
      }
      if_stack.push(z.hend+1);  // save JMP target location
    }
    else if (op==ENDIF) {
      int a=if_stack.pop();  // jump target address
      assert(a>comp_begin && a<int(z.hend));
      int j=z.hend-a-1;  // jump offset
      assert(j>=0);
      if (z.header[a-1]!=LJ) {
        assert(z.header[a-1]==JT || z.header[a-1]==JF || z.header[a-1]==JMP);
        if (j>127) syntaxError("IF too big, try IFL, IFNOTL, ELSEL\n");
        z.header[a]=j;
      }
      else {
        assert(a+1<int(z.hend));
        j=z.hend-comp_begin;
        z.header[a]=j&255;
        z.header[a+1]=(j>>8)&255;
      }
    }
    else if (op==DO) {
      do_stack.push(z.hend);
    }
    else if (op==WHILE || op==UNTIL || op==FOREVER) {
      int a=do_stack.pop();
      assert(a>=comp_begin && a<int(z.hend));
      int j=a-z.hend-2;
      assert(j<=-2);
      if (j>=-127) {  // backward short jump
        if (op==WHILE) op=JT;
        if (op==UNTIL) op=JF;
        if (op==FOREVER) op=JMP;
        operand=j&255;
      }
      else {  // backward long jump
        j=a-comp_begin;
        assert(j>=0 && j<int(z.hend)-comp_begin);
        if (op==WHILE) {
          z.header[z.hend++]=(JF);
          z.header[z.hend++]=(3);
        }
        if (op==UNTIL) {
          z.header[z.hend++]=(JT);
          z.header[z.hend++]=(3);
        }
        op=LJ;
        operand=j&255;
        operand2=j>>8;
      }
    }
    else if ((op&7)==7) { // 2 byte operand, read N
      if (op==LJ) {
        operand=rtoken(0, 65535);
        operand2=operand>>8;
        operand&=255;
      }
      else if (op==JT || op==JF || op==JMP) {
        operand=rtoken(-128, 127);
        operand&=255;
      }
      else
        operand=rtoken(0, 255);
    }
    if (op>=0 && op<=255)
      z.header[z.hend++]=(op);
    if (operand>=0)
      z.header[z.hend++]=(operand);
    if (operand2>=0)
      z.header[z.hend++]=(operand2);
    if (z.hend>=z.header.isize()-130 || z.hend-z.hbegin+z.cend-2>65535)
      syntaxError("program too big");
  }
  z.header[z.hend++]=(0); // END
  return op;
}

// Compile a configuration file. Store COMP/HCOMP section in hcomp.
// If there is a PCOMP section, store it in pcomp and store the PCOMP
// command in pcomp_cmd. Replace "$1..$9+n" with args[0..8]+n

Compiler::Compiler(const char* in_, int* args_, ZPAQL& hz_, ZPAQL& pz_,
                   Writer* out2_): in(in_), args(args_), hz(hz_), pz(pz_),
                   out2(out2_), if_stack(1000), do_stack(1000) {
  line=1;
  state=0;
  hz.clear();
  pz.clear();
  hz.header.resize(68000); 

  // Compile the COMP section of header
  rtoken("comp");
  hz.header[2]=rtoken(0, 255);  // hh
  hz.header[3]=rtoken(0, 255);  // hm
  hz.header[4]=rtoken(0, 255);  // ph
  hz.header[5]=rtoken(0, 255);  // pm
  const int n=hz.header[6]=rtoken(0, 255);  // n
  hz.cend=7;
  for (int i=0; i<n; ++i) {
    rtoken(i, i);
    CompType type=CompType(rtoken(compname));
    hz.header[hz.cend++]=type;
    int clen=libzpaq::compsize[type&255];
    if (clen<1 || clen>10) syntaxError("invalid component");
    for (int j=1; j<clen; ++j)
      hz.header[hz.cend++]=rtoken(0, 255);  // component arguments
  }
  hz.header[hz.cend++];  // end
  hz.hbegin=hz.hend=hz.cend+128;

  // Compile HCOMP
  rtoken("hcomp");
  int op=compile_comp(hz);

  // Compute header size
  int hsize=hz.cend-2+hz.hend-hz.hbegin;
  hz.header[0]=hsize&255;
  hz.header[1]=hsize>>8;

  // Compile POST 0 END
  if (op==POST) {
    rtoken(0, 0);
    rtoken("end");
  }

  // Compile PCOMP pcomp_cmd ; program... END
  else if (op==PCOMP) {
    pz.header.resize(68000);
    pz.header[4]=hz.header[4];  // ph
    pz.header[5]=hz.header[5];  // pm
    pz.cend=8;
    pz.hbegin=pz.hend=pz.cend+128;

    // get pcomp_cmd ending with ";" (case sensitive)
    next();
    while (*in && *in!=';') {
      if (out2)
        out2->put(*in);
      ++in;
    }
    if (*in) ++in;

    // Compile PCOMP
    op=compile_comp(pz);
    int len=pz.cend-2+pz.hend-pz.hbegin;  // insert header size
    assert(len>=0);
    pz.header[0]=len&255;
    pz.header[1]=len>>8;
    if (op!=END)
      syntaxError("expected END");
  }
  else if (op!=END)
    syntaxError("expected END or POST 0 END or PCOMP cmd ; ... END");
}

///////////////////// Compressor //////////////////////

// Write 13 byte start tag
// "\x37\x6B\x53\x74\xA0\x31\x83\xD3\x8C\xB2\x28\xB0\xD3"
void Compressor::writeTag() {
  assert(state==INIT);
  enc.out->put(0x37);
  enc.out->put(0x6b);
  enc.out->put(0x53);
  enc.out->put(0x74);
  enc.out->put(0xa0);
  enc.out->put(0x31);
  enc.out->put(0x83);
  enc.out->put(0xd3);
  enc.out->put(0x8c);
  enc.out->put(0xb2);
  enc.out->put(0x28);
  enc.out->put(0xb0);
  enc.out->put(0xd3);
}

void Compressor::startBlock(int level) {

  // Model 1 - min.cfg
  static const char models[]={
  26,0,1,2,0,0,2,3,16,8,19,0,0,96,4,28,
  59,10,59,112,25,10,59,10,59,112,56,0,

  // Model 2 - mid.cfg
  69,0,3,3,0,0,8,3,5,8,13,0,8,17,1,8,
  18,2,8,18,3,8,19,4,4,22,24,7,16,0,7,24,
  (char)-1,0,17,104,74,4,95,1,59,112,10,25,59,112,10,25,
  59,112,10,25,59,112,10,25,59,112,10,25,59,10,59,112,
  25,69,(char)-49,8,112,56,0,

  // Model 3 - max.cfg
  (char)-60,0,5,9,0,0,22,1,(char)-96,3,5,8,13,1,8,16,
  2,8,18,3,8,19,4,8,19,5,8,20,6,4,22,24,
  3,17,8,19,9,3,13,3,13,3,13,3,14,7,16,0,
  15,24,(char)-1,7,8,0,16,10,(char)-1,6,0,15,16,24,0,9,
  8,17,32,(char)-1,6,8,17,18,16,(char)-1,9,16,19,32,(char)-1,6,
  0,19,20,16,0,0,17,104,74,4,95,2,59,112,10,25,
  59,112,10,25,59,112,10,25,59,112,10,25,59,112,10,25,
  59,10,59,112,10,25,59,112,10,25,69,(char)-73,32,(char)-17,64,47,
  14,(char)-25,91,47,10,25,60,26,48,(char)-122,(char)-105,20,112,63,9,70,
  (char)-33,0,39,3,25,112,26,52,25,25,74,10,4,59,112,25,
  10,4,59,112,25,10,4,59,112,25,65,(char)-113,(char)-44,72,4,59,
  112,8,(char)-113,(char)-40,8,68,(char)-81,60,60,25,69,(char)-49,9,112,25,25,
  25,25,25,112,56,0,

  0,0}; // 0,0 = end of list

  if (level<1) error("compression level must be at least 1");
  const char* p=models;
  int i;
  for (i=1; i<level && toU16(p); ++i)
    p+=toU16(p)+2;
  if (toU16(p)<1) error("compression level too high");
  startBlock(p);
}

// Memory reader
class MemoryReader: public Reader {
  const char* p;
public:
  MemoryReader(const char* p_): p(p_) {}
  int get() {return *p++&255;}
};

void Compressor::startBlock(const char* hcomp) {
  assert(state==INIT);
  MemoryReader m(hcomp);
  z.read(&m);
  pz.sha1=&sha1;
  assert(z.header.isize()>6);
  enc.out->put('z');
  enc.out->put('P');
  enc.out->put('Q');
  enc.out->put(1+(z.header[6]==0));  // level 1 or 2
  enc.out->put(1);
  z.write(enc.out, false);
  state=BLOCK1;
}

void Compressor::startBlock(const char* config, int* args, Writer* pcomp_cmd) {
  assert(state==INIT);
  Compiler(config, args, z, pz, pcomp_cmd);
  pz.sha1=&sha1;
  assert(z.header.isize()>6);
  enc.out->put('z');
  enc.out->put('P');
  enc.out->put('Q');
  enc.out->put(1+(z.header[6]==0));  // level 1 or 2
  enc.out->put(1);
  z.write(enc.out, false);
  state=BLOCK1;
}

// Write a segment header
void Compressor::startSegment(const char* filename, const char* comment) {
  assert(state==BLOCK1 || state==BLOCK2);
  enc.out->put(1);
  while (filename && *filename)
    enc.out->put(*filename++);
  enc.out->put(0);
  while (comment && *comment)
    enc.out->put(*comment++);
  enc.out->put(0);
  enc.out->put(0);
  if (state==BLOCK1) state=SEG1;
  if (state==BLOCK2) state=SEG2;
}

// Initialize encoding and write pcomp to first segment
// If len is 0 then length is encoded in pcomp[0..1]
// if pcomp is 0 then get pcomp from pz.header
void Compressor::postProcess(const char* pcomp, int len) {
  if (state==SEG2) return;
  assert(state==SEG1);
  enc.init();
  if (!pcomp) {
    len=pz.hend-pz.hbegin;
    if (len>0) {
      assert(pz.header.isize()>pz.hend);
      assert(pz.hbegin>=0);
      pcomp=(const char*)&pz.header[pz.hbegin];
    }
    assert(len>=0);
  }
  else if (len==0) {
    len=toU16(pcomp);
    pcomp+=2;
  }
  if (len>0) {
    enc.compress(1);
    enc.compress(len&255);
    enc.compress((len>>8)&255);
    for (int i=0; i<len; ++i)
      enc.compress(pcomp[i]&255);
    if (verify)
      pz.initp();
  }
  else
    enc.compress(0);
  state=SEG2;
}

// Compress n bytes, or to EOF if n < 0
bool Compressor::compress(int n) {
  if (state==SEG1)
    postProcess();
  assert(state==SEG2);

  const int BUFSIZE=1<<14;
  char buf[BUFSIZE];  // input buffer
  while (n) {
    int nbuf=BUFSIZE;  // bytes read into buf
    if (n>=0 && n<nbuf) nbuf=n;
    int nr=in->read(buf, nbuf);
    if (nr<0 || nr>BUFSIZE || nr>nbuf) error("invalid read size");
    if (nr<=0) return false;
    if (n>=0) n-=nr;
    for (int i=0; i<nr; ++i) {
      int ch=U8(buf[i]);
      enc.compress(ch);
      if (verify) {
        if (pz.hend) pz.run(ch);
        else sha1.put(ch);
      }
    }
  }
  return true;
}

// End segment, write sha1string if present
void Compressor::endSegment(const char* sha1string) {
  if (state==SEG1)
    postProcess();
  assert(state==SEG2);
  enc.compress(-1);
  if (verify && pz.hend) {
    pz.run(-1);
    pz.flush();
  }
  enc.out->put(0);
  enc.out->put(0);
  enc.out->put(0);
  enc.out->put(0);
  if (sha1string) {
    enc.out->put(253);
    for (int i=0; i<20; ++i)
      enc.out->put(sha1string[i]);
  }
  else
    enc.out->put(254);
  state=BLOCK2;
}

// End segment, write checksum and size is verify is true
char* Compressor::endSegmentChecksum(int64_t* size, bool dosha1) {
  if (state==SEG1)
    postProcess();
  assert(state==SEG2);
  enc.compress(-1);
  if (verify && pz.hend) {
    pz.run(-1);
    pz.flush();
  }
  enc.out->put(0);
  enc.out->put(0);
  enc.out->put(0);
  enc.out->put(0);
  if (verify) {
    if (size) *size=sha1.usize();
    memcpy(sha1result, sha1.result(), 20);
  }
  if (verify && dosha1) {
    enc.out->put(253);
    for (int i=0; i<20; ++i)
      enc.out->put(sha1result[i]);
  }
  else
    enc.out->put(254);
  state=BLOCK2;
  return verify ? sha1result : 0;
}

// End block
void Compressor::endBlock() {
  assert(state==BLOCK2);
  enc.out->put(255);
  state=INIT;
}

/////////////////////////// compress() ///////////////////////

void compress(Reader* in, Writer* out, const char* method,
              const char* filename, const char* comment, bool dosha1) {

  // Get block size
  int bs=4;
  if (method && method[0] && method[1]>='0' && method[1]<='9') {
    bs=method[1]-'0';
    if (method[2]>='0' && method[2]<='9') bs=bs*10+method[2]-'0';
    if (bs>11) bs=11;
  }
  bs=(0x100000<<bs)-4096;

  // Compress in blocks
  StringBuffer sb(bs);
  sb.write(0, bs);
  int n=0;
  while (in && (n=in->read((char*)sb.data(), bs))>0) {
    sb.resize(n);
    compressBlock(&sb, out, method, filename, comment, dosha1);
    filename=0;
    comment=0;
    sb.resize(0);
  }
}

//////////////////////// ZPAQL::assemble() ////////////////////

#ifndef NOJIT
/*
assemble();

Assembles the ZPAQL code in hcomp[0..hlen-1] and stores x86-32 or x86-64
code in rcode[0..rcode_size-1]. Execution begins at rcode[0]. It will not
write beyond the end of rcode, but in any case it returns the number of
bytes that would have been written. It returns 0 in case of error.

The assembled code implements int run() and returns 0 if successful,
1 if the ZPAQL code executes an invalid instruction or jumps out of
bounds, or 2 if OUT throws bad_alloc, or 3 for other OUT exceptions.

A ZPAQL virtual machine has the following state. All values are
unsigned and initially 0:

  a, b, c, d: 32 bit registers (pointed to by their respective parameters)
  f: 1 bit flag register (pointed to)
  r[0..255]: 32 bit registers
  m[0..msize-1]: 8 bit registers, where msize is a power of 2
  h[0..hsize-1]: 32 bit registers, where hsize is a power of 2
  out: pointer to a Writer
  sha1: pointer to a SHA1

Generally a ZPAQL machine is used to compute contexts which are
placed in h. A second machine might post-process, and write its
output to out and sha1. In either case, a machine is called with
its input in a, representing a single byte (0..255) or
(for a postprocessor) EOF (0xffffffff). Execution returs after a
ZPAQL halt instruction.

ZPAQL instructions are 1 byte unless the last 3 bits are 1.
In this case, a second operand byte follows. Opcode 255 is
the only 3 byte instruction. They are organized:

  00dddxxx = unary opcode xxx on destination ddd (ddd < 111)
  00111xxx = special instruction xxx
  01dddsss = assignment: ddd = sss (ddd < 111)
  1xxxxsss = operation xxxx from sss to a

The meaning of sss and ddd are as follows:

  000 = a   (accumulator)
  001 = b
  010 = c
  011 = d
  100 = *b  (means m[b mod msize])
  101 = *c  (means m[c mod msize])
  110 = *d  (means h[d mod hsize])
  111 = n   (constant 0..255 in second byte of instruction)

For example, 01001110 assigns *d to b. The other instructions xxx
are as follows:

Group 00dddxxx where ddd < 111 and xxx is:
  000 = ddd<>a, swap with a (except 00000000 is an error, and swap
        with *b or *c leaves the high bits of a unchanged)
  001 = ddd++, increment
  010 = ddd--, decrement
  011 = ddd!, not (invert all bits)
  100 = ddd=0, clear (set all bits of ddd to 0)
  101 = not used (error)
  110 = not used
  111 = ddd=r n, assign from r[n] to ddd, n=0..255 in next opcode byte
Except:
  00100111 = jt n, jump if f is true (n = -128..127, relative to next opcode)
  00101111 = jf n, jump if f is false (n = -128..127)
  00110111 = r=a n, assign r[n] = a (n = 0..255)

Group 00111xxx where xxx is:
  000 = halt (return)
  001 = output a
  010 = not used
  011 = hash: a = (a + *b + 512) * 773
  100 = hashd: *d = (*d + a + 512) * 773
  101 = not used
  110 = not used
  111 = unconditional jump (n = -128 to 127, relative to next opcode)
  
Group 1xxxxsss where xxxx is:
  0000 = a += sss (add, subtract, multiply, divide sss to a)
  0001 = a -= sss
  0010 = a *= sss
  0011 = a /= sss (unsigned, except set a = 0 if sss is 0)
  0100 = a %= sss (remainder, except set a = 0 if sss is 0)
  0101 = a &= sss (bitwise AND)
  0110 = a &= ~sss (bitwise AND with complement of sss)
  0111 = a |= sss (bitwise OR)
  1000 = a ^= sss (bitwise XOR)
  1001 = a <<= (sss % 32) (left shift by low 5 bits of sss)
  1010 = a >>= (sss % 32) (unsigned, zero bits shifted in)
  1011 = a == sss (compare, set f = true if equal or false otherwise)
  1100 = a < sss (unsigned compare, result in f)
  1101 = a > sss (unsigned compare)
  1110 = not used
  1111 = not used except 11111111 is a 3 byte jump to the absolute address
         in the next 2 bytes in little-endian (LSB first) order.

assemble() translates ZPAQL to 32 bit x86 code to be executed by run().
Registers are mapped as follows:

  eax = source sss from *b, *c, *d or sometimes n
  ecx = pointer to destination *b, *c, *d, or spare
  edx = a
  ebx = f (1 for true, 0 for false)
  esp = stack pointer
  ebp = d
  esi = b
  edi = c

run() saves non-volatile registers (ebp, esi, edi, ebx) on the stack,
loads a, b, c, d, f, and executes the translated instructions.
A halt instruction saves a, b, c, d, f, pops the saved registers
and returns. Invalid instructions or jumps outside of the range
of the ZPAQL code call libzpaq::error().

In 64 bit mode, the following additional registers are used:

  r12 = h
  r14 = r
  r15 = m

*/

// Called by out
static int flush1(ZPAQL* z) {
  try {
    z->flush();
    return 0;
  }
  catch(std::bad_alloc& x) {
    return 2;
  }
  catch(...) {
    return 3;
  }
}

// return true if op is an undefined ZPAQL instruction
static bool iserr(int op) {
  return op==0 || (op>=120 && op<=127) || (op>=240 && op<=254)
    || op==58 || (op<64 && (op%8==5 || op%8==6));
}

// Return length of ZPAQL instruction at hcomp[0]. Assume 0 padding at end.
// A run of identical ++ or -- is counted as 1 instruction.
static int oplen(const U8* hcomp) {
  if (*hcomp==255) return 3;
  if (*hcomp%8==7) return 2;
  if (*hcomp<51 && (*hcomp%8-1)/2==0) {  // ++ or -- opcode
    int i;
    for (i=1; i<127 && hcomp[i]==hcomp[0]; ++i);
    return i;
  }
  return 1;
}

// Write k bytes of x to rcode[o++] MSB first
static void put(U8* rcode, int n, int& o, U32 x, int k) {
  while (k-->0) {
    if (o<n) rcode[o]=(x>>(k*8))&255;
    ++o;
  }
}

// Write 4 bytes of x to rcode[o++] LSB first
static void put4lsb(U8* rcode, int n, int& o, U32 x) {
  for (int k=0; k<4; ++k) {
    if (o<n) rcode[o]=(x>>(k*8))&255;
    ++o;
  }
}

// Write a 1-4 byte x86 opcode without or with an 4 byte operand
// to rcode[o...]
#define put1(x) put(rcode, rcode_size, o, (x), 1)
#define put2(x) put(rcode, rcode_size, o, (x), 2)
#define put3(x) put(rcode, rcode_size, o, (x), 3)
#define put4(x) put(rcode, rcode_size, o, (x), 4)
#define put5(x,y) put4(x), put1(y)
#define put6(x,y) put4(x), put2(y)
#define put4r(x) put4lsb(rcode, rcode_size, o, x)
#define puta(x) t=U32(size_t(x)), put4r(t)
#define put1a(x,y) put1(x), puta(y)
#define put2a(x,y) put2(x), puta(y)
#define put3a(x,y) put3(x), puta(y)
#define put4a(x,y) put4(x), puta(y)
#define put5a(x,y,z) put4(x), put1(y), puta(z)
#define put2l(x,y) put2(x), t=U32(size_t(y)), put4r(t), \
  t=U32(size_t(y)>>(S*4)), put4r(t)

// Assemble ZPAQL in in the HCOMP section of header to rcode,
// but do not write beyond rcode_size. Return the number of
// bytes output or that would have been output.
// Execution starts at rcode[0] and returns 1 if successful or 0
// in case of a ZPAQL execution error.
int ZPAQL::assemble() {

  // x86? (not foolproof)
  const int S=sizeof(char*);      // 4 = x86, 8 = x86-64
  U32 t=0x12345678;
  if (*(char*)&t!=0x78 || (S!=4 && S!=8))
    error("JIT supported only for x86-32 and x86-64");

  const U8* hcomp=&header[hbegin];
  const int hlen=hend-hbegin+2;
  const int msize=m.size();
  const int hsize=h.size();
  static const int regcode[8]={2,6,7,5}; // a,b,c,d.. -> edx,esi,edi,ebp,eax..
  Array<int> it(hlen);            // hcomp -> rcode locations
  int done=0;  // number of instructions assembled (0..hlen)
  int o=5;  // rcode output index, reserve space for jmp

  // Code for the halt instruction (restore registers and return)
  const int halt=o;
  if (S==8) {
    put2l(0x48b9, &a);        // mov rcx, a
    put2(0x8911);             // mov [rcx], edx
    put2l(0x48b9, &b);        // mov rcx, b
    put2(0x8931);             // mov [rcx], esi
    put2l(0x48b9, &c);        // mov rcx, c
    put2(0x8939);             // mov [rcx], edi
    put2l(0x48b9, &d);        // mov rcx, d
    put2(0x8929);             // mov [rcx], ebp
    put2l(0x48b9, &f);        // mov rcx, f
    put2(0x8919);             // mov [rcx], ebx
    put4(0x4883c408);         // add rsp, 8
    put2(0x415f);             // pop r15
    put2(0x415e);             // pop r14
    put2(0x415d);             // pop r13
    put2(0x415c);             // pop r12
  }
  else {
    put2a(0x8915, &a);        // mov [a], edx
    put2a(0x8935, &b);        // mov [b], esi
    put2a(0x893d, &c);        // mov [c], edi
    put2a(0x892d, &d);        // mov [d], ebp
    put2a(0x891d, &f);        // mov [f], ebx
    put3(0x83c40c);           // add esp, 12
  }
  put1(0x5b);                 // pop ebx
  put1(0x5f);                 // pop edi
  put1(0x5e);                 // pop esi
  put1(0x5d);                 // pop ebp
  put1(0xc3);                 // ret

  // Code for the out instruction.
  // Store a=edx at outbuf[bufptr++]. If full, call flush1().
  const int outlabel=o;
  if (S==8) {
    put2l(0x48b8, &outbuf[0]);// mov rax, outbuf.p
    put2l(0x49ba, &bufptr);   // mov r10, &bufptr
    put3(0x418b0a);           // mov rcx, [r10]
    put3(0x881408);           // mov [rax+rcx], dl
    put2(0xffc1);             // inc rcx
    put3(0x41890a);           // mov [r10], ecx
    put2a(0x81f9, outbuf.size());  // cmp rcx, outbuf.size()
    put2(0x7403);             // jz L1
    put2(0x31c0);             // xor eax, eax
    put1(0xc3);               // ret

    put1(0x55);               // L1: push rbp ; call flush1(this)
    put1(0x57);               // push rdi
    put1(0x56);               // push rsi
    put1(0x52);               // push rdx
    put1(0x51);               // push rcx
    put3(0x4889e5);           // mov rbp, rsp
    put4(0x4883c570);         // add rbp, 112
#if defined(unix) && !defined(__CYGWIN__)
    put2l(0x48bf, this);      // mov rdi, this
#else  // Windows
    put2l(0x48b9, this);      // mov rcx, this
#endif
    put2l(0x49bb, &flush1);   // mov r11, &flush1
    put3(0x41ffd3);           // call r11
    put1(0x59);               // pop rcx
    put1(0x5a);               // pop rdx
    put1(0x5e);               // pop rsi
    put1(0x5f);               // pop rdi
    put1(0x5d);               // pop rbp
  }
  else {
    put1a(0xb8, &outbuf[0]);  // mov eax, outbuf.p
    put2a(0x8b0d, &bufptr);   // mov ecx, [bufptr]
    put3(0x881408);           // mov [eax+ecx], dl
    put2(0xffc1);             // inc ecx
    put2a(0x890d, &bufptr);   // mov [bufptr], ecx
    put2a(0x81f9, outbuf.size());  // cmp ecx, outbuf.size()
    put2(0x7403);             // jz L1
    put2(0x31c0);             // xor eax, eax
    put1(0xc3);               // ret
    put3(0x83ec0c);           // L1: sub esp, 12
    put4(0x89542404);         // mov [esp+4], edx
    put3a(0xc70424, this);    // mov [esp], this
    put1a(0xb8, &flush1);     // mov eax, &flush1
    put2(0xffd0);             // call eax
    put4(0x8b542404);         // mov edx, [esp+4]
    put3(0x83c40c);           // add esp, 12
  }
  put1(0xc3);               // ret

  // Set it[i]=1 for each ZPAQL instruction reachable from the previous
  // instruction + 2 if reachable by a jump (or 3 if both).
  it[0]=2;
  assert(hlen>0 && hcomp[hlen-1]==0);  // ends with error
  do {
    done=0;
    const int NONE=0x80000000;
    for (int i=0; i<hlen; ++i) {
      int op=hcomp[i];
      if (it[i]) {
        int next1=i+oplen(hcomp+i), next2=NONE; // next and jump targets
        if (iserr(op)) next1=NONE;  // error
        if (op==56) next1=NONE, next2=0;  // halt
        if (op==255) next1=NONE, next2=hcomp[i+1]+256*hcomp[i+2]; // lj
        if (op==39||op==47||op==63)next2=i+2+(hcomp[i+1]<<24>>24);// jt,jf,jmp
        if (op==63) next1=NONE;  // jmp
        if ((next2<0 || next2>=hlen) && next2!=NONE) next2=hlen-1; // error
        if (next1>=0 && next1<hlen && !(it[next1]&1)) it[next1]|=1, ++done;
        if (next2>=0 && next2<hlen && !(it[next2]&2)) it[next2]|=2, ++done;
      }
    }
  } while (done>0);

  // Set it[i] bits 2-3 to 4, 8, or 12 if a comparison
  //  (==, <, > respectively) does not need to save the result in f,
  // or if a conditional jump (jt, jf) does not need to read f.
  // This is true if a comparison is followed directly by a jt/jf,
  // the jt/jf is not a jump target, the byte before is not a jump
  // target (for a 2 byte comparison), and for the comparison instruction
  // if both paths after the jt/jf lead to another comparison or error
  // before another jt/jf. At most hlen steps are traced because after
  // that it must be an infinite loop.
  for (int i=0; i<hlen; ++i) {
    const int op1=hcomp[i]; // 216..239 = comparison
    const int i2=i+1+(op1%8==7);  // address of next instruction
    const int op2=hcomp[i2];  // 39,47 = jt,jf
    if (it[i] && op1>=216 && op1<240 && (op2==39 || op2==47)
        && it[i2]==1 && (i2==i+1 || it[i+1]==0)) {
      int code=(op1-208)/8*4; // 4,8,12 is ==,<,>
      it[i2]+=code;  // OK to test CF, ZF instead of f
      for (int j=0; j<2 && code; ++j) {  // trace each path from i2
        int k=i2+2; // branch not taken
        if (j==1) k=i2+2+(hcomp[i2+1]<<24>>24);  // branch taken
        for (int l=0; l<hlen && code; ++l) {  // trace at most hlen steps
          if (k<0 || k>=hlen) break;  // out of bounds, pass
          const int op=hcomp[k];
          if (op==39 || op==47) code=0;  // jt,jf, fail
          else if (op>=216 && op<240) break;  // ==,<,>, pass
          else if (iserr(op)) break;  // error, pass
          else if (op==255) k=hcomp[k+1]+256*hcomp[k+2]; // lj
          else if (op==63) k=k+2+(hcomp[k+1]<<24>>24);  // jmp
          else if (op==56) k=0;  // halt
          else k=k+1+(op%8==7);  // ordinary instruction
        }
      }
      it[i]+=code;  // if > 0 then OK to not save flags in f (bl)
    }
  }

  // Start of run(): Save x86 and load ZPAQL registers
  const int start=o;
  assert(start>=16);
  put1(0x55);          // push ebp/rbp
  put1(0x56);          // push esi/rsi
  put1(0x57);          // push edi/rdi
  put1(0x53);          // push ebx/rbx
  if (S==8) {
    put2(0x4154);      // push r12
    put2(0x4155);      // push r13
    put2(0x4156);      // push r14
    put2(0x4157);      // push r15
    put4(0x4883ec08);  // sub rsp, 8
    put2l(0x48b8, &a); // mov rax, a
    put2(0x8b10);      // mov edx, [rax]
    put2l(0x48b8, &b); // mov rax, b
    put2(0x8b30);      // mov esi, [rax]
    put2l(0x48b8, &c); // mov rax, c
    put2(0x8b38);      // mov edi, [rax]
    put2l(0x48b8, &d); // mov rax, d
    put2(0x8b28);      // mov ebp, [rax]
    put2l(0x48b8, &f); // mov rax, f
    put2(0x8b18);      // mov ebx, [rax]
    put2l(0x49bc, &h[0]);   // mov r12, h
    put2l(0x49bd, &outbuf[0]); // mov r13, outbuf.p
    put2l(0x49be, &r[0]);   // mov r14, r
    put2l(0x49bf, &m[0]);   // mov r15, m
  }
  else {
    put3(0x83ec0c);    // sub esp, 12
    put2a(0x8b15, &a); // mov edx, [a]
    put2a(0x8b35, &b); // mov esi, [b]
    put2a(0x8b3d, &c); // mov edi, [c]
    put2a(0x8b2d, &d); // mov ebp, [d]
    put2a(0x8b1d, &f); // mov ebx, [f]
  }

  // Assemble in multiple passes until every byte of hcomp has a translation
  for (int istart=0; istart<hlen; ++istart) {
    int inc=0;
    for (int i=istart; i<hlen && it[i]; i+=inc) {
      const int code=it[i];
      inc=oplen(hcomp+i);

      // If already assembled, then assemble a jump to it
      U32 t;
      assert(it.isize()>i);
      assert(i>=0 && i<hlen);
      if (code>=16) {
        if (i>istart) {
          int a=code-o;
          if (a>-120 && a<120)
            put2(0xeb00+((a-2)&255)); // jmp short o
          else
            put1a(0xe9, a-5);  // jmp near o
        }
        break;
      }

      // Else assemble the instruction at hcomp[i] to rcode[o]
      else {
        assert(i>=0 && i<it.isize());
        assert(it[i]>0 && it[i]<16);
        assert(o>=16);
        it[i]=o;
        ++done;
        const int op=hcomp[i];
        const int arg=hcomp[i+1]+((op==255)?256*hcomp[i+2]:0);
        const int ddd=op/8%8;
        const int sss=op%8;

        // error instruction: return 1
        if (iserr(op)) {
          put1a(0xb8, 1);         // mov eax, 1
          put1a(0xe9, halt-o-4);  // jmp near halt
          continue;
        }

        // Load source *b, *c, *d, or hash (*b) into eax except:
        // {a,b,c,d}=*d, a{+,-,*,&,|,^,=,==,>,>}=*d: load address to eax
        // {a,b,c,d}={*b,*c}: load source into ddd
        if (op==59 || (op>=64 && op<240 && op%8>=4 && op%8<7)) {
          put2(0x89c0+8*regcode[sss-3+(op==59)]);  // mov eax, {esi,edi,ebp}
          const int sz=(sss==6?hsize:msize)-1;
          if (sz>=128) put1a(0x25, sz);            // and eax, dword msize-1
          else put3(0x83e000+sz);                  // and eax, byte msize-1
          const int move=(op>=64 && op<112); // = or else ddd is eax
          if (sss<6) { // ddd={a,b,c,d,*b,*c}
            if (S==8) put5(0x410fb604+8*move*regcode[ddd],0x07);
                                                   // movzx ddd, byte [r15+rax]
            else put3a(0x0fb680+8*move*regcode[ddd], &m[0]);
                                                   // movzx ddd, byte [m+eax]
          }
          else if ((0x06587000>>(op/8))&1) {// {*b,*c,*d,a/,a%,a&~,a<<,a>>}=*d
            if (S==8) put4(0x418b0484);            // mov eax, [r12+rax*4]
            else put3a(0x8b0485, &h[0]);           // mov eax, [h+eax*4]
          }
        }

        // Load destination address *b, *c, *d or hashd (*d) into ecx
        if ((op>=32 && op<56 && op%8<5) || (op>=96 && op<120) || op==60) {
          put2(0x89c1+8*regcode[op/8%8-3-(op==60)]);// mov ecx,{esi,edi,ebp}
          const int sz=(ddd==6||op==60?hsize:msize)-1;
          if (sz>=128) put2a(0x81e1, sz);   // and ecx, dword sz
          else put3(0x83e100+sz);           // and ecx, byte sz
          if (op/8%8==6 || op==60) { // *d
            if (S==8) put4(0x498d0c8c);     // lea rcx, [r12+rcx*4]
            else put3a(0x8d0c8d, &h[0]);    // lea ecx, [ecx*4+h]
          }
          else { // *b, *c
            if (S==8) put4(0x498d0c0f);     // lea rcx, [r15+rcx]
            else put2a(0x8d89, &m[0]);      // lea ecx, [ecx+h]
          }
        }

        // Translate by opcode
        switch((op/8)&31) {
          case 0:  // ddd = a
          case 1:  // ddd = b
          case 2:  // ddd = c
          case 3:  // ddd = d
            switch(sss) {
              case 0:  // ddd<>a (swap)
                put2(0x87d0+regcode[ddd]);   // xchg edx, ddd
                break;
              case 1:  // ddd++
                put3(0x83c000+256*regcode[ddd]+inc); // add ddd, inc
                break;
              case 2:  // ddd--
                put3(0x83e800+256*regcode[ddd]+inc); // sub ddd, inc
                break;
              case 3:  // ddd!
                put2(0xf7d0+regcode[ddd]);   // not ddd
                break;
              case 4:  // ddd=0
                put2(0x31c0+9*regcode[ddd]); // xor ddd,ddd
                break;
              case 7:  // ddd=r n
                if (S==8)
                  put3a(0x418b86+8*regcode[ddd], arg*4); // mov ddd, [r14+n*4]
                else
                  put2a(0x8b05+8*regcode[ddd], (&r[arg]));//mov ddd, [r+n]
                break;
            }
            break;
          case 4:  // ddd = *b
          case 5:  // ddd = *c
            switch(sss) {
              case 0:  // ddd<>a (swap)
                put2(0x8611);                // xchg dl, [ecx]
                break;
              case 1:  // ddd++
                put3(0x800100+inc);          // add byte [ecx], inc
                break;
              case 2:  // ddd--
                put3(0x802900+inc);          // sub byte [ecx], inc
                break;
              case 3:  // ddd!
                put2(0xf611);                // not byte [ecx]
                break;
              case 4:  // ddd=0
                put2(0x31c0);                // xor eax, eax
                put2(0x8801);                // mov [ecx], al
                break;
              case 7:  // jt, jf
              {
                assert(code>=0 && code<16);
                static const unsigned char jtab[2][4]={{5,4,2,7},{4,5,3,6}};
                               // jnz,je,jb,ja, jz,jne,jae,jbe
                if (code<4) put2(0x84db);    // test bl, bl
                if (arg>=128 && arg-257-i>=0 && o-it[arg-257-i]<120)
                  put2(0x7000+256*jtab[op==47][code/4]); // jx short 0
                else
                  put2a(0x0f80+jtab[op==47][code/4], 0); // jx near 0
                break;
              }
            }
            break;
          case 6:  // ddd = *d
            switch(sss) {
              case 0:  // ddd<>a (swap)
                put2(0x8711);             // xchg edx, [ecx]
                break;
              case 1:  // ddd++
                put3(0x830100+inc);       // add dword [ecx], inc
                break;
              case 2:  // ddd--
                put3(0x832900+inc);       // sub dword [ecx], inc
                break;
              case 3:  // ddd!
                put2(0xf711);             // not dword [ecx]
                break;
              case 4:  // ddd=0
                put2(0x31c0);             // xor eax, eax
                put2(0x8901);             // mov [ecx], eax
                break;
              case 7:  // ddd=r n
                if (S==8)
                  put3a(0x418996, arg*4); // mov [r14+n*4], edx
                else
                  put2a(0x8915, &r[arg]); // mov [r+n], edx
                break;
            }
            break;
          case 7:  // special
            switch(op) {
              case 56: // halt
                put2(0x31c0);             // xor eax, eax  ; return 0
                put1a(0xe9, halt-o-4);    // jmp near halt
                break;
              case 57:  // out
                put1a(0xe8, outlabel-o-4);// call outlabel
                put3(0x83f800);           // cmp eax, 0  ; returned error code
                put2(0x7405);             // je L1:
                put1a(0xe9, halt-o-4);    // jmp near halt ; L1:
                break;
              case 59:  // hash: a = (a + *b + 512) * 773
                put3a(0x8d8410, 512);     // lea edx, [eax+edx+512]
                put2a(0x69d0, 773);       // imul edx, eax, 773
                break;
              case 60:  // hashd: *d = (*d + a + 512) * 773
                put2(0x8b01);             // mov eax, [ecx]
                put3a(0x8d8410, 512);     // lea eax, [eax+edx+512]
                put2a(0x69c0, 773);       // imul eax, eax, 773
                put2(0x8901);             // mov [ecx], eax
                break;
              case 63:  // jmp
                put1a(0xe9, 0);           // jmp near 0 (fill in target later)
                break;
            }
            break;
          case 8:   // a=
          case 9:   // b=
          case 10:  // c=
          case 11:  // d=
            if (sss==7)  // n
              put1a(0xb8+regcode[ddd], arg);         // mov ddd, n
            else if (sss==6) { // *d
              if (S==8)
                put4(0x418b0484+(regcode[ddd]<<11)); // mov ddd, [r12+rax*4]
              else
                put3a(0x8b0485+(regcode[ddd]<<11),&h[0]);// mov ddd, [h+eax*4]
            }
            else if (sss<4) // a, b, c, d
              put2(0x89c0+regcode[ddd]+8*regcode[sss]);// mov ddd,sss
            break;
          case 12:  // *b=
          case 13:  // *c=
            if (sss==7) put3(0xc60100+arg);          // mov byte [ecx], n
            else if (sss==0) put2(0x8811);           // mov byte [ecx], dl
            else {
              if (sss<4) put2(0x89c0+8*regcode[sss]);// mov eax, sss
              put2(0x8801);                          // mov byte [ecx], al
            }
            break;
          case 14:  // *d=
            if (sss<7) put2(0x8901+8*regcode[sss]);  // mov [ecx], sss
            else put2a(0xc701, arg);                 // mov dword [ecx], n
            break;
          case 15: break; // not used
          case 16:  // a+=
            if (sss==6) {
              if (S==8) put4(0x41031484);            // add edx, [r12+rax*4]
              else put3a(0x031485, &h[0]);           // add edx, [h+eax*4]
            }
            else if (sss<7) put2(0x01c2+8*regcode[sss]);// add edx, sss
            else if (arg>=128) put2a(0x81c2, arg);   // add edx, n
            else put3(0x83c200+arg);                 // add edx, byte n
            break;
          case 17:  // a-=
            if (sss==6) {
              if (S==8) put4(0x412b1484);            // sub edx, [r12+rax*4]
              else put3a(0x2b1485, &h[0]);           // sub edx, [h+eax*4]
            }
            else if (sss<7) put2(0x29c2+8*regcode[sss]);// sub edx, sss
            else if (arg>=128) put2a(0x81ea, arg);   // sub edx, n
            else put3(0x83ea00+arg);                 // sub edx, byte n
            break;
          case 18:  // a*=
            if (sss==6) {
              if (S==8) put5(0x410faf14,0x84);       // imul edx, [r12+rax*4]
              else put4a(0x0faf1485, &h[0]);         // imul edx, [h+eax*4]
            }
            else if (sss<7) put3(0x0fafd0+regcode[sss]);// imul edx, sss
            else if (arg>=128) put2a(0x69d2, arg);   // imul edx, n
            else put3(0x6bd200+arg);                 // imul edx, byte n
            break;
          case 19:  // a/=
          case 20:  // a%=
            if (sss<7) put2(0x89c1+8*regcode[sss]);  // mov ecx, sss
            else put1a(0xb9, arg);                   // mov ecx, n
            put2(0x85c9);                            // test ecx, ecx
            put3(0x0f44d1);                          // cmovz edx, ecx
            put2(0x7408-2*(op/8==20));               // jz (over rest)
            put2(0x89d0);                            // mov eax, edx
            put2(0x31d2);                            // xor edx, edx
            put2(0xf7f1);                            // div ecx
            if (op/8==19) put2(0x89c2);              // mov edx, eax
            break;
          case 21:  // a&=
            if (sss==6) {
              if (S==8) put4(0x41231484);            // and edx, [r12+rax*4]
              else put3a(0x231485, &h[0]);           // and edx, [h+eax*4]
            }
            else if (sss<7) put2(0x21c2+8*regcode[sss]);// and edx, sss
            else if (arg>=128) put2a(0x81e2, arg);   // and edx, n
            else put3(0x83e200+arg);                 // and edx, byte n
            break;
          case 22:  // a&~
            if (sss==7) {
              if (arg<128) put3(0x83e200+(~arg&255));// and edx, byte ~n
              else put2a(0x81e2, ~arg);              // and edx, ~n
            }
            else {
              if (sss<4) put2(0x89c0+8*regcode[sss]);// mov eax, sss
              put2(0xf7d0);                          // not eax
              put2(0x21c2);                          // and edx, eax
            }
            break;
          case 23:  // a|=
            if (sss==6) {
              if (S==8) put4(0x410b1484);            // or edx, [r12+rax*4]
              else put3a(0x0b1485, &h[0]);           // or edx, [h+eax*4]
            }
            else if (sss<7) put2(0x09c2+8*regcode[sss]);// or edx, sss
            else if (arg>=128) put2a(0x81ca, arg);   // or edx, n
            else put3(0x83ca00+arg);                 // or edx, byte n
            break;
          case 24:  // a^=
            if (sss==6) {
              if (S==8) put4(0x41331484);            // xor edx, [r12+rax*4]
              else put3a(0x331485, &h[0]);           // xor edx, [h+eax*4]
            }
            else if (sss<7) put2(0x31c2+8*regcode[sss]);// xor edx, sss
            else if (arg>=128) put2a(0x81f2, arg);   // xor edx, byte n
            else put3(0x83f200+arg);                 // xor edx, n
            break;
          case 25:  // a<<=
          case 26:  // a>>=
            if (sss==7)  // sss = n
              put3(0xc1e200+8*256*(op/8==26)+arg);   // shl/shr n
            else {
              put2(0x89c1+8*regcode[sss]);           // mov ecx, sss
              put2(0xd3e2+8*(op/8==26));             // shl/shr edx, cl
            }
            break;
          case 27:  // a==
          case 28:  // a<
          case 29:  // a>
            if (sss==6) {
              if (S==8) put4(0x413b1484);            // cmp edx, [r12+rax*4]
              else put3a(0x3b1485, &h[0]);           // cmp edx, [h+eax*4]
            }
            else if (sss==7)  // sss = n
              put2a(0x81fa, arg);                    // cmp edx, dword n
            else
              put2(0x39c2+8*regcode[sss]);           // cmp edx, sss
            if (code<4) {
              if (op/8==27) put3(0x0f94c3);          // setz bl
              if (op/8==28) put3(0x0f92c3);          // setc bl
              if (op/8==29) put3(0x0f97c3);          // seta bl
            }
            break;
          case 30:  // not used
          case 31:  // 255 = lj
            if (op==255) put1a(0xe9, 0);             // jmp near
            break;
        }
      }
    }
  }

  // Finish first pass
  const int rsize=o;
  if (o>rcode_size) return rsize;

  // Fill in jump addresses (second pass)
  for (int i=0; i<hlen; ++i) {
    if (it[i]<16) continue;
    int op=hcomp[i];
    if (op==39 || op==47 || op==63 || op==255) {  // jt, jf, jmp, lj
      int target=hcomp[i+1];
      if (op==255) target+=hcomp[i+2]*256;  // lj
      else {
        if (target>=128) target-=256;
        target+=i+2;
      }
      if (target<0 || target>=hlen) target=hlen-1;  // runtime ZPAQL error
      o=it[i];
      assert(o>=16 && o<rcode_size);
      if ((op==39 || op==47) && rcode[o]==0x84) o+=2;  // jt, jf -> skip test
      assert(o>=16 && o<rcode_size);
      if (rcode[o]==0x0f) ++o;  // first byte of jz near, jnz near
      assert(o<rcode_size);
      op=rcode[o++];  // x86 opcode
      target=it[target]-o;
      if ((op>=0x72 && op<0x78) || op==0xeb) {  // jx, jmp short
        --target;
        if (target<-128 || target>127)
          error("Cannot code x86 short jump");
        assert(o<rcode_size);
        rcode[o]=target&255;
      }
      else if ((op>=0x82 && op<0x88) || op==0xe9) // jx, jmp near
      {
        target-=4;
        puta(target);
      }
      else assert(false);  // not a x86 jump
    }
  }

  // Jump to start
  o=0;
  put1a(0xe9, start-5);  // jmp near start
  return rsize;
}

//////////////////////// Predictor::assemble_p() /////////////////////

// Assemble the ZPAQL code in the HCOMP section of z.header to pcomp and
// return the number of bytes of x86 or x86-64 code written, or that would
// be written if pcomp were large enough. The code for predict() begins
// at pr.pcomp[0] and update() at pr.pcomp[5], both as jmp instructions.

// The assembled code is equivalent to int predict(Predictor*)
// and void update(Predictor*, int y); The Preditor address is placed in
// edi/rdi. The update bit y is placed in ebp/rbp.

int Predictor::assemble_p() {
  Predictor& pr=*this;
  U8* rcode=pr.pcode;         // x86 output array
  int rcode_size=pcode_size;  // output size
  int o=0;                    // output index in pcode
  const int S=sizeof(char*);  // 4 or 8
  U8* hcomp=&pr.z.header[0];  // The code to translate
#define off(x)  ((char*)&(pr.x)-(char*)&pr)
#define offc(x) ((char*)&(pr.comp[i].x)-(char*)&pr)

  // test for little-endian (probably x86)
  U32 t=0x12345678;
  if (*(char*)&t!=0x78 || (S!=4 && S!=8))
    error("JIT supported only for x86-32 and x86-64");

  // Initialize for predict(). Put predictor address in edi/rdi
  put1a(0xe9, 5);             // jmp predict
  put1a(0, 0x90909000);       // reserve space for jmp update
  put1(0x53);                 // push ebx/rbx
  put1(0x55);                 // push ebp/rbp
  put1(0x56);                 // push esi/rsi
  put1(0x57);                 // push edi/rdi
  if (S==4)
    put4(0x8b7c2414);         // mov edi,[esp+0x14] ; pr
  else {
#if !defined(unix) || defined(__CYGWIN__)
    put3(0x4889cf);           // mov rdi, rcx (1st arg in Win64)
#endif
  }

  // Code predict() for each component
  const int n=hcomp[6];  // number of components
  U8* cp=hcomp+7;
  for (int i=0; i<n; ++i, cp+=compsize[cp[0]]) {
    if (cp-hcomp>=pr.z.cend) error("comp too big");
    if (cp[0]<1 || cp[0]>9) error("invalid component");
    assert(compsize[cp[0]]>0 && compsize[cp[0]]<8);
    switch (cp[0]) {

      case CONS:  // c
        break;

      case CM:  // sizebits limit
        // Component& cr=comp[i];
        // cr.cxt=h[i]^hmap4;
        // p[i]=stretch(cr.cm(cr.cxt)>>17);

        put2a(0x8b87, off(h[i]));              // mov eax, [edi+&h[i]]
        put2a(0x3387, off(hmap4));             // xor eax, [edi+&hmap4]
        put1a(0x25, (1<<cp[1])-1);             // and eax, size-1
        put2a(0x8987, offc(cxt));              // mov [edi+cxt], eax
        if (S==8) put1(0x48);                  // rex.w (esi->rsi)
        put2a(0x8bb7, offc(cm));               // mov esi, [edi+&cm]
        put3(0x8b0486);                        // mov eax, [esi+eax*4]
        put3(0xc1e811);                        // shr eax, 17
        put4a(0x0fbf8447, off(stretcht));      // movsx eax,word[edi+eax*2+..]
        put2a(0x8987, off(p[i]));              // mov [edi+&p[i]], eax
        break;

      case ISSE:  // sizebits j -- c=hi, cxt=bh
        // assert((hmap4&15)>0);
        // if (c8==1 || (c8&0xf0)==16)
        //   cr.c=find(cr.ht, cp[1]+2, h[i]+16*c8);
        // cr.cxt=cr.ht[cr.c+(hmap4&15)];  // bit history
        // int *wt=(int*)&cr.cm[cr.cxt*2];
        // p[i]=clamp2k((wt[0]*p[cp[2]]+wt[1]*64)>>16);

      case ICM: // sizebits
        // assert((hmap4&15)>0);
        // if (c8==1 || (c8&0xf0)==16) cr.c=find(cr.ht, cp[1]+2, h[i]+16*c8);
        // cr.cxt=cr.ht[cr.c+(hmap4&15)];
        // p[i]=stretch(cr.cm(cr.cxt)>>8);
        //
        // Find cxt row in hash table ht. ht has rows of 16 indexed by the low
        // sizebits of cxt with element 0 having the next higher 8 bits for
        // collision detection. If not found after 3 adjacent tries, replace
        // row with lowest element 1 as priority. Return index of row.
        //
        // size_t Predictor::find(Array<U8>& ht, int sizebits, U32 cxt) {
        //  assert(ht.size()==size_t(16)<<sizebits);
        //  int chk=cxt>>sizebits&255;
        //  size_t h0=(cxt*16)&(ht.size()-16);
        //  if (ht[h0]==chk) return h0;
        //  size_t h1=h0^16;
        //  if (ht[h1]==chk) return h1;
        //  size_t h2=h0^32;
        //  if (ht[h2]==chk) return h2;
        //  if (ht[h0+1]<=ht[h1+1] && ht[h0+1]<=ht[h2+1])
        //    return memset(&ht[h0], 0, 16), ht[h0]=chk, h0;
        //  else if (ht[h1+1]<ht[h2+1])
        //    return memset(&ht[h1], 0, 16), ht[h1]=chk, h1;
        //  else
        //    return memset(&ht[h2], 0, 16), ht[h2]=chk, h2;
        // }

        if (S==8) put1(0x48);                  // rex.w
        put2a(0x8bb7, offc(ht));               // mov esi, [edi+&ht]
        put2(0x8b07);                          // mov eax, edi ; c8
        put2(0x89c1);                          // mov ecx, eax ; c8
        put3(0x83f801);                        // cmp eax, 1
        put2(0x740a);                          // je L1
        put1a(0x25, 240);                      // and eax, 0xf0
        put3(0x83f810);                        // cmp eax, 16
        put2(0x7576);                          // jne L2 ; skip find()
           // L1: ; find cxt in ht, return index in eax
        put3(0xc1e104);                        // shl ecx, 4
        put2a(0x038f, off(h[i]));              // add [edi+&h[i]]
        put2(0x89c8);                          // mov eax, ecx ; cxt
        put3(0xc1e902+cp[1]);                  // shr ecx, sizebits+2
        put2a(0x81e1, 255);                    // and eax, 255 ; chk
        put3(0xc1e004);                        // shl eax, 4
        put1a(0x25, (64<<cp[1])-16);           // and eax, ht.size()-16 = h0
        put3(0x3a0c06);                        // cmp cl, [esi+eax] ; ht[h0]
        put2(0x744d);                          // je L3 ; match h0
        put3(0x83f010);                        // xor eax, 16 ; h1
        put3(0x3a0c06);                        // cmp cl, [esi+eax]
        put2(0x7445);                          // je L3 ; match h1
        put3(0x83f030);                        // xor eax, 48 ; h2
        put3(0x3a0c06);                        // cmp cl, [esi+eax]
        put2(0x743d);                          // je L3 ; match h2
          // No checksum match, so replace the lowest priority among h0,h1,h2
        put3(0x83f021);                        // xor eax, 33 ; h0+1
        put3(0x8a1c06);                        // mov bl, [esi+eax] ; ht[h0+1]
        put2(0x89c2);                          // mov edx, eax ; h0+1
        put3(0x83f220);                        // xor edx, 32  ; h2+1
        put3(0x3a1c16);                        // cmp bl, [esi+edx]
        put2(0x7708);                          // ja L4 ; test h1 vs h2
        put3(0x83f230);                        // xor edx, 48  ; h1+1
        put3(0x3a1c16);                        // cmp bl, [esi+edx]
        put2(0x7611);                          // jbe L7 ; replace h0
          // L4: ; h0 is not lowest, so replace h1 or h2
        put3(0x83f010);                        // xor eax, 16 ; h1+1
        put3(0x8a1c06);                        // mov bl, [esi+eax]
        put3(0x83f030);                        // xor eax, 48 ; h2+1
        put3(0x3a1c06);                        // cmp bl, [esi+eax]
        put2(0x7303);                          // jae L7
        put3(0x83f030);                        // xor eax, 48 ; h1+1
          // L7: ; replace row pointed to by eax = h0,h1,h2
        put3(0x83f001);                        // xor eax, 1
        put3(0x890c06);                        // mov [esi+eax], ecx ; chk
        put2(0x31c9);                          // xor ecx, ecx
        put4(0x894c0604);                      // mov [esi+eax+4], ecx
        put4(0x894c0608);                      // mov [esi+eax+8], ecx
        put4(0x894c060c);                      // mov [esi+eax+12], ecx
          // L3: ; save nibble context (in eax) in c
        put2a(0x8987, offc(c));                // mov [edi+c], eax
        put2(0xeb06);                          // jmp L8
          // L2: ; get nibble context
        put2a(0x8b87, offc(c));                // mov eax, [edi+c]
          // L8: ; nibble context is in eax
        put2a(0x8b97, off(hmap4));             // mov edx, [edi+&hmap4]
        put3(0x83e20f);                        // and edx, 15  ; hmap4
        put2(0x01d0);                          // add eax, edx ; c+(hmap4&15)
        put4(0x0fb61406);                      // movzx edx, byte [esi+eax]
        put2a(0x8997, offc(cxt));              // mov [edi+&cxt], edx ; cxt=bh
        if (S==8) put1(0x48);                  // rex.w
        put2a(0x8bb7, offc(cm));               // mov esi, [edi+&cm] ; cm

        // esi points to cm[256] (ICM) or cm[512] (ISSE) with 23 bit
        // prediction (ICM) or a pair of 20 bit signed weights (ISSE).
        // cxt = bit history bh (0..255) is in edx.
        if (cp[0]==ICM) {
          put3(0x8b0496);                      // mov eax, [esi+edx*4];cm[bh]
          put3(0xc1e808);                      // shr eax, 8
          put4a(0x0fbf8447, off(stretcht));    // movsx eax,word[edi+eax*2+..]
        }
        else {  // ISSE
          put2a(0x8b87, off(p[cp[2]]));        // mov eax, [edi+&p[j]]
          put4(0x0faf04d6);                    // imul eax, [esi+edx*8] ;wt[0]
          put4(0x8b4cd604);                    // mov ecx, [esi+edx*8+4];wt[1]
          put3(0xc1e106);                      // shl ecx, 6
          put2(0x01c8);                        // add eax, ecx
          put3(0xc1f810);                      // sar eax, 16
          put1a(0xb9, 2047);                   // mov ecx, 2047
          put2(0x39c8);                        // cmp eax, ecx
          put3(0x0f4fc1);                      // cmovg eax, ecx
          put1a(0xb9, -2048);                  // mov ecx, -2048
          put2(0x39c8);                        // cmp eax, ecx
          put3(0x0f4cc1);                      // cmovl eax, ecx

        }
        put2a(0x8987, off(p[i]));              // mov [edi+&p[i]], eax
        break;

      case MATCH: // sizebits bufbits: a=len, b=offset, c=bit, cxt=bitpos,
                  //                   ht=buf, limit=pos
        // assert(cr.cm.size()==(size_t(1)<<cp[1]));
        // assert(cr.ht.size()==(size_t(1)<<cp[2]));
        // assert(cr.a<=255);
        // assert(cr.c==0 || cr.c==1);
        // assert(cr.cxt<8);
        // assert(cr.limit<cr.ht.size());
        // if (cr.a==0) p[i]=0;
        // else {
        //   cr.c=(cr.ht(cr.limit-cr.b)>>(7-cr.cxt))&1; // predicted bit
        //   p[i]=stretch(dt2k[cr.a]*(cr.c*-2+1)&32767);
        // }

        if (S==8) put1(0x48);          // rex.w
        put2a(0x8bb7, offc(ht));       // mov esi, [edi+&ht]

        // If match length (a) is 0 then p[i]=0
        put2a(0x8b87, offc(a));        // mov eax, [edi+&a]
        put2(0x85c0);                  // test eax, eax
        put2(0x7449);                  // jz L2 ; p[i]=0

        // Else put predicted bit in c
        put1a(0xb9, 7);                // mov ecx, 7
        put2a(0x2b8f, offc(cxt));      // sub ecx, [edi+&cxt]
        put2a(0x8b87, offc(limit));    // mov eax, [edi+&limit]
        put2a(0x2b87, offc(b));        // sub eax, [edi+&b]
        put1a(0x25, (1<<cp[2])-1);     // and eax, ht.size()-1
        put4(0x0fb60406);              // movzx eax, byte [esi+eax]
        put2(0xd3e8);                  // shr eax, cl
        put3(0x83e001);                // and eax, 1  ; predicted bit
        put2a(0x8987, offc(c));        // mov [edi+&c], eax ; c

        // p[i]=stretch(dt2k[cr.a]*(cr.c*-2+1)&32767);
        put2a(0x8b87, offc(a));        // mov eax, [edi+&a]
        put3a(0x8b8487, off(dt2k));    // mov eax, [edi+eax*4+&dt2k] ; weight
        put2(0x7402);                  // jz L1 ; z if c==0
        put2(0xf7d8);                  // neg eax
        put1a(0x25, 0x7fff);           // L1: and eax, 32767
        put4a(0x0fbf8447, off(stretcht)); //movsx eax, word [edi+eax*2+...]
        put2a(0x8987, off(p[i]));      // L2: mov [edi+&p[i]], eax
        break;

      case AVG: // j k wt
        // p[i]=(p[cp[1]]*cp[3]+p[cp[2]]*(256-cp[3]))>>8;

        put2a(0x8b87, off(p[cp[1]]));  // mov eax, [edi+&p[j]]
        put2a(0x2b87, off(p[cp[2]]));  // sub eax, [edi+&p[k]]
        put2a(0x69c0, cp[3]);          // imul eax, wt
        put3(0xc1f808);                // sar eax, 8
        put2a(0x0387, off(p[cp[2]]));  // add eax, [edi+&p[k]]
        put2a(0x8987, off(p[i]));      // mov [edi+&p[i]], eax
        break;

      case MIX2:   // sizebits j k rate mask
                   // c=size cm=wt[size] cxt=input
        // cr.cxt=((h[i]+(c8&cp[5]))&(cr.c-1));
        // assert(cr.cxt<cr.a16.size());
        // int w=cr.a16[cr.cxt];
        // assert(w>=0 && w<65536);
        // p[i]=(w*p[cp[2]]+(65536-w)*p[cp[3]])>>16;
        // assert(p[i]>=-2048 && p[i]<2048);

        put2(0x8b07);                  // mov eax, [edi] ; c8
        put1a(0x25, cp[5]);            // and eax, mask
        put2a(0x0387, off(h[i]));      // add eax, [edi+&h[i]]
        put1a(0x25, (1<<cp[1])-1);     // and eax, size-1
        put2a(0x8987, offc(cxt));      // mov [edi+&cxt], eax ; cxt
        if (S==8) put1(0x48);          // rex.w
        put2a(0x8bb7, offc(a16));      // mov esi, [edi+&a16]
        put4(0x0fb70446);              // movzx eax, word [edi+eax*2] ; w
        put2a(0x8b8f, off(p[cp[2]]));  // mov ecx, [edi+&p[j]]
        put2a(0x8b97, off(p[cp[3]]));  // mov edx, [edi+&p[k]]
        put2(0x29d1);                  // sub ecx, edx
        put3(0x0fafc8);                // imul ecx, eax
        put3(0xc1e210);                // shl edx, 16
        put2(0x01d1);                  // add ecx, edx
        put3(0xc1f910);                // sar ecx, 16
        put2a(0x898f, off(p[i]));      // mov [edi+&p[i]]
        break;

      case MIX:    // sizebits j m rate mask
                   // c=size cm=wt[size][m] cxt=index of wt in cm
        // int m=cp[3];
        // assert(m>=1 && m<=i);
        // cr.cxt=h[i]+(c8&cp[5]);
        // cr.cxt=(cr.cxt&(cr.c-1))*m; // pointer to row of weights
        // assert(cr.cxt<=cr.cm.size()-m);
        // int* wt=(int*)&cr.cm[cr.cxt];
        // p[i]=0;
        // for (int j=0; j<m; ++j)
        //   p[i]+=(wt[j]>>8)*p[cp[2]+j];
        // p[i]=clamp2k(p[i]>>8);

        put2(0x8b07);                          // mov eax, [edi] ; c8
        put1a(0x25, cp[5]);                    // and eax, mask
        put2a(0x0387, off(h[i]));              // add eax, [edi+&h[i]]
        put1a(0x25, (1<<cp[1])-1);             // and eax, size-1
        put2a(0x69c0, cp[3]);                  // imul eax, m
        put2a(0x8987, offc(cxt));              // mov [edi+&cxt], eax ; cxt
        if (S==8) put1(0x48);                  // rex.w
        put2a(0x8bb7, offc(cm));               // mov esi, [edi+&cm]
        if (S==8) put1(0x48);                  // rex.w
        put3(0x8d3486);                        // lea esi, [esi+eax*4] ; wt

        // Unroll summation loop: esi=wt[0..m-1]
        for (int k=0; k<cp[3]; k+=8) {
          const int tail=cp[3]-k;  // number of elements remaining

          // pack 8 elements of wt in xmm1, 8 elements of p in xmm3
          put4a(0xf30f6f8e, k*4);              // movdqu xmm1, [esi+k*4]
          if (tail>3) put4a(0xf30f6f96, k*4+16);//movdqu xmm2, [esi+k*4+16]
          put5(0x660f72e1,0x08);               // psrad xmm1, 8
          if (tail>3) put5(0x660f72e2,0x08);   // psrad xmm2, 8
          put4(0x660f6bca);                    // packssdw xmm1, xmm2
          put4a(0xf30f6f9f, off(p[cp[2]+k]));  // movdqu xmm3, [edi+&p[j+k]]
          if (tail>3)
            put4a(0xf30f6fa7,off(p[cp[2]+k+4]));//movdqu xmm4, [edi+&p[j+k+4]]
          put4(0x660f6bdc);                    // packssdw, xmm3, xmm4
          if (tail>0 && tail<8) {  // last loop, mask extra weights
            put4(0x660f76ed);                  // pcmpeqd xmm5, xmm5 ; -1
            put5(0x660f73dd, 16-tail*2);       // psrldq xmm5, 16-tail*2
            put4(0x660fdbcd);                  // pand xmm1, xmm5
          }
          if (k==0) {  // first loop, initialize sum in xmm0
            put4(0xf30f6fc1);                  // movdqu xmm0, xmm1
            put4(0x660ff5c3);                  // pmaddwd xmm0, xmm3
          }
          else {  // accumulate sum in xmm0
            put4(0x660ff5cb);                  // pmaddwd xmm1, xmm3
            put4(0x660ffec1);                  // paddd xmm0, xmm1
          }
        }

        // Add up the 4 elements of xmm0 = p[i] in the first element
        put4(0xf30f6fc8);                      // movdqu xmm1, xmm0
        put5(0x660f73d9,0x08);                 // psrldq xmm1, 8
        put4(0x660ffec1);                      // paddd xmm0, xmm1
        put4(0xf30f6fc8);                      // movdqu xmm1, xmm0
        put5(0x660f73d9,0x04);                 // psrldq xmm1, 4
        put4(0x660ffec1);                      // paddd xmm0, xmm1
        put4(0x660f7ec0);                      // movd eax, xmm0 ; p[i]
        put3(0xc1f808);                        // sar eax, 8
        put1a(0x3d, 2047);                     // cmp eax, 2047
        put2(0x7e05);                          // jle L1
        put1a(0xb8, 2047);                     // mov eax, 2047
        put1a(0x3d, -2048);                    // L1: cmp eax, -2048
        put2(0x7d05);                          // jge, L2
        put1a(0xb8, -2048);                    // mov eax, -2048
        put2a(0x8987, off(p[i]));              // L2: mov [edi+&p[i]], eax
        break;

      case SSE:  // sizebits j start limit
        // cr.cxt=(h[i]+c8)*32;
        // int pq=p[cp[2]]+992;
        // if (pq<0) pq=0;
        // if (pq>1983) pq=1983;
        // int wt=pq&63;
        // pq>>=6;
        // assert(pq>=0 && pq<=30);
        // cr.cxt+=pq;
        // p[i]=stretch(((cr.cm(cr.cxt)>>10)*(64-wt)       // p0
        //               +(cr.cm(cr.cxt+1)>>10)*wt)>>13);  // p1
        // // p = p0*(64-wt)+p1*wt = (p1-p0)*wt + p0*64
        // cr.cxt+=wt>>5;

        put2a(0x8b8f, off(h[i]));      // mov ecx, [edi+&h[i]]
        put2(0x030f);                  // add ecx, [edi]  ; c0
        put2a(0x81e1, (1<<cp[1])-1);   // and ecx, size-1
        put3(0xc1e105);                // shl ecx, 5  ; cxt in 0..size*32-32
        put2a(0x8b87, off(p[cp[2]]));  // mov eax, [edi+&p[j]] ; pq
        put1a(0x05, 992);              // add eax, 992
        put2(0x31d2);                  // xor edx, edx ; 0
        put2(0x39d0);                  // cmp eax, edx
        put3(0x0f4cc2);                // cmovl eax, edx
        put1a(0xba, 1983);             // mov edx, 1983
        put2(0x39d0);                  // cmp eax, edx
        put3(0x0f4fc2);                // cmovg eax, edx ; pq in 0..1983
        put2(0x89c2);                  // mov edx, eax
        put3(0x83e23f);                // and edx, 63  ; wt in 0..63
        put3(0xc1e806);                // shr eax, 6   ; pq in 0..30
        put2(0x01c1);                  // add ecx, eax ; cxt in 0..size*32-2
        if (S==8) put1(0x48);          // rex.w
        put2a(0x8bb7, offc(cm));       // mov esi, [edi+cm]
        put3(0x8b048e);                // mov eax, [esi+ecx*4] ; cm[cxt]
        put4(0x8b5c8e04);              // mov ebx, [esi+ecx*4+4] ; cm[cxt+1]
        put3(0x83fa20);                // cmp edx, 32  ; wt
        put3(0x83d9ff);                // sbb ecx, -1  ; cxt+=wt>>5
        put2a(0x898f, offc(cxt));      // mov [edi+cxt], ecx  ; cxt saved
        put3(0xc1e80a);                // shr eax, 10 ; p0 = cm[cxt]>>10
        put3(0xc1eb0a);                // shr ebx, 10 ; p1 = cm[cxt+1]>>10
        put2(0x29c3);                  // sub ebx, eax, ; p1-p0
        put3(0x0fafda);                // imul ebx, edx ; (p1-p0)*wt
        put3(0xc1e006);                // shr eax, 6
        put2(0x01d8);                  // add eax, ebx ; p in 0..2^28-1
        put3(0xc1e80d);                // shr eax, 13  ; p in 0..32767
        put4a(0x0fbf8447, off(stretcht));  // movsx eax, word [edi+eax*2+...]
        put2a(0x8987, off(p[i]));      // mov [edi+&p[i]], eax
        break;

      default:
        error("invalid ZPAQ component");
    }
  }

  // return squash(p[n-1])
  put2a(0x8b87, off(p[n-1]));          // mov eax, [edi+...]
  put1a(0x05, 0x800);                  // add eax, 2048
  put4a(0x0fbf8447, off(squasht[0]));  // movsx eax, word [edi+eax*2+...]
  put1(0x5f);                          // pop edi
  put1(0x5e);                          // pop esi
  put1(0x5d);                          // pop ebp
  put1(0x5b);                          // pop ebx
  put1(0xc3);                          // ret

  // Initialize for update() Put predictor address in edi/rdi
  // and bit y=0..1 in ebp
  int save_o=o;
  o=5;
  put1a(0xe9, save_o-10);      // jmp update
  o=save_o;
  put1(0x53);                  // push ebx/rbx
  put1(0x55);                  // push ebp/rbp
  put1(0x56);                  // push esi/rsi
  put1(0x57);                  // push edi/rdi
  if (S==4) {
    put4(0x8b7c2414);          // mov edi,[esp+0x14] ; (1st arg = pr)
    put4(0x8b6c2418);          // mov ebp,[esp+0x18] ; (2nd arg = y)
  }
  else {
#if defined(unix) && !defined(__CYGWIN__)  // (1st arg already in rdi)
    put3(0x4889f5);            // mov rbp, rsi (2nd arg in Linux-64)
#else
    put3(0x4889cf);            // mov rdi, rcx (1st arg in Win64)
    put3(0x4889d5);            // mov rbp, rdx (2nd arg)
#endif
  }

  // Code update() for each component
  cp=hcomp+7;
  for (int i=0; i<n; ++i, cp+=compsize[cp[0]]) {
    assert(cp-hcomp<pr.z.cend);
    assert (cp[0]>=1 && cp[0]<=9);
    assert(compsize[cp[0]]>0 && compsize[cp[0]]<8);
    switch (cp[0]) {

      case CONS:  // c
        break;

      case SSE:  // sizebits j start limit
      case CM:   // sizebits limit
        // train(cr, y);
        //
        // reduce prediction error in cr.cm
        // void train(Component& cr, int y) {
        //   assert(y==0 || y==1);
        //   U32& pn=cr.cm(cr.cxt);
        //   U32 count=pn&0x3ff;
        //   int error=y*32767-(cr.cm(cr.cxt)>>17);
        //   pn+=(error*dt[count]&-1024)+(count<cr.limit);

        if (S==8) put1(0x48);          // rex.w (esi->rsi)
        put2a(0x8bb7, offc(cm));       // mov esi,[edi+cm]  ; cm
        put2a(0x8b87, offc(cxt));      // mov eax,[edi+cxt] ; cxt
        put1a(0x25, pr.comp[i].cm.size()-1);  // and eax, size-1
        if (S==8) put1(0x48);          // rex.w
        put3(0x8d3486);                // lea esi,[esi+eax*4] ; &cm[cxt]
        put2(0x8b06);                  // mov eax,[esi] ; cm[cxt]
        put2(0x89c2);                  // mov edx, eax  ; cm[cxt]
        put3(0xc1e811);                // shr eax, 17   ; cm[cxt]>>17
        put2(0x89e9);                  // mov ecx, ebp  ; y
        put3(0xc1e10f);                // shl ecx, 15   ; y*32768
        put2(0x29e9);                  // sub ecx, ebp  ; y*32767
        put2(0x29c1);                  // sub ecx, eax  ; error
        put2a(0x81e2, 0x3ff);          // and edx, 1023 ; count
        put3a(0x8b8497, off(dt));      // mov eax,[edi+edx*4+dt] ; dt[count]
        put3(0x0fafc8);                // imul ecx, eax ; error*dt[count]
        put2a(0x81e1, 0xfffffc00);     // and ecx, -1024
        put2a(0x81fa, cp[2+2*(cp[0]==SSE)]*4); // cmp edx, limit*4
        put2(0x110e);                  // adc [esi], ecx ; pn+=...
        break;

      case ICM:   // sizebits: cxt=bh, ht[c][0..15]=bh row
        // cr.ht[cr.c+(hmap4&15)]=st.next(cr.ht[cr.c+(hmap4&15)], y);
        // U32& pn=cr.cm(cr.cxt);
        // pn+=int(y*32767-(pn>>8))>>2;

      case ISSE:  // sizebits j  -- c=hi, cxt=bh
        // assert(cr.cxt==cr.ht[cr.c+(hmap4&15)]);
        // int err=y*32767-squash(p[i]);
        // int *wt=(int*)&cr.cm[cr.cxt*2];
        // wt[0]=clamp512k(wt[0]+((err*p[cp[2]]+(1<<12))>>13));
        // wt[1]=clamp512k(wt[1]+((err+16)>>5));
        // cr.ht[cr.c+(hmap4&15)]=st.next(cr.cxt, y);

        // update bit history bh to next(bh,y=ebp) in ht[c+(hmap4&15)]
        put3(0x8b4700+off(hmap4));     // mov eax, [edi+&hmap4]
        put3(0x83e00f);                // and eax, 15
        put2a(0x0387, offc(c));        // add eax [edi+&c] ; cxt
        if (S==8) put1(0x48);          // rex.w
        put2a(0x8bb7, offc(ht));       // mov esi, [edi+&ht]
        put4(0x0fb61406);              // movzx edx, byte [esi+eax] ; bh
        put4(0x8d5c9500);              // lea ebx, [ebp+edx*4] ; index to st
        put4a(0x0fb69c1f, off(st));    // movzx ebx,byte[edi+ebx+st]; next bh
        put3(0x881c06);                // mov [esi+eax], bl ; save next bh
        if (S==8) put1(0x48);          // rex.w
        put2a(0x8bb7, offc(cm));       // mov esi, [edi+&cm]

        // ICM: update cm[cxt=edx=bit history] to reduce prediction error
        // esi = &cm
        if (cp[0]==ICM) {
          if (S==8) put1(0x48);        // rex.w
          put3(0x8d3496);              // lea esi, [esi+edx*4] ; &cm[bh]
          put2(0x8b06);                // mov eax, [esi] ; pn
          put3(0xc1e808);              // shr eax, 8 ; pn>>8
          put2(0x89e9);                // mov ecx, ebp ; y
          put3(0xc1e10f);              // shl ecx, 15
          put2(0x29e9);                // sub ecx, ebp ; y*32767
          put2(0x29c1);                // sub ecx, eax
          put3(0xc1f902);              // sar ecx, 2
          put2(0x010e);                // add [esi], ecx
        }

        // ISSE: update weights. edx=cxt=bit history (0..255), esi=cm[512]
        else {
          put2a(0x8b87, off(p[i]));    // mov eax, [edi+&p[i]]
          put1a(0x05, 2048);           // add eax, 2048
          put4a(0x0fb78447, off(squasht)); // movzx eax, word [edi+eax*2+..]
          put2(0x89e9);                // mov ecx, ebp ; y
          put3(0xc1e10f);              // shl ecx, 15
          put2(0x29e9);                // sub ecx, ebp ; y*32767
          put2(0x29c1);                // sub ecx, eax ; err
          put2a(0x8b87, off(p[cp[2]]));// mov eax, [edi+&p[j]]
          put3(0x0fafc1);              // imul eax, ecx
          put1a(0x05, (1<<12));        // add eax, 4096
          put3(0xc1f80d);              // sar eax, 13
          put3(0x0304d6);              // add eax, [esi+edx*8] ; wt[0]
          put1a(0x3d, (1<<19)-1);      // cmp eax, (1<<19)-1
          put2(0x7e05);                // jle L1
          put1a(0xb8, (1<<19)-1);      // mov eax, (1<<19)-1
          put1a(0x3d, 0xfff80000);     // cmp eax, -1<<19
          put2(0x7d05);                // jge L2
          put1a(0xb8, 0xfff80000);     // mov eax, -1<<19
          put3(0x8904d6);              // L2: mov [esi+edx*8], eax
          put3(0x83c110);              // add ecx, 16 ; err
          put3(0xc1f905);              // sar ecx, 5
          put4(0x034cd604);            // add ecx, [esi+edx*8+4] ; wt[1]
          put2a(0x81f9, (1<<19)-1);    // cmp ecx, (1<<19)-1
          put2(0x7e05);                // jle L3
          put1a(0xb9, (1<<19)-1);      // mov ecx, (1<<19)-1
          put2a(0x81f9, 0xfff80000);   // cmp ecx, -1<<19
          put2(0x7d05);                // jge L4
          put1a(0xb9, 0xfff80000);     // mov ecx, -1<<19
          put4(0x894cd604);            // L4: mov [esi+edx*8+4], ecx
        }
        break;

      case MATCH: // sizebits bufbits:
                  //   a=len, b=offset, c=bit, cm=index, cxt=bitpos
                  //   ht=buf, limit=pos
        // assert(cr.a<=255);
        // assert(cr.c==0 || cr.c==1);
        // assert(cr.cxt<8);
        // assert(cr.cm.size()==(size_t(1)<<cp[1]));
        // assert(cr.ht.size()==(size_t(1)<<cp[2]));
        // if (int(cr.c)!=y) cr.a=0;  // mismatch?
        // cr.ht(cr.limit)+=cr.ht(cr.limit)+y;
        // if (++cr.cxt==8) {
        //   cr.cxt=0;
        //   ++cr.limit;
        //   cr.limit&=(1<<cp[2])-1;
        //   if (cr.a==0) {  // look for a match
        //     cr.b=cr.limit-cr.cm(h[i]);
        //     if (cr.b&(cr.ht.size()-1))
        //       while (cr.a<255
        //              && cr.ht(cr.limit-cr.a-1)==cr.ht(cr.limit-cr.a-cr.b-1))
        //         ++cr.a;
        //   }
        //   else cr.a+=cr.a<255;
        //   cr.cm(h[i])=cr.limit;
        // }

        // Set pointers ebx=&cm, esi=&ht
        if (S==8) put1(0x48);          // rex.w
        put2a(0x8bb7, offc(ht));       // mov esi, [edi+&ht]
        if (S==8) put1(0x48);          // rex.w
        put2a(0x8b9f, offc(cm));       // mov ebx, [edi+&cm]

        // if (c!=y) a=0;
        put2a(0x8b87, offc(c));        // mov eax, [edi+&c]
        put2(0x39e8);                  // cmp eax, ebp ; y
        put2(0x7408);                  // jz L1
        put2(0x31c0);                  // xor eax, eax
        put2a(0x8987, offc(a));        // mov [edi+&a], eax

        // ht(limit)+=ht(limit)+y  (1E)
        put2a(0x8b87, offc(limit));    // mov eax, [edi+&limit]
        put4(0x0fb60c06);              // movzx, ecx, byte [esi+eax]
        put2(0x01c9);                  // add ecx, ecx
        put2(0x01e9);                  // add ecx, ebp
        put3(0x880c06);                // mov [esi+eax], cl

        // if (++cxt==8)
        put2a(0x8b87, offc(cxt));      // mov eax, [edi+&cxt]
        put2(0xffc0);                  // inc eax
        put3(0x83e007);                // and eax,byte +0x7
        put2a(0x8987, offc(cxt));      // mov [edi+&cxt],eax
        put2a(0x0f85, 0x9b);           // jnz L8

        // ++limit;
        // limit&=bufsize-1;
        put2a(0x8b87, offc(limit));    // mov eax,[edi+&limit]
        put2(0xffc0);                  // inc eax
        put1a(0x25, (1<<cp[2])-1);     // and eax, bufsize-1
        put2a(0x8987, offc(limit));    // mov [edi+&limit],eax

        // if (a==0)
        put2a(0x8b87, offc(a));        // mov eax, [edi+&a]
        put2(0x85c0);                  // test eax,eax
        put2(0x755c);                  // jnz L6

        //   b=limit-cm(h[i])
        put2a(0x8b8f, off(h[i]));      // mov ecx,[edi+h[i]]
        put2a(0x81e1, (1<<cp[1])-1);   // and ecx, size-1
        put2a(0x8b87, offc(limit));    // mov eax,[edi-&limit]
        put3(0x2b048b);                // sub eax,[ebx+ecx*4]
        put2a(0x8987, offc(b));        // mov [edi+&b],eax

        //   if (b&(bufsize-1))
        put1a(0xa9, (1<<cp[2])-1);     // test eax, bufsize-1
        put2(0x7448);                  // jz L7

        //      while (a<255 && ht(limit-a-1)==ht(limit-a-b-1)) ++a;
        put1(0x53);                    // push ebx
        put2a(0x8b9f, offc(limit));    // mov ebx,[edi+&limit]
        put2(0x89da);                  // mov edx,ebx
        put2(0x29c3);                  // sub ebx,eax  ; limit-b
        put2(0x31c9);                  // xor ecx,ecx  ; a=0
        put2a(0x81f9, 0xff);           // L2: cmp ecx,0xff ; while
        put2(0x741c);                  // jz L3 ; break
        put2(0xffca);                  // dec edx
        put2(0xffcb);                  // dec ebx
        put2a(0x81e2, (1<<cp[2])-1);   // and edx, bufsize-1
        put2a(0x81e3, (1<<cp[2])-1);   // and ebx, bufsize-1
        put3(0x8a0416);                // mov al,[esi+edx]
        put3(0x3a041e);                // cmp al,[esi+ebx]
        put2(0x7504);                  // jnz L3 ; break
        put2(0xffc1);                  // inc ecx
        put2(0xebdc);                  // jmp short L2 ; end while
        put1(0x5b);                    // L3: pop ebx
        put2a(0x898f, offc(a));        // mov [edi+&a],ecx
        put2(0xeb0e);                  // jmp short L7

        // a+=(a<255)
        put1a(0x3d, 0xff);             // L6: cmp eax, 0xff ; a
        put3(0x83d000);                // adc eax, 0
        put2a(0x8987, offc(a));        // mov [edi+&a],eax

        // cm(h[i])=limit
        put2a(0x8b87, off(h[i]));      // L7: mov eax,[edi+&h[i]]
        put1a(0x25, (1<<cp[1])-1);     // and eax, size-1
        put2a(0x8b8f, offc(limit));    // mov ecx,[edi+&limit]
        put3(0x890c83);                // mov [ebx+eax*4],ecx
                                       // L8:
        break;

      case AVG:  // j k wt
        break;

      case MIX2: // sizebits j k rate mask
                 // cm=wt[size], cxt=input
        // assert(cr.a16.size()==cr.c);
        // assert(cr.cxt<cr.a16.size());
        // int err=(y*32767-squash(p[i]))*cp[4]>>5;
        // int w=cr.a16[cr.cxt];
        // w+=(err*(p[cp[2]]-p[cp[3]])+(1<<12))>>13;
        // if (w<0) w=0;
        // if (w>65535) w=65535;
        // cr.a16[cr.cxt]=w;

        // set ecx=err
        put2a(0x8b87, off(p[i]));      // mov eax, [edi+&p[i]]
        put1a(0x05, 2048);             // add eax, 2048
        put4a(0x0fb78447, off(squasht));//movzx eax, word [edi+eax*2+&squasht]
        put2(0x89e9);                  // mov ecx, ebp ; y
        put3(0xc1e10f);                // shl ecx, 15
        put2(0x29e9);                  // sub ecx, ebp ; y*32767
        put2(0x29c1);                  // sub ecx, eax
        put2a(0x69c9, cp[4]);          // imul ecx, rate
        put3(0xc1f905);                // sar ecx, 5  ; err

        // Update w
        put2a(0x8b87, offc(cxt));      // mov eax, [edi+&cxt]
        if (S==8) put1(0x48);          // rex.w
        put2a(0x8bb7, offc(a16));      // mov esi, [edi+&a16]
        if (S==8) put1(0x48);          // rex.w
        put3(0x8d3446);                // lea esi, [esi+eax*2] ; &w
        put2a(0x8b87, off(p[cp[2]]));  // mov eax, [edi+&p[j]]
        put2a(0x2b87, off(p[cp[3]]));  // sub eax, [edi+&p[k]] ; p[j]-p[k]
        put3(0x0fafc1);                // imul eax, ecx  ; * err
        put1a(0x05, 1<<12);            // add eax, 4096
        put3(0xc1f80d);                // sar eax, 13
        put3(0x0fb716);                // movzx edx, word [esi] ; w
        put2(0x01d0);                  // add eax, edx
        put1a(0xba, 0xffff);           // mov edx, 65535
        put2(0x39d0);                  // cmp eax, edx
        put3(0x0f4fc2);                // cmovg eax, edx
        put2(0x31d2);                  // xor edx, edx
        put2(0x39d0);                  // cmp eax, edx
        put3(0x0f4cc2);                // cmovl eax, edx
        put3(0x668906);                // mov word [esi], ax
        break;

      case MIX: // sizebits j m rate mask
                // cm=wt[size][m], cxt=input
        // int m=cp[3];
        // assert(m>0 && m<=i);
        // assert(cr.cm.size()==m*cr.c);
        // assert(cr.cxt+m<=cr.cm.size());
        // int err=(y*32767-squash(p[i]))*cp[4]>>4;
        // int* wt=(int*)&cr.cm[cr.cxt];
        // for (int j=0; j<m; ++j)
        //   wt[j]=clamp512k(wt[j]+((err*p[cp[2]+j]+(1<<12))>>13));

        // set ecx=err
        put2a(0x8b87, off(p[i]));      // mov eax, [edi+&p[i]]
        put1a(0x05, 2048);             // add eax, 2048
        put4a(0x0fb78447, off(squasht));//movzx eax, word [edi+eax*2+&squasht]
        put2(0x89e9);                  // mov ecx, ebp ; y
        put3(0xc1e10f);                // shl ecx, 15
        put2(0x29e9);                  // sub ecx, ebp ; y*32767
        put2(0x29c1);                  // sub ecx, eax
        put2a(0x69c9, cp[4]);          // imul ecx, rate
        put3(0xc1f904);                // sar ecx, 4  ; err

        // set esi=wt
        put2a(0x8b87, offc(cxt));      // mov eax, [edi+&cxt] ; cxt
        if (S==8) put1(0x48);          // rex.w
        put2a(0x8bb7, offc(cm));       // mov esi, [edi+&cm]
        if (S==8) put1(0x48);          // rex.w
        put3(0x8d3486);                // lea esi, [esi+eax*4] ; wt

        for (int k=0; k<cp[3]; ++k) {
          put2a(0x8b87,off(p[cp[2]+k]));//mov eax, [edi+&p[cp[2]+k]
          put3(0x0fafc1);              // imul eax, ecx
          put1a(0x05, 1<<12);          // add eax, 1<<12
          put3(0xc1f80d);              // sar eax, 13
          put2(0x0306);                // add eax, [esi]
          put1a(0x3d, (1<<19)-1);      // cmp eax, (1<<19)-1
          put2(0x7e05);                // jge L1
          put1a(0xb8, (1<<19)-1);      // mov eax, (1<<19)-1
          put1a(0x3d, 0xfff80000);     // cmp eax, -1<<19
          put2(0x7d05);                // jle L2
          put1a(0xb8, 0xfff80000);     // mov eax, -1<<19
          put2(0x8906);                // L2: mov [esi], eax
          if (k<cp[3]-1) {
            if (S==8) put1(0x48);      // rex.w
            put3(0x83c604);            // add esi, 4
          }
        }
        break;

      default:
        error("invalid ZPAQ component");
    }
  }

  // return from update()
  put1(0x5f);                 // pop edi
  put1(0x5e);                 // pop esi
  put1(0x5d);                 // pop ebp
  put1(0x5b);                 // pop ebx
  put1(0xc3);                 // ret

  return o;
}

#endif // ifndef NOJIT

// Return a prediction of the next bit in range 0..32767
// Use JIT code starting at pcode[0] if available, or else create it.
int Predictor::predict() {
#ifdef NOJIT
  return predict0();
#else
  if (!pcode) {
    allocx(pcode, pcode_size, (z.cend*100+4096)&-4096);
    int n=assemble_p();
    if (n>pcode_size) {
      allocx(pcode, pcode_size, n);
      n=assemble_p();
    }
    if (!pcode || n<15 || pcode_size<15)
      error("run JIT failed");
  }
  assert(pcode && pcode[0]);
  return ((int(*)(Predictor*))&pcode[10])(this);
#endif
}

// Update the model with bit y = 0..1
// Use the JIT code starting at pcode[5].
void Predictor::update(int y) {
#ifdef NOJIT
  update0(y);
#else
  assert(pcode && pcode[5]);
  ((void(*)(Predictor*, int))&pcode[5])(this, y);

  // Save bit y in c8, hmap4 (not implemented in JIT)
  c8+=c8+y;
  if (c8>=256) {
    z.run(c8-256);
    hmap4=1;
    c8=1;
    for (int i=0; i<z.header[6]; ++i) h[i]=z.H(i);
  }
  else if (c8>=16 && c8<32)
    hmap4=(hmap4&0xf)<<5|y<<4|1;
  else
    hmap4=(hmap4&0x1f0)|(((hmap4&0xf)*2+y)&0xf);
#endif
}

// Execute the ZPAQL code with input byte or -1 for EOF.
// Use JIT code at rcode if available, or else create it.
void ZPAQL::run(U32 input) {
#ifdef NOJIT
  run0(input);
#else
  if (!rcode) {
    allocx(rcode, rcode_size, (hend*10+4096)&-4096);
    int n=assemble();
    if (n>rcode_size) {
      allocx(rcode, rcode_size, n);
      n=assemble();
    }
    if (!rcode || n<10 || rcode_size<10)
      error("run JIT failed");
  }
  a=input;
  const U32 rc=((int(*)())(&rcode[0]))();
  if (rc==0) return;
  else if (rc==1) libzpaq::error("Bad ZPAQL opcode");
  else if (rc==2) libzpaq::error("Out of memory");
  else if (rc==3) libzpaq::error("Write error");
  else libzpaq::error("ZPAQL execution error");
#endif
}

////////////////////////// divsufsort ///////////////////////////////

/*
 * divsufsort.c for libdivsufsort-lite
 * Copyright (c) 2003-2008 Yuta Mori All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*- Constants -*/
#define INLINE __inline
#if defined(ALPHABET_SIZE) && (ALPHABET_SIZE < 1)
# undef ALPHABET_SIZE
#endif
#if !defined(ALPHABET_SIZE)
# define ALPHABET_SIZE (256)
#endif
#define BUCKET_A_SIZE (ALPHABET_SIZE)
#define BUCKET_B_SIZE (ALPHABET_SIZE * ALPHABET_SIZE)
#if defined(SS_INSERTIONSORT_THRESHOLD)
# if SS_INSERTIONSORT_THRESHOLD < 1
#  undef SS_INSERTIONSORT_THRESHOLD
#  define SS_INSERTIONSORT_THRESHOLD (1)
# endif
#else
# define SS_INSERTIONSORT_THRESHOLD (8)
#endif
#if defined(SS_BLOCKSIZE)
# if SS_BLOCKSIZE < 0
#  undef SS_BLOCKSIZE
#  define SS_BLOCKSIZE (0)
# elif 32768 <= SS_BLOCKSIZE
#  undef SS_BLOCKSIZE
#  define SS_BLOCKSIZE (32767)
# endif
#else
# define SS_BLOCKSIZE (1024)
#endif
/* minstacksize = log(SS_BLOCKSIZE) / log(3) * 2 */
#if SS_BLOCKSIZE == 0
# define SS_MISORT_STACKSIZE (96)
#elif SS_BLOCKSIZE <= 4096
# define SS_MISORT_STACKSIZE (16)
#else
# define SS_MISORT_STACKSIZE (24)
#endif
#define SS_SMERGE_STACKSIZE (32)
#define TR_INSERTIONSORT_THRESHOLD (8)
#define TR_STACKSIZE (64)


/*- Macros -*/
#ifndef SWAP
# define SWAP(_a, _b) do { t = (_a); (_a) = (_b); (_b) = t; } while(0)
#endif /* SWAP */
#ifndef MIN
# define MIN(_a, _b) (((_a) < (_b)) ? (_a) : (_b))
#endif /* MIN */
#ifndef MAX
# define MAX(_a, _b) (((_a) > (_b)) ? (_a) : (_b))
#endif /* MAX */
#define STACK_PUSH(_a, _b, _c, _d)\
  do {\
    assert(ssize < STACK_SIZE);\
    stack[ssize].a = (_a), stack[ssize].b = (_b),\
    stack[ssize].c = (_c), stack[ssize++].d = (_d);\
  } while(0)
#define STACK_PUSH5(_a, _b, _c, _d, _e)\
  do {\
    assert(ssize < STACK_SIZE);\
    stack[ssize].a = (_a), stack[ssize].b = (_b),\
    stack[ssize].c = (_c), stack[ssize].d = (_d), stack[ssize++].e = (_e);\
  } while(0)
#define STACK_POP(_a, _b, _c, _d)\
  do {\
    assert(0 <= ssize);\
    if(ssize == 0) { return; }\
    (_a) = stack[--ssize].a, (_b) = stack[ssize].b,\
    (_c) = stack[ssize].c, (_d) = stack[ssize].d;\
  } while(0)
#define STACK_POP5(_a, _b, _c, _d, _e)\
  do {\
    assert(0 <= ssize);\
    if(ssize == 0) { return; }\
    (_a) = stack[--ssize].a, (_b) = stack[ssize].b,\
    (_c) = stack[ssize].c, (_d) = stack[ssize].d, (_e) = stack[ssize].e;\
  } while(0)
#define BUCKET_A(_c0) bucket_A[(_c0)]
#if ALPHABET_SIZE == 256
#define BUCKET_B(_c0, _c1) (bucket_B[((_c1) << 8) | (_c0)])
#define BUCKET_BSTAR(_c0, _c1) (bucket_B[((_c0) << 8) | (_c1)])
#else
#define BUCKET_B(_c0, _c1) (bucket_B[(_c1) * ALPHABET_SIZE + (_c0)])
#define BUCKET_BSTAR(_c0, _c1) (bucket_B[(_c0) * ALPHABET_SIZE + (_c1)])
#endif


/*- Private Functions -*/

static const int lg_table[256]= {
 -1,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

#if (SS_BLOCKSIZE == 0) || (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE)

static INLINE
int
ss_ilg(int n) {
#if SS_BLOCKSIZE == 0
  return (n & 0xffff0000) ?
          ((n & 0xff000000) ?
            24 + lg_table[(n >> 24) & 0xff] :
            16 + lg_table[(n >> 16) & 0xff]) :
          ((n & 0x0000ff00) ?
             8 + lg_table[(n >>  8) & 0xff] :
             0 + lg_table[(n >>  0) & 0xff]);
#elif SS_BLOCKSIZE < 256
  return lg_table[n];
#else
  return (n & 0xff00) ?
          8 + lg_table[(n >> 8) & 0xff] :
          0 + lg_table[(n >> 0) & 0xff];
#endif
}

#endif /* (SS_BLOCKSIZE == 0) || (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE) */

#if SS_BLOCKSIZE != 0

static const int sqq_table[256] = {
  0,  16,  22,  27,  32,  35,  39,  42,  45,  48,  50,  53,  55,  57,  59,  61,
 64,  65,  67,  69,  71,  73,  75,  76,  78,  80,  81,  83,  84,  86,  87,  89,
 90,  91,  93,  94,  96,  97,  98,  99, 101, 102, 103, 104, 106, 107, 108, 109,
110, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
128, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
143, 144, 144, 145, 146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 155, 155,
156, 157, 158, 159, 160, 160, 161, 162, 163, 163, 164, 165, 166, 167, 167, 168,
169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 178, 179, 180,
181, 181, 182, 183, 183, 184, 185, 185, 186, 187, 187, 188, 189, 189, 190, 191,
192, 192, 193, 193, 194, 195, 195, 196, 197, 197, 198, 199, 199, 200, 201, 201,
202, 203, 203, 204, 204, 205, 206, 206, 207, 208, 208, 209, 209, 210, 211, 211,
212, 212, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 219, 220, 221,
221, 222, 222, 223, 224, 224, 225, 225, 226, 226, 227, 227, 228, 229, 229, 230,
230, 231, 231, 232, 232, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238,
239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247,
247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255
};

static INLINE
int
ss_isqrt(int x) {
  int y, e;

  if(x >= (SS_BLOCKSIZE * SS_BLOCKSIZE)) { return SS_BLOCKSIZE; }
  e = (x & 0xffff0000) ?
        ((x & 0xff000000) ?
          24 + lg_table[(x >> 24) & 0xff] :
          16 + lg_table[(x >> 16) & 0xff]) :
        ((x & 0x0000ff00) ?
           8 + lg_table[(x >>  8) & 0xff] :
           0 + lg_table[(x >>  0) & 0xff]);

  if(e >= 16) {
    y = sqq_table[x >> ((e - 6) - (e & 1))] << ((e >> 1) - 7);
    if(e >= 24) { y = (y + 1 + x / y) >> 1; }
    y = (y + 1 + x / y) >> 1;
  } else if(e >= 8) {
    y = (sqq_table[x >> ((e - 6) - (e & 1))] >> (7 - (e >> 1))) + 1;
  } else {
    return sqq_table[x] >> 4;
  }

  return (x < (y * y)) ? y - 1 : y;
}

#endif /* SS_BLOCKSIZE != 0 */


/*---------------------------------------------------------------------------*/

/* Compares two suffixes. */
static INLINE
int
ss_compare(const unsigned char *T,
           const int *p1, const int *p2,
           int depth) {
  const unsigned char *U1, *U2, *U1n, *U2n;

  for(U1 = T + depth + *p1,
      U2 = T + depth + *p2,
      U1n = T + *(p1 + 1) + 2,
      U2n = T + *(p2 + 1) + 2;
      (U1 < U1n) && (U2 < U2n) && (*U1 == *U2);
      ++U1, ++U2) {
  }

  return U1 < U1n ?
        (U2 < U2n ? *U1 - *U2 : 1) :
        (U2 < U2n ? -1 : 0);
}


/*---------------------------------------------------------------------------*/

#if (SS_BLOCKSIZE != 1) && (SS_INSERTIONSORT_THRESHOLD != 1)

/* Insertionsort for small size groups */
static
void
ss_insertionsort(const unsigned char *T, const int *PA,
                 int *first, int *last, int depth) {
  int *i, *j;
  int t;
  int r;

  for(i = last - 2; first <= i; --i) {
    for(t = *i, j = i + 1; 0 < (r = ss_compare(T, PA + t, PA + *j, depth));) {
      do { *(j - 1) = *j; } while((++j < last) && (*j < 0));
      if(last <= j) { break; }
    }
    if(r == 0) { *j = ~*j; }
    *(j - 1) = t;
  }
}

#endif /* (SS_BLOCKSIZE != 1) && (SS_INSERTIONSORT_THRESHOLD != 1) */


/*---------------------------------------------------------------------------*/

#if (SS_BLOCKSIZE == 0) || (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE)

static INLINE
void
ss_fixdown(const unsigned char *Td, const int *PA,
           int *SA, int i, int size) {
  int j, k;
  int v;
  int c, d, e;

  for(v = SA[i], c = Td[PA[v]]; (j = 2 * i + 1) < size; SA[i] = SA[k], i = k) {
    d = Td[PA[SA[k = j++]]];
    if(d < (e = Td[PA[SA[j]]])) { k = j; d = e; }
    if(d <= c) { break; }
  }
  SA[i] = v;
}

/* Simple top-down heapsort. */
static
void
ss_heapsort(const unsigned char *Td, const int *PA, int *SA, int size) {
  int i, m;
  int t;

  m = size;
  if((size % 2) == 0) {
    m--;
    if(Td[PA[SA[m / 2]]] < Td[PA[SA[m]]]) { SWAP(SA[m], SA[m / 2]); }
  }

  for(i = m / 2 - 1; 0 <= i; --i) { ss_fixdown(Td, PA, SA, i, m); }
  if((size % 2) == 0) { SWAP(SA[0], SA[m]); ss_fixdown(Td, PA, SA, 0, m); }
  for(i = m - 1; 0 < i; --i) {
    t = SA[0], SA[0] = SA[i];
    ss_fixdown(Td, PA, SA, 0, i);
    SA[i] = t;
  }
}


/*---------------------------------------------------------------------------*/

/* Returns the median of three elements. */
static INLINE
int *
ss_median3(const unsigned char *Td, const int *PA,
           int *v1, int *v2, int *v3) {
  int *t;
  if(Td[PA[*v1]] > Td[PA[*v2]]) { SWAP(v1, v2); }
  if(Td[PA[*v2]] > Td[PA[*v3]]) {
    if(Td[PA[*v1]] > Td[PA[*v3]]) { return v1; }
    else { return v3; }
  }
  return v2;
}

/* Returns the median of five elements. */
static INLINE
int *
ss_median5(const unsigned char *Td, const int *PA,
           int *v1, int *v2, int *v3, int *v4, int *v5) {
  int *t;
  if(Td[PA[*v2]] > Td[PA[*v3]]) { SWAP(v2, v3); }
  if(Td[PA[*v4]] > Td[PA[*v5]]) { SWAP(v4, v5); }
  if(Td[PA[*v2]] > Td[PA[*v4]]) { SWAP(v2, v4); SWAP(v3, v5); }
  if(Td[PA[*v1]] > Td[PA[*v3]]) { SWAP(v1, v3); }
  if(Td[PA[*v1]] > Td[PA[*v4]]) { SWAP(v1, v4); SWAP(v3, v5); }
  if(Td[PA[*v3]] > Td[PA[*v4]]) { return v4; }
  return v3;
}

/* Returns the pivot element. */
static INLINE
int *
ss_pivot(const unsigned char *Td, const int *PA, int *first, int *last) {
  int *middle;
  int t;

  t = last - first;
  middle = first + t / 2;

  if(t <= 512) {
    if(t <= 32) {
      return ss_median3(Td, PA, first, middle, last - 1);
    } else {
      t >>= 2;
      return ss_median5(Td, PA, first, first + t, middle, last - 1 - t, last - 1);
    }
  }
  t >>= 3;
  first  = ss_median3(Td, PA, first, first + t, first + (t << 1));
  middle = ss_median3(Td, PA, middle - t, middle, middle + t);
  last   = ss_median3(Td, PA, last - 1 - (t << 1), last - 1 - t, last - 1);
  return ss_median3(Td, PA, first, middle, last);
}


/*---------------------------------------------------------------------------*/

/* Binary partition for substrings. */
static INLINE
int *
ss_partition(const int *PA,
                    int *first, int *last, int depth) {
  int *a, *b;
  int t;
  for(a = first - 1, b = last;;) {
    for(; (++a < b) && ((PA[*a] + depth) >= (PA[*a + 1] + 1));) { *a = ~*a; }
    for(; (a < --b) && ((PA[*b] + depth) <  (PA[*b + 1] + 1));) { }
    if(b <= a) { break; }
    t = ~*b;
    *b = *a;
    *a = t;
  }
  if(first < a) { *first = ~*first; }
  return a;
}

/* Multikey introsort for medium size groups. */
static
void
ss_mintrosort(const unsigned char *T, const int *PA,
              int *first, int *last,
              int depth) {
#define STACK_SIZE SS_MISORT_STACKSIZE
  struct { int *a, *b, c; int d; } stack[STACK_SIZE];
  const unsigned char *Td;
  int *a, *b, *c, *d, *e, *f;
  int s, t;
  int ssize;
  int limit;
  int v, x = 0;

  for(ssize = 0, limit = ss_ilg(last - first);;) {

    if((last - first) <= SS_INSERTIONSORT_THRESHOLD) {
#if 1 < SS_INSERTIONSORT_THRESHOLD
      if(1 < (last - first)) { ss_insertionsort(T, PA, first, last, depth); }
#endif
      STACK_POP(first, last, depth, limit);
      continue;
    }

    Td = T + depth;
    if(limit-- == 0) { ss_heapsort(Td, PA, first, last - first); }
    if(limit < 0) {
      for(a = first + 1, v = Td[PA[*first]]; a < last; ++a) {
        if((x = Td[PA[*a]]) != v) {
          if(1 < (a - first)) { break; }
          v = x;
          first = a;
        }
      }
      if(Td[PA[*first] - 1] < v) {
        first = ss_partition(PA, first, a, depth);
      }
      if((a - first) <= (last - a)) {
        if(1 < (a - first)) {
          STACK_PUSH(a, last, depth, -1);
          last = a, depth += 1, limit = ss_ilg(a - first);
        } else {
          first = a, limit = -1;
        }
      } else {
        if(1 < (last - a)) {
          STACK_PUSH(first, a, depth + 1, ss_ilg(a - first));
          first = a, limit = -1;
        } else {
          last = a, depth += 1, limit = ss_ilg(a - first);
        }
      }
      continue;
    }

    /* choose pivot */
    a = ss_pivot(Td, PA, first, last);
    v = Td[PA[*a]];
    SWAP(*first, *a);

    /* partition */
    for(b = first; (++b < last) && ((x = Td[PA[*b]]) == v);) { }
    if(((a = b) < last) && (x < v)) {
      for(; (++b < last) && ((x = Td[PA[*b]]) <= v);) {
        if(x == v) { SWAP(*b, *a); ++a; }
      }
    }
    for(c = last; (b < --c) && ((x = Td[PA[*c]]) == v);) { }
    if((b < (d = c)) && (x > v)) {
      for(; (b < --c) && ((x = Td[PA[*c]]) >= v);) {
        if(x == v) { SWAP(*c, *d); --d; }
      }
    }
    for(; b < c;) {
      SWAP(*b, *c);
      for(; (++b < c) && ((x = Td[PA[*b]]) <= v);) {
        if(x == v) { SWAP(*b, *a); ++a; }
      }
      for(; (b < --c) && ((x = Td[PA[*c]]) >= v);) {
        if(x == v) { SWAP(*c, *d); --d; }
      }
    }

    if(a <= d) {
      c = b - 1;

      if((s = a - first) > (t = b - a)) { s = t; }
      for(e = first, f = b - s; 0 < s; --s, ++e, ++f) { SWAP(*e, *f); }
      if((s = d - c) > (t = last - d - 1)) { s = t; }
      for(e = b, f = last - s; 0 < s; --s, ++e, ++f) { SWAP(*e, *f); }

      a = first + (b - a), c = last - (d - c);
      b = (v <= Td[PA[*a] - 1]) ? a : ss_partition(PA, a, c, depth);

      if((a - first) <= (last - c)) {
        if((last - c) <= (c - b)) {
          STACK_PUSH(b, c, depth + 1, ss_ilg(c - b));
          STACK_PUSH(c, last, depth, limit);
          last = a;
        } else if((a - first) <= (c - b)) {
          STACK_PUSH(c, last, depth, limit);
          STACK_PUSH(b, c, depth + 1, ss_ilg(c - b));
          last = a;
        } else {
          STACK_PUSH(c, last, depth, limit);
          STACK_PUSH(first, a, depth, limit);
          first = b, last = c, depth += 1, limit = ss_ilg(c - b);
        }
      } else {
        if((a - first) <= (c - b)) {
          STACK_PUSH(b, c, depth + 1, ss_ilg(c - b));
          STACK_PUSH(first, a, depth, limit);
          first = c;
        } else if((last - c) <= (c - b)) {
          STACK_PUSH(first, a, depth, limit);
          STACK_PUSH(b, c, depth + 1, ss_ilg(c - b));
          first = c;
        } else {
          STACK_PUSH(first, a, depth, limit);
          STACK_PUSH(c, last, depth, limit);
          first = b, last = c, depth += 1, limit = ss_ilg(c - b);
        }
      }
    } else {
      limit += 1;
      if(Td[PA[*first] - 1] < v) {
        first = ss_partition(PA, first, last, depth);
        limit = ss_ilg(last - first);
      }
      depth += 1;
    }
  }
#undef STACK_SIZE
}

#endif /* (SS_BLOCKSIZE == 0) || (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE) */


/*---------------------------------------------------------------------------*/

#if SS_BLOCKSIZE != 0

static INLINE
void
ss_blockswap(int *a, int *b, int n) {
  int t;
  for(; 0 < n; --n, ++a, ++b) {
    t = *a, *a = *b, *b = t;
  }
}

static INLINE
void
ss_rotate(int *first, int *middle, int *last) {
  int *a, *b, t;
  int l, r;
  l = middle - first, r = last - middle;
  for(; (0 < l) && (0 < r);) {
    if(l == r) { ss_blockswap(first, middle, l); break; }
    if(l < r) {
      a = last - 1, b = middle - 1;
      t = *a;
      do {
        *a-- = *b, *b-- = *a;
        if(b < first) {
          *a = t;
          last = a;
          if((r -= l + 1) <= l) { break; }
          a -= 1, b = middle - 1;
          t = *a;
        }
      } while(1);
    } else {
      a = first, b = middle;
      t = *a;
      do {
        *a++ = *b, *b++ = *a;
        if(last <= b) {
          *a = t;
          first = a + 1;
          if((l -= r + 1) <= r) { break; }
          a += 1, b = middle;
          t = *a;
        }
      } while(1);
    }
  }
}


/*---------------------------------------------------------------------------*/

static
void
ss_inplacemerge(const unsigned char *T, const int *PA,
                int *first, int *middle, int *last,
                int depth) {
  const int *p;
  int *a, *b;
  int len, half;
  int q, r;
  int x;

  for(;;) {
    if(*(last - 1) < 0) { x = 1; p = PA + ~*(last - 1); }
    else                { x = 0; p = PA +  *(last - 1); }
    for(a = first, len = middle - first, half = len >> 1, r = -1;
        0 < len;
        len = half, half >>= 1) {
      b = a + half;
      q = ss_compare(T, PA + ((0 <= *b) ? *b : ~*b), p, depth);
      if(q < 0) {
        a = b + 1;
        half -= (len & 1) ^ 1;
      } else {
        r = q;
      }
    }
    if(a < middle) {
      if(r == 0) { *a = ~*a; }
      ss_rotate(a, middle, last);
      last -= middle - a;
      middle = a;
      if(first == middle) { break; }
    }
    --last;
    if(x != 0) { while(*--last < 0) { } }
    if(middle == last) { break; }
  }
}


/*---------------------------------------------------------------------------*/

/* Merge-forward with internal buffer. */
static
void
ss_mergeforward(const unsigned char *T, const int *PA,
                int *first, int *middle, int *last,
                int *buf, int depth) {
  int *a, *b, *c, *bufend;
  int t;
  int r;

  bufend = buf + (middle - first) - 1;
  ss_blockswap(buf, first, middle - first);

  for(t = *(a = first), b = buf, c = middle;;) {
    r = ss_compare(T, PA + *b, PA + *c, depth);
    if(r < 0) {
      do {
        *a++ = *b;
        if(bufend <= b) { *bufend = t; return; }
        *b++ = *a;
      } while(*b < 0);
    } else if(r > 0) {
      do {
        *a++ = *c, *c++ = *a;
        if(last <= c) {
          while(b < bufend) { *a++ = *b, *b++ = *a; }
          *a = *b, *b = t;
          return;
        }
      } while(*c < 0);
    } else {
      *c = ~*c;
      do {
        *a++ = *b;
        if(bufend <= b) { *bufend = t; return; }
        *b++ = *a;
      } while(*b < 0);

      do {
        *a++ = *c, *c++ = *a;
        if(last <= c) {
          while(b < bufend) { *a++ = *b, *b++ = *a; }
          *a = *b, *b = t;
          return;
        }
      } while(*c < 0);
    }
  }
}

/* Merge-backward with internal buffer. */
static
void
ss_mergebackward(const unsigned char *T, const int *PA,
                 int *first, int *middle, int *last,
                 int *buf, int depth) {
  const int *p1, *p2;
  int *a, *b, *c, *bufend;
  int t;
  int r;
  int x;

  bufend = buf + (last - middle) - 1;
  ss_blockswap(buf, middle, last - middle);

  x = 0;
  if(*bufend < 0)       { p1 = PA + ~*bufend; x |= 1; }
  else                  { p1 = PA +  *bufend; }
  if(*(middle - 1) < 0) { p2 = PA + ~*(middle - 1); x |= 2; }
  else                  { p2 = PA +  *(middle - 1); }
  for(t = *(a = last - 1), b = bufend, c = middle - 1;;) {
    r = ss_compare(T, p1, p2, depth);
    if(0 < r) {
      if(x & 1) { do { *a-- = *b, *b-- = *a; } while(*b < 0); x ^= 1; }
      *a-- = *b;
      if(b <= buf) { *buf = t; break; }
      *b-- = *a;
      if(*b < 0) { p1 = PA + ~*b; x |= 1; }
      else       { p1 = PA +  *b; }
    } else if(r < 0) {
      if(x & 2) { do { *a-- = *c, *c-- = *a; } while(*c < 0); x ^= 2; }
      *a-- = *c, *c-- = *a;
      if(c < first) {
        while(buf < b) { *a-- = *b, *b-- = *a; }
        *a = *b, *b = t;
        break;
      }
      if(*c < 0) { p2 = PA + ~*c; x |= 2; }
      else       { p2 = PA +  *c; }
    } else {
      if(x & 1) { do { *a-- = *b, *b-- = *a; } while(*b < 0); x ^= 1; }
      *a-- = ~*b;
      if(b <= buf) { *buf = t; break; }
      *b-- = *a;
      if(x & 2) { do { *a-- = *c, *c-- = *a; } while(*c < 0); x ^= 2; }
      *a-- = *c, *c-- = *a;
      if(c < first) {
        while(buf < b) { *a-- = *b, *b-- = *a; }
        *a = *b, *b = t;
        break;
      }
      if(*b < 0) { p1 = PA + ~*b; x |= 1; }
      else       { p1 = PA +  *b; }
      if(*c < 0) { p2 = PA + ~*c; x |= 2; }
      else       { p2 = PA +  *c; }
    }
  }
}

/* D&C based merge. */
static
void
ss_swapmerge(const unsigned char *T, const int *PA,
             int *first, int *middle, int *last,
             int *buf, int bufsize, int depth) {
#define STACK_SIZE SS_SMERGE_STACKSIZE
#define GETIDX(a) ((0 <= (a)) ? (a) : (~(a)))
#define MERGE_CHECK(a, b, c)\
  do {\
    if(((c) & 1) ||\
       (((c) & 2) && (ss_compare(T, PA + GETIDX(*((a) - 1)), PA + *(a), depth) == 0))) {\
      *(a) = ~*(a);\
    }\
    if(((c) & 4) && ((ss_compare(T, PA + GETIDX(*((b) - 1)), PA + *(b), depth) == 0))) {\
      *(b) = ~*(b);\
    }\
  } while(0)
  struct { int *a, *b, *c; int d; } stack[STACK_SIZE];
  int *l, *r, *lm, *rm;
  int m, len, half;
  int ssize;
  int check, next;

  for(check = 0, ssize = 0;;) {
    if((last - middle) <= bufsize) {
      if((first < middle) && (middle < last)) {
        ss_mergebackward(T, PA, first, middle, last, buf, depth);
      }
      MERGE_CHECK(first, last, check);
      STACK_POP(first, middle, last, check);
      continue;
    }

    if((middle - first) <= bufsize) {
      if(first < middle) {
        ss_mergeforward(T, PA, first, middle, last, buf, depth);
      }
      MERGE_CHECK(first, last, check);
      STACK_POP(first, middle, last, check);
      continue;
    }

    for(m = 0, len = MIN(middle - first, last - middle), half = len >> 1;
        0 < len;
        len = half, half >>= 1) {
      if(ss_compare(T, PA + GETIDX(*(middle + m + half)),
                       PA + GETIDX(*(middle - m - half - 1)), depth) < 0) {
        m += half + 1;
        half -= (len & 1) ^ 1;
      }
    }

    if(0 < m) {
      lm = middle - m, rm = middle + m;
      ss_blockswap(lm, middle, m);
      l = r = middle, next = 0;
      if(rm < last) {
        if(*rm < 0) {
          *rm = ~*rm;
          if(first < lm) { for(; *--l < 0;) { } next |= 4; }
          next |= 1;
        } else if(first < lm) {
          for(; *r < 0; ++r) { }
          next |= 2;
        }
      }

      if((l - first) <= (last - r)) {
        STACK_PUSH(r, rm, last, (next & 3) | (check & 4));
        middle = lm, last = l, check = (check & 3) | (next & 4);
      } else {
        if((next & 2) && (r == middle)) { next ^= 6; }
        STACK_PUSH(first, lm, l, (check & 3) | (next & 4));
        first = r, middle = rm, check = (next & 3) | (check & 4);
      }
    } else {
      if(ss_compare(T, PA + GETIDX(*(middle - 1)), PA + *middle, depth) == 0) {
        *middle = ~*middle;
      }
      MERGE_CHECK(first, last, check);
      STACK_POP(first, middle, last, check);
    }
  }
#undef STACK_SIZE
}

#endif /* SS_BLOCKSIZE != 0 */


/*---------------------------------------------------------------------------*/

/* Substring sort */
static
void
sssort(const unsigned char *T, const int *PA,
       int *first, int *last,
       int *buf, int bufsize,
       int depth, int n, int lastsuffix) {
  int *a;
#if SS_BLOCKSIZE != 0
  int *b, *middle, *curbuf;
  int j, k, curbufsize, limit;
#endif
  int i;

  if(lastsuffix != 0) { ++first; }

#if SS_BLOCKSIZE == 0
  ss_mintrosort(T, PA, first, last, depth);
#else
  if((bufsize < SS_BLOCKSIZE) &&
      (bufsize < (last - first)) &&
      (bufsize < (limit = ss_isqrt(last - first)))) {
    if(SS_BLOCKSIZE < limit) { limit = SS_BLOCKSIZE; }
    buf = middle = last - limit, bufsize = limit;
  } else {
    middle = last, limit = 0;
  }
  for(a = first, i = 0; SS_BLOCKSIZE < (middle - a); a += SS_BLOCKSIZE, ++i) {
#if SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE
    ss_mintrosort(T, PA, a, a + SS_BLOCKSIZE, depth);
#elif 1 < SS_BLOCKSIZE
    ss_insertionsort(T, PA, a, a + SS_BLOCKSIZE, depth);
#endif
    curbufsize = last - (a + SS_BLOCKSIZE);
    curbuf = a + SS_BLOCKSIZE;
    if(curbufsize <= bufsize) { curbufsize = bufsize, curbuf = buf; }
    for(b = a, k = SS_BLOCKSIZE, j = i; j & 1; b -= k, k <<= 1, j >>= 1) {
      ss_swapmerge(T, PA, b - k, b, b + k, curbuf, curbufsize, depth);
    }
  }
#if SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE
  ss_mintrosort(T, PA, a, middle, depth);
#elif 1 < SS_BLOCKSIZE
  ss_insertionsort(T, PA, a, middle, depth);
#endif
  for(k = SS_BLOCKSIZE; i != 0; k <<= 1, i >>= 1) {
    if(i & 1) {
      ss_swapmerge(T, PA, a - k, a, middle, buf, bufsize, depth);
      a -= k;
    }
  }
  if(limit != 0) {
#if SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE
    ss_mintrosort(T, PA, middle, last, depth);
#elif 1 < SS_BLOCKSIZE
    ss_insertionsort(T, PA, middle, last, depth);
#endif
    ss_inplacemerge(T, PA, first, middle, last, depth);
  }
#endif

  if(lastsuffix != 0) {
    /* Insert last type B* suffix. */
    int PAi[2]; PAi[0] = PA[*(first - 1)], PAi[1] = n - 2;
    for(a = first, i = *(first - 1);
        (a < last) && ((*a < 0) || (0 < ss_compare(T, &(PAi[0]), PA + *a, depth)));
        ++a) {
      *(a - 1) = *a;
    }
    *(a - 1) = i;
  }
}


/*---------------------------------------------------------------------------*/

static INLINE
int
tr_ilg(int n) {
  return (n & 0xffff0000) ?
          ((n & 0xff000000) ?
            24 + lg_table[(n >> 24) & 0xff] :
            16 + lg_table[(n >> 16) & 0xff]) :
          ((n & 0x0000ff00) ?
             8 + lg_table[(n >>  8) & 0xff] :
             0 + lg_table[(n >>  0) & 0xff]);
}


/*---------------------------------------------------------------------------*/

/* Simple insertionsort for small size groups. */
static
void
tr_insertionsort(const int *ISAd, int *first, int *last) {
  int *a, *b;
  int t, r;

  for(a = first + 1; a < last; ++a) {
    for(t = *a, b = a - 1; 0 > (r = ISAd[t] - ISAd[*b]);) {
      do { *(b + 1) = *b; } while((first <= --b) && (*b < 0));
      if(b < first) { break; }
    }
    if(r == 0) { *b = ~*b; }
    *(b + 1) = t;
  }
}


/*---------------------------------------------------------------------------*/

static INLINE
void
tr_fixdown(const int *ISAd, int *SA, int i, int size) {
  int j, k;
  int v;
  int c, d, e;

  for(v = SA[i], c = ISAd[v]; (j = 2 * i + 1) < size; SA[i] = SA[k], i = k) {
    d = ISAd[SA[k = j++]];
    if(d < (e = ISAd[SA[j]])) { k = j; d = e; }
    if(d <= c) { break; }
  }
  SA[i] = v;
}

/* Simple top-down heapsort. */
static
void
tr_heapsort(const int *ISAd, int *SA, int size) {
  int i, m;
  int t;

  m = size;
  if((size % 2) == 0) {
    m--;
    if(ISAd[SA[m / 2]] < ISAd[SA[m]]) { SWAP(SA[m], SA[m / 2]); }
  }

  for(i = m / 2 - 1; 0 <= i; --i) { tr_fixdown(ISAd, SA, i, m); }
  if((size % 2) == 0) { SWAP(SA[0], SA[m]); tr_fixdown(ISAd, SA, 0, m); }
  for(i = m - 1; 0 < i; --i) {
    t = SA[0], SA[0] = SA[i];
    tr_fixdown(ISAd, SA, 0, i);
    SA[i] = t;
  }
}


/*---------------------------------------------------------------------------*/

/* Returns the median of three elements. */
static INLINE
int *
tr_median3(const int *ISAd, int *v1, int *v2, int *v3) {
  int *t;
  if(ISAd[*v1] > ISAd[*v2]) { SWAP(v1, v2); }
  if(ISAd[*v2] > ISAd[*v3]) {
    if(ISAd[*v1] > ISAd[*v3]) { return v1; }
    else { return v3; }
  }
  return v2;
}

/* Returns the median of five elements. */
static INLINE
int *
tr_median5(const int *ISAd,
           int *v1, int *v2, int *v3, int *v4, int *v5) {
  int *t;
  if(ISAd[*v2] > ISAd[*v3]) { SWAP(v2, v3); }
  if(ISAd[*v4] > ISAd[*v5]) { SWAP(v4, v5); }
  if(ISAd[*v2] > ISAd[*v4]) { SWAP(v2, v4); SWAP(v3, v5); }
  if(ISAd[*v1] > ISAd[*v3]) { SWAP(v1, v3); }
  if(ISAd[*v1] > ISAd[*v4]) { SWAP(v1, v4); SWAP(v3, v5); }
  if(ISAd[*v3] > ISAd[*v4]) { return v4; }
  return v3;
}

/* Returns the pivot element. */
static INLINE
int *
tr_pivot(const int *ISAd, int *first, int *last) {
  int *middle;
  int t;

  t = last - first;
  middle = first + t / 2;

  if(t <= 512) {
    if(t <= 32) {
      return tr_median3(ISAd, first, middle, last - 1);
    } else {
      t >>= 2;
      return tr_median5(ISAd, first, first + t, middle, last - 1 - t, last - 1);
    }
  }
  t >>= 3;
  first  = tr_median3(ISAd, first, first + t, first + (t << 1));
  middle = tr_median3(ISAd, middle - t, middle, middle + t);
  last   = tr_median3(ISAd, last - 1 - (t << 1), last - 1 - t, last - 1);
  return tr_median3(ISAd, first, middle, last);
}


/*---------------------------------------------------------------------------*/

typedef struct _trbudget_t trbudget_t;
struct _trbudget_t {
  int chance;
  int remain;
  int incval;
  int count;
};

static INLINE
void
trbudget_init(trbudget_t *budget, int chance, int incval) {
  budget->chance = chance;
  budget->remain = budget->incval = incval;
}

static INLINE
int
trbudget_check(trbudget_t *budget, int size) {
  if(size <= budget->remain) { budget->remain -= size; return 1; }
  if(budget->chance == 0) { budget->count += size; return 0; }
  budget->remain += budget->incval - size;
  budget->chance -= 1;
  return 1;
}


/*---------------------------------------------------------------------------*/

static INLINE
void
tr_partition(const int *ISAd,
             int *first, int *middle, int *last,
             int **pa, int **pb, int v) {
  int *a, *b, *c, *d, *e, *f;
  int t, s;
  int x = 0;

  for(b = middle - 1; (++b < last) && ((x = ISAd[*b]) == v);) { }
  if(((a = b) < last) && (x < v)) {
    for(; (++b < last) && ((x = ISAd[*b]) <= v);) {
      if(x == v) { SWAP(*b, *a); ++a; }
    }
  }
  for(c = last; (b < --c) && ((x = ISAd[*c]) == v);) { }
  if((b < (d = c)) && (x > v)) {
    for(; (b < --c) && ((x = ISAd[*c]) >= v);) {
      if(x == v) { SWAP(*c, *d); --d; }
    }
  }
  for(; b < c;) {
    SWAP(*b, *c);
    for(; (++b < c) && ((x = ISAd[*b]) <= v);) {
      if(x == v) { SWAP(*b, *a); ++a; }
    }
    for(; (b < --c) && ((x = ISAd[*c]) >= v);) {
      if(x == v) { SWAP(*c, *d); --d; }
    }
  }

  if(a <= d) {
    c = b - 1;
    if((s = a - first) > (t = b - a)) { s = t; }
    for(e = first, f = b - s; 0 < s; --s, ++e, ++f) { SWAP(*e, *f); }
    if((s = d - c) > (t = last - d - 1)) { s = t; }
    for(e = b, f = last - s; 0 < s; --s, ++e, ++f) { SWAP(*e, *f); }
    first += (b - a), last -= (d - c);
  }
  *pa = first, *pb = last;
}

static
void
tr_copy(int *ISA, const int *SA,
        int *first, int *a, int *b, int *last,
        int depth) {
  /* sort suffixes of middle partition
     by using sorted order of suffixes of left and right partition. */
  int *c, *d, *e;
  int s, v;

  v = b - SA - 1;
  for(c = first, d = a - 1; c <= d; ++c) {
    if((0 <= (s = *c - depth)) && (ISA[s] == v)) {
      *++d = s;
      ISA[s] = d - SA;
    }
  }
  for(c = last - 1, e = d + 1, d = b; e < d; --c) {
    if((0 <= (s = *c - depth)) && (ISA[s] == v)) {
      *--d = s;
      ISA[s] = d - SA;
    }
  }
}

static
void
tr_partialcopy(int *ISA, const int *SA,
               int *first, int *a, int *b, int *last,
               int depth) {
  int *c, *d, *e;
  int s, v;
  int rank, lastrank, newrank = -1;

  v = b - SA - 1;
  lastrank = -1;
  for(c = first, d = a - 1; c <= d; ++c) {
    if((0 <= (s = *c - depth)) && (ISA[s] == v)) {
      *++d = s;
      rank = ISA[s + depth];
      if(lastrank != rank) { lastrank = rank; newrank = d - SA; }
      ISA[s] = newrank;
    }
  }

  lastrank = -1;
  for(e = d; first <= e; --e) {
    rank = ISA[*e];
    if(lastrank != rank) { lastrank = rank; newrank = e - SA; }
    if(newrank != rank) { ISA[*e] = newrank; }
  }

  lastrank = -1;
  for(c = last - 1, e = d + 1, d = b; e < d; --c) {
    if((0 <= (s = *c - depth)) && (ISA[s] == v)) {
      *--d = s;
      rank = ISA[s + depth];
      if(lastrank != rank) { lastrank = rank; newrank = d - SA; }
      ISA[s] = newrank;
    }
  }
}

static
void
tr_introsort(int *ISA, const int *ISAd,
             int *SA, int *first, int *last,
             trbudget_t *budget) {
#define STACK_SIZE TR_STACKSIZE
  struct { const int *a; int *b, *c; int d, e; }stack[STACK_SIZE];
  int *a, *b, *c;
  int t;
  int v, x = 0;
  int incr = ISAd - ISA;
  int limit, next;
  int ssize, trlink = -1;

  for(ssize = 0, limit = tr_ilg(last - first);;) {

    if(limit < 0) {
      if(limit == -1) {
        /* tandem repeat partition */
        tr_partition(ISAd - incr, first, first, last, &a, &b, last - SA - 1);

        /* update ranks */
        if(a < last) {
          for(c = first, v = a - SA - 1; c < a; ++c) { ISA[*c] = v; }
        }
        if(b < last) {
          for(c = a, v = b - SA - 1; c < b; ++c) { ISA[*c] = v; }
        }

        /* push */
        if(1 < (b - a)) {
          STACK_PUSH5(NULL, a, b, 0, 0);
          STACK_PUSH5(ISAd - incr, first, last, -2, trlink);
          trlink = ssize - 2;
        }
        if((a - first) <= (last - b)) {
          if(1 < (a - first)) {
            STACK_PUSH5(ISAd, b, last, tr_ilg(last - b), trlink);
            last = a, limit = tr_ilg(a - first);
          } else if(1 < (last - b)) {
            first = b, limit = tr_ilg(last - b);
          } else {
            STACK_POP5(ISAd, first, last, limit, trlink);
          }
        } else {
          if(1 < (last - b)) {
            STACK_PUSH5(ISAd, first, a, tr_ilg(a - first), trlink);
            first = b, limit = tr_ilg(last - b);
          } else if(1 < (a - first)) {
            last = a, limit = tr_ilg(a - first);
          } else {
            STACK_POP5(ISAd, first, last, limit, trlink);
          }
        }
      } else if(limit == -2) {
        /* tandem repeat copy */
        a = stack[--ssize].b, b = stack[ssize].c;
        if(stack[ssize].d == 0) {
          tr_copy(ISA, SA, first, a, b, last, ISAd - ISA);
        } else {
          if(0 <= trlink) { stack[trlink].d = -1; }
          tr_partialcopy(ISA, SA, first, a, b, last, ISAd - ISA);
        }
        STACK_POP5(ISAd, first, last, limit, trlink);
      } else {
        /* sorted partition */
        if(0 <= *first) {
          a = first;
          do { ISA[*a] = a - SA; } while((++a < last) && (0 <= *a));
          first = a;
        }
        if(first < last) {
          a = first; do { *a = ~*a; } while(*++a < 0);
          next = (ISA[*a] != ISAd[*a]) ? tr_ilg(a - first + 1) : -1;
          if(++a < last) { for(b = first, v = a - SA - 1; b < a; ++b) { ISA[*b] = v; } }

          /* push */
          if(trbudget_check(budget, a - first)) {
            if((a - first) <= (last - a)) {
              STACK_PUSH5(ISAd, a, last, -3, trlink);
              ISAd += incr, last = a, limit = next;
            } else {
              if(1 < (last - a)) {
                STACK_PUSH5(ISAd + incr, first, a, next, trlink);
                first = a, limit = -3;
              } else {
                ISAd += incr, last = a, limit = next;
              }
            }
          } else {
            if(0 <= trlink) { stack[trlink].d = -1; }
            if(1 < (last - a)) {
              first = a, limit = -3;
            } else {
              STACK_POP5(ISAd, first, last, limit, trlink);
            }
          }
        } else {
          STACK_POP5(ISAd, first, last, limit, trlink);
        }
      }
      continue;
    }

    if((last - first) <= TR_INSERTIONSORT_THRESHOLD) {
      tr_insertionsort(ISAd, first, last);
      limit = -3;
      continue;
    }

    if(limit-- == 0) {
      tr_heapsort(ISAd, first, last - first);
      for(a = last - 1; first < a; a = b) {
        for(x = ISAd[*a], b = a - 1; (first <= b) && (ISAd[*b] == x); --b) { *b = ~*b; }
      }
      limit = -3;
      continue;
    }

    /* choose pivot */
    a = tr_pivot(ISAd, first, last);
    SWAP(*first, *a);
    v = ISAd[*first];

    /* partition */
    tr_partition(ISAd, first, first + 1, last, &a, &b, v);
    if((last - first) != (b - a)) {
      next = (ISA[*a] != v) ? tr_ilg(b - a) : -1;

      /* update ranks */
      for(c = first, v = a - SA - 1; c < a; ++c) { ISA[*c] = v; }
      if(b < last) { for(c = a, v = b - SA - 1; c < b; ++c) { ISA[*c] = v; } }

      /* push */
      if((1 < (b - a)) && (trbudget_check(budget, b - a))) {
        if((a - first) <= (last - b)) {
          if((last - b) <= (b - a)) {
            if(1 < (a - first)) {
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              STACK_PUSH5(ISAd, b, last, limit, trlink);
              last = a;
            } else if(1 < (last - b)) {
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              first = b;
            } else {
              ISAd += incr, first = a, last = b, limit = next;
            }
          } else if((a - first) <= (b - a)) {
            if(1 < (a - first)) {
              STACK_PUSH5(ISAd, b, last, limit, trlink);
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              last = a;
            } else {
              STACK_PUSH5(ISAd, b, last, limit, trlink);
              ISAd += incr, first = a, last = b, limit = next;
            }
          } else {
            STACK_PUSH5(ISAd, b, last, limit, trlink);
            STACK_PUSH5(ISAd, first, a, limit, trlink);
            ISAd += incr, first = a, last = b, limit = next;
          }
        } else {
          if((a - first) <= (b - a)) {
            if(1 < (last - b)) {
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              STACK_PUSH5(ISAd, first, a, limit, trlink);
              first = b;
            } else if(1 < (a - first)) {
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              last = a;
            } else {
              ISAd += incr, first = a, last = b, limit = next;
            }
          } else if((last - b) <= (b - a)) {
            if(1 < (last - b)) {
              STACK_PUSH5(ISAd, first, a, limit, trlink);
              STACK_PUSH5(ISAd + incr, a, b, next, trlink);
              first = b;
            } else {
              STACK_PUSH5(ISAd, first, a, limit, trlink);
              ISAd += incr, first = a, last = b, limit = next;
            }
          } else {
            STACK_PUSH5(ISAd, first, a, limit, trlink);
            STACK_PUSH5(ISAd, b, last, limit, trlink);
            ISAd += incr, first = a, last = b, limit = next;
          }
        }
      } else {
        if((1 < (b - a)) && (0 <= trlink)) { stack[trlink].d = -1; }
        if((a - first) <= (last - b)) {
          if(1 < (a - first)) {
            STACK_PUSH5(ISAd, b, last, limit, trlink);
            last = a;
          } else if(1 < (last - b)) {
            first = b;
          } else {
            STACK_POP5(ISAd, first, last, limit, trlink);
          }
        } else {
          if(1 < (last - b)) {
            STACK_PUSH5(ISAd, first, a, limit, trlink);
            first = b;
          } else if(1 < (a - first)) {
            last = a;
          } else {
            STACK_POP5(ISAd, first, last, limit, trlink);
          }
        }
      }
    } else {
      if(trbudget_check(budget, last - first)) {
        limit = tr_ilg(last - first), ISAd += incr;
      } else {
        if(0 <= trlink) { stack[trlink].d = -1; }
        STACK_POP5(ISAd, first, last, limit, trlink);
      }
    }
  }
#undef STACK_SIZE
}



/*---------------------------------------------------------------------------*/

/* Tandem repeat sort */
static
void
trsort(int *ISA, int *SA, int n, int depth) {
  int *ISAd;
  int *first, *last;
  trbudget_t budget;
  int t, skip, unsorted;

  trbudget_init(&budget, tr_ilg(n) * 2 / 3, n);
/*  trbudget_init(&budget, tr_ilg(n) * 3 / 4, n); */
  for(ISAd = ISA + depth; -n < *SA; ISAd += ISAd - ISA) {
    first = SA;
    skip = 0;
    unsorted = 0;
    do {
      if((t = *first) < 0) { first -= t; skip += t; }
      else {
        if(skip != 0) { *(first + skip) = skip; skip = 0; }
        last = SA + ISA[t] + 1;
        if(1 < (last - first)) {
          budget.count = 0;
          tr_introsort(ISA, ISAd, SA, first, last, &budget);
          if(budget.count != 0) { unsorted += budget.count; }
          else { skip = first - last; }
        } else if((last - first) == 1) {
          skip = -1;
        }
        first = last;
      }
    } while(first < (SA + n));
    if(skip != 0) { *(first + skip) = skip; }
    if(unsorted == 0) { break; }
  }
}


/*---------------------------------------------------------------------------*/

/* Sorts suffixes of type B*. */
static
int
sort_typeBstar(const unsigned char *T, int *SA,
               int *bucket_A, int *bucket_B,
               int n) {
  int *PAb, *ISAb, *buf;
#ifdef _OPENMP
  int *curbuf;
  int l;
#endif
  int i, j, k, t, m, bufsize;
  int c0, c1;
#ifdef _OPENMP
  int d0, d1;
  int tmp;
#endif

  /* Initialize bucket arrays. */
  for(i = 0; i < BUCKET_A_SIZE; ++i) { bucket_A[i] = 0; }
  for(i = 0; i < BUCKET_B_SIZE; ++i) { bucket_B[i] = 0; }

  /* Count the number of occurrences of the first one or two characters of each
     type A, B and B* suffix. Moreover, store the beginning position of all
     type B* suffixes into the array SA. */
  for(i = n - 1, m = n, c0 = T[n - 1]; 0 <= i;) {
    /* type A suffix. */
    do { ++BUCKET_A(c1 = c0); } while((0 <= --i) && ((c0 = T[i]) >= c1));
    if(0 <= i) {
      /* type B* suffix. */
      ++BUCKET_BSTAR(c0, c1);
      SA[--m] = i;
      /* type B suffix. */
      for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) <= c1); --i, c1 = c0) {
        ++BUCKET_B(c0, c1);
      }
    }
  }
  m = n - m;
/*
note:
  A type B* suffix is lexicographically smaller than a type B suffix that
  begins with the same first two characters.
*/

  /* Calculate the index of start/end point of each bucket. */
  for(c0 = 0, i = 0, j = 0; c0 < ALPHABET_SIZE; ++c0) {
    t = i + BUCKET_A(c0);
    BUCKET_A(c0) = i + j; /* start point */
    i = t + BUCKET_B(c0, c0);
    for(c1 = c0 + 1; c1 < ALPHABET_SIZE; ++c1) {
      j += BUCKET_BSTAR(c0, c1);
      BUCKET_BSTAR(c0, c1) = j; /* end point */
      i += BUCKET_B(c0, c1);
    }
  }

  if(0 < m) {
    /* Sort the type B* suffixes by their first two characters. */
    PAb = SA + n - m; ISAb = SA + m;
    for(i = m - 2; 0 <= i; --i) {
      t = PAb[i], c0 = T[t], c1 = T[t + 1];
      SA[--BUCKET_BSTAR(c0, c1)] = i;
    }
    t = PAb[m - 1], c0 = T[t], c1 = T[t + 1];
    SA[--BUCKET_BSTAR(c0, c1)] = m - 1;

    /* Sort the type B* substrings using sssort. */
#ifdef _OPENMP
    tmp = omp_get_max_threads();
    buf = SA + m, bufsize = (n - (2 * m)) / tmp;
    c0 = ALPHABET_SIZE - 2, c1 = ALPHABET_SIZE - 1, j = m;
#pragma omp parallel default(shared) private(curbuf, k, l, d0, d1, tmp)
    {
      tmp = omp_get_thread_num();
      curbuf = buf + tmp * bufsize;
      k = 0;
      for(;;) {
        #pragma omp critical(sssort_lock)
        {
          if(0 < (l = j)) {
            d0 = c0, d1 = c1;
            do {
              k = BUCKET_BSTAR(d0, d1);
              if(--d1 <= d0) {
                d1 = ALPHABET_SIZE - 1;
                if(--d0 < 0) { break; }
              }
            } while(((l - k) <= 1) && (0 < (l = k)));
            c0 = d0, c1 = d1, j = k;
          }
        }
        if(l == 0) { break; }
        sssort(T, PAb, SA + k, SA + l,
               curbuf, bufsize, 2, n, *(SA + k) == (m - 1));
      }
    }
#else
    buf = SA + m, bufsize = n - (2 * m);
    for(c0 = ALPHABET_SIZE - 2, j = m; 0 < j; --c0) {
      for(c1 = ALPHABET_SIZE - 1; c0 < c1; j = i, --c1) {
        i = BUCKET_BSTAR(c0, c1);
        if(1 < (j - i)) {
          sssort(T, PAb, SA + i, SA + j,
                 buf, bufsize, 2, n, *(SA + i) == (m - 1));
        }
      }
    }
#endif

    /* Compute ranks of type B* substrings. */
    for(i = m - 1; 0 <= i; --i) {
      if(0 <= SA[i]) {
        j = i;
        do { ISAb[SA[i]] = i; } while((0 <= --i) && (0 <= SA[i]));
        SA[i + 1] = i - j;
        if(i <= 0) { break; }
      }
      j = i;
      do { ISAb[SA[i] = ~SA[i]] = j; } while(SA[--i] < 0);
      ISAb[SA[i]] = j;
    }

    /* Construct the inverse suffix array of type B* suffixes using trsort. */
    trsort(ISAb, SA, m, 1);

    /* Set the sorted order of tyoe B* suffixes. */
    for(i = n - 1, j = m, c0 = T[n - 1]; 0 <= i;) {
      for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) >= c1); --i, c1 = c0) { }
      if(0 <= i) {
        t = i;
        for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) <= c1); --i, c1 = c0) { }
        SA[ISAb[--j]] = ((t == 0) || (1 < (t - i))) ? t : ~t;
      }
    }

    /* Calculate the index of start/end point of each bucket. */
    BUCKET_B(ALPHABET_SIZE - 1, ALPHABET_SIZE - 1) = n; /* end point */
    for(c0 = ALPHABET_SIZE - 2, k = m - 1; 0 <= c0; --c0) {
      i = BUCKET_A(c0 + 1) - 1;
      for(c1 = ALPHABET_SIZE - 1; c0 < c1; --c1) {
        t = i - BUCKET_B(c0, c1);
        BUCKET_B(c0, c1) = i; /* end point */

        /* Move all type B* suffixes to the correct position. */
        for(i = t, j = BUCKET_BSTAR(c0, c1);
            j <= k;
            --i, --k) { SA[i] = SA[k]; }
      }
      BUCKET_BSTAR(c0, c0 + 1) = i - BUCKET_B(c0, c0) + 1; /* start point */
      BUCKET_B(c0, c0) = i; /* end point */
    }
  }

  return m;
}

/* Constructs the suffix array by using the sorted order of type B* suffixes. */
static
void
construct_SA(const unsigned char *T, int *SA,
             int *bucket_A, int *bucket_B,
             int n, int m) {
  int *i, *j, *k;
  int s;
  int c0, c1, c2;

  if(0 < m) {
    /* Construct the sorted order of type B suffixes by using
       the sorted order of type B* suffixes. */
    for(c1 = ALPHABET_SIZE - 2; 0 <= c1; --c1) {
      /* Scan the suffix array from right to left. */
      for(i = SA + BUCKET_BSTAR(c1, c1 + 1),
          j = SA + BUCKET_A(c1 + 1) - 1, k = NULL, c2 = -1;
          i <= j;
          --j) {
        if(0 < (s = *j)) {
          assert(T[s] == c1);
          assert(((s + 1) < n) && (T[s] <= T[s + 1]));
          assert(T[s - 1] <= T[s]);
          *j = ~s;
          c0 = T[--s];
          if((0 < s) && (T[s - 1] > c0)) { s = ~s; }
          if(c0 != c2) {
            if(0 <= c2) { BUCKET_B(c2, c1) = k - SA; }
            k = SA + BUCKET_B(c2 = c0, c1);
          }
          assert(k < j);
          *k-- = s;
        } else {
          assert(((s == 0) && (T[s] == c1)) || (s < 0));
          *j = ~s;
        }
      }
    }
  }

  /* Construct the suffix array by using
     the sorted order of type B suffixes. */
  k = SA + BUCKET_A(c2 = T[n - 1]);
  *k++ = (T[n - 2] < c2) ? ~(n - 1) : (n - 1);
  /* Scan the suffix array from left to right. */
  for(i = SA, j = SA + n; i < j; ++i) {
    if(0 < (s = *i)) {
      assert(T[s - 1] >= T[s]);
      c0 = T[--s];
      if((s == 0) || (T[s - 1] < c0)) { s = ~s; }
      if(c0 != c2) {
        BUCKET_A(c2) = k - SA;
        k = SA + BUCKET_A(c2 = c0);
      }
      assert(i < k);
      *k++ = s;
    } else {
      assert(s < 0);
      *i = ~s;
    }
  }
}

/* Constructs the burrows-wheeler transformed string directly
   by using the sorted order of type B* suffixes. */
static
int
construct_BWT(const unsigned char *T, int *SA,
              int *bucket_A, int *bucket_B,
              int n, int m) {
  int *i, *j, *k, *orig;
  int s;
  int c0, c1, c2;

  if(0 < m) {
    /* Construct the sorted order of type B suffixes by using
       the sorted order of type B* suffixes. */
    for(c1 = ALPHABET_SIZE - 2; 0 <= c1; --c1) {
      /* Scan the suffix array from right to left. */
      for(i = SA + BUCKET_BSTAR(c1, c1 + 1),
          j = SA + BUCKET_A(c1 + 1) - 1, k = NULL, c2 = -1;
          i <= j;
          --j) {
        if(0 < (s = *j)) {
          assert(T[s] == c1);
          assert(((s + 1) < n) && (T[s] <= T[s + 1]));
          assert(T[s - 1] <= T[s]);
          c0 = T[--s];
          *j = ~((int)c0);
          if((0 < s) && (T[s - 1] > c0)) { s = ~s; }
          if(c0 != c2) {
            if(0 <= c2) { BUCKET_B(c2, c1) = k - SA; }
            k = SA + BUCKET_B(c2 = c0, c1);
          }
          assert(k < j);
          *k-- = s;
        } else if(s != 0) {
          *j = ~s;
#ifndef NDEBUG
        } else {
          assert(T[s] == c1);
#endif
        }
      }
    }
  }

  /* Construct the BWTed string by using
     the sorted order of type B suffixes. */
  k = SA + BUCKET_A(c2 = T[n - 1]);
  *k++ = (T[n - 2] < c2) ? ~((int)T[n - 2]) : (n - 1);
  /* Scan the suffix array from left to right. */
  for(i = SA, j = SA + n, orig = SA; i < j; ++i) {
    if(0 < (s = *i)) {
      assert(T[s - 1] >= T[s]);
      c0 = T[--s];
      *i = c0;
      if((0 < s) && (T[s - 1] < c0)) { s = ~((int)T[s - 1]); }
      if(c0 != c2) {
        BUCKET_A(c2) = k - SA;
        k = SA + BUCKET_A(c2 = c0);
      }
      assert(i < k);
      *k++ = s;
    } else if(s != 0) {
      *i = ~s;
    } else {
      orig = i;
    }
  }

  return orig - SA;
}


/*---------------------------------------------------------------------------*/

/*- Function -*/

int
divsufsort(const unsigned char *T, int *SA, int n) {
  int *bucket_A, *bucket_B;
  int m;
  int err = 0;

  /* Check arguments. */
  if((T == NULL) || (SA == NULL) || (n < 0)) { return -1; }
  else if(n == 0) { return 0; }
  else if(n == 1) { SA[0] = 0; return 0; }
  else if(n == 2) { m = (T[0] < T[1]); SA[m ^ 1] = 0, SA[m] = 1; return 0; }

  bucket_A = (int *)malloc(BUCKET_A_SIZE * sizeof(int));
  bucket_B = (int *)malloc(BUCKET_B_SIZE * sizeof(int));

  /* Suffixsort. */
  if((bucket_A != NULL) && (bucket_B != NULL)) {
    m = sort_typeBstar(T, SA, bucket_A, bucket_B, n);
    construct_SA(T, SA, bucket_A, bucket_B, n, m);
  } else {
    err = -2;
  }

  free(bucket_B);
  free(bucket_A);

  return err;
}

int
divbwt(const unsigned char *T, unsigned char *U, int *A, int n) {
  int *B;
  int *bucket_A, *bucket_B;
  int m, pidx, i;

  /* Check arguments. */
  if((T == NULL) || (U == NULL) || (n < 0)) { return -1; }
  else if(n <= 1) { if(n == 1) { U[0] = T[0]; } return n; }

  if((B = A) == NULL) { B = (int *)malloc((size_t)(n + 1) * sizeof(int)); }
  bucket_A = (int *)malloc(BUCKET_A_SIZE * sizeof(int));
  bucket_B = (int *)malloc(BUCKET_B_SIZE * sizeof(int));

  /* Burrows-Wheeler Transform. */
  if((B != NULL) && (bucket_A != NULL) && (bucket_B != NULL)) {
    m = sort_typeBstar(T, B, bucket_A, bucket_B, n);
    pidx = construct_BWT(T, B, bucket_A, bucket_B, n, m);

    /* Copy to output string. */
    U[0] = T[n - 1];
    for(i = 0; i < pidx; ++i) { U[i + 1] = (unsigned char)B[i]; }
    for(i += 1; i < n; ++i) { U[i] = (unsigned char)B[i]; }
    pidx += 1;
  } else {
    pidx = -2;
  }

  free(bucket_B);
  free(bucket_A);
  if(A == NULL) { free(B); }

  return pidx;
}

// End divsufsort.c

/////////////////////////////// add ///////////////////////////////////

// Convert non-negative decimal number x to string of at least n digits
std::string itos(int64_t x, int n=1) {
  assert(x>=0);
  assert(n>=0);
  std::string r;
  for (; x || n>0; x/=10, --n) r=std::string(1, '0'+x%10)+r;
  return r;
}

// E8E9 transform of buf[0..n-1] to improve compression of .exe and .dll.
// Patterns (E8|E9 xx xx xx 00|FF) at offset i replace the 3 middle
// bytes with x+i mod 2^24, LSB first, reading backward.
void e8e9(unsigned char* buf, int n) {
  for (int i=n-5; i>=0; --i) {
    if (((buf[i]&254)==0xe8) && ((buf[i+4]+1)&254)==0) {
      unsigned a=(buf[i+1]|buf[i+2]<<8|buf[i+3]<<16)+i;
      buf[i+1]=a;
      buf[i+2]=a>>8;
      buf[i+3]=a>>16;
    }
  }
}

// Encode inbuf to buf using LZ77. args are as follows:
// args[0] is log2 buffer size in MB.
// args[1] is level (1=var. length, 2=byte aligned lz77, 3=bwt) + 4 if E8E9.
// args[2] is the lz77 minimum match length and context order.
// args[3] is the lz77 higher context order to search first, or else 0.
// args[4] is the log2 hash bucket size (number of searches).
// args[5] is the log2 hash table size. If 21+args[0] then use a suffix array.
// args[6] is the secondary context look ahead
// sap is pointer to external suffix array of inbuf or 0. If supplied and
//   args[0]=5..7 then it is assumed that E8E9 was already applied to
//   both the input and sap and the input buffer is not modified.

class LZBuffer: public libzpaq::Reader {
  libzpaq::Array<unsigned> ht;// hash table, confirm in low bits, or SA+ISA
  const unsigned char* in;    // input pointer
  const int checkbits;        // hash confirmation size or lg(ISA size)
  const int level;            // 1=var length LZ77, 2=byte aligned LZ77, 3=BWT
  const unsigned htsize;      // size of hash table
  const unsigned n;           // input length
  unsigned i;                 // current location in in (0 <= i < n)
  const unsigned minMatch;    // minimum match length
  const unsigned minMatch2;   // second context order or 0 if not used
  const unsigned maxMatch;    // longest match length allowed
  const unsigned maxLiteral;  // longest literal length allowed
  const unsigned lookahead;   // second context look ahead
  unsigned h1, h2;            // low, high order context hashes of in[i..]
  const unsigned bucket;      // number of matches to search per hash - 1
  const unsigned shift1, shift2;  // how far to shift h1, h2 per hash
  const int minMatchBoth;     // max(minMatch, minMatch2)
  const unsigned rb;          // number of level 1 r bits in match code
  unsigned bits;              // pending output bits (level 1)
  unsigned nbits;             // number of bits in bits
  unsigned rpos, wpos;        // read, write pointers
  unsigned idx;               // BWT index
  const unsigned* sa;         // suffix array for BWT or LZ77-SA
  unsigned* isa;              // inverse suffix array for LZ77-SA
  enum {BUFSIZE=1<<14};       // output buffer size
  unsigned char buf[BUFSIZE]; // output buffer

  void write_literal(unsigned i, unsigned& lit);
  void write_match(unsigned len, unsigned off);
  void fill();  // encode to buf

  // write k bits of x
  void putb(unsigned x, int k) {
    x&=(1<<k)-1;
    bits|=x<<nbits;
    nbits+=k;
    while (nbits>7) {
      assert(wpos<BUFSIZE);
      buf[wpos++]=bits, bits>>=8, nbits-=8;
    }
  }

  // write last byte
  void flush() {
    assert(wpos<BUFSIZE);
    if (nbits>0) buf[wpos++]=bits;
    bits=nbits=0;
  }

  // write 1 byte
  void put(int c) {
    assert(wpos<BUFSIZE);
    buf[wpos++]=c;
  }

public:
  LZBuffer(StringBuffer& inbuf, int args[], const unsigned* sap=0);

  // return 1 byte of compressed output (overrides Reader)
  int get() {
    int c=-1;
    if (rpos==wpos) fill();
    if (rpos<wpos) c=buf[rpos++];
    if (rpos==wpos) rpos=wpos=0;
    return c;
  }

  // Read up to p[0..n-1] and return bytes read.
  int read(char* p, int n);
};

// LZ/BWT preprocessor for levels 1..3 compression and e8e9 filter.
// Level 1 uses variable length LZ77 codes like in the lazy compressor:
//
//   00,n,L[n] = n literal bytes
//   mm,mmm,n,ll,r,q (mm > 00) = match 4*n+ll at offset (q<<rb)+r-1
//
// where q is written in 8mm+mmm-8 (0..23) bits with an implied leading 1 bit
// and n is written using interleaved Elias Gamma coding, i.e. the leading
// 1 bit is implied, remaining bits are preceded by a 1 and terminated by
// a 0. e.g. abc is written 1,b,1,c,0. Codes are packed LSB first and
// padded with leading 0 bits in the last byte. r is a number with rb bits,
// where rb = log2(blocksize) - 24.
//
// Level 2 is byte oriented LZ77 with minimum match length m = $4 = args[3]
// with m in 1..64. Lengths and offsets are MSB first:
// 00xxxxxx   x+1 (1..64) literals follow
// yyxxxxxx   y+1 (2..4) offset bytes follow, match length x+m (m..m+63)
//
// Level 3 is BWT with the end of string byte coded as 255 and the
// last 4 bytes giving its position LSB first.

// floor(log2(x)) + 1 = number of bits excluding leading zeros (0..32)
int lg(unsigned x) {
  unsigned r=0;
  if (x>=65536) r=16, x>>=16;
  if (x>=256) r+=8, x>>=8;
  if (x>=16) r+=4, x>>=4;
  assert(x>=0 && x<16);
  return
    "\x00\x01\x02\x02\x03\x03\x03\x03\x04\x04\x04\x04\x04\x04\x04\x04"[x]+r;
}

// return number of 1 bits in x
int nbits(unsigned x) {
  int r;
  for (r=0; x; x>>=1) r+=x&1;
  return r;
}

// Read n bytes of compressed output into p and return number of
// bytes read in 0..n. 0 signals EOF (overrides Reader).
int LZBuffer::read(char* p, int n) {
  if (rpos==wpos) fill();
  int nr=n;
  if (nr>int(wpos-rpos)) nr=wpos-rpos;
  if (nr) memcpy(p, buf+rpos, nr);
  rpos+=nr;
  assert(rpos<=wpos);
  if (rpos==wpos) rpos=wpos=0;
  return nr;
}

LZBuffer::LZBuffer(StringBuffer& inbuf, int args[], const unsigned* sap):
    ht((args[1]&3)==3 ? (inbuf.size()+1)*!sap      // for BWT suffix array
        : args[5]-args[0]<21 ? 1u<<args[5]         // for LZ77 hash table
        : (inbuf.size()*!sap)+(1u<<17<<args[0])),  // for LZ77 SA and ISA
    in(inbuf.data()),
    checkbits(args[5]-args[0]<21 ? 12-args[0] : 17+args[0]),
    level(args[1]&3),
    htsize(ht.size()),
    n(inbuf.size()),
    i(0),
    minMatch(args[2]),
    minMatch2(args[3]),
    maxMatch(BUFSIZE*3),
    maxLiteral(BUFSIZE/4),
    lookahead(args[6]),
    h1(0), h2(0),
    bucket((1<<args[4])-1), 
    shift1(minMatch>0 ? (args[5]-1)/minMatch+1 : 1),
    shift2(minMatch2>0 ? (args[5]-1)/minMatch2+1 : 0),
    minMatchBoth(MAX(minMatch, minMatch2+lookahead)+4),
    rb(args[0]>4 ? args[0]-4 : 0),
    bits(0), nbits(0), rpos(0), wpos(0),
    idx(0), sa(0), isa(0) {
  assert(args[0]>=0);
  assert(n<=(1u<<20<<args[0]));
  assert(args[1]>=1 && args[1]<=7 && args[1]!=4);
  assert(level>=1 && level<=3);
  if ((minMatch<4 && level==1) || (minMatch<1 && level==2))
    error("match length $3 too small");

  // e8e9 transform
  if (args[1]>4 && !sap) e8e9(inbuf.data(), n);

  // build suffix array if not supplied
  if (args[5]-args[0]>=21 || level==3) {  // LZ77-SA or BWT
    if (sap)
      sa=sap;
    else {
      assert(ht.size()>=n);
      assert(ht.size()>0);
      sa=&ht[0];
      if (n>0) divsufsort((const unsigned char*)in, (int*)sa, n);
    }
    if (level<3) {
      assert(ht.size()>=(n*(sap==0))+(1u<<17<<args[0]));
      isa=&ht[n*(sap==0)];
    }
  }
}

// Encode from in to buf until end of input or buf is not empty
void LZBuffer::fill() {

  // BWT
  if (level==3) {
    assert(in || n==0);
    assert(sa);
    for (; wpos<BUFSIZE && i<n+5; ++i) {
      if (i==0) put(n>0 ? in[n-1] : 255);
      else if (i>n) put(idx&255), idx>>=8;
      else if (sa[i-1]==0) idx=i, put(255);
      else put(in[sa[i-1]-1]);
    }
    return;
  }

  // LZ77: scan the input
  unsigned lit=0;  // number of output literals pending
  const unsigned mask=(1<<checkbits)-1;
  while (i<n && wpos*2<BUFSIZE) {

    // Search for longest match, or pick closest in case of tie
    unsigned blen=minMatch-1;  // best match length
    unsigned bp=0;  // pointer to best match
    unsigned blit=0;  // literals before best match
    int bscore=0;  // best cost

    // Look up contexts in suffix array
    if (isa) {
      if (sa[isa[i&mask]]!=i) // rebuild ISA
        for (unsigned j=0; j<n; ++j)
          if ((sa[j]&~mask)==(i&~mask))
            isa[sa[j]&mask]=j;
      for (unsigned h=0; h<=lookahead; ++h) {
        unsigned q=isa[(h+i)&mask];  // location of h+i in SA
        assert(q<n);
        if (sa[q]!=h+i) continue;
        for (int j=-1; j<=1; j+=2) {  // search backward and forward
          for (unsigned k=1; k<=bucket; ++k) {
            unsigned p;  // match to be tested
            if (q+j*k<n && (p=sa[q+j*k]-h)<i) {
              assert(p<n);
              unsigned l, l1;  // length of match, leading literals
              for (l=h; i+l<n && l<maxMatch && in[p+l]==in[i+l]; ++l);
              for (l1=h; l1>0 && in[p+l1-1]==in[i+l1-1]; --l1);
              int score=int(l-l1)*8-lg(i-p)-4*(lit==0 && l1>0)-11;
              for (unsigned a=0; a<h; ++a) score=score*5/8;
              if (score>bscore) blen=l, bp=p, blit=l1, bscore=score;
              if (l<blen || l<minMatch || l>255) break;
            }
          }
        }
        if (bscore<=0 || blen<minMatch) break;
      }
    }

    // Look up contexts in a hash table.
    // Try the longest context orders first. If a match is found, then
    // skip the lower order as a speed optimization.
    else if (level==1 || minMatch<=64) {
      if (minMatch2>0) {
        for (unsigned k=0; k<=bucket; ++k) {
          unsigned p=ht[h2^k];
          if (p && (p&mask)==(in[i+3]&mask)) {
            p>>=checkbits;
            if (p<i && i+blen<=n && in[p+blen-1]==in[i+blen-1]) {
              unsigned l;  // match length from lookahead
              for (l=lookahead; i+l<n && l<maxMatch && in[p+l]==in[i+l]; ++l);
              if (l>=minMatch2+lookahead) {
                int l1;  // length back from lookahead
                for (l1=lookahead; l1>0 && in[p+l1-1]==in[i+l1-1]; --l1);
                assert(l1>=0 && l1<=int(lookahead));
                int score=int(l-l1)*8-lg(i-p)-8*(lit==0 && l1>0)-11;
                if (score>bscore) blen=l, bp=p, blit=l1, bscore=score;
              }
            }
          }
          if (blen>=128) break;
        }
      }

      // Search the lower order context
      if (!minMatch2 || blen<minMatch2) {
        for (unsigned k=0; k<=bucket; ++k) {
          unsigned p=ht[h1^k];
          if (p && i+3<n && (p&mask)==(in[i+3]&mask)) {
            p>>=checkbits;
            if (p<i && i+blen<=n && in[p+blen-1]==in[i+blen-1]) {
              unsigned l;
              for (l=0; i+l<n && l<maxMatch && in[p+l]==in[i+l]; ++l);
              int score=l*8-lg(i-p)-2*(lit>0)-11;
              if (score>bscore) blen=l, bp=p, blit=0, bscore=score;
            }
          }
          if (blen>=128) break;
        }
      }
    }

    // If match is long enough, then output any pending literals first,
    // and then the match. blen is the length of the match.
    assert(i>=bp);
    const unsigned off=i-bp;  // offset
    if (off>0 && bscore>0
        && blen-blit>=minMatch+(level==2)*((off>=(1<<16))+(off>=(1<<24)))) {
      lit+=blit;
      write_literal(i+blit, lit);
      write_match(blen-blit, off);
    }

    // Otherwise add to literal length
    else {
      blen=1;
      ++lit;
    }

    // Update index, advance blen bytes
    if (isa)
      i+=blen;
    else {
      while (blen--) {
        if (i+minMatchBoth<n) {
          unsigned ih=((i*1234547)>>19)&bucket;
          const unsigned p=(i<<checkbits)|(in[i+3]&mask);
          assert(ih<=bucket);
          if (minMatch2) {
            ht[h2^ih]=p;
            h2=(((h2*9)<<shift2)
                +(in[i+minMatch2+lookahead]+1)*23456789u)&(htsize-1);
          }
          ht[h1^ih]=p;
          h1=(((h1*5)<<shift1)+(in[i+minMatch]+1)*123456791u)&(htsize-1);
        }
        ++i;
      }
    }

    // Write long literals to keep buf from filling up
    if (lit>=maxLiteral)
      write_literal(i, lit);
  }

  // Write pending literals at end of input
  assert(i<=n);
  if (i==n) {
    write_literal(n, lit);
    flush();
  }
}

// Write literal sequence in[i-lit..i-1], set lit=0
void LZBuffer::write_literal(unsigned i, unsigned& lit) {
  assert(lit>=0);
  assert(i>=0 && i<=n);
  assert(i>=lit);
  if (level==1) {
    if (lit<1) return;
    int ll=lg(lit);
    assert(ll>=1 && ll<=24);
    putb(0, 2);
    --ll;
    while (--ll>=0) {
      putb(1, 1);
      putb((lit>>ll)&1, 1);
    }
    putb(0, 1);
    while (lit) putb(in[i-lit--], 8);
  }
  else {
    assert(level==2);
    while (lit>0) {
      unsigned lit1=lit;
      if (lit1>64) lit1=64;
      put(lit1-1);
      for (unsigned j=i-lit; j<i-lit+lit1; ++j) put(in[j]);
      lit-=lit1;
    }
  }
}

// Write match sequence of given length and offset
void LZBuffer::write_match(unsigned len, unsigned off) {

  // mm,mmm,n,ll,r,q[mmmmm-8] = match n*4+ll, offset ((q-1)<<rb)+r+1
  if (level==1) {
    assert(len>=minMatch && len<=maxMatch);
    assert(off>0);
    assert(len>=4);
    assert(rb>=0 && rb<=8);
    int ll=lg(len)-1;
    assert(ll>=2);
    off+=(1<<rb)-1;
    int lo=lg(off)-1-rb;
    assert(lo>=0 && lo<=23);
    putb((lo+8)>>3, 2);// mm
    putb(lo&7, 3);     // mmm
    while (--ll>=2) {  // n
      putb(1, 1);
      putb((len>>ll)&1, 1);
    }
    putb(0, 1);
    putb(len&3, 2);    // ll
    putb(off, rb);     // r
    putb(off>>rb, lo); // q
  }

  // x[2]:len[6] off[x-1] 
  else {
    assert(level==2);
    assert(minMatch>=1 && minMatch<=64);
    --off;
    while (len>0) {  // Split long matches to len1=minMatch..minMatch+63
      const unsigned len1=len>minMatch*2+63 ? minMatch+63 :
          len>minMatch+63 ? len-minMatch : len;
      assert(wpos<BUFSIZE-5);
      assert(len1>=minMatch && len1<minMatch+64);
      if (off<(1<<16)) {
        put(64+len1-minMatch);
        put(off>>8);
        put(off);
      }
      else if (off<(1<<24)) {
        put(128+len1-minMatch);
        put(off>>16);
        put(off>>8);
        put(off);
      }
      else {
        put(192+len1-minMatch);
        put(off>>24);
        put(off>>16);
        put(off>>8);
        put(off);
      }
      len-=len1;
    }
  }
}

// Generate a config file from the method argument with syntax:
// {0|x|s|i}[N1[,N2]...][{ciamtswf<cfg>}[N1[,N2]]...]...
std::string makeConfig(const char* method, int args[]) {
  assert(method);
  const char type=method[0];
  assert(type=='x' || type=='s' || type=='0' || type=='i');

  // Read "{x|s|i|0}N1,N2...N9" into args[0..8] ($1..$9)
  args[0]=0;  // log block size in MiB
  args[1]=0;  // 0=none, 1=var-LZ77, 2=byte-LZ77, 3=BWT, 4..7 adds E8E9
  args[2]=0;  // lz77 minimum match length
  args[3]=0;  // secondary context length
  args[4]=0;  // log searches
  args[5]=0;  // lz77 hash table size or SA if args[0]+21
  args[6]=0;  // secondary context look ahead
  args[7]=0;  // not used
  args[8]=0;  // not used
  if (isdigit(*++method)) args[0]=0;
  for (int i=0; i<9 && (isdigit(*method) || *method==',' || *method=='.');) {
    if (isdigit(*method))
      args[i]=args[i]*10+*method-'0';
    else if (++i<9)
      args[i]=0;
    ++method;
  }

  // "0..." = No compression
  if (type=='0')
    return "comp 0 0 0 0 0 hcomp end\n";

  // Generate the postprocessor
  std::string hdr, pcomp;
  const int level=args[1]&3;
  const bool doe8=args[1]>=4 && args[1]<=7;

  // LZ77+Huffman, with or without E8E9
  if (level==1) {
    const int rb=args[0]>4 ? args[0]-4 : 0;
    hdr="comp 9 16 0 $1+20 ";
    pcomp=
    "pcomp lazy2 3 ;\n"
    " (r1 = state\n"
    "  r2 = len - match or literal length\n"
    "  r3 = m - number of offset bits expected\n"
    "  r4 = ptr to buf\n"
    "  r5 = r - low bits of offset\n"
    "  c = bits - input buffer\n"
    "  d = n - number of bits in c)\n"
    "\n"
    "  a> 255 if\n";
    if (doe8)
      pcomp+=
      "    b=0 d=r 4 do (for b=0..d-1, d = end of buf)\n"
      "      a=b a==d ifnot\n"
      "        a+= 4 a<d if\n"
      "          a=*b a&= 254 a== 232 if (e8 or e9?)\n"
      "            c=b b++ b++ b++ b++ a=*b a++ a&= 254 a== 0 if (00 or ff)\n"
      "              b-- a=*b\n"
      "              b-- a<<= 8 a+=*b\n"
      "              b-- a<<= 8 a+=*b\n"
      "              a-=b a++\n"
      "              *b=a a>>= 8 b++\n"
      "              *b=a a>>= 8 b++\n"
      "              *b=a b++\n"
      "            endif\n"
      "            b=c\n"
      "          endif\n"
      "        endif\n"
      "        a=*b out b++\n"
      "      forever\n"
      "    endif\n"
      "\n";
    pcomp+=
    "    (reset state)\n"
    "    a=0 b=0 c=0 d=0 r=a 1 r=a 2 r=a 3 r=a 4\n"
    "    halt\n"
    "  endif\n"
    "\n"
    "  a<<=d a+=c c=a               (bits+=a<<n)\n"
    "  a= 8 a+=d d=a                (n+=8)\n"
    "\n"
    "  (if state==0 (expect new code))\n"
    "  a=r 1 a== 0 if (match code mm,mmm)\n"
    "    a= 1 r=a 2                 (len=1)\n"
    "    a=c a&= 3 a> 0 if          (if (bits&3))\n"
    "      a-- a<<= 3 r=a 3           (m=((bits&3)-1)*8)\n"
    "      a=c a>>= 2 c=a             (bits>>=2)\n"
    "      b=r 3 a&= 7 a+=b r=a 3     (m+=bits&7)\n"
    "      a=c a>>= 3 c=a             (bits>>=3)\n"
    "      a=d a-= 5 d=a              (n-=5)\n"
    "      a= 1 r=a 1                 (state=1)\n"
    "    else (literal, discard 00)\n"
    "      a=c a>>= 2 c=a             (bits>>=2)\n"
    "      d-- d--                    (n-=2)\n"
    "      a= 3 r=a 1                 (state=3)\n"
    "    endif\n"
    "  endif\n"
    "\n"
    "  (while state==1 && n>=3 (expect match length n*4+ll -> r2))\n"
    "  do a=r 1 a== 1 if a=d a> 2 if\n"
    "    a=c a&= 1 a== 1 if         (if bits&1)\n"
    "      a=c a>>= 1 c=a             (bits>>=1)\n"
    "      b=r 2 a=c a&= 1 a+=b a+=b r=a 2 (len+=len+(bits&1))\n"
    "      a=c a>>= 1 c=a             (bits>>=1)\n"
    "      d-- d--                    (n-=2)\n"
    "    else\n"
    "      a=c a>>= 1 c=a             (bits>>=1)\n"
    "      a=r 2 a<<= 2 b=a           (len<<=2)\n"
    "      a=c a&= 3 a+=b r=a 2       (len+=bits&3)\n"
    "      a=c a>>= 2 c=a             (bits>>=2)\n"
    "      d-- d-- d--                (n-=3)\n";
    if (rb)
      pcomp+="      a= 5 r=a 1                 (state=5)\n";
    else
      pcomp+="      a= 2 r=a 1                 (state=2)\n";
    pcomp+=
    "    endif\n"
    "  forever endif endif\n"
    "\n";
    if (rb) pcomp+=  // save r in r5
      "  (if state==5 && n>=8) (expect low bits of offset to put in r5)\n"
      "  a=r 1 a== 5 if a=d a> "+itos(rb-1)+" if\n"
      "    a=c a&= "+itos((1<<rb)-1)+" r=a 5            (save r in r5)\n"
      "    a=c a>>= "+itos(rb)+" c=a\n"
      "    a=d a-= "+itos(rb)+ " d=a\n"
      "    a= 2 r=a 1                   (go to state 2)\n"
      "  endif endif\n"
      "\n";
    pcomp+=
    "  (if state==2 && n>=m) (expect m offset bits)\n"
    "  a=r 1 a== 2 if a=r 3 a>d ifnot\n"
    "    a=c r=a 6 a=d r=a 7          (save c=bits, d=n in r6,r7)\n"
    "    b=r 3 a= 1 a<<=b d=a         (d=1<<m)\n"
    "    a-- a&=c a+=d                (d=offset=bits&((1<<m)-1)|(1<<m))\n";
    if (rb)
      pcomp+=  // insert r into low bits of d
      "    a<<= "+itos(rb)+" d=r 5 a+=d a-= "+itos((1<<rb)-1)+"\n";
    pcomp+=
    "    d=a b=r 4 a=b a-=d c=a       (c=p=(b=ptr)-offset)\n"
    "\n"
    "    (while len-- (copy and output match d bytes from *c to *b))\n"
    "    d=r 2 do a=d a> 0 if d--\n"
    "      a=*c *b=a c++ b++          (buf[ptr++]-buf[p++])\n";
    if (!doe8) pcomp+=" out\n";
    pcomp+=
    "    forever endif\n"
    "    a=b r=a 4\n"
    "\n"
    "    a=r 6 b=r 3 a>>=b c=a        (bits>>=m)\n"
    "    a=r 7 a-=b d=a               (n-=m)\n"
    "    a=0 r=a 1                    (state=0)\n"
    "  endif endif\n"
    "\n"
    "  (while state==3 && n>=2 (expect literal length))\n"
    "  do a=r 1 a== 3 if a=d a> 1 if\n"
    "    a=c a&= 1 a== 1 if         (if bits&1)\n"
    "      a=c a>>= 1 c=a              (bits>>=1)\n"
    "      b=r 2 a&= 1 a+=b a+=b r=a 2 (len+=len+(bits&1))\n"
    "      a=c a>>= 1 c=a              (bits>>=1)\n"
    "      d-- d--                     (n-=2)\n"
    "    else\n"
    "      a=c a>>= 1 c=a              (bits>>=1)\n"
    "      d--                         (--n)\n"
    "      a= 4 r=a 1                  (state=4)\n"
    "    endif\n"
    "  forever endif endif\n"
    "\n"
    "  (if state==4 && n>=8 (expect len literals))\n"
    "  a=r 1 a== 4 if a=d a> 7 if\n"
    "    b=r 4 a=c *b=a\n";
    if (!doe8) pcomp+=" out\n";
    pcomp+=
    "    b++ a=b r=a 4                 (buf[ptr++]=bits)\n"
    "    a=c a>>= 8 c=a                (bits>>=8)\n"
    "    a=d a-= 8 d=a                 (n-=8)\n"
    "    a=r 2 a-- r=a 2 a== 0 if      (if --len<1)\n"
    "      a=0 r=a 1                     (state=0)\n"
    "    endif\n"
    "  endif endif\n"
    "  halt\n"
    "end\n";
  }

  // Byte aligned LZ77, with or without E8E9
  else if (level==2) {
    hdr="comp 9 16 0 $1+20 ";
    pcomp=
    "pcomp lzpre c ;\n"
    "  (Decode LZ77: d=state, M=output buffer, b=size)\n"
    "  a> 255 if (at EOF decode e8e9 and output)\n";
    if (doe8)
      pcomp+=
      "    d=b b=0 do (for b=0..d-1, d = end of buf)\n"
      "      a=b a==d ifnot\n"
      "        a+= 4 a<d if\n"
      "          a=*b a&= 254 a== 232 if (e8 or e9?)\n"
      "            c=b b++ b++ b++ b++ a=*b a++ a&= 254 a== 0 if (00 or ff)\n"
      "              b-- a=*b\n"
      "              b-- a<<= 8 a+=*b\n"
      "              b-- a<<= 8 a+=*b\n"
      "              a-=b a++\n"
      "              *b=a a>>= 8 b++\n"
      "              *b=a a>>= 8 b++\n"
      "              *b=a b++\n"
      "            endif\n"
      "            b=c\n"
      "          endif\n"
      "        endif\n"
      "        a=*b out b++\n"
      "      forever\n"
      "    endif\n";
    pcomp+=
    "    b=0 c=0 d=0 a=0 r=a 1 r=a 2 (reset state)\n"
    "  halt\n"
    "  endif\n"
    "\n"
    "  (in state d==0, expect a new code)\n"
    "  (put length in r1 and inital part of offset in r2)\n"
    "  c=a a=d a== 0 if\n"
    "    a=c a>>= 6 a++ d=a\n"
    "    a== 1 if (literal?)\n"
    "      a+=c r=a 1 a=0 r=a 2\n"
    "    else (3 to 5 byte match)\n"
    "      d++ a=c a&= 63 a+= $3 r=a 1 a=0 r=a 2\n"
    "    endif\n"
    "  else\n"
    "    a== 1 if (writing literal)\n"
    "      a=c *b=a b++\n";
    if (!doe8) pcomp+=" out\n";
    pcomp+=
    "      a=r 1 a-- a== 0 if d=0 endif r=a 1 (if (--len==0) state=0)\n"
    "    else\n"
    "      a> 2 if (reading offset)\n"
    "        a=r 2 a<<= 8 a|=c r=a 2 d-- (off=off<<8|c, --state)\n"
    "      else (state==2, write match)\n"
    "        a=r 2 a<<= 8 a|=c c=a a=b a-=c a-- c=a (c=i-off-1)\n"
    "        d=r 1 (d=len)\n"
    "        do (copy and output d=len bytes)\n"
    "          a=*c *b=a c++ b++\n";
    if (!doe8) pcomp+=" out\n";
    pcomp+=
    "        d-- a=d a> 0 while\n"
    "        (d=state=0. off, len don\'t matter)\n"
    "      endif\n"
    "    endif\n"
    "  endif\n"
    "  halt\n"
    "end\n";
  }

  // BWT with or without E8E9
  else if (level==3) {  // IBWT
    hdr="comp 9 16 $1+20 $1+20 ";  // 2^$1 = block size in MB
    pcomp=
    "pcomp bwtrle c ;\n"
    "\n"
    "  (read BWT, index into M, size in b)\n"
    "  a> 255 ifnot\n"
    "    *b=a b++\n"
    "\n"
    "  (inverse BWT)\n"
    "  elsel\n"
    "\n"
    "    (index in last 4 bytes, put in c and R1)\n"
    "    b-- a=*b\n"
    "    b-- a<<= 8 a+=*b\n"
    "    b-- a<<= 8 a+=*b\n"
    "    b-- a<<= 8 a+=*b c=a r=a 1\n"
    "\n"
    "    (save size in R2)\n"
    "    a=b r=a 2\n"
    "\n"
    "    (count bytes in H[~1..~255, ~0])\n"
    "    do\n"
    "      a=b a> 0 if\n"
    "        b-- a=*b a++ a&= 255 d=a d! *d++\n"
    "      forever\n"
    "    endif\n"
    "\n"
    "    (cumulative counts: H[~i=0..255] = count of bytes before i)\n"
    "    d=0 d! *d= 1 a=0\n"
    "    do\n"
    "      a+=*d *d=a d--\n"
    "    d<>a a! a> 255 a! d<>a until\n"
    "\n"
    "    (build first part of linked list in H[0..idx-1])\n"
    "    b=0 do\n"
    "      a=c a>b if\n"
    "        d=*b d! *d++ d=*d d-- *d=b\n"
    "      b++ forever\n"
    "    endif\n"
    "\n"
    "    (rest of list in H[idx+1..n-1])\n"
    "    b=c b++ c=r 2 do\n"
    "      a=c a>b if\n"
    "        d=*b d! *d++ d=*d d-- *d=b\n"
    "      b++ forever\n"
    "    endif\n"
    "\n";
    if (args[0]<=4) {  // faster IBWT list traversal limited to 16 MB blocks
      pcomp+=
      "    (copy M to low 8 bits of H to reduce cache misses in next loop)\n"
      "    b=0 do\n"
      "      a=c a>b if\n"
      "        d=b a=*d a<<= 8 a+=*b *d=a\n"
      "      b++ forever\n"
      "    endif\n"
      "\n"
      "    (traverse list and output or copy to M)\n"
      "    d=r 1 b=0 do\n"
      "      a=d a== 0 ifnot\n"
      "        a=*d a>>= 8 d=a\n";
      if (doe8) pcomp+=" *b=*d b++\n";
      else      pcomp+=" a=*d out\n";
      pcomp+=
      "      forever\n"
      "    endif\n"
      "\n";
      if (doe8)  // IBWT+E8E9
        pcomp+=
        "    (e8e9 transform to out)\n"
        "    d=b b=0 do (for b=0..d-1, d = end of buf)\n"
        "      a=b a==d ifnot\n"
        "        a+= 4 a<d if\n"
        "          a=*b a&= 254 a== 232 if\n"
        "            c=b b++ b++ b++ b++ a=*b a++ a&= 254 a== 0 if\n"
        "              b-- a=*b\n"
        "              b-- a<<= 8 a+=*b\n"
        "              b-- a<<= 8 a+=*b\n"
        "              a-=b a++\n"
        "              *b=a a>>= 8 b++\n"
        "              *b=a a>>= 8 b++\n"
        "              *b=a b++\n"
        "            endif\n"
        "            b=c\n"
        "          endif\n"
        "        endif\n"
        "        a=*b out b++\n"
        "      forever\n"
        "    endif\n";
      pcomp+=
      "  endif\n"
      "  halt\n"
      "end\n";
    }
    else {  // slower IBWT list traversal for all sized blocks
      if (doe8) {  // E8E9 after IBWT
        pcomp+=
        "    (R2 = output size without EOS)\n"
        "    a=r 2 a-- r=a 2\n"
        "\n"
        "    (traverse list (d = IBWT pointer) and output inverse e8e9)\n"
        "    (C = offset = 0..R2-1)\n"
        "    (R4 = last 4 bytes shifted in from MSB end)\n"
        "    (R5 = temp pending output byte)\n"
        "    c=0 d=r 1 do\n"
        "      a=d a== 0 ifnot\n"
        "        d=*d\n"
        "\n"
        "        (store byte in R4 and shift out to R5)\n"
        "        b=d a=*b a<<= 24 b=a\n"
        "        a=r 4 r=a 5 a>>= 8 a|=b r=a 4\n"
        "\n"
        "        (if E8|E9 xx xx xx 00|FF in R4:R5 then subtract c from x)\n"
        "        a=c a> 3 if\n"
        "          a=r 5 a&= 254 a== 232 if\n"
        "            a=r 4 a>>= 24 b=a a++ a&= 254 a< 2 if\n"
        "              a=r 4 a-=c a+= 4 a<<= 8 a>>= 8 \n"
        "              b<>a a<<= 24 a+=b r=a 4\n"
        "            endif\n"
        "          endif\n"
        "        endif\n"
        "\n"
        "        (output buffered byte)\n"
        "        a=c a> 3 if a=r 5 out endif c++\n"
        "\n"
        "      forever\n"
        "    endif\n"
        "\n"
        "    (output up to 4 pending bytes in R4)\n"
        "    b=r 4\n"
        "    a=c a> 3 a=b if out endif a>>= 8 b=a\n"
        "    a=c a> 2 a=b if out endif a>>= 8 b=a\n"
        "    a=c a> 1 a=b if out endif a>>= 8 b=a\n"
        "    a=c a> 0 a=b if out endif\n"
        "\n"
        "  endif\n"
        "  halt\n"
        "end\n";
      }
      else {
        pcomp+=
        "    (traverse list and output)\n"
        "    d=r 1 do\n"
        "      a=d a== 0 ifnot\n"
        "        d=*d\n"
        "        b=d a=*b out\n"
        "      forever\n"
        "    endif\n"
        "  endif\n"
        "  halt\n"
        "end\n";
      }
    }
  }

  // E8E9 or no preprocessing
  else if (level==0) {
    hdr="comp 9 16 0 0 ";
    if (doe8) { // E8E9?
      pcomp=
      "pcomp e8e9 d ;\n"
      "  a> 255 if\n"
      "    a=c a> 4 if\n"
      "      c= 4\n"
      "    else\n"
      "      a! a+= 5 a<<= 3 d=a a=b a>>=d b=a\n"
      "    endif\n"
      "    do a=c a> 0 if\n"
      "      a=b out a>>= 8 b=a c--\n"
      "    forever endif\n"
      "  else\n"
      "    *b=b a<<= 24 d=a a=b a>>= 8 a+=d b=a c++\n"
      "    a=c a> 4 if\n"
      "      a=*b out\n"
      "      a&= 254 a== 232 if\n"
      "        a=b a>>= 24 a++ a&= 254 a== 0 if\n"
      "          a=b a>>= 24 a<<= 24 d=a\n"
      "          a=b a-=c a+= 5\n"
      "          a<<= 8 a>>= 8 a|=d b=a\n"
      "        endif\n"
      "      endif\n"
      "    endif\n"
      "  endif\n"
      "  halt\n"
      "end\n";
    }
    else
      pcomp="end\n";
  }
  else
    error("Unsupported method");
  
  // Build context model (comp, hcomp) assuming:
  // H[0..254] = contexts
  // H[255..511] = location of last byte i-255
  // M = last 64K bytes, filling backward
  // C = pointer to most recent byte
  // R1 = level 2 lz77 1+bytes expected until next code, 0=init
  // R2 = level 2 lz77 first byte of code
  int ncomp=0;  // number of components
  const int membits=args[0]+20;
  int sb=5;  // bits in last context
  std::string comp;
  std::string hcomp="hcomp\n"
    "c-- *c=a a+= 255 d=a *d=c\n";
  if (level==2) {  // put level 2 lz77 parse state in R1, R2
    hcomp+=
    "  (decode lz77 into M. Codes:\n"
    "  00xxxxxx = literal length xxxxxx+1\n"
    "  xx......, xx > 0 = match with xx offset bytes to follow)\n"
    "\n"
    "  a=r 1 a== 0 if (init)\n"
    "    a= "+itos(111+57*doe8)+" (skip post code)\n"
    "  else a== 1 if  (new code?)\n"
    "    a=*c r=a 2  (save code in R2)\n"
    "    a> 63 if a>>= 6 a++ a++  (match)\n"
    "    else a++ a++ endif  (literal)\n"
    "  else (read rest of code)\n"
    "    a--\n"
    "  endif endif\n"
    "  r=a 1  (R1 = 1+expected bytes to next code)\n";
  }

  // Generate the context model
  while (*method && ncomp<254) {

    // parse command C[N1[,N2]...] into v = {C, N1, N2...}
    std::vector<int> v;
    v.push_back(*method++);
    if (isdigit(*method)) {
      v.push_back(*method++-'0');
      while (isdigit(*method) || *method==',' || *method=='.') {
        if (isdigit(*method))
          v.back()=v.back()*10+*method++-'0';
        else {
          v.push_back(0);
          ++method;
        }
      }
    }

    // c: context model
    // N1%1000: 0=ICM 1..256=CM limit N1-1
    // N1/1000: number of times to halve memory
    // N2: 1..255=offset mod N2. 1000..1255=distance to N2-1000
    // N3...: 0..255=byte mask + 256=lz77 state. 1000+=run of N3-1000 zeros.
    if (v[0]=='c') {
      while (v.size()<3) v.push_back(0);
      comp+=itos(ncomp)+" ";
      sb=11;  // count context bits
      if (v[2]<256) sb+=lg(v[2]);
      else sb+=6;
      for (unsigned i=3; i<v.size(); ++i)
        if (v[i]<512) sb+=nbits(v[i])*3/4;
      if (sb>membits) sb=membits;
      if (v[1]%1000==0) comp+="icm "+itos(sb-6-v[1]/1000)+"\n";
      else comp+="cm "+itos(sb-2-v[1]/1000)+" "+itos(v[1]%1000-1)+"\n";

      // special contexts
      hcomp+="d= "+itos(ncomp)+" *d=0\n";
      if (v[2]>1 && v[2]<=255) {  // periodic context
        if (lg(v[2])!=lg(v[2]-1))
          hcomp+="a=c a&= "+itos(v[2]-1)+" hashd\n";
        else
          hcomp+="a=c a%= "+itos(v[2])+" hashd\n";
      }
      else if (v[2]>=1000 && v[2]<=1255)  // distance context
        hcomp+="a= 255 a+= "+itos(v[2]-1000)+
               " d=a a=*d a-=c a> 255 if a= 255 endif d= "+
               itos(ncomp)+" hashd\n";

      // Masked context
      for (unsigned i=3; i<v.size(); ++i) {
        if (i==3) hcomp+="b=c ";
        if (v[i]==255)
          hcomp+="a=*b hashd\n";  // ordinary byte
        else if (v[i]>0 && v[i]<255)
          hcomp+="a=*b a&= "+itos(v[i])+" hashd\n";  // masked byte
        else if (v[i]>=256 && v[i]<512) { // lz77 state or masked literal byte
          hcomp+=
          "a=r 1 a> 1 if\n"  // expect literal or offset
          "  a=r 2 a< 64 if\n"  // expect literal
          "    a=*b ";
          if (v[i]<511) hcomp+="a&= "+itos(v[i]-256);
          hcomp+=" hashd\n"
          "  else\n"  // expect match offset byte
          "    a>>= 6 hashd a=r 1 hashd\n"
          "  endif\n"
          "else\n"  // expect new code
          "  a= 255 hashd a=r 2 hashd\n"
          "endif\n";
        }
        else if (v[i]>=1256)  // skip v[i]-1000 bytes
          hcomp+="a= "+itos(((v[i]-1000)>>8)&255)+" a<<= 8 a+= "
               +itos((v[i]-1000)&255)+
          " a+=b b=a\n";
        else if (v[i]>1000)
          hcomp+="a= "+itos(v[i]-1000)+" a+=b b=a\n";
        if (v[i]<512 && i<v.size()-1)
          hcomp+="b++ ";
      }
      ++ncomp;
    }

    // m,8,24: MIX, size, rate
    // t,8,24: MIX2, size, rate
    // s,8,32,255: SSE, size, start, limit
    if (strchr("mts", v[0]) && ncomp>int(v[0]=='t')) {
      if (v.size()<=1) v.push_back(8);
      if (v.size()<=2) v.push_back(24+8*(v[0]=='s'));
      if (v[0]=='s' && v.size()<=3) v.push_back(255);
      comp+=itos(ncomp);
      sb=5+v[1]*3/4;
      if (v[0]=='m')
        comp+=" mix "+itos(v[1])+" 0 "+itos(ncomp)+" "+itos(v[2])+" 255\n";
      else if (v[0]=='t')
        comp+=" mix2 "+itos(v[1])+" "+itos(ncomp-1)+" "+itos(ncomp-2)
            +" "+itos(v[2])+" 255\n";
      else // s
        comp+=" sse "+itos(v[1])+" "+itos(ncomp-1)+" "+itos(v[2])+" "
            +itos(v[3])+"\n";
      if (v[1]>8) {
        hcomp+="d= "+itos(ncomp)+" *d=0 b=c a=0\n";
        for (; v[1]>=16; v[1]-=8) {
          hcomp+="a<<= 8 a+=*b";
          if (v[1]>16) hcomp+=" b++";
          hcomp+="\n";
        }
        if (v[1]>8)
          hcomp+="a<<= 8 a+=*b a>>= "+itos(16-v[1])+"\n";
        hcomp+="a<<= 8 *d=a\n";
      }
      ++ncomp;
    }

    // i: ISSE chain with order increasing by N1,N2...
    if (v[0]=='i' && ncomp>0) {
      assert(sb>=5);
      hcomp+="d= "+itos(ncomp-1)+" b=c a=*d d++\n";
      for (unsigned i=1; i<v.size() && ncomp<254; ++i) {
        for (int j=0; j<v[i]%10; ++j) {
          hcomp+="hash ";
          if (i<v.size()-1 || j<v[i]%10-1) hcomp+="b++ ";
          sb+=6;
        }
        hcomp+="*d=a";
        if (i<v.size()-1) hcomp+=" d++";
        hcomp+="\n";
        if (sb>membits) sb=membits;
        comp+=itos(ncomp)+" isse "+itos(sb-6-v[i]/10)+" "+itos(ncomp-1)+"\n";
        ++ncomp;
      }
    }

    // a24,0,0: MATCH. N1=hash multiplier. N2,N3=halve buf, table.
    if (v[0]=='a') {
      if (v.size()<=1) v.push_back(24);
      while (v.size()<4) v.push_back(0);
      comp+=itos(ncomp)+" match "+itos(membits-v[3]-2)+" "
          +itos(membits-v[2])+"\n";
      hcomp+="d= "+itos(ncomp)+" a=*d a*= "+itos(v[1])
           +" a+=*c a++ *d=a\n";
      sb=5+(membits-v[2])*3/4;
      ++ncomp;
    }

    // w1,65,26,223,20,0: ICM-ISSE chain of length N1 with word contexts,
    // where a word is a sequence of c such that c&N4 is in N2..N2+N3-1.
    // Word is hashed by: hash := hash*N5+c+1
    // Decrease memory by 2^-N6.
    if (v[0]=='w') {
      if (v.size()<=1) v.push_back(1);
      if (v.size()<=2) v.push_back(65);
      if (v.size()<=3) v.push_back(26);
      if (v.size()<=4) v.push_back(223);
      if (v.size()<=5) v.push_back(20);
      if (v.size()<=6) v.push_back(0);
      comp+=itos(ncomp)+" icm "+itos(membits-6-v[6])+"\n";
      for (int i=1; i<v[1]; ++i)
        comp+=itos(ncomp+i)+" isse "+itos(membits-6-v[6])+" "
            +itos(ncomp+i-1)+"\n";
      hcomp+="a=*c a&= "+itos(v[4])+" a-= "+itos(v[2])+" a&= 255 a< "
           +itos(v[3])+" if\n";
      for (int i=0; i<v[1]; ++i) {
        if (i==0) hcomp+="  d= "+itos(ncomp);
        else hcomp+="  d++";
        hcomp+=" a=*d a*= "+itos(v[5])+" a+=*c a++ *d=a\n";
      }
      hcomp+="else\n";
      for (int i=v[1]-1; i>0; --i)
        hcomp+="  d= "+itos(ncomp+i-1)+" a=*d d++ *d=a\n";
      hcomp+="  d= "+itos(ncomp)+" *d=0\n"
           "endif\n";
      ncomp+=v[1]-1;
      sb=membits-v[6];
      ++ncomp;
    }
  }
  return hdr+itos(ncomp)+"\n"+comp+hcomp+"halt\n"+pcomp;
}

// Compress from in to out in 1 segment in 1 block using the algorithm
// descried in method. If method begins with a digit then choose
// a method depending on type. Save filename and comment
// in the segment header. If comment is 0 then the default is the input size
// as a decimal string, plus " jDC\x01" for a journaling method (method[0]
// is not 's'). Write the generated method to methodOut if not 0.
void compressBlock(StringBuffer* in, Writer* out, const char* method_,
                   const char* filename, const char* comment, bool dosha1) {
  assert(in);
  assert(out);
  assert(method_);
  assert(method_[0]);
  std::string method=method_;
  const unsigned n=in->size();  // input size
  const int arg0=MAX(lg(n+4095)-20, 0);  // block size
  assert((1u<<(arg0+20))>=n+4096);

  // Get type from method "LB,R,t" where L is level 0..5, B is block
  // size 0..11, R is redundancy 0..255, t = 0..3 = binary, text, exe, both.
  unsigned type=0;
  if (isdigit(method[0])) {
    int commas=0, arg[4]={0};
    for (int i=1; i<int(method.size()) && commas<4; ++i) {
      if (method[i]==',' || method[i]=='.') ++commas;
      else if (isdigit(method[i])) arg[commas]=arg[commas]*10+method[i]-'0';
    }
    if (commas==0) type=512;
    else type=arg[1]*4+arg[2];
  }

  // Get hash of input
  libzpaq::SHA1 sha1;
  const char* sha1ptr=0;
#ifdef DEBUG
  if (true) {
#else
  if (dosha1) {
#endif
    sha1.write(in->c_str(), n);
    sha1ptr=sha1.result();
  }

  // Expand default methods
  if (isdigit(method[0])) {
    const int level=method[0]-'0';
    assert(level>=0 && level<=9);

    // build models
    const int doe8=(type&2)*2;
    method="x"+itos(arg0);
    std::string htsz=","+itos(19+arg0+(arg0<=6));  // lz77 hash table size
    std::string sasz=","+itos(21+arg0);            // lz77 suffix array size

    // store uncompressed
    if (level==0)
      method="0"+itos(arg0)+",0";

    // LZ77, no model. Store if hard to compress
    else if (level==1) {
      if (type<40) method+=",0";
      else {
        method+=","+itos(1+doe8)+",";
        if      (type<80)  method+="4,0,1,15";
        else if (type<128) method+="4,0,2,16";
        else if (type<256) method+="4,0,2"+htsz;
        else if (type<960) method+="5,0,3"+htsz;
        else               method+="6,0,3"+htsz;
      }
    }

    // LZ77 with longer search
    else if (level==2) {
      if (type<32) method+=",0";
      else {
        method+=","+itos(1+doe8)+",";
        if (type<64) method+="4,0,3"+htsz;
        else method+="4,0,7"+sasz+",1";
      }
    }

    // LZ77 with CM depending on redundancy
    else if (level==3) {
      if (type<20)  // store if not compressible
        method+=",0";
      else if (type<48)  // fast LZ77 if barely compressible
        method+=","+itos(1+doe8)+",4,0,3"+htsz;
      else if (type>=640 || (type&1))  // BWT if text or highly compressible
        method+=","+itos(3+doe8)+"ci1";
      else  // LZ77 with O0-1 compression of up to 12 literals
        method+=","+itos(2+doe8)+",12,0,7"+sasz+",1c0,0,511i2";
    }

    // LZ77+CM, fast CM, or BWT depending on type
    else if (level==4) {
      if (type<12)
        method+=",0";
      else if (type<24)
        method+=","+itos(1+doe8)+",4,0,3"+htsz;
      else if (type<48)
        method+=","+itos(2+doe8)+",5,0,7"+sasz+"1c0,0,511";
      else if (type<900) {
        method+=","+itos(doe8)+"ci1,1,1,1,2a";
        if (type&1) method+="w";
        method+="m";
      }
      else
        method+=","+itos(3+doe8)+"ci1";
    }

    // Slow CM with lots of models
    else {  // 5..9

      // Model text files
      method+=","+itos(doe8);
      if (type&1) method+="w2c0,1010,255i1";
      else method+="w1i1";
      method+="c256ci1,1,1,1,1,1,2a";

      // Analyze the data
      const int NR=1<<12;
      int pt[256]={0};  // position of last occurrence
      int r[NR]={0};    // count repetition gaps of length r
      const unsigned char* p=in->data();
      if (level>0) {
        for (unsigned i=0; i<n; ++i) {
          const int k=i-pt[p[i]];
          if (k>0 && k<NR) ++r[k];
          pt[p[i]]=i;
        }
      }

      // Add periodic models
      int n1=n-r[1]-r[2]-r[3];
      for (int i=0; i<2; ++i) {
        int period=0;
        double score=0;
        int t=0;
        for (int j=5; j<NR && t<n1; ++j) {
          const double s=r[j]/(256.0+n1-t);
          if (s>score) score=s, period=j;
          t+=r[j];
        }
        if (period>4 && score>0.1) {
          method+="c0,0,"+itos(999+period)+",255i1";
          if (period<=255)
            method+="c0,"+itos(period)+"i1";
          n1-=r[period];
          r[period]=0;
        }
        else
          break;
      }
      method+="c0,2,0,255i1c0,3,0,0,255i1c0,4,0,0,0,255i1mm16ts19t0";
    }
  }

  // Compress
  std::string config;
  int args[9]={0};
  config=makeConfig(method.c_str(), args);
  assert(n<=(0x100000u<<args[0])-4096);
  libzpaq::Compressor co;
  co.setOutput(out);
#ifdef DEBUG
  co.setVerify(true);
#endif
  StringBuffer pcomp_cmd;
  co.writeTag();
  co.startBlock(config.c_str(), args, &pcomp_cmd);
  std::string cs=itos(n);
  if (comment) cs=cs+" "+comment;
  co.startSegment(filename, cs.c_str());
  if (args[1]>=1 && args[1]<=7 && args[1]!=4) {  // LZ77 or BWT
    LZBuffer lz(*in, args);
    co.setInput(&lz);
    co.compress();
  }
  else {  // compress with e8e9 or no preprocessing
    if (args[1]>=4 && args[1]<=7)
      e8e9(in->data(), in->size());
    co.setInput(in);
    co.compress();
  }
#ifdef DEBUG  // verify pre-post processing are inverses
  int64_t outsize;
  const char* sha1result=co.endSegmentChecksum(&outsize, dosha1);
  assert(sha1result);
  assert(sha1ptr);
  if (memcmp(sha1result, sha1ptr, 20)!=0)
    error("Pre/post-processor test failed");
#else
  co.endSegment(sha1ptr);
#endif
  co.endBlock();
}

}  // end namespace libzpaq








using std::string;
using std::vector;
using std::map;
using std::min;
using std::max;
using libzpaq::StringBuffer;

int unz(const char * archive,const char * key,bool all); // paranoid unzpaq 2.06

string g_exec_error;
string g_exec_ok;
string g_exec_text;




enum ealgoritmi		{ ALGO_SIZE,ALGO_SHA1,ALGO_CRC32C,ALGO_CRC32,ALGO_XXHASH,ALGO_SHA256,ALGO_WYHASH,ALGO_LAST };
typedef std::map<int,std::string> algoritmi;
const algoritmi::value_type rawData[] = {
   algoritmi::value_type(ALGO_SIZE,"Size"),
   algoritmi::value_type(ALGO_SHA1,"SHA1"),
   algoritmi::value_type(ALGO_CRC32,"CRC32"),
   algoritmi::value_type(ALGO_CRC32C,"CRC-32C"),
   algoritmi::value_type(ALGO_XXHASH,"XXH3"),
   algoritmi::value_type(ALGO_SHA256,"SHA256"),
   algoritmi::value_type(ALGO_WYHASH,"WYHASH"),
};
const int numElems = sizeof rawData / sizeof rawData[0];
algoritmi myalgoritmi(rawData, rawData + numElems);


// Handle errors in libzpaq and elsewhere
void libzpaq::error(const char* msg) {
	g_exec_text=msg;
  if (strstr(msg, "ut of memory")) throw std::bad_alloc();
  throw std::runtime_error(msg);
}
using libzpaq::error;

// Portable thread types and functions for Windows and Linux. Use like this:
//
// // Create mutex for locking thread-unsafe code
// Mutex mutex;            // shared by all threads
// init_mutex(mutex);      // initialize in unlocked state
// Semaphore sem(n);       // n >= 0 is initial state
//
// // Declare a thread function
// ThreadReturn thread(void *arg) {  // arg points to in/out parameters
//   lock(mutex);          // wait if another thread has it first
//   release(mutex);       // allow another waiting thread to continue
//   sem.wait();           // wait until n>0, then --n
//   sem.signal();         // ++n to allow waiting threads to continue
//   return 0;             // must return 0 to exit thread
// }
//
// // Start a thread
// ThreadID tid;
// run(tid, thread, &arg); // runs in parallel
// join(tid);              // wait for thread to return
// destroy_mutex(mutex);   // deallocate resources used by mutex
// sem.destroy();          // deallocate resources used by semaphore

#ifdef PTHREAD
typedef void* ThreadReturn;                                // job return type
typedef pthread_t ThreadID;                                // job ID type
void run(ThreadID& tid, ThreadReturn(*f)(void*), void* arg)// start job
  {pthread_create(&tid, NULL, f, arg);}
void join(ThreadID tid) {pthread_join(tid, NULL);}         // wait for job
typedef pthread_mutex_t Mutex;                             // mutex type
void init_mutex(Mutex& m) {pthread_mutex_init(&m, 0);}     // init mutex
void lock(Mutex& m) {pthread_mutex_lock(&m);}              // wait for mutex
void release(Mutex& m) {pthread_mutex_unlock(&m);}         // release mutex
void destroy_mutex(Mutex& m) {pthread_mutex_destroy(&m);}  // destroy mutex

class Semaphore {
public:
  Semaphore() {sem=-1;}
  void init(int n) {
    assert(n>=0);
    assert(sem==-1);
    pthread_cond_init(&cv, 0);
    pthread_mutex_init(&mutex, 0);
    sem=n;
  }
  void destroy() {
    assert(sem>=0);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cv);
  }
  int wait() {
    assert(sem>=0);
    pthread_mutex_lock(&mutex);
    int r=0;
    if (sem==0) r=pthread_cond_wait(&cv, &mutex);
    assert(sem>0);
    --sem;
    pthread_mutex_unlock(&mutex);
    return r;
  }
  void signal() {
    assert(sem>=0);
    pthread_mutex_lock(&mutex);
    ++sem;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mutex);
  }
private:
  pthread_cond_t cv;  // to signal FINISHED
  pthread_mutex_t mutex; // protects cv
  int sem;  // semaphore count
};

#else  // Windows
typedef DWORD ThreadReturn;
typedef HANDLE ThreadID;
void run(ThreadID& tid, ThreadReturn(*f)(void*), void* arg) {
  tid=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f, arg, 0, NULL);
  if (tid==NULL) error("CreateThread failed");
}
void join(ThreadID& tid) {WaitForSingleObject(tid, INFINITE);}
typedef HANDLE Mutex;
void init_mutex(Mutex& m) {m=CreateMutex(NULL, FALSE, NULL);}
void lock(Mutex& m) {WaitForSingleObject(m, INFINITE);}
void release(Mutex& m) {ReleaseMutex(m);}
void destroy_mutex(Mutex& m) {CloseHandle(m);}

class Semaphore {
public:
  enum {MAXCOUNT=2000000000};
  Semaphore(): h(NULL) {}
  void init(int n) {assert(!h); h=CreateSemaphore(NULL, n, MAXCOUNT, NULL);}
  void destroy() {assert(h); CloseHandle(h);}
  int wait() {assert(h); return WaitForSingleObject(h, INFINITE);}
  void signal() {assert(h); ReleaseSemaphore(h, 1, NULL);}
private:
  HANDLE h;  // Windows semaphore
};

#endif



// Global variables
int64_t global_start=0;  	// set to mtime() at start of main()

bool flag715;
bool flagforcezfs;
bool flagdebug; // very verbose output
bool flagnoeta;					// -noeta (do not show ETA, for batch file)
bool flagpakka;					// different output
bool flagvss;					// make VSS on Windows with admin rights
bool flagverbose;             	// show more
bool flagverify;			// read from filesystem
bool flagkill;
bool flagutf;
bool flagflat;
bool flagfix255;
bool flagfixeml;

   bool flagcrc32c;			// flag use CRC-32c instead
  bool flagxxhash;			// flag use XXH3
  bool flagcrc32;			// flag use CRC32
  bool flagsha256;			// use SHA256
 bool flagwyhash;
int64_t g_dimensione=0;
int64_t g_scritti=0;// note: not thread safe, but who care?
int64_t g_zerotime=0;

int64_t g_bytescanned=0;
int64_t g_filescanned=0;
int64_t g_worked=0;


/*
atomic_int64_t g_bytescanned;
atomic_int64_t g_filescanned;
*/

bool getcaptcha(const string i_captcha,const string i_reason)
{
	if (i_captcha=="")
		return false;
	if (i_reason=="")
		return false;
	printf("\nTo confirm a dangerous command\n");
	printf(">>> %s\n",i_reason.c_str());
	printf("enter EXACTLY the capcha, then press CR (return)\n");
	printf("Entering anything else will quit.\n");
	printf("\nCaptcha to continue:     %s\n",i_captcha.c_str());
	char myline[81];
    scanf("%80s", myline);
	if (myline!=i_captcha)
	{
		printf("Wrong captcha\n");
		return false;
	}
	printf("Captcha OK\n");
	return true;
}

  
// In Windows, convert 16-bit wide string to UTF-8 and \ to /
#ifdef _WIN32
bool windows7_or_above=false; //windows version (for using FindFirstFileExW)

string wtou(const wchar_t* s) {
  assert(sizeof(wchar_t)==2);  // Not true in Linux
  assert((wchar_t)(-1)==65535);
  string r;
  if (!s) return r;
  for (; *s; ++s) {
    if (*s=='\\') r+='/';
    else if (*s<128) r+=*s;
    else if (*s<2048) r+=192+*s/64, r+=128+*s%64;
    else r+=224+*s/4096, r+=128+*s/64%64, r+=128+*s%64;
  }
  return r;
}

// In Windows, convert UTF-8 string to wide string ignoring
// invalid UTF-8 or >64K. Convert "/" to slash (default "\").
std::wstring utow(const char* ss, char slash='\\') {
  assert(sizeof(wchar_t)==2);
  assert((wchar_t)(-1)==65535);
  std::wstring r;
  if (!ss) return r;
  const unsigned char* s=(const unsigned char*)ss;
  for (; s && *s; ++s) {
    if (s[0]=='/') r+=slash;
    else if (s[0]<128) r+=s[0];
    else if (s[0]>=192 && s[0]<224 && s[1]>=128 && s[1]<192)
      r+=(s[0]-192)*64+s[1]-128, ++s;
    else if (s[0]>=224 && s[0]<240 && s[1]>=128 && s[1]<192
             && s[2]>=128 && s[2]<192)
      r+=(s[0]-224)*4096+(s[1]-128)*64+s[2]-128, s+=2;
  }
  return r;
}
#endif



/////////////	case insensitive compare

char* stristr( const char* str1, const char* str2 )
{
    const char* p1 = str1;
    const char* p2 = str2;
    const char* r = *p2 == 0 ? str1 : 0 ;

    while( *p1 != 0 && *p2 != 0 )
    {
        if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
        {
            if( r == 0 )
                r = p1 ;
            p2++ ;
        }
        else
        {
            p2 = str2 ;
            if( r != 0 )
                p1 = r + 1 ;

            if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
            {
                r = p1 ;
                p2++ ;
            }
            else
                r = 0 ;
        }

        p1++ ;
    }

    return *p2 == 0 ? (char*)r : 0 ;
}
bool isdirectory(string i_filename)
{
	if (i_filename.length()==0)
		return false;
	else
	return
		i_filename[i_filename.size()-1]=='/';
}
bool havedoublequote(string i_filename)
{
	if (i_filename.length()==0)
		return false;
	else
	return
		i_filename[i_filename.size()-1]=='"';
}
string cutdoublequote(const string& i_string)
{
	string temp=i_string;
	if (havedoublequote(i_string))
		temp.pop_back();
	return temp;
}

bool isextension(const char* i_filename,const char* i_ext)
{
	if (!i_filename)
		return false;
	if (!i_ext)
		return false;
	if (isdirectory(i_filename))
		return false;

	char * posizione=stristr(i_filename, i_ext);
	if (!posizione)
		return false;
	/*
	printf("Strlen filename %d\n",strlen(i_filename));
	printf("Strlen i_ext    %d\n",strlen(i_ext));
	printf("Delta  %d\n",posizione-i_filename);
	*/
	return (posizione-i_filename)+strlen(i_ext)==strlen(i_filename);
}
bool isxls(string i_filename)
{
	return isextension(i_filename.c_str(), ".xls");
}
bool isads(string i_filename)
{
	if (i_filename.length()==0)
		return false;
	else
		return strstr(i_filename.c_str(), ":$DATA")!=0;  
}
bool iszfs(string i_filename)
{
	if (i_filename.length()==0)
		return false;
	else
		return strstr(i_filename.c_str(), ".zfs")!=0;  
}


// Print a UTF-8 string to f (stdout, stderr) so it displays properly
void printUTF8(const char* s, FILE* f=stdout) {
  assert(f);
  assert(s);
#ifdef unix
  fprintf(f, "%s", s);
#else
  const HANDLE h=(HANDLE)_get_osfhandle(_fileno(f));
  DWORD ft=GetFileType(h);
  if (ft==FILE_TYPE_CHAR) {
    fflush(f);
    std::wstring w=utow(s, '/');  // Windows console: convert to UTF-16
    DWORD n=0;
    WriteConsole(h, w.c_str(), w.size(), &n, 0);
  }
  else  // stdout redirected to file
    fprintf(f, "%s", s);
#endif
}


// Return relative time in milliseconds
int64_t mtime() {
#ifdef unix
  timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec*1000LL+tv.tv_usec/1000;
#else
  int64_t t=GetTickCount();
  if (t<global_start) t+=0x100000000LL;
  return t;
#endif
}

// Convert 64 bit decimal YYYYMMDDHHMMSS to "YYYY-MM-DD HH:MM:SS"
// where -1 = unknown date, 0 = deleted.
string dateToString(int64_t date) {
  if (date<=0) return "                   ";
  string s="0000-00-00 00:00:00";
  static const int t[]={18,17,15,14,12,11,9,8,6,5,3,2,1,0};
  for (int i=0; i<14; ++i) s[t[i]]+=int(date%10), date/=10;
  return s;
}

// Convert attributes to a readable format
string attrToString(int64_t attrib) {
  string r="     ";
  if ((attrib&255)=='u') {
    r[0]="0pc3d5b7 9lBsDEF"[(attrib>>20)&15];
    for (int i=0; i<4; ++i)
      r[4-i]=(attrib>>(8+3*i))%8+'0';
  }
  else if ((attrib&255)=='w') {
    for (int i=0, j=0; i<32; ++i) {
      if ((attrib>>(i+8))&1) {
        char c="RHS DAdFTprCoIEivs89012345678901"[i];
        if (j<5) r[j]=c;
        else r+=c;
        ++j;
      }
    }
  }
  return r;
}

// Convert seconds since 0000 1/1/1970 to 64 bit decimal YYYYMMDDHHMMSS
// Valid from 1970 to 2099.
int64_t decimal_time(time_t tt) {
  if (tt==-1) tt=0;
  int64_t t=(sizeof(tt)==4) ? unsigned(tt) : tt;
  const int second=t%60;
  const int minute=t/60%60;
  const int hour=t/3600%24;
  t/=86400;  // days since Jan 1 1970
  const int term=t/1461;  // 4 year terms since 1970
  t%=1461;
  t+=(t>=59);  // insert Feb 29 on non leap years
  t+=(t>=425);
  t+=(t>=1157);
  const int year=term*4+t/366+1970;  // actual year
  t%=366;
  t+=(t>=60)*2;  // make Feb. 31 days
  t+=(t>=123);   // insert Apr 31
  t+=(t>=185);   // insert June 31
  t+=(t>=278);   // insert Sept 31
  t+=(t>=340);   // insert Nov 31
  const int month=t/31+1;
  const int day=t%31+1;
  return year*10000000000LL+month*100000000+day*1000000
         +hour*10000+minute*100+second;
}

// Convert decimal date to time_t - inverse of decimal_time()
time_t unix_time(int64_t date) {
  if (date<=0) return -1;
  static const int days[12]={0,31,59,90,120,151,181,212,243,273,304,334};
  const int year=date/10000000000LL%10000;
  const int month=(date/100000000%100-1)%12;
  const int day=date/1000000%100;
  const int hour=date/10000%100;
  const int min=date/100%100;
  const int sec=date%100;
  return (day-1+days[month]+(year%4==0 && month>1)+((year-1970)*1461+1)/4)
    *86400+hour*3600+min*60+sec;
}








/////////////////////////////// File //////////////////////////////////

// Windows/Linux compatible file type
#ifdef unix
typedef FILE* FP;
const FP FPNULL=NULL;
const char* const RB="rb";
const char* const WB="wb";
const char* const RBPLUS="rb+";

#else // Windows
typedef HANDLE FP;
const FP FPNULL=INVALID_HANDLE_VALUE;
typedef enum {RB, WB, RBPLUS, WBPLUS} MODE;  // fopen modes

// Few lines from https://gist.github.com/nu774/10381075, and few from milo1012 TC zpaq plugin.
std::wstring GetFullPathNameX(const std::wstring &path)
{
    DWORD length = GetFullPathNameW(path.c_str(), 0, 0, 0);
    std::vector<wchar_t> buffer(length);
    length = GetFullPathNameW(path.c_str(), buffer.size(), &buffer[0], 0);
    return std::wstring(&buffer[0], &buffer[length]);
}

// Code from https://gist.github.com/nu774/10381075 and Milo's ő zpaq plugin.
std::wstring make_long_path(const char* filename) {

  if (strlen(filename) > 2 && filename[0] == '/' && filename[1] == '/' && filename[2] == '?')
    return utow(filename);
  else {
    std::wstring ws =  L"\\\\?\\" + GetFullPathNameX(utow(filename));
    //wprintf(L"\nCONV: (%d) %ls\n\n", ws.length(), ws.c_str());
    return ws;
  }
}

	

// Open file. Only modes "rb", "wb", "rb+" and "wb+" are supported.
FP fopen(const char* filename, MODE mode) {
  assert(filename);
  DWORD access=0;
  if (mode!=WB) access=GENERIC_READ;
  if (mode!=RB) access|=GENERIC_WRITE;
  DWORD disp=OPEN_ALWAYS;  // wb or wb+
  if (mode==RB || mode==RBPLUS) disp=OPEN_EXISTING;
  DWORD share=FILE_SHARE_READ;
  if (mode==RB) share|=FILE_SHARE_WRITE|FILE_SHARE_DELETE;
  return CreateFile(utow(filename).c_str(), access, share,
                    NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);
}


// Close file
int fclose(FP fp) {
  return CloseHandle(fp) ? 0 : EOF;
}

// Read nobj objects of size size into ptr. Return number of objects read.
size_t fread(void* ptr, size_t size, size_t nobj, FP fp) {
  DWORD r=0;
  ReadFile(fp, ptr, size*nobj, &r, NULL);
  if (size>1) r/=size;
  return r;
}

// Write nobj objects of size size from ptr to fp. Return number written.
size_t fwrite(const void* ptr, size_t size, size_t nobj, FP fp) {
  DWORD r=0;
  WriteFile(fp, ptr, size*nobj, &r, NULL);
  if (size>1) r/=size;
  return r;
}

// Move file pointer by offset. origin is SEEK_SET (from start), SEEK_CUR,
// (from current position), or SEEK_END (from end).
int fseeko(FP fp, int64_t offset, int origin) {
  if (origin==SEEK_SET) origin=FILE_BEGIN;
  else if (origin==SEEK_CUR) origin=FILE_CURRENT;
  else if (origin==SEEK_END) origin=FILE_END;
  LONG h=uint64_t(offset)>>32;
  SetFilePointer(fp, offset&0xffffffffull, &h, origin);
  return GetLastError()!=NO_ERROR;
}

// Get file position
int64_t ftello(FP fp) {
  LONG h=0;
  DWORD r=SetFilePointer(fp, 0, &h, FILE_CURRENT);
  return r+(uint64_t(h)<<32);
}

#endif


/// We want to read the damn UTF8 files. 
/// In the case of Windows using the function for UTF16, after conversion

FILE* freadopen(const char* i_filename)
{
#ifdef _WIN32
	wstring widename=utow(i_filename);
	FILE* myfp=_wfopen(widename.c_str(), L"rb" );
#else
	FILE* myfp=fopen(i_filename, "rb" );
#endif
	if (myfp==NULL)
	{
		printf( "\nfreadopen cannot open:");
		printUTF8(i_filename);
		printf("\n");
		return 0;
	}
	return myfp;
}

/////////////////// calculate CRC32 

// //////////////////////////////////////////////////////////
// Crc32.h
// Copyright (c) 2011-2019 Stephan Brumme. All rights reserved.
// Slicing-by-16 contributed by Bulat Ziganshin
// Tableless bytewise CRC contributed by Hagai Gold
// see http://create.stephan-brumme.com/disclaimer.html
//

#define CRC32_USE_LOOKUP_TABLE_SLICING_BY_16

// uint8_t, uint32_t, int32_t


uint32_t crc32_combine (uint32_t crcA, uint32_t crcB, size_t lengthB);
uint32_t crc32_16bytes (const void* data, size_t length, uint32_t previousCrc32 = 0);


#ifndef __LITTLE_ENDIAN
  #define __LITTLE_ENDIAN 1234
#endif
#ifndef __BIG_ENDIAN
  #define __BIG_ENDIAN    4321
#endif

// define endianess and some integer data types
#if defined(_MSC_VER) || defined(__MINGW32__)
  // Windows always little endian
  #define __BYTE_ORDER __LITTLE_ENDIAN

  // intrinsics / prefetching
  #include <xmmintrin.h>
  #ifdef __MINGW32__
    #define PREFETCH(location) __builtin_prefetch(location)
  #else
    #define PREFETCH(location) _mm_prefetch(location, _MM_HINT_T0)
  #endif
#else
  // defines __BYTE_ORDER as __LITTLE_ENDIAN or __BIG_ENDIAN
  #include <sys/param.h>

  // intrinsics / prefetching
  #ifdef __GNUC__
    #define PREFETCH(location) __builtin_prefetch(location)
  #else
    // no prefetching
    #define PREFETCH(location) ;
  #endif
#endif


#define __BYTE_ORDER __LITTLE_ENDIAN


namespace
{
  /// zlib's CRC32 polynomial
  const uint32_t Polynomial = 0xEDB88320;
//esx
  /// swap endianess
  static inline uint32_t swap(uint32_t x)
  {
  #if defined(__GNUC__) || defined(__clang__) && !defined(ESX)
    return __builtin_bswap32(x);
  #else
    return (x >> 24) |
          ((x >>  8) & 0x0000FF00) |
          ((x <<  8) & 0x00FF0000) |
           (x << 24);
  #endif
  }

  /// Slicing-By-16
  #ifdef CRC32_USE_LOOKUP_TABLE_SLICING_BY_16
  const size_t MaxSlice = 16;
  #elif defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_8)
  const size_t MaxSlice = 8;
  #elif defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_4)
  const size_t MaxSlice = 4;
  #elif defined(CRC32_USE_LOOKUP_TABLE_BYTE)
  const size_t MaxSlice = 1;
  #else
    #define NO_LUT // don't need Crc32Lookup at all
  #endif

} // anonymous namespace

/// forward declaration, table is at the end of this file
extern const uint32_t Crc32Lookup[MaxSlice][256]; // extern is needed to keep compiler happy





uint32_t crc32_16bytes(const void* data, size_t length, uint32_t previousCrc32)
{
  uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
  const uint32_t* current = (const uint32_t*) data;

  // enabling optimization (at least -O2) automatically unrolls the inner for-loop
  const size_t Unroll = 4;
  const size_t BytesAtOnce = 16 * Unroll;

  while (length >= BytesAtOnce)
  {
    for (size_t unrolling = 0; unrolling < Unroll; unrolling++)
    {
#if __BYTE_ORDER == __BIG_ENDIAN
    uint32_t one   = *current++ ^ swap(crc);
    uint32_t two   = *current++;
    uint32_t three = *current++;
    uint32_t four  = *current++;
    crc  = Crc32Lookup[ 0][ four         & 0xFF] ^
           Crc32Lookup[ 1][(four  >>  8) & 0xFF] ^
           Crc32Lookup[ 2][(four  >> 16) & 0xFF] ^
           Crc32Lookup[ 3][(four  >> 24) & 0xFF] ^
           Crc32Lookup[ 4][ three        & 0xFF] ^
           Crc32Lookup[ 5][(three >>  8) & 0xFF] ^
           Crc32Lookup[ 6][(three >> 16) & 0xFF] ^
           Crc32Lookup[ 7][(three >> 24) & 0xFF] ^
           Crc32Lookup[ 8][ two          & 0xFF] ^
           Crc32Lookup[ 9][(two   >>  8) & 0xFF] ^
           Crc32Lookup[10][(two   >> 16) & 0xFF] ^
           Crc32Lookup[11][(two   >> 24) & 0xFF] ^
           Crc32Lookup[12][ one          & 0xFF] ^
           Crc32Lookup[13][(one   >>  8) & 0xFF] ^
           Crc32Lookup[14][(one   >> 16) & 0xFF] ^
           Crc32Lookup[15][(one   >> 24) & 0xFF];
#else
    uint32_t one   = *current++ ^ crc;
    uint32_t two   = *current++;
    uint32_t three = *current++;
    uint32_t four  = *current++;
    crc  = Crc32Lookup[ 0][(four  >> 24) & 0xFF] ^
           Crc32Lookup[ 1][(four  >> 16) & 0xFF] ^
           Crc32Lookup[ 2][(four  >>  8) & 0xFF] ^
           Crc32Lookup[ 3][ four         & 0xFF] ^
           Crc32Lookup[ 4][(three >> 24) & 0xFF] ^
           Crc32Lookup[ 5][(three >> 16) & 0xFF] ^
           Crc32Lookup[ 6][(three >>  8) & 0xFF] ^
           Crc32Lookup[ 7][ three        & 0xFF] ^
           Crc32Lookup[ 8][(two   >> 24) & 0xFF] ^
           Crc32Lookup[ 9][(two   >> 16) & 0xFF] ^
           Crc32Lookup[10][(two   >>  8) & 0xFF] ^
           Crc32Lookup[11][ two          & 0xFF] ^
           Crc32Lookup[12][(one   >> 24) & 0xFF] ^
           Crc32Lookup[13][(one   >> 16) & 0xFF] ^
           Crc32Lookup[14][(one   >>  8) & 0xFF] ^
           Crc32Lookup[15][ one          & 0xFF];
#endif
    }

    length -= BytesAtOnce;
  }

  const uint8_t* currentChar = (const uint8_t*) current;
  // remaining 1 to 63 bytes (standard algorithm)
  while (length-- != 0)
    crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

  return ~crc; // same as crc ^ 0xFFFFFFFF
}




/// merge two CRC32 such that result = crc32(dataB, lengthB, crc32(dataA, lengthA))
uint32_t crc32_combine(uint32_t crcA, uint32_t crcB, size_t lengthB)
{

  // degenerated case
  if (lengthB == 0)
    return crcA;

  /// CRC32 => 32 bits
  const uint32_t CrcBits = 32;

  uint32_t odd [CrcBits]; // odd-power-of-two  zeros operator
  uint32_t even[CrcBits]; // even-power-of-two zeros operator

  // put operator for one zero bit in odd
  odd[0] = Polynomial;    // CRC-32 polynomial
  for (unsigned int i = 1; i < CrcBits; i++)
    odd[i] = 1 << (i - 1);

  // put operator for two zero bits in even
  // same as gf2_matrix_square(even, odd);
  for (unsigned int i = 0; i < CrcBits; i++)
  {
    uint32_t vec = odd[i];
    even[i] = 0;
    for (int j = 0; vec != 0; j++, vec >>= 1)
      if (vec & 1)
        even[i] ^= odd[j];
  }
  // put operator for four zero bits in odd
  // same as gf2_matrix_square(odd, even);
  for (unsigned int i = 0; i < CrcBits; i++)
  {
    uint32_t vec = even[i];
    odd[i] = 0;
    for (int j = 0; vec != 0; j++, vec >>= 1)
      if (vec & 1)
        odd[i] ^= even[j];
  }

  // the following loop becomes much shorter if I keep swapping even and odd
  uint32_t* a = even;
  uint32_t* b = odd;
  // apply secondLength zeros to firstCrc32
  for (; lengthB > 0; lengthB >>= 1)
  {
    // same as gf2_matrix_square(a, b);
    for (unsigned int i = 0; i < CrcBits; i++)
    {
      uint32_t vec = b[i];
      a[i] = 0;
      for (int j = 0; vec != 0; j++, vec >>= 1)
        if (vec & 1)
          a[i] ^= b[j];
    }

    // apply zeros operator for this bit
    if (lengthB & 1)
    {
      // same as firstCrc32 = gf2_matrix_times(a, firstCrc32);
      uint32_t sum = 0;
      for (int i = 0; crcA != 0; i++, crcA >>= 1)
        if (crcA & 1)
          sum ^= a[i];
      crcA = sum;
    }

    // switch even and odd
    uint32_t* t = a; a = b; b = t;
  }

  // return combined crc
  return crcA ^ crcB;
}


// //////////////////////////////////////////////////////////
// constants


#ifndef NO_LUT
/// look-up table, already declared above
const uint32_t Crc32Lookup[MaxSlice][256] =
{
  {
    // note: the first number of every second row corresponds to the half-byte look-up table !
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
    0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
    0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
    0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
    0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,
    0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
    0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
    0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
    0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
    0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
    0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,
    0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
    0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,
    0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
    0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
    0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,
    0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
    0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,
    0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
    0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
    0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
    0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,
    0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
    0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,
    0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
    0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
    0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
    0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,
    0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
    0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,
    0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D,
  }

#if defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_4) || defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_8) || defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_16)
  // beyond this point only relevant for Slicing-by-4, Slicing-by-8 and Slicing-by-16
  ,{
    0x00000000,0x191B3141,0x32366282,0x2B2D53C3,0x646CC504,0x7D77F445,0x565AA786,0x4F4196C7,
    0xC8D98A08,0xD1C2BB49,0xFAEFE88A,0xE3F4D9CB,0xACB54F0C,0xB5AE7E4D,0x9E832D8E,0x87981CCF,
    0x4AC21251,0x53D92310,0x78F470D3,0x61EF4192,0x2EAED755,0x37B5E614,0x1C98B5D7,0x05838496,
    0x821B9859,0x9B00A918,0xB02DFADB,0xA936CB9A,0xE6775D5D,0xFF6C6C1C,0xD4413FDF,0xCD5A0E9E,
    0x958424A2,0x8C9F15E3,0xA7B24620,0xBEA97761,0xF1E8E1A6,0xE8F3D0E7,0xC3DE8324,0xDAC5B265,
    0x5D5DAEAA,0x44469FEB,0x6F6BCC28,0x7670FD69,0x39316BAE,0x202A5AEF,0x0B07092C,0x121C386D,
    0xDF4636F3,0xC65D07B2,0xED705471,0xF46B6530,0xBB2AF3F7,0xA231C2B6,0x891C9175,0x9007A034,
    0x179FBCFB,0x0E848DBA,0x25A9DE79,0x3CB2EF38,0x73F379FF,0x6AE848BE,0x41C51B7D,0x58DE2A3C,
    0xF0794F05,0xE9627E44,0xC24F2D87,0xDB541CC6,0x94158A01,0x8D0EBB40,0xA623E883,0xBF38D9C2,
    0x38A0C50D,0x21BBF44C,0x0A96A78F,0x138D96CE,0x5CCC0009,0x45D73148,0x6EFA628B,0x77E153CA,
    0xBABB5D54,0xA3A06C15,0x888D3FD6,0x91960E97,0xDED79850,0xC7CCA911,0xECE1FAD2,0xF5FACB93,
    0x7262D75C,0x6B79E61D,0x4054B5DE,0x594F849F,0x160E1258,0x0F152319,0x243870DA,0x3D23419B,
    0x65FD6BA7,0x7CE65AE6,0x57CB0925,0x4ED03864,0x0191AEA3,0x188A9FE2,0x33A7CC21,0x2ABCFD60,
    0xAD24E1AF,0xB43FD0EE,0x9F12832D,0x8609B26C,0xC94824AB,0xD05315EA,0xFB7E4629,0xE2657768,
    0x2F3F79F6,0x362448B7,0x1D091B74,0x04122A35,0x4B53BCF2,0x52488DB3,0x7965DE70,0x607EEF31,
    0xE7E6F3FE,0xFEFDC2BF,0xD5D0917C,0xCCCBA03D,0x838A36FA,0x9A9107BB,0xB1BC5478,0xA8A76539,
    0x3B83984B,0x2298A90A,0x09B5FAC9,0x10AECB88,0x5FEF5D4F,0x46F46C0E,0x6DD93FCD,0x74C20E8C,
    0xF35A1243,0xEA412302,0xC16C70C1,0xD8774180,0x9736D747,0x8E2DE606,0xA500B5C5,0xBC1B8484,
    0x71418A1A,0x685ABB5B,0x4377E898,0x5A6CD9D9,0x152D4F1E,0x0C367E5F,0x271B2D9C,0x3E001CDD,
    0xB9980012,0xA0833153,0x8BAE6290,0x92B553D1,0xDDF4C516,0xC4EFF457,0xEFC2A794,0xF6D996D5,
    0xAE07BCE9,0xB71C8DA8,0x9C31DE6B,0x852AEF2A,0xCA6B79ED,0xD37048AC,0xF85D1B6F,0xE1462A2E,
    0x66DE36E1,0x7FC507A0,0x54E85463,0x4DF36522,0x02B2F3E5,0x1BA9C2A4,0x30849167,0x299FA026,
    0xE4C5AEB8,0xFDDE9FF9,0xD6F3CC3A,0xCFE8FD7B,0x80A96BBC,0x99B25AFD,0xB29F093E,0xAB84387F,
    0x2C1C24B0,0x350715F1,0x1E2A4632,0x07317773,0x4870E1B4,0x516BD0F5,0x7A468336,0x635DB277,
    0xCBFAD74E,0xD2E1E60F,0xF9CCB5CC,0xE0D7848D,0xAF96124A,0xB68D230B,0x9DA070C8,0x84BB4189,
    0x03235D46,0x1A386C07,0x31153FC4,0x280E0E85,0x674F9842,0x7E54A903,0x5579FAC0,0x4C62CB81,
    0x8138C51F,0x9823F45E,0xB30EA79D,0xAA1596DC,0xE554001B,0xFC4F315A,0xD7626299,0xCE7953D8,
    0x49E14F17,0x50FA7E56,0x7BD72D95,0x62CC1CD4,0x2D8D8A13,0x3496BB52,0x1FBBE891,0x06A0D9D0,
    0x5E7EF3EC,0x4765C2AD,0x6C48916E,0x7553A02F,0x3A1236E8,0x230907A9,0x0824546A,0x113F652B,
    0x96A779E4,0x8FBC48A5,0xA4911B66,0xBD8A2A27,0xF2CBBCE0,0xEBD08DA1,0xC0FDDE62,0xD9E6EF23,
    0x14BCE1BD,0x0DA7D0FC,0x268A833F,0x3F91B27E,0x70D024B9,0x69CB15F8,0x42E6463B,0x5BFD777A,
    0xDC656BB5,0xC57E5AF4,0xEE530937,0xF7483876,0xB809AEB1,0xA1129FF0,0x8A3FCC33,0x9324FD72,
  },

  {
    0x00000000,0x01C26A37,0x0384D46E,0x0246BE59,0x0709A8DC,0x06CBC2EB,0x048D7CB2,0x054F1685,
    0x0E1351B8,0x0FD13B8F,0x0D9785D6,0x0C55EFE1,0x091AF964,0x08D89353,0x0A9E2D0A,0x0B5C473D,
    0x1C26A370,0x1DE4C947,0x1FA2771E,0x1E601D29,0x1B2F0BAC,0x1AED619B,0x18ABDFC2,0x1969B5F5,
    0x1235F2C8,0x13F798FF,0x11B126A6,0x10734C91,0x153C5A14,0x14FE3023,0x16B88E7A,0x177AE44D,
    0x384D46E0,0x398F2CD7,0x3BC9928E,0x3A0BF8B9,0x3F44EE3C,0x3E86840B,0x3CC03A52,0x3D025065,
    0x365E1758,0x379C7D6F,0x35DAC336,0x3418A901,0x3157BF84,0x3095D5B3,0x32D36BEA,0x331101DD,
    0x246BE590,0x25A98FA7,0x27EF31FE,0x262D5BC9,0x23624D4C,0x22A0277B,0x20E69922,0x2124F315,
    0x2A78B428,0x2BBADE1F,0x29FC6046,0x283E0A71,0x2D711CF4,0x2CB376C3,0x2EF5C89A,0x2F37A2AD,
    0x709A8DC0,0x7158E7F7,0x731E59AE,0x72DC3399,0x7793251C,0x76514F2B,0x7417F172,0x75D59B45,
    0x7E89DC78,0x7F4BB64F,0x7D0D0816,0x7CCF6221,0x798074A4,0x78421E93,0x7A04A0CA,0x7BC6CAFD,
    0x6CBC2EB0,0x6D7E4487,0x6F38FADE,0x6EFA90E9,0x6BB5866C,0x6A77EC5B,0x68315202,0x69F33835,
    0x62AF7F08,0x636D153F,0x612BAB66,0x60E9C151,0x65A6D7D4,0x6464BDE3,0x662203BA,0x67E0698D,
    0x48D7CB20,0x4915A117,0x4B531F4E,0x4A917579,0x4FDE63FC,0x4E1C09CB,0x4C5AB792,0x4D98DDA5,
    0x46C49A98,0x4706F0AF,0x45404EF6,0x448224C1,0x41CD3244,0x400F5873,0x4249E62A,0x438B8C1D,
    0x54F16850,0x55330267,0x5775BC3E,0x56B7D609,0x53F8C08C,0x523AAABB,0x507C14E2,0x51BE7ED5,
    0x5AE239E8,0x5B2053DF,0x5966ED86,0x58A487B1,0x5DEB9134,0x5C29FB03,0x5E6F455A,0x5FAD2F6D,
    0xE1351B80,0xE0F771B7,0xE2B1CFEE,0xE373A5D9,0xE63CB35C,0xE7FED96B,0xE5B86732,0xE47A0D05,
    0xEF264A38,0xEEE4200F,0xECA29E56,0xED60F461,0xE82FE2E4,0xE9ED88D3,0xEBAB368A,0xEA695CBD,
    0xFD13B8F0,0xFCD1D2C7,0xFE976C9E,0xFF5506A9,0xFA1A102C,0xFBD87A1B,0xF99EC442,0xF85CAE75,
    0xF300E948,0xF2C2837F,0xF0843D26,0xF1465711,0xF4094194,0xF5CB2BA3,0xF78D95FA,0xF64FFFCD,
    0xD9785D60,0xD8BA3757,0xDAFC890E,0xDB3EE339,0xDE71F5BC,0xDFB39F8B,0xDDF521D2,0xDC374BE5,
    0xD76B0CD8,0xD6A966EF,0xD4EFD8B6,0xD52DB281,0xD062A404,0xD1A0CE33,0xD3E6706A,0xD2241A5D,
    0xC55EFE10,0xC49C9427,0xC6DA2A7E,0xC7184049,0xC25756CC,0xC3953CFB,0xC1D382A2,0xC011E895,
    0xCB4DAFA8,0xCA8FC59F,0xC8C97BC6,0xC90B11F1,0xCC440774,0xCD866D43,0xCFC0D31A,0xCE02B92D,
    0x91AF9640,0x906DFC77,0x922B422E,0x93E92819,0x96A63E9C,0x976454AB,0x9522EAF2,0x94E080C5,
    0x9FBCC7F8,0x9E7EADCF,0x9C381396,0x9DFA79A1,0x98B56F24,0x99770513,0x9B31BB4A,0x9AF3D17D,
    0x8D893530,0x8C4B5F07,0x8E0DE15E,0x8FCF8B69,0x8A809DEC,0x8B42F7DB,0x89044982,0x88C623B5,
    0x839A6488,0x82580EBF,0x801EB0E6,0x81DCDAD1,0x8493CC54,0x8551A663,0x8717183A,0x86D5720D,
    0xA9E2D0A0,0xA820BA97,0xAA6604CE,0xABA46EF9,0xAEEB787C,0xAF29124B,0xAD6FAC12,0xACADC625,
    0xA7F18118,0xA633EB2F,0xA4755576,0xA5B73F41,0xA0F829C4,0xA13A43F3,0xA37CFDAA,0xA2BE979D,
    0xB5C473D0,0xB40619E7,0xB640A7BE,0xB782CD89,0xB2CDDB0C,0xB30FB13B,0xB1490F62,0xB08B6555,
    0xBBD72268,0xBA15485F,0xB853F606,0xB9919C31,0xBCDE8AB4,0xBD1CE083,0xBF5A5EDA,0xBE9834ED,
  },

  {
    0x00000000,0xB8BC6765,0xAA09C88B,0x12B5AFEE,0x8F629757,0x37DEF032,0x256B5FDC,0x9DD738B9,
    0xC5B428EF,0x7D084F8A,0x6FBDE064,0xD7018701,0x4AD6BFB8,0xF26AD8DD,0xE0DF7733,0x58631056,
    0x5019579F,0xE8A530FA,0xFA109F14,0x42ACF871,0xDF7BC0C8,0x67C7A7AD,0x75720843,0xCDCE6F26,
    0x95AD7F70,0x2D111815,0x3FA4B7FB,0x8718D09E,0x1ACFE827,0xA2738F42,0xB0C620AC,0x087A47C9,
    0xA032AF3E,0x188EC85B,0x0A3B67B5,0xB28700D0,0x2F503869,0x97EC5F0C,0x8559F0E2,0x3DE59787,
    0x658687D1,0xDD3AE0B4,0xCF8F4F5A,0x7733283F,0xEAE41086,0x525877E3,0x40EDD80D,0xF851BF68,
    0xF02BF8A1,0x48979FC4,0x5A22302A,0xE29E574F,0x7F496FF6,0xC7F50893,0xD540A77D,0x6DFCC018,
    0x359FD04E,0x8D23B72B,0x9F9618C5,0x272A7FA0,0xBAFD4719,0x0241207C,0x10F48F92,0xA848E8F7,
    0x9B14583D,0x23A83F58,0x311D90B6,0x89A1F7D3,0x1476CF6A,0xACCAA80F,0xBE7F07E1,0x06C36084,
    0x5EA070D2,0xE61C17B7,0xF4A9B859,0x4C15DF3C,0xD1C2E785,0x697E80E0,0x7BCB2F0E,0xC377486B,
    0xCB0D0FA2,0x73B168C7,0x6104C729,0xD9B8A04C,0x446F98F5,0xFCD3FF90,0xEE66507E,0x56DA371B,
    0x0EB9274D,0xB6054028,0xA4B0EFC6,0x1C0C88A3,0x81DBB01A,0x3967D77F,0x2BD27891,0x936E1FF4,
    0x3B26F703,0x839A9066,0x912F3F88,0x299358ED,0xB4446054,0x0CF80731,0x1E4DA8DF,0xA6F1CFBA,
    0xFE92DFEC,0x462EB889,0x549B1767,0xEC277002,0x71F048BB,0xC94C2FDE,0xDBF98030,0x6345E755,
    0x6B3FA09C,0xD383C7F9,0xC1366817,0x798A0F72,0xE45D37CB,0x5CE150AE,0x4E54FF40,0xF6E89825,
    0xAE8B8873,0x1637EF16,0x048240F8,0xBC3E279D,0x21E91F24,0x99557841,0x8BE0D7AF,0x335CB0CA,
    0xED59B63B,0x55E5D15E,0x47507EB0,0xFFEC19D5,0x623B216C,0xDA874609,0xC832E9E7,0x708E8E82,
    0x28ED9ED4,0x9051F9B1,0x82E4565F,0x3A58313A,0xA78F0983,0x1F336EE6,0x0D86C108,0xB53AA66D,
    0xBD40E1A4,0x05FC86C1,0x1749292F,0xAFF54E4A,0x322276F3,0x8A9E1196,0x982BBE78,0x2097D91D,
    0x78F4C94B,0xC048AE2E,0xD2FD01C0,0x6A4166A5,0xF7965E1C,0x4F2A3979,0x5D9F9697,0xE523F1F2,
    0x4D6B1905,0xF5D77E60,0xE762D18E,0x5FDEB6EB,0xC2098E52,0x7AB5E937,0x680046D9,0xD0BC21BC,
    0x88DF31EA,0x3063568F,0x22D6F961,0x9A6A9E04,0x07BDA6BD,0xBF01C1D8,0xADB46E36,0x15080953,
    0x1D724E9A,0xA5CE29FF,0xB77B8611,0x0FC7E174,0x9210D9CD,0x2AACBEA8,0x38191146,0x80A57623,
    0xD8C66675,0x607A0110,0x72CFAEFE,0xCA73C99B,0x57A4F122,0xEF189647,0xFDAD39A9,0x45115ECC,
    0x764DEE06,0xCEF18963,0xDC44268D,0x64F841E8,0xF92F7951,0x41931E34,0x5326B1DA,0xEB9AD6BF,
    0xB3F9C6E9,0x0B45A18C,0x19F00E62,0xA14C6907,0x3C9B51BE,0x842736DB,0x96929935,0x2E2EFE50,
    0x2654B999,0x9EE8DEFC,0x8C5D7112,0x34E11677,0xA9362ECE,0x118A49AB,0x033FE645,0xBB838120,
    0xE3E09176,0x5B5CF613,0x49E959FD,0xF1553E98,0x6C820621,0xD43E6144,0xC68BCEAA,0x7E37A9CF,
    0xD67F4138,0x6EC3265D,0x7C7689B3,0xC4CAEED6,0x591DD66F,0xE1A1B10A,0xF3141EE4,0x4BA87981,
    0x13CB69D7,0xAB770EB2,0xB9C2A15C,0x017EC639,0x9CA9FE80,0x241599E5,0x36A0360B,0x8E1C516E,
    0x866616A7,0x3EDA71C2,0x2C6FDE2C,0x94D3B949,0x090481F0,0xB1B8E695,0xA30D497B,0x1BB12E1E,
    0x43D23E48,0xFB6E592D,0xE9DBF6C3,0x516791A6,0xCCB0A91F,0x740CCE7A,0x66B96194,0xDE0506F1,
  }
#endif // defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_4) || defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_8) || defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_16)
#if defined (CRC32_USE_LOOKUP_TABLE_SLICING_BY_8) || defined(CRC32_USE_LOOKUP_TABLE_SLICING_BY_16)
  // beyond this point only relevant for Slicing-by-8 and Slicing-by-16
  ,{
    0x00000000,0x3D6029B0,0x7AC05360,0x47A07AD0,0xF580A6C0,0xC8E08F70,0x8F40F5A0,0xB220DC10,
    0x30704BC1,0x0D106271,0x4AB018A1,0x77D03111,0xC5F0ED01,0xF890C4B1,0xBF30BE61,0x825097D1,
    0x60E09782,0x5D80BE32,0x1A20C4E2,0x2740ED52,0x95603142,0xA80018F2,0xEFA06222,0xD2C04B92,
    0x5090DC43,0x6DF0F5F3,0x2A508F23,0x1730A693,0xA5107A83,0x98705333,0xDFD029E3,0xE2B00053,
    0xC1C12F04,0xFCA106B4,0xBB017C64,0x866155D4,0x344189C4,0x0921A074,0x4E81DAA4,0x73E1F314,
    0xF1B164C5,0xCCD14D75,0x8B7137A5,0xB6111E15,0x0431C205,0x3951EBB5,0x7EF19165,0x4391B8D5,
    0xA121B886,0x9C419136,0xDBE1EBE6,0xE681C256,0x54A11E46,0x69C137F6,0x2E614D26,0x13016496,
    0x9151F347,0xAC31DAF7,0xEB91A027,0xD6F18997,0x64D15587,0x59B17C37,0x1E1106E7,0x23712F57,
    0x58F35849,0x659371F9,0x22330B29,0x1F532299,0xAD73FE89,0x9013D739,0xD7B3ADE9,0xEAD38459,
    0x68831388,0x55E33A38,0x124340E8,0x2F236958,0x9D03B548,0xA0639CF8,0xE7C3E628,0xDAA3CF98,
    0x3813CFCB,0x0573E67B,0x42D39CAB,0x7FB3B51B,0xCD93690B,0xF0F340BB,0xB7533A6B,0x8A3313DB,
    0x0863840A,0x3503ADBA,0x72A3D76A,0x4FC3FEDA,0xFDE322CA,0xC0830B7A,0x872371AA,0xBA43581A,
    0x9932774D,0xA4525EFD,0xE3F2242D,0xDE920D9D,0x6CB2D18D,0x51D2F83D,0x167282ED,0x2B12AB5D,
    0xA9423C8C,0x9422153C,0xD3826FEC,0xEEE2465C,0x5CC29A4C,0x61A2B3FC,0x2602C92C,0x1B62E09C,
    0xF9D2E0CF,0xC4B2C97F,0x8312B3AF,0xBE729A1F,0x0C52460F,0x31326FBF,0x7692156F,0x4BF23CDF,
    0xC9A2AB0E,0xF4C282BE,0xB362F86E,0x8E02D1DE,0x3C220DCE,0x0142247E,0x46E25EAE,0x7B82771E,
    0xB1E6B092,0x8C869922,0xCB26E3F2,0xF646CA42,0x44661652,0x79063FE2,0x3EA64532,0x03C66C82,
    0x8196FB53,0xBCF6D2E3,0xFB56A833,0xC6368183,0x74165D93,0x49767423,0x0ED60EF3,0x33B62743,
    0xD1062710,0xEC660EA0,0xABC67470,0x96A65DC0,0x248681D0,0x19E6A860,0x5E46D2B0,0x6326FB00,
    0xE1766CD1,0xDC164561,0x9BB63FB1,0xA6D61601,0x14F6CA11,0x2996E3A1,0x6E369971,0x5356B0C1,
    0x70279F96,0x4D47B626,0x0AE7CCF6,0x3787E546,0x85A73956,0xB8C710E6,0xFF676A36,0xC2074386,
    0x4057D457,0x7D37FDE7,0x3A978737,0x07F7AE87,0xB5D77297,0x88B75B27,0xCF1721F7,0xF2770847,
    0x10C70814,0x2DA721A4,0x6A075B74,0x576772C4,0xE547AED4,0xD8278764,0x9F87FDB4,0xA2E7D404,
    0x20B743D5,0x1DD76A65,0x5A7710B5,0x67173905,0xD537E515,0xE857CCA5,0xAFF7B675,0x92979FC5,
    0xE915E8DB,0xD475C16B,0x93D5BBBB,0xAEB5920B,0x1C954E1B,0x21F567AB,0x66551D7B,0x5B3534CB,
    0xD965A31A,0xE4058AAA,0xA3A5F07A,0x9EC5D9CA,0x2CE505DA,0x11852C6A,0x562556BA,0x6B457F0A,
    0x89F57F59,0xB49556E9,0xF3352C39,0xCE550589,0x7C75D999,0x4115F029,0x06B58AF9,0x3BD5A349,
    0xB9853498,0x84E51D28,0xC34567F8,0xFE254E48,0x4C059258,0x7165BBE8,0x36C5C138,0x0BA5E888,
    0x28D4C7DF,0x15B4EE6F,0x521494BF,0x6F74BD0F,0xDD54611F,0xE03448AF,0xA794327F,0x9AF41BCF,
    0x18A48C1E,0x25C4A5AE,0x6264DF7E,0x5F04F6CE,0xED242ADE,0xD044036E,0x97E479BE,0xAA84500E,
    0x4834505D,0x755479ED,0x32F4033D,0x0F942A8D,0xBDB4F69D,0x80D4DF2D,0xC774A5FD,0xFA148C4D,
    0x78441B9C,0x4524322C,0x028448FC,0x3FE4614C,0x8DC4BD5C,0xB0A494EC,0xF704EE3C,0xCA64C78C,
  },

  {
    0x00000000,0xCB5CD3A5,0x4DC8A10B,0x869472AE,0x9B914216,0x50CD91B3,0xD659E31D,0x1D0530B8,
    0xEC53826D,0x270F51C8,0xA19B2366,0x6AC7F0C3,0x77C2C07B,0xBC9E13DE,0x3A0A6170,0xF156B2D5,
    0x03D6029B,0xC88AD13E,0x4E1EA390,0x85427035,0x9847408D,0x531B9328,0xD58FE186,0x1ED33223,
    0xEF8580F6,0x24D95353,0xA24D21FD,0x6911F258,0x7414C2E0,0xBF481145,0x39DC63EB,0xF280B04E,
    0x07AC0536,0xCCF0D693,0x4A64A43D,0x81387798,0x9C3D4720,0x57619485,0xD1F5E62B,0x1AA9358E,
    0xEBFF875B,0x20A354FE,0xA6372650,0x6D6BF5F5,0x706EC54D,0xBB3216E8,0x3DA66446,0xF6FAB7E3,
    0x047A07AD,0xCF26D408,0x49B2A6A6,0x82EE7503,0x9FEB45BB,0x54B7961E,0xD223E4B0,0x197F3715,
    0xE82985C0,0x23755665,0xA5E124CB,0x6EBDF76E,0x73B8C7D6,0xB8E41473,0x3E7066DD,0xF52CB578,
    0x0F580A6C,0xC404D9C9,0x4290AB67,0x89CC78C2,0x94C9487A,0x5F959BDF,0xD901E971,0x125D3AD4,
    0xE30B8801,0x28575BA4,0xAEC3290A,0x659FFAAF,0x789ACA17,0xB3C619B2,0x35526B1C,0xFE0EB8B9,
    0x0C8E08F7,0xC7D2DB52,0x4146A9FC,0x8A1A7A59,0x971F4AE1,0x5C439944,0xDAD7EBEA,0x118B384F,
    0xE0DD8A9A,0x2B81593F,0xAD152B91,0x6649F834,0x7B4CC88C,0xB0101B29,0x36846987,0xFDD8BA22,
    0x08F40F5A,0xC3A8DCFF,0x453CAE51,0x8E607DF4,0x93654D4C,0x58399EE9,0xDEADEC47,0x15F13FE2,
    0xE4A78D37,0x2FFB5E92,0xA96F2C3C,0x6233FF99,0x7F36CF21,0xB46A1C84,0x32FE6E2A,0xF9A2BD8F,
    0x0B220DC1,0xC07EDE64,0x46EAACCA,0x8DB67F6F,0x90B34FD7,0x5BEF9C72,0xDD7BEEDC,0x16273D79,
    0xE7718FAC,0x2C2D5C09,0xAAB92EA7,0x61E5FD02,0x7CE0CDBA,0xB7BC1E1F,0x31286CB1,0xFA74BF14,
    0x1EB014D8,0xD5ECC77D,0x5378B5D3,0x98246676,0x852156CE,0x4E7D856B,0xC8E9F7C5,0x03B52460,
    0xF2E396B5,0x39BF4510,0xBF2B37BE,0x7477E41B,0x6972D4A3,0xA22E0706,0x24BA75A8,0xEFE6A60D,
    0x1D661643,0xD63AC5E6,0x50AEB748,0x9BF264ED,0x86F75455,0x4DAB87F0,0xCB3FF55E,0x006326FB,
    0xF135942E,0x3A69478B,0xBCFD3525,0x77A1E680,0x6AA4D638,0xA1F8059D,0x276C7733,0xEC30A496,
    0x191C11EE,0xD240C24B,0x54D4B0E5,0x9F886340,0x828D53F8,0x49D1805D,0xCF45F2F3,0x04192156,
    0xF54F9383,0x3E134026,0xB8873288,0x73DBE12D,0x6EDED195,0xA5820230,0x2316709E,0xE84AA33B,
    0x1ACA1375,0xD196C0D0,0x5702B27E,0x9C5E61DB,0x815B5163,0x4A0782C6,0xCC93F068,0x07CF23CD,
    0xF6999118,0x3DC542BD,0xBB513013,0x700DE3B6,0x6D08D30E,0xA65400AB,0x20C07205,0xEB9CA1A0,
    0x11E81EB4,0xDAB4CD11,0x5C20BFBF,0x977C6C1A,0x8A795CA2,0x41258F07,0xC7B1FDA9,0x0CED2E0C,
    0xFDBB9CD9,0x36E74F7C,0xB0733DD2,0x7B2FEE77,0x662ADECF,0xAD760D6A,0x2BE27FC4,0xE0BEAC61,
    0x123E1C2F,0xD962CF8A,0x5FF6BD24,0x94AA6E81,0x89AF5E39,0x42F38D9C,0xC467FF32,0x0F3B2C97,
    0xFE6D9E42,0x35314DE7,0xB3A53F49,0x78F9ECEC,0x65FCDC54,0xAEA00FF1,0x28347D5F,0xE368AEFA,
    0x16441B82,0xDD18C827,0x5B8CBA89,0x90D0692C,0x8DD55994,0x46898A31,0xC01DF89F,0x0B412B3A,
    0xFA1799EF,0x314B4A4A,0xB7DF38E4,0x7C83EB41,0x6186DBF9,0xAADA085C,0x2C4E7AF2,0xE712A957,
    0x15921919,0xDECECABC,0x585AB812,0x93066BB7,0x8E035B0F,0x455F88AA,0xC3CBFA04,0x089729A1,
    0xF9C19B74,0x329D48D1,0xB4093A7F,0x7F55E9DA,0x6250D962,0xA90C0AC7,0x2F987869,0xE4C4ABCC,
  },

  {
    0x00000000,0xA6770BB4,0x979F1129,0x31E81A9D,0xF44F2413,0x52382FA7,0x63D0353A,0xC5A73E8E,
    0x33EF4E67,0x959845D3,0xA4705F4E,0x020754FA,0xC7A06A74,0x61D761C0,0x503F7B5D,0xF64870E9,
    0x67DE9CCE,0xC1A9977A,0xF0418DE7,0x56368653,0x9391B8DD,0x35E6B369,0x040EA9F4,0xA279A240,
    0x5431D2A9,0xF246D91D,0xC3AEC380,0x65D9C834,0xA07EF6BA,0x0609FD0E,0x37E1E793,0x9196EC27,
    0xCFBD399C,0x69CA3228,0x582228B5,0xFE552301,0x3BF21D8F,0x9D85163B,0xAC6D0CA6,0x0A1A0712,
    0xFC5277FB,0x5A257C4F,0x6BCD66D2,0xCDBA6D66,0x081D53E8,0xAE6A585C,0x9F8242C1,0x39F54975,
    0xA863A552,0x0E14AEE6,0x3FFCB47B,0x998BBFCF,0x5C2C8141,0xFA5B8AF5,0xCBB39068,0x6DC49BDC,
    0x9B8CEB35,0x3DFBE081,0x0C13FA1C,0xAA64F1A8,0x6FC3CF26,0xC9B4C492,0xF85CDE0F,0x5E2BD5BB,
    0x440B7579,0xE27C7ECD,0xD3946450,0x75E36FE4,0xB044516A,0x16335ADE,0x27DB4043,0x81AC4BF7,
    0x77E43B1E,0xD19330AA,0xE07B2A37,0x460C2183,0x83AB1F0D,0x25DC14B9,0x14340E24,0xB2430590,
    0x23D5E9B7,0x85A2E203,0xB44AF89E,0x123DF32A,0xD79ACDA4,0x71EDC610,0x4005DC8D,0xE672D739,
    0x103AA7D0,0xB64DAC64,0x87A5B6F9,0x21D2BD4D,0xE47583C3,0x42028877,0x73EA92EA,0xD59D995E,
    0x8BB64CE5,0x2DC14751,0x1C295DCC,0xBA5E5678,0x7FF968F6,0xD98E6342,0xE86679DF,0x4E11726B,
    0xB8590282,0x1E2E0936,0x2FC613AB,0x89B1181F,0x4C162691,0xEA612D25,0xDB8937B8,0x7DFE3C0C,
    0xEC68D02B,0x4A1FDB9F,0x7BF7C102,0xDD80CAB6,0x1827F438,0xBE50FF8C,0x8FB8E511,0x29CFEEA5,
    0xDF879E4C,0x79F095F8,0x48188F65,0xEE6F84D1,0x2BC8BA5F,0x8DBFB1EB,0xBC57AB76,0x1A20A0C2,
    0x8816EAF2,0x2E61E146,0x1F89FBDB,0xB9FEF06F,0x7C59CEE1,0xDA2EC555,0xEBC6DFC8,0x4DB1D47C,
    0xBBF9A495,0x1D8EAF21,0x2C66B5BC,0x8A11BE08,0x4FB68086,0xE9C18B32,0xD82991AF,0x7E5E9A1B,
    0xEFC8763C,0x49BF7D88,0x78576715,0xDE206CA1,0x1B87522F,0xBDF0599B,0x8C184306,0x2A6F48B2,
    0xDC27385B,0x7A5033EF,0x4BB82972,0xEDCF22C6,0x28681C48,0x8E1F17FC,0xBFF70D61,0x198006D5,
    0x47ABD36E,0xE1DCD8DA,0xD034C247,0x7643C9F3,0xB3E4F77D,0x1593FCC9,0x247BE654,0x820CEDE0,
    0x74449D09,0xD23396BD,0xE3DB8C20,0x45AC8794,0x800BB91A,0x267CB2AE,0x1794A833,0xB1E3A387,
    0x20754FA0,0x86024414,0xB7EA5E89,0x119D553D,0xD43A6BB3,0x724D6007,0x43A57A9A,0xE5D2712E,
    0x139A01C7,0xB5ED0A73,0x840510EE,0x22721B5A,0xE7D525D4,0x41A22E60,0x704A34FD,0xD63D3F49,
    0xCC1D9F8B,0x6A6A943F,0x5B828EA2,0xFDF58516,0x3852BB98,0x9E25B02C,0xAFCDAAB1,0x09BAA105,
    0xFFF2D1EC,0x5985DA58,0x686DC0C5,0xCE1ACB71,0x0BBDF5FF,0xADCAFE4B,0x9C22E4D6,0x3A55EF62,
    0xABC30345,0x0DB408F1,0x3C5C126C,0x9A2B19D8,0x5F8C2756,0xF9FB2CE2,0xC813367F,0x6E643DCB,
    0x982C4D22,0x3E5B4696,0x0FB35C0B,0xA9C457BF,0x6C636931,0xCA146285,0xFBFC7818,0x5D8B73AC,
    0x03A0A617,0xA5D7ADA3,0x943FB73E,0x3248BC8A,0xF7EF8204,0x519889B0,0x6070932D,0xC6079899,
    0x304FE870,0x9638E3C4,0xA7D0F959,0x01A7F2ED,0xC400CC63,0x6277C7D7,0x539FDD4A,0xF5E8D6FE,
    0x647E3AD9,0xC209316D,0xF3E12BF0,0x55962044,0x90311ECA,0x3646157E,0x07AE0FE3,0xA1D90457,
    0x579174BE,0xF1E67F0A,0xC00E6597,0x66796E23,0xA3DE50AD,0x05A95B19,0x34414184,0x92364A30,
  },

  {
    0x00000000,0xCCAA009E,0x4225077D,0x8E8F07E3,0x844A0EFA,0x48E00E64,0xC66F0987,0x0AC50919,
    0xD3E51BB5,0x1F4F1B2B,0x91C01CC8,0x5D6A1C56,0x57AF154F,0x9B0515D1,0x158A1232,0xD92012AC,
    0x7CBB312B,0xB01131B5,0x3E9E3656,0xF23436C8,0xF8F13FD1,0x345B3F4F,0xBAD438AC,0x767E3832,
    0xAF5E2A9E,0x63F42A00,0xED7B2DE3,0x21D12D7D,0x2B142464,0xE7BE24FA,0x69312319,0xA59B2387,
    0xF9766256,0x35DC62C8,0xBB53652B,0x77F965B5,0x7D3C6CAC,0xB1966C32,0x3F196BD1,0xF3B36B4F,
    0x2A9379E3,0xE639797D,0x68B67E9E,0xA41C7E00,0xAED97719,0x62737787,0xECFC7064,0x205670FA,
    0x85CD537D,0x496753E3,0xC7E85400,0x0B42549E,0x01875D87,0xCD2D5D19,0x43A25AFA,0x8F085A64,
    0x562848C8,0x9A824856,0x140D4FB5,0xD8A74F2B,0xD2624632,0x1EC846AC,0x9047414F,0x5CED41D1,
    0x299DC2ED,0xE537C273,0x6BB8C590,0xA712C50E,0xADD7CC17,0x617DCC89,0xEFF2CB6A,0x2358CBF4,
    0xFA78D958,0x36D2D9C6,0xB85DDE25,0x74F7DEBB,0x7E32D7A2,0xB298D73C,0x3C17D0DF,0xF0BDD041,
    0x5526F3C6,0x998CF358,0x1703F4BB,0xDBA9F425,0xD16CFD3C,0x1DC6FDA2,0x9349FA41,0x5FE3FADF,
    0x86C3E873,0x4A69E8ED,0xC4E6EF0E,0x084CEF90,0x0289E689,0xCE23E617,0x40ACE1F4,0x8C06E16A,
    0xD0EBA0BB,0x1C41A025,0x92CEA7C6,0x5E64A758,0x54A1AE41,0x980BAEDF,0x1684A93C,0xDA2EA9A2,
    0x030EBB0E,0xCFA4BB90,0x412BBC73,0x8D81BCED,0x8744B5F4,0x4BEEB56A,0xC561B289,0x09CBB217,
    0xAC509190,0x60FA910E,0xEE7596ED,0x22DF9673,0x281A9F6A,0xE4B09FF4,0x6A3F9817,0xA6959889,
    0x7FB58A25,0xB31F8ABB,0x3D908D58,0xF13A8DC6,0xFBFF84DF,0x37558441,0xB9DA83A2,0x7570833C,
    0x533B85DA,0x9F918544,0x111E82A7,0xDDB48239,0xD7718B20,0x1BDB8BBE,0x95548C5D,0x59FE8CC3,
    0x80DE9E6F,0x4C749EF1,0xC2FB9912,0x0E51998C,0x04949095,0xC83E900B,0x46B197E8,0x8A1B9776,
    0x2F80B4F1,0xE32AB46F,0x6DA5B38C,0xA10FB312,0xABCABA0B,0x6760BA95,0xE9EFBD76,0x2545BDE8,
    0xFC65AF44,0x30CFAFDA,0xBE40A839,0x72EAA8A7,0x782FA1BE,0xB485A120,0x3A0AA6C3,0xF6A0A65D,
    0xAA4DE78C,0x66E7E712,0xE868E0F1,0x24C2E06F,0x2E07E976,0xE2ADE9E8,0x6C22EE0B,0xA088EE95,
    0x79A8FC39,0xB502FCA7,0x3B8DFB44,0xF727FBDA,0xFDE2F2C3,0x3148F25D,0xBFC7F5BE,0x736DF520,
    0xD6F6D6A7,0x1A5CD639,0x94D3D1DA,0x5879D144,0x52BCD85D,0x9E16D8C3,0x1099DF20,0xDC33DFBE,
    0x0513CD12,0xC9B9CD8C,0x4736CA6F,0x8B9CCAF1,0x8159C3E8,0x4DF3C376,0xC37CC495,0x0FD6C40B,
    0x7AA64737,0xB60C47A9,0x3883404A,0xF42940D4,0xFEEC49CD,0x32464953,0xBCC94EB0,0x70634E2E,
    0xA9435C82,0x65E95C1C,0xEB665BFF,0x27CC5B61,0x2D095278,0xE1A352E6,0x6F2C5505,0xA386559B,
    0x061D761C,0xCAB77682,0x44387161,0x889271FF,0x825778E6,0x4EFD7878,0xC0727F9B,0x0CD87F05,
    0xD5F86DA9,0x19526D37,0x97DD6AD4,0x5B776A4A,0x51B26353,0x9D1863CD,0x1397642E,0xDF3D64B0,
    0x83D02561,0x4F7A25FF,0xC1F5221C,0x0D5F2282,0x079A2B9B,0xCB302B05,0x45BF2CE6,0x89152C78,
    0x50353ED4,0x9C9F3E4A,0x121039A9,0xDEBA3937,0xD47F302E,0x18D530B0,0x965A3753,0x5AF037CD,
    0xFF6B144A,0x33C114D4,0xBD4E1337,0x71E413A9,0x7B211AB0,0xB78B1A2E,0x39041DCD,0xF5AE1D53,
    0x2C8E0FFF,0xE0240F61,0x6EAB0882,0xA201081C,0xA8C40105,0x646E019B,0xEAE10678,0x264B06E6,
  }
#endif // CRC32_USE_LOOKUP_TABLE_SLICING_BY_8 || CRC32_USE_LOOKUP_TABLE_SLICING_BY_16
#ifdef CRC32_USE_LOOKUP_TABLE_SLICING_BY_16
  // beyond this point only relevant for Slicing-by-16
  ,{
    0x00000000,0x177B1443,0x2EF62886,0x398D3CC5,0x5DEC510C,0x4A97454F,0x731A798A,0x64616DC9,
    0xBBD8A218,0xACA3B65B,0x952E8A9E,0x82559EDD,0xE634F314,0xF14FE757,0xC8C2DB92,0xDFB9CFD1,
    0xACC04271,0xBBBB5632,0x82366AF7,0x954D7EB4,0xF12C137D,0xE657073E,0xDFDA3BFB,0xC8A12FB8,
    0x1718E069,0x0063F42A,0x39EEC8EF,0x2E95DCAC,0x4AF4B165,0x5D8FA526,0x640299E3,0x73798DA0,
    0x82F182A3,0x958A96E0,0xAC07AA25,0xBB7CBE66,0xDF1DD3AF,0xC866C7EC,0xF1EBFB29,0xE690EF6A,
    0x392920BB,0x2E5234F8,0x17DF083D,0x00A41C7E,0x64C571B7,0x73BE65F4,0x4A335931,0x5D484D72,
    0x2E31C0D2,0x394AD491,0x00C7E854,0x17BCFC17,0x73DD91DE,0x64A6859D,0x5D2BB958,0x4A50AD1B,
    0x95E962CA,0x82927689,0xBB1F4A4C,0xAC645E0F,0xC80533C6,0xDF7E2785,0xE6F31B40,0xF1880F03,
    0xDE920307,0xC9E91744,0xF0642B81,0xE71F3FC2,0x837E520B,0x94054648,0xAD887A8D,0xBAF36ECE,
    0x654AA11F,0x7231B55C,0x4BBC8999,0x5CC79DDA,0x38A6F013,0x2FDDE450,0x1650D895,0x012BCCD6,
    0x72524176,0x65295535,0x5CA469F0,0x4BDF7DB3,0x2FBE107A,0x38C50439,0x014838FC,0x16332CBF,
    0xC98AE36E,0xDEF1F72D,0xE77CCBE8,0xF007DFAB,0x9466B262,0x831DA621,0xBA909AE4,0xADEB8EA7,
    0x5C6381A4,0x4B1895E7,0x7295A922,0x65EEBD61,0x018FD0A8,0x16F4C4EB,0x2F79F82E,0x3802EC6D,
    0xE7BB23BC,0xF0C037FF,0xC94D0B3A,0xDE361F79,0xBA5772B0,0xAD2C66F3,0x94A15A36,0x83DA4E75,
    0xF0A3C3D5,0xE7D8D796,0xDE55EB53,0xC92EFF10,0xAD4F92D9,0xBA34869A,0x83B9BA5F,0x94C2AE1C,
    0x4B7B61CD,0x5C00758E,0x658D494B,0x72F65D08,0x169730C1,0x01EC2482,0x38611847,0x2F1A0C04,
    0x6655004F,0x712E140C,0x48A328C9,0x5FD83C8A,0x3BB95143,0x2CC24500,0x154F79C5,0x02346D86,
    0xDD8DA257,0xCAF6B614,0xF37B8AD1,0xE4009E92,0x8061F35B,0x971AE718,0xAE97DBDD,0xB9ECCF9E,
    0xCA95423E,0xDDEE567D,0xE4636AB8,0xF3187EFB,0x97791332,0x80020771,0xB98F3BB4,0xAEF42FF7,
    0x714DE026,0x6636F465,0x5FBBC8A0,0x48C0DCE3,0x2CA1B12A,0x3BDAA569,0x025799AC,0x152C8DEF,
    0xE4A482EC,0xF3DF96AF,0xCA52AA6A,0xDD29BE29,0xB948D3E0,0xAE33C7A3,0x97BEFB66,0x80C5EF25,
    0x5F7C20F4,0x480734B7,0x718A0872,0x66F11C31,0x029071F8,0x15EB65BB,0x2C66597E,0x3B1D4D3D,
    0x4864C09D,0x5F1FD4DE,0x6692E81B,0x71E9FC58,0x15889191,0x02F385D2,0x3B7EB917,0x2C05AD54,
    0xF3BC6285,0xE4C776C6,0xDD4A4A03,0xCA315E40,0xAE503389,0xB92B27CA,0x80A61B0F,0x97DD0F4C,
    0xB8C70348,0xAFBC170B,0x96312BCE,0x814A3F8D,0xE52B5244,0xF2504607,0xCBDD7AC2,0xDCA66E81,
    0x031FA150,0x1464B513,0x2DE989D6,0x3A929D95,0x5EF3F05C,0x4988E41F,0x7005D8DA,0x677ECC99,
    0x14074139,0x037C557A,0x3AF169BF,0x2D8A7DFC,0x49EB1035,0x5E900476,0x671D38B3,0x70662CF0,
    0xAFDFE321,0xB8A4F762,0x8129CBA7,0x9652DFE4,0xF233B22D,0xE548A66E,0xDCC59AAB,0xCBBE8EE8,
    0x3A3681EB,0x2D4D95A8,0x14C0A96D,0x03BBBD2E,0x67DAD0E7,0x70A1C4A4,0x492CF861,0x5E57EC22,
    0x81EE23F3,0x969537B0,0xAF180B75,0xB8631F36,0xDC0272FF,0xCB7966BC,0xF2F45A79,0xE58F4E3A,
    0x96F6C39A,0x818DD7D9,0xB800EB1C,0xAF7BFF5F,0xCB1A9296,0xDC6186D5,0xE5ECBA10,0xF297AE53,
    0x2D2E6182,0x3A5575C1,0x03D84904,0x14A35D47,0x70C2308E,0x67B924CD,0x5E341808,0x494F0C4B,
  },

  {
    0x00000000,0xEFC26B3E,0x04F5D03D,0xEB37BB03,0x09EBA07A,0xE629CB44,0x0D1E7047,0xE2DC1B79,
    0x13D740F4,0xFC152BCA,0x172290C9,0xF8E0FBF7,0x1A3CE08E,0xF5FE8BB0,0x1EC930B3,0xF10B5B8D,
    0x27AE81E8,0xC86CEAD6,0x235B51D5,0xCC993AEB,0x2E452192,0xC1874AAC,0x2AB0F1AF,0xC5729A91,
    0x3479C11C,0xDBBBAA22,0x308C1121,0xDF4E7A1F,0x3D926166,0xD2500A58,0x3967B15B,0xD6A5DA65,
    0x4F5D03D0,0xA09F68EE,0x4BA8D3ED,0xA46AB8D3,0x46B6A3AA,0xA974C894,0x42437397,0xAD8118A9,
    0x5C8A4324,0xB348281A,0x587F9319,0xB7BDF827,0x5561E35E,0xBAA38860,0x51943363,0xBE56585D,
    0x68F38238,0x8731E906,0x6C065205,0x83C4393B,0x61182242,0x8EDA497C,0x65EDF27F,0x8A2F9941,
    0x7B24C2CC,0x94E6A9F2,0x7FD112F1,0x901379CF,0x72CF62B6,0x9D0D0988,0x763AB28B,0x99F8D9B5,
    0x9EBA07A0,0x71786C9E,0x9A4FD79D,0x758DBCA3,0x9751A7DA,0x7893CCE4,0x93A477E7,0x7C661CD9,
    0x8D6D4754,0x62AF2C6A,0x89989769,0x665AFC57,0x8486E72E,0x6B448C10,0x80733713,0x6FB15C2D,
    0xB9148648,0x56D6ED76,0xBDE15675,0x52233D4B,0xB0FF2632,0x5F3D4D0C,0xB40AF60F,0x5BC89D31,
    0xAAC3C6BC,0x4501AD82,0xAE361681,0x41F47DBF,0xA32866C6,0x4CEA0DF8,0xA7DDB6FB,0x481FDDC5,
    0xD1E70470,0x3E256F4E,0xD512D44D,0x3AD0BF73,0xD80CA40A,0x37CECF34,0xDCF97437,0x333B1F09,
    0xC2304484,0x2DF22FBA,0xC6C594B9,0x2907FF87,0xCBDBE4FE,0x24198FC0,0xCF2E34C3,0x20EC5FFD,
    0xF6498598,0x198BEEA6,0xF2BC55A5,0x1D7E3E9B,0xFFA225E2,0x10604EDC,0xFB57F5DF,0x14959EE1,
    0xE59EC56C,0x0A5CAE52,0xE16B1551,0x0EA97E6F,0xEC756516,0x03B70E28,0xE880B52B,0x0742DE15,
    0xE6050901,0x09C7623F,0xE2F0D93C,0x0D32B202,0xEFEEA97B,0x002CC245,0xEB1B7946,0x04D91278,
    0xF5D249F5,0x1A1022CB,0xF12799C8,0x1EE5F2F6,0xFC39E98F,0x13FB82B1,0xF8CC39B2,0x170E528C,
    0xC1AB88E9,0x2E69E3D7,0xC55E58D4,0x2A9C33EA,0xC8402893,0x278243AD,0xCCB5F8AE,0x23779390,
    0xD27CC81D,0x3DBEA323,0xD6891820,0x394B731E,0xDB976867,0x34550359,0xDF62B85A,0x30A0D364,
    0xA9580AD1,0x469A61EF,0xADADDAEC,0x426FB1D2,0xA0B3AAAB,0x4F71C195,0xA4467A96,0x4B8411A8,
    0xBA8F4A25,0x554D211B,0xBE7A9A18,0x51B8F126,0xB364EA5F,0x5CA68161,0xB7913A62,0x5853515C,
    0x8EF68B39,0x6134E007,0x8A035B04,0x65C1303A,0x871D2B43,0x68DF407D,0x83E8FB7E,0x6C2A9040,
    0x9D21CBCD,0x72E3A0F3,0x99D41BF0,0x761670CE,0x94CA6BB7,0x7B080089,0x903FBB8A,0x7FFDD0B4,
    0x78BF0EA1,0x977D659F,0x7C4ADE9C,0x9388B5A2,0x7154AEDB,0x9E96C5E5,0x75A17EE6,0x9A6315D8,
    0x6B684E55,0x84AA256B,0x6F9D9E68,0x805FF556,0x6283EE2F,0x8D418511,0x66763E12,0x89B4552C,
    0x5F118F49,0xB0D3E477,0x5BE45F74,0xB426344A,0x56FA2F33,0xB938440D,0x520FFF0E,0xBDCD9430,
    0x4CC6CFBD,0xA304A483,0x48331F80,0xA7F174BE,0x452D6FC7,0xAAEF04F9,0x41D8BFFA,0xAE1AD4C4,
    0x37E20D71,0xD820664F,0x3317DD4C,0xDCD5B672,0x3E09AD0B,0xD1CBC635,0x3AFC7D36,0xD53E1608,
    0x24354D85,0xCBF726BB,0x20C09DB8,0xCF02F686,0x2DDEEDFF,0xC21C86C1,0x292B3DC2,0xC6E956FC,
    0x104C8C99,0xFF8EE7A7,0x14B95CA4,0xFB7B379A,0x19A72CE3,0xF66547DD,0x1D52FCDE,0xF29097E0,
    0x039BCC6D,0xEC59A753,0x076E1C50,0xE8AC776E,0x0A706C17,0xE5B20729,0x0E85BC2A,0xE147D714,
  },

  {
    0x00000000,0xC18EDFC0,0x586CB9C1,0x99E26601,0xB0D97382,0x7157AC42,0xE8B5CA43,0x293B1583,
    0xBAC3E145,0x7B4D3E85,0xE2AF5884,0x23218744,0x0A1A92C7,0xCB944D07,0x52762B06,0x93F8F4C6,
    0xAEF6C4CB,0x6F781B0B,0xF69A7D0A,0x3714A2CA,0x1E2FB749,0xDFA16889,0x46430E88,0x87CDD148,
    0x1435258E,0xD5BBFA4E,0x4C599C4F,0x8DD7438F,0xA4EC560C,0x656289CC,0xFC80EFCD,0x3D0E300D,
    0x869C8FD7,0x47125017,0xDEF03616,0x1F7EE9D6,0x3645FC55,0xF7CB2395,0x6E294594,0xAFA79A54,
    0x3C5F6E92,0xFDD1B152,0x6433D753,0xA5BD0893,0x8C861D10,0x4D08C2D0,0xD4EAA4D1,0x15647B11,
    0x286A4B1C,0xE9E494DC,0x7006F2DD,0xB1882D1D,0x98B3389E,0x593DE75E,0xC0DF815F,0x01515E9F,
    0x92A9AA59,0x53277599,0xCAC51398,0x0B4BCC58,0x2270D9DB,0xE3FE061B,0x7A1C601A,0xBB92BFDA,
    0xD64819EF,0x17C6C62F,0x8E24A02E,0x4FAA7FEE,0x66916A6D,0xA71FB5AD,0x3EFDD3AC,0xFF730C6C,
    0x6C8BF8AA,0xAD05276A,0x34E7416B,0xF5699EAB,0xDC528B28,0x1DDC54E8,0x843E32E9,0x45B0ED29,
    0x78BEDD24,0xB93002E4,0x20D264E5,0xE15CBB25,0xC867AEA6,0x09E97166,0x900B1767,0x5185C8A7,
    0xC27D3C61,0x03F3E3A1,0x9A1185A0,0x5B9F5A60,0x72A44FE3,0xB32A9023,0x2AC8F622,0xEB4629E2,
    0x50D49638,0x915A49F8,0x08B82FF9,0xC936F039,0xE00DE5BA,0x21833A7A,0xB8615C7B,0x79EF83BB,
    0xEA17777D,0x2B99A8BD,0xB27BCEBC,0x73F5117C,0x5ACE04FF,0x9B40DB3F,0x02A2BD3E,0xC32C62FE,
    0xFE2252F3,0x3FAC8D33,0xA64EEB32,0x67C034F2,0x4EFB2171,0x8F75FEB1,0x169798B0,0xD7194770,
    0x44E1B3B6,0x856F6C76,0x1C8D0A77,0xDD03D5B7,0xF438C034,0x35B61FF4,0xAC5479F5,0x6DDAA635,
    0x77E1359F,0xB66FEA5F,0x2F8D8C5E,0xEE03539E,0xC738461D,0x06B699DD,0x9F54FFDC,0x5EDA201C,
    0xCD22D4DA,0x0CAC0B1A,0x954E6D1B,0x54C0B2DB,0x7DFBA758,0xBC757898,0x25971E99,0xE419C159,
    0xD917F154,0x18992E94,0x817B4895,0x40F59755,0x69CE82D6,0xA8405D16,0x31A23B17,0xF02CE4D7,
    0x63D41011,0xA25ACFD1,0x3BB8A9D0,0xFA367610,0xD30D6393,0x1283BC53,0x8B61DA52,0x4AEF0592,
    0xF17DBA48,0x30F36588,0xA9110389,0x689FDC49,0x41A4C9CA,0x802A160A,0x19C8700B,0xD846AFCB,
    0x4BBE5B0D,0x8A3084CD,0x13D2E2CC,0xD25C3D0C,0xFB67288F,0x3AE9F74F,0xA30B914E,0x62854E8E,
    0x5F8B7E83,0x9E05A143,0x07E7C742,0xC6691882,0xEF520D01,0x2EDCD2C1,0xB73EB4C0,0x76B06B00,
    0xE5489FC6,0x24C64006,0xBD242607,0x7CAAF9C7,0x5591EC44,0x941F3384,0x0DFD5585,0xCC738A45,
    0xA1A92C70,0x6027F3B0,0xF9C595B1,0x384B4A71,0x11705FF2,0xD0FE8032,0x491CE633,0x889239F3,
    0x1B6ACD35,0xDAE412F5,0x430674F4,0x8288AB34,0xABB3BEB7,0x6A3D6177,0xF3DF0776,0x3251D8B6,
    0x0F5FE8BB,0xCED1377B,0x5733517A,0x96BD8EBA,0xBF869B39,0x7E0844F9,0xE7EA22F8,0x2664FD38,
    0xB59C09FE,0x7412D63E,0xEDF0B03F,0x2C7E6FFF,0x05457A7C,0xC4CBA5BC,0x5D29C3BD,0x9CA71C7D,
    0x2735A3A7,0xE6BB7C67,0x7F591A66,0xBED7C5A6,0x97ECD025,0x56620FE5,0xCF8069E4,0x0E0EB624,
    0x9DF642E2,0x5C789D22,0xC59AFB23,0x041424E3,0x2D2F3160,0xECA1EEA0,0x754388A1,0xB4CD5761,
    0x89C3676C,0x484DB8AC,0xD1AFDEAD,0x1021016D,0x391A14EE,0xF894CB2E,0x6176AD2F,0xA0F872EF,
    0x33008629,0xF28E59E9,0x6B6C3FE8,0xAAE2E028,0x83D9F5AB,0x42572A6B,0xDBB54C6A,0x1A3B93AA,
  },

  {
    0x00000000,0x9BA54C6F,0xEC3B9E9F,0x779ED2F0,0x03063B7F,0x98A37710,0xEF3DA5E0,0x7498E98F,
    0x060C76FE,0x9DA93A91,0xEA37E861,0x7192A40E,0x050A4D81,0x9EAF01EE,0xE931D31E,0x72949F71,
    0x0C18EDFC,0x97BDA193,0xE0237363,0x7B863F0C,0x0F1ED683,0x94BB9AEC,0xE325481C,0x78800473,
    0x0A149B02,0x91B1D76D,0xE62F059D,0x7D8A49F2,0x0912A07D,0x92B7EC12,0xE5293EE2,0x7E8C728D,
    0x1831DBF8,0x83949797,0xF40A4567,0x6FAF0908,0x1B37E087,0x8092ACE8,0xF70C7E18,0x6CA93277,
    0x1E3DAD06,0x8598E169,0xF2063399,0x69A37FF6,0x1D3B9679,0x869EDA16,0xF10008E6,0x6AA54489,
    0x14293604,0x8F8C7A6B,0xF812A89B,0x63B7E4F4,0x172F0D7B,0x8C8A4114,0xFB1493E4,0x60B1DF8B,
    0x122540FA,0x89800C95,0xFE1EDE65,0x65BB920A,0x11237B85,0x8A8637EA,0xFD18E51A,0x66BDA975,
    0x3063B7F0,0xABC6FB9F,0xDC58296F,0x47FD6500,0x33658C8F,0xA8C0C0E0,0xDF5E1210,0x44FB5E7F,
    0x366FC10E,0xADCA8D61,0xDA545F91,0x41F113FE,0x3569FA71,0xAECCB61E,0xD95264EE,0x42F72881,
    0x3C7B5A0C,0xA7DE1663,0xD040C493,0x4BE588FC,0x3F7D6173,0xA4D82D1C,0xD346FFEC,0x48E3B383,
    0x3A772CF2,0xA1D2609D,0xD64CB26D,0x4DE9FE02,0x3971178D,0xA2D45BE2,0xD54A8912,0x4EEFC57D,
    0x28526C08,0xB3F72067,0xC469F297,0x5FCCBEF8,0x2B545777,0xB0F11B18,0xC76FC9E8,0x5CCA8587,
    0x2E5E1AF6,0xB5FB5699,0xC2658469,0x59C0C806,0x2D582189,0xB6FD6DE6,0xC163BF16,0x5AC6F379,
    0x244A81F4,0xBFEFCD9B,0xC8711F6B,0x53D45304,0x274CBA8B,0xBCE9F6E4,0xCB772414,0x50D2687B,
    0x2246F70A,0xB9E3BB65,0xCE7D6995,0x55D825FA,0x2140CC75,0xBAE5801A,0xCD7B52EA,0x56DE1E85,
    0x60C76FE0,0xFB62238F,0x8CFCF17F,0x1759BD10,0x63C1549F,0xF86418F0,0x8FFACA00,0x145F866F,
    0x66CB191E,0xFD6E5571,0x8AF08781,0x1155CBEE,0x65CD2261,0xFE686E0E,0x89F6BCFE,0x1253F091,
    0x6CDF821C,0xF77ACE73,0x80E41C83,0x1B4150EC,0x6FD9B963,0xF47CF50C,0x83E227FC,0x18476B93,
    0x6AD3F4E2,0xF176B88D,0x86E86A7D,0x1D4D2612,0x69D5CF9D,0xF27083F2,0x85EE5102,0x1E4B1D6D,
    0x78F6B418,0xE353F877,0x94CD2A87,0x0F6866E8,0x7BF08F67,0xE055C308,0x97CB11F8,0x0C6E5D97,
    0x7EFAC2E6,0xE55F8E89,0x92C15C79,0x09641016,0x7DFCF999,0xE659B5F6,0x91C76706,0x0A622B69,
    0x74EE59E4,0xEF4B158B,0x98D5C77B,0x03708B14,0x77E8629B,0xEC4D2EF4,0x9BD3FC04,0x0076B06B,
    0x72E22F1A,0xE9476375,0x9ED9B185,0x057CFDEA,0x71E41465,0xEA41580A,0x9DDF8AFA,0x067AC695,
    0x50A4D810,0xCB01947F,0xBC9F468F,0x273A0AE0,0x53A2E36F,0xC807AF00,0xBF997DF0,0x243C319F,
    0x56A8AEEE,0xCD0DE281,0xBA933071,0x21367C1E,0x55AE9591,0xCE0BD9FE,0xB9950B0E,0x22304761,
    0x5CBC35EC,0xC7197983,0xB087AB73,0x2B22E71C,0x5FBA0E93,0xC41F42FC,0xB381900C,0x2824DC63,
    0x5AB04312,0xC1150F7D,0xB68BDD8D,0x2D2E91E2,0x59B6786D,0xC2133402,0xB58DE6F2,0x2E28AA9D,
    0x489503E8,0xD3304F87,0xA4AE9D77,0x3F0BD118,0x4B933897,0xD03674F8,0xA7A8A608,0x3C0DEA67,
    0x4E997516,0xD53C3979,0xA2A2EB89,0x3907A7E6,0x4D9F4E69,0xD63A0206,0xA1A4D0F6,0x3A019C99,
    0x448DEE14,0xDF28A27B,0xA8B6708B,0x33133CE4,0x478BD56B,0xDC2E9904,0xABB04BF4,0x3015079B,
    0x428198EA,0xD924D485,0xAEBA0675,0x351F4A1A,0x4187A395,0xDA22EFFA,0xADBC3D0A,0x36197165,
  },

  {
    0x00000000,0xDD96D985,0x605CB54B,0xBDCA6CCE,0xC0B96A96,0x1D2FB313,0xA0E5DFDD,0x7D730658,
    0x5A03D36D,0x87950AE8,0x3A5F6626,0xE7C9BFA3,0x9ABAB9FB,0x472C607E,0xFAE60CB0,0x2770D535,
    0xB407A6DA,0x69917F5F,0xD45B1391,0x09CDCA14,0x74BECC4C,0xA92815C9,0x14E27907,0xC974A082,
    0xEE0475B7,0x3392AC32,0x8E58C0FC,0x53CE1979,0x2EBD1F21,0xF32BC6A4,0x4EE1AA6A,0x937773EF,
    0xB37E4BF5,0x6EE89270,0xD322FEBE,0x0EB4273B,0x73C72163,0xAE51F8E6,0x139B9428,0xCE0D4DAD,
    0xE97D9898,0x34EB411D,0x89212DD3,0x54B7F456,0x29C4F20E,0xF4522B8B,0x49984745,0x940E9EC0,
    0x0779ED2F,0xDAEF34AA,0x67255864,0xBAB381E1,0xC7C087B9,0x1A565E3C,0xA79C32F2,0x7A0AEB77,
    0x5D7A3E42,0x80ECE7C7,0x3D268B09,0xE0B0528C,0x9DC354D4,0x40558D51,0xFD9FE19F,0x2009381A,
    0xBD8D91AB,0x601B482E,0xDDD124E0,0x0047FD65,0x7D34FB3D,0xA0A222B8,0x1D684E76,0xC0FE97F3,
    0xE78E42C6,0x3A189B43,0x87D2F78D,0x5A442E08,0x27372850,0xFAA1F1D5,0x476B9D1B,0x9AFD449E,
    0x098A3771,0xD41CEEF4,0x69D6823A,0xB4405BBF,0xC9335DE7,0x14A58462,0xA96FE8AC,0x74F93129,
    0x5389E41C,0x8E1F3D99,0x33D55157,0xEE4388D2,0x93308E8A,0x4EA6570F,0xF36C3BC1,0x2EFAE244,
    0x0EF3DA5E,0xD36503DB,0x6EAF6F15,0xB339B690,0xCE4AB0C8,0x13DC694D,0xAE160583,0x7380DC06,
    0x54F00933,0x8966D0B6,0x34ACBC78,0xE93A65FD,0x944963A5,0x49DFBA20,0xF415D6EE,0x29830F6B,
    0xBAF47C84,0x6762A501,0xDAA8C9CF,0x073E104A,0x7A4D1612,0xA7DBCF97,0x1A11A359,0xC7877ADC,
    0xE0F7AFE9,0x3D61766C,0x80AB1AA2,0x5D3DC327,0x204EC57F,0xFDD81CFA,0x40127034,0x9D84A9B1,
    0xA06A2517,0x7DFCFC92,0xC036905C,0x1DA049D9,0x60D34F81,0xBD459604,0x008FFACA,0xDD19234F,
    0xFA69F67A,0x27FF2FFF,0x9A354331,0x47A39AB4,0x3AD09CEC,0xE7464569,0x5A8C29A7,0x871AF022,
    0x146D83CD,0xC9FB5A48,0x74313686,0xA9A7EF03,0xD4D4E95B,0x094230DE,0xB4885C10,0x691E8595,
    0x4E6E50A0,0x93F88925,0x2E32E5EB,0xF3A43C6E,0x8ED73A36,0x5341E3B3,0xEE8B8F7D,0x331D56F8,
    0x13146EE2,0xCE82B767,0x7348DBA9,0xAEDE022C,0xD3AD0474,0x0E3BDDF1,0xB3F1B13F,0x6E6768BA,
    0x4917BD8F,0x9481640A,0x294B08C4,0xF4DDD141,0x89AED719,0x54380E9C,0xE9F26252,0x3464BBD7,
    0xA713C838,0x7A8511BD,0xC74F7D73,0x1AD9A4F6,0x67AAA2AE,0xBA3C7B2B,0x07F617E5,0xDA60CE60,
    0xFD101B55,0x2086C2D0,0x9D4CAE1E,0x40DA779B,0x3DA971C3,0xE03FA846,0x5DF5C488,0x80631D0D,
    0x1DE7B4BC,0xC0716D39,0x7DBB01F7,0xA02DD872,0xDD5EDE2A,0x00C807AF,0xBD026B61,0x6094B2E4,
    0x47E467D1,0x9A72BE54,0x27B8D29A,0xFA2E0B1F,0x875D0D47,0x5ACBD4C2,0xE701B80C,0x3A976189,
    0xA9E01266,0x7476CBE3,0xC9BCA72D,0x142A7EA8,0x695978F0,0xB4CFA175,0x0905CDBB,0xD493143E,
    0xF3E3C10B,0x2E75188E,0x93BF7440,0x4E29ADC5,0x335AAB9D,0xEECC7218,0x53061ED6,0x8E90C753,
    0xAE99FF49,0x730F26CC,0xCEC54A02,0x13539387,0x6E2095DF,0xB3B64C5A,0x0E7C2094,0xD3EAF911,
    0xF49A2C24,0x290CF5A1,0x94C6996F,0x495040EA,0x342346B2,0xE9B59F37,0x547FF3F9,0x89E92A7C,
    0x1A9E5993,0xC7088016,0x7AC2ECD8,0xA754355D,0xDA273305,0x07B1EA80,0xBA7B864E,0x67ED5FCB,
    0x409D8AFE,0x9D0B537B,0x20C13FB5,0xFD57E630,0x8024E068,0x5DB239ED,0xE0785523,0x3DEE8CA6,
  },

  {
    0x00000000,0x9D0FE176,0xE16EC4AD,0x7C6125DB,0x19AC8F1B,0x84A36E6D,0xF8C24BB6,0x65CDAAC0,
    0x33591E36,0xAE56FF40,0xD237DA9B,0x4F383BED,0x2AF5912D,0xB7FA705B,0xCB9B5580,0x5694B4F6,
    0x66B23C6C,0xFBBDDD1A,0x87DCF8C1,0x1AD319B7,0x7F1EB377,0xE2115201,0x9E7077DA,0x037F96AC,
    0x55EB225A,0xC8E4C32C,0xB485E6F7,0x298A0781,0x4C47AD41,0xD1484C37,0xAD2969EC,0x3026889A,
    0xCD6478D8,0x506B99AE,0x2C0ABC75,0xB1055D03,0xD4C8F7C3,0x49C716B5,0x35A6336E,0xA8A9D218,
    0xFE3D66EE,0x63328798,0x1F53A243,0x825C4335,0xE791E9F5,0x7A9E0883,0x06FF2D58,0x9BF0CC2E,
    0xABD644B4,0x36D9A5C2,0x4AB88019,0xD7B7616F,0xB27ACBAF,0x2F752AD9,0x53140F02,0xCE1BEE74,
    0x988F5A82,0x0580BBF4,0x79E19E2F,0xE4EE7F59,0x8123D599,0x1C2C34EF,0x604D1134,0xFD42F042,
    0x41B9F7F1,0xDCB61687,0xA0D7335C,0x3DD8D22A,0x581578EA,0xC51A999C,0xB97BBC47,0x24745D31,
    0x72E0E9C7,0xEFEF08B1,0x938E2D6A,0x0E81CC1C,0x6B4C66DC,0xF64387AA,0x8A22A271,0x172D4307,
    0x270BCB9D,0xBA042AEB,0xC6650F30,0x5B6AEE46,0x3EA74486,0xA3A8A5F0,0xDFC9802B,0x42C6615D,
    0x1452D5AB,0x895D34DD,0xF53C1106,0x6833F070,0x0DFE5AB0,0x90F1BBC6,0xEC909E1D,0x719F7F6B,
    0x8CDD8F29,0x11D26E5F,0x6DB34B84,0xF0BCAAF2,0x95710032,0x087EE144,0x741FC49F,0xE91025E9,
    0xBF84911F,0x228B7069,0x5EEA55B2,0xC3E5B4C4,0xA6281E04,0x3B27FF72,0x4746DAA9,0xDA493BDF,
    0xEA6FB345,0x77605233,0x0B0177E8,0x960E969E,0xF3C33C5E,0x6ECCDD28,0x12ADF8F3,0x8FA21985,
    0xD936AD73,0x44394C05,0x385869DE,0xA55788A8,0xC09A2268,0x5D95C31E,0x21F4E6C5,0xBCFB07B3,
    0x8373EFE2,0x1E7C0E94,0x621D2B4F,0xFF12CA39,0x9ADF60F9,0x07D0818F,0x7BB1A454,0xE6BE4522,
    0xB02AF1D4,0x2D2510A2,0x51443579,0xCC4BD40F,0xA9867ECF,0x34899FB9,0x48E8BA62,0xD5E75B14,
    0xE5C1D38E,0x78CE32F8,0x04AF1723,0x99A0F655,0xFC6D5C95,0x6162BDE3,0x1D039838,0x800C794E,
    0xD698CDB8,0x4B972CCE,0x37F60915,0xAAF9E863,0xCF3442A3,0x523BA3D5,0x2E5A860E,0xB3556778,
    0x4E17973A,0xD318764C,0xAF795397,0x3276B2E1,0x57BB1821,0xCAB4F957,0xB6D5DC8C,0x2BDA3DFA,
    0x7D4E890C,0xE041687A,0x9C204DA1,0x012FACD7,0x64E20617,0xF9EDE761,0x858CC2BA,0x188323CC,
    0x28A5AB56,0xB5AA4A20,0xC9CB6FFB,0x54C48E8D,0x3109244D,0xAC06C53B,0xD067E0E0,0x4D680196,
    0x1BFCB560,0x86F35416,0xFA9271CD,0x679D90BB,0x02503A7B,0x9F5FDB0D,0xE33EFED6,0x7E311FA0,
    0xC2CA1813,0x5FC5F965,0x23A4DCBE,0xBEAB3DC8,0xDB669708,0x4669767E,0x3A0853A5,0xA707B2D3,
    0xF1930625,0x6C9CE753,0x10FDC288,0x8DF223FE,0xE83F893E,0x75306848,0x09514D93,0x945EACE5,
    0xA478247F,0x3977C509,0x4516E0D2,0xD81901A4,0xBDD4AB64,0x20DB4A12,0x5CBA6FC9,0xC1B58EBF,
    0x97213A49,0x0A2EDB3F,0x764FFEE4,0xEB401F92,0x8E8DB552,0x13825424,0x6FE371FF,0xF2EC9089,
    0x0FAE60CB,0x92A181BD,0xEEC0A466,0x73CF4510,0x1602EFD0,0x8B0D0EA6,0xF76C2B7D,0x6A63CA0B,
    0x3CF77EFD,0xA1F89F8B,0xDD99BA50,0x40965B26,0x255BF1E6,0xB8541090,0xC435354B,0x593AD43D,
    0x691C5CA7,0xF413BDD1,0x8872980A,0x157D797C,0x70B0D3BC,0xEDBF32CA,0x91DE1711,0x0CD1F667,
    0x5A454291,0xC74AA3E7,0xBB2B863C,0x2624674A,0x43E9CD8A,0xDEE62CFC,0xA2870927,0x3F88E851,
  },

  {
    0x00000000,0xB9FBDBE8,0xA886B191,0x117D6A79,0x8A7C6563,0x3387BE8B,0x22FAD4F2,0x9B010F1A,
    0xCF89CC87,0x7672176F,0x670F7D16,0xDEF4A6FE,0x45F5A9E4,0xFC0E720C,0xED731875,0x5488C39D,
    0x44629F4F,0xFD9944A7,0xECE42EDE,0x551FF536,0xCE1EFA2C,0x77E521C4,0x66984BBD,0xDF639055,
    0x8BEB53C8,0x32108820,0x236DE259,0x9A9639B1,0x019736AB,0xB86CED43,0xA911873A,0x10EA5CD2,
    0x88C53E9E,0x313EE576,0x20438F0F,0x99B854E7,0x02B95BFD,0xBB428015,0xAA3FEA6C,0x13C43184,
    0x474CF219,0xFEB729F1,0xEFCA4388,0x56319860,0xCD30977A,0x74CB4C92,0x65B626EB,0xDC4DFD03,
    0xCCA7A1D1,0x755C7A39,0x64211040,0xDDDACBA8,0x46DBC4B2,0xFF201F5A,0xEE5D7523,0x57A6AECB,
    0x032E6D56,0xBAD5B6BE,0xABA8DCC7,0x1253072F,0x89520835,0x30A9D3DD,0x21D4B9A4,0x982F624C,
    0xCAFB7B7D,0x7300A095,0x627DCAEC,0xDB861104,0x40871E1E,0xF97CC5F6,0xE801AF8F,0x51FA7467,
    0x0572B7FA,0xBC896C12,0xADF4066B,0x140FDD83,0x8F0ED299,0x36F50971,0x27886308,0x9E73B8E0,
    0x8E99E432,0x37623FDA,0x261F55A3,0x9FE48E4B,0x04E58151,0xBD1E5AB9,0xAC6330C0,0x1598EB28,
    0x411028B5,0xF8EBF35D,0xE9969924,0x506D42CC,0xCB6C4DD6,0x7297963E,0x63EAFC47,0xDA1127AF,
    0x423E45E3,0xFBC59E0B,0xEAB8F472,0x53432F9A,0xC8422080,0x71B9FB68,0x60C49111,0xD93F4AF9,
    0x8DB78964,0x344C528C,0x253138F5,0x9CCAE31D,0x07CBEC07,0xBE3037EF,0xAF4D5D96,0x16B6867E,
    0x065CDAAC,0xBFA70144,0xAEDA6B3D,0x1721B0D5,0x8C20BFCF,0x35DB6427,0x24A60E5E,0x9D5DD5B6,
    0xC9D5162B,0x702ECDC3,0x6153A7BA,0xD8A87C52,0x43A97348,0xFA52A8A0,0xEB2FC2D9,0x52D41931,
    0x4E87F0BB,0xF77C2B53,0xE601412A,0x5FFA9AC2,0xC4FB95D8,0x7D004E30,0x6C7D2449,0xD586FFA1,
    0x810E3C3C,0x38F5E7D4,0x29888DAD,0x90735645,0x0B72595F,0xB28982B7,0xA3F4E8CE,0x1A0F3326,
    0x0AE56FF4,0xB31EB41C,0xA263DE65,0x1B98058D,0x80990A97,0x3962D17F,0x281FBB06,0x91E460EE,
    0xC56CA373,0x7C97789B,0x6DEA12E2,0xD411C90A,0x4F10C610,0xF6EB1DF8,0xE7967781,0x5E6DAC69,
    0xC642CE25,0x7FB915CD,0x6EC47FB4,0xD73FA45C,0x4C3EAB46,0xF5C570AE,0xE4B81AD7,0x5D43C13F,
    0x09CB02A2,0xB030D94A,0xA14DB333,0x18B668DB,0x83B767C1,0x3A4CBC29,0x2B31D650,0x92CA0DB8,
    0x8220516A,0x3BDB8A82,0x2AA6E0FB,0x935D3B13,0x085C3409,0xB1A7EFE1,0xA0DA8598,0x19215E70,
    0x4DA99DED,0xF4524605,0xE52F2C7C,0x5CD4F794,0xC7D5F88E,0x7E2E2366,0x6F53491F,0xD6A892F7,
    0x847C8BC6,0x3D87502E,0x2CFA3A57,0x9501E1BF,0x0E00EEA5,0xB7FB354D,0xA6865F34,0x1F7D84DC,
    0x4BF54741,0xF20E9CA9,0xE373F6D0,0x5A882D38,0xC1892222,0x7872F9CA,0x690F93B3,0xD0F4485B,
    0xC01E1489,0x79E5CF61,0x6898A518,0xD1637EF0,0x4A6271EA,0xF399AA02,0xE2E4C07B,0x5B1F1B93,
    0x0F97D80E,0xB66C03E6,0xA711699F,0x1EEAB277,0x85EBBD6D,0x3C106685,0x2D6D0CFC,0x9496D714,
    0x0CB9B558,0xB5426EB0,0xA43F04C9,0x1DC4DF21,0x86C5D03B,0x3F3E0BD3,0x2E4361AA,0x97B8BA42,
    0xC33079DF,0x7ACBA237,0x6BB6C84E,0xD24D13A6,0x494C1CBC,0xF0B7C754,0xE1CAAD2D,0x583176C5,
    0x48DB2A17,0xF120F1FF,0xE05D9B86,0x59A6406E,0xC2A74F74,0x7B5C949C,0x6A21FEE5,0xD3DA250D,
    0x8752E690,0x3EA93D78,0x2FD45701,0x962F8CE9,0x0D2E83F3,0xB4D5581B,0xA5A83262,0x1C53E98A,
  },

  {
    0x00000000,0xAE689191,0x87A02563,0x29C8B4F2,0xD4314C87,0x7A59DD16,0x539169E4,0xFDF9F875,
    0x73139F4F,0xDD7B0EDE,0xF4B3BA2C,0x5ADB2BBD,0xA722D3C8,0x094A4259,0x2082F6AB,0x8EEA673A,
    0xE6273E9E,0x484FAF0F,0x61871BFD,0xCFEF8A6C,0x32167219,0x9C7EE388,0xB5B6577A,0x1BDEC6EB,
    0x9534A1D1,0x3B5C3040,0x129484B2,0xBCFC1523,0x4105ED56,0xEF6D7CC7,0xC6A5C835,0x68CD59A4,
    0x173F7B7D,0xB957EAEC,0x909F5E1E,0x3EF7CF8F,0xC30E37FA,0x6D66A66B,0x44AE1299,0xEAC68308,
    0x642CE432,0xCA4475A3,0xE38CC151,0x4DE450C0,0xB01DA8B5,0x1E753924,0x37BD8DD6,0x99D51C47,
    0xF11845E3,0x5F70D472,0x76B86080,0xD8D0F111,0x25290964,0x8B4198F5,0xA2892C07,0x0CE1BD96,
    0x820BDAAC,0x2C634B3D,0x05ABFFCF,0xABC36E5E,0x563A962B,0xF85207BA,0xD19AB348,0x7FF222D9,
    0x2E7EF6FA,0x8016676B,0xA9DED399,0x07B64208,0xFA4FBA7D,0x54272BEC,0x7DEF9F1E,0xD3870E8F,
    0x5D6D69B5,0xF305F824,0xDACD4CD6,0x74A5DD47,0x895C2532,0x2734B4A3,0x0EFC0051,0xA09491C0,
    0xC859C864,0x663159F5,0x4FF9ED07,0xE1917C96,0x1C6884E3,0xB2001572,0x9BC8A180,0x35A03011,
    0xBB4A572B,0x1522C6BA,0x3CEA7248,0x9282E3D9,0x6F7B1BAC,0xC1138A3D,0xE8DB3ECF,0x46B3AF5E,
    0x39418D87,0x97291C16,0xBEE1A8E4,0x10893975,0xED70C100,0x43185091,0x6AD0E463,0xC4B875F2,
    0x4A5212C8,0xE43A8359,0xCDF237AB,0x639AA63A,0x9E635E4F,0x300BCFDE,0x19C37B2C,0xB7ABEABD,
    0xDF66B319,0x710E2288,0x58C6967A,0xF6AE07EB,0x0B57FF9E,0xA53F6E0F,0x8CF7DAFD,0x229F4B6C,
    0xAC752C56,0x021DBDC7,0x2BD50935,0x85BD98A4,0x784460D1,0xD62CF140,0xFFE445B2,0x518CD423,
    0x5CFDEDF4,0xF2957C65,0xDB5DC897,0x75355906,0x88CCA173,0x26A430E2,0x0F6C8410,0xA1041581,
    0x2FEE72BB,0x8186E32A,0xA84E57D8,0x0626C649,0xFBDF3E3C,0x55B7AFAD,0x7C7F1B5F,0xD2178ACE,
    0xBADAD36A,0x14B242FB,0x3D7AF609,0x93126798,0x6EEB9FED,0xC0830E7C,0xE94BBA8E,0x47232B1F,
    0xC9C94C25,0x67A1DDB4,0x4E696946,0xE001F8D7,0x1DF800A2,0xB3909133,0x9A5825C1,0x3430B450,
    0x4BC29689,0xE5AA0718,0xCC62B3EA,0x620A227B,0x9FF3DA0E,0x319B4B9F,0x1853FF6D,0xB63B6EFC,
    0x38D109C6,0x96B99857,0xBF712CA5,0x1119BD34,0xECE04541,0x4288D4D0,0x6B406022,0xC528F1B3,
    0xADE5A817,0x038D3986,0x2A458D74,0x842D1CE5,0x79D4E490,0xD7BC7501,0xFE74C1F3,0x501C5062,
    0xDEF63758,0x709EA6C9,0x5956123B,0xF73E83AA,0x0AC77BDF,0xA4AFEA4E,0x8D675EBC,0x230FCF2D,
    0x72831B0E,0xDCEB8A9F,0xF5233E6D,0x5B4BAFFC,0xA6B25789,0x08DAC618,0x211272EA,0x8F7AE37B,
    0x01908441,0xAFF815D0,0x8630A122,0x285830B3,0xD5A1C8C6,0x7BC95957,0x5201EDA5,0xFC697C34,
    0x94A42590,0x3ACCB401,0x130400F3,0xBD6C9162,0x40956917,0xEEFDF886,0xC7354C74,0x695DDDE5,
    0xE7B7BADF,0x49DF2B4E,0x60179FBC,0xCE7F0E2D,0x3386F658,0x9DEE67C9,0xB426D33B,0x1A4E42AA,
    0x65BC6073,0xCBD4F1E2,0xE21C4510,0x4C74D481,0xB18D2CF4,0x1FE5BD65,0x362D0997,0x98459806,
    0x16AFFF3C,0xB8C76EAD,0x910FDA5F,0x3F674BCE,0xC29EB3BB,0x6CF6222A,0x453E96D8,0xEB560749,
    0x839B5EED,0x2DF3CF7C,0x043B7B8E,0xAA53EA1F,0x57AA126A,0xF9C283FB,0xD00A3709,0x7E62A698,
    0xF088C1A2,0x5EE05033,0x7728E4C1,0xD9407550,0x24B98D25,0x8AD11CB4,0xA319A846,0x0D7139D7,
  }
#endif // CRC32_USE_LOOKUP_TABLE_SLICING_BY_16
};
#endif // NO_LUT


/////////////// CRC-32C with hardware acceleration


/* crc32c.c -- compute CRC-32C using the Intel crc32 instruction
 * Copyright (C) 2013 Mark Adler
 * Version 1.1  1 Aug 2013  Mark Adler
 */
/* Table for a quadword-at-a-time software crc. */
static pthread_once_t crc32c_once_sw = PTHREAD_ONCE_INIT;
static uint32_t crc32c_table[8][256];
#define POLY 0x82f63b78

/* Construct table for software CRC-32C calculation. */
static void crc32c_init_sw(void)
{
    uint32_t n, crc, k;

    for (n = 0; n < 256; n++) {
        crc = n;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc32c_table[0][n] = crc;
    }
    for (n = 0; n < 256; n++) {
        crc = crc32c_table[0][n];
        for (k = 1; k < 8; k++) {
            crc = crc32c_table[0][crc & 0xff] ^ (crc >> 8);
            crc32c_table[k][n] = crc;
        }
    }
}

/* Table-driven software version as a fall-back.  This is about 15 times slower
   than using the hardware instructions.  This assumes little-endian integers,
   as is the case on Intel processors that the assembler code here is for. */
static uint32_t crc32c_sw(uint32_t crci, const unsigned char *buf, size_t len)
{
    const unsigned char *next = buf;
    uint64_t crc;

    pthread_once(&crc32c_once_sw, crc32c_init_sw);
    crc = crci ^ 0xffffffff;
    while (len && ((uintptr_t)next & 7) != 0) {
        crc = crc32c_table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
        len--;
    }
    while (len >= 8) {
        crc ^= *(uint64_t *)next;
        crc = crc32c_table[7][crc & 0xff] ^
              crc32c_table[6][(crc >> 8) & 0xff] ^
              crc32c_table[5][(crc >> 16) & 0xff] ^
              crc32c_table[4][(crc >> 24) & 0xff] ^
              crc32c_table[3][(crc >> 32) & 0xff] ^
              crc32c_table[2][(crc >> 40) & 0xff] ^
              crc32c_table[1][(crc >> 48) & 0xff] ^
              crc32c_table[0][crc >> 56];
        next += 8;
        len -= 8;
    }
    while (len) {
        crc = crc32c_table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
        len--;
    }
    return (uint32_t)crc ^ 0xffffffff;
}

/* Multiply a matrix times a vector over the Galois field of two elements,
   GF(2).  Each element is a bit in an unsigned integer.  mat must have at
   least as many entries as the power of two for most significant one bit in
   vec. */
static inline uint32_t gf2_matrix_times(uint32_t *mat, uint32_t vec)
{
    uint32_t sum;

    sum = 0;
    while (vec) {
        if (vec & 1)
            sum ^= *mat;
        vec >>= 1;
        mat++;
    }
    return sum;
}

/* Multiply a matrix by itself over GF(2).  Both mat and square must have 32
   rows. */
static inline void gf2_matrix_square(uint32_t *square, uint32_t *mat)
{
    int n;

    for (n = 0; n < 32; n++)
        square[n] = gf2_matrix_times(mat, mat[n]);
}

/* Construct an operator to apply len zeros to a crc.  len must be a power of
   two.  If len is not a power of two, then the result is the same as for the
   largest power of two less than len.  The result for len == 0 is the same as
   for len == 1.  A version of this routine could be easily written for any
   len, but that is not needed for this application. */
static void crc32c_zeros_op(uint32_t *even, size_t len)
{
    int n;
    uint32_t row;
    uint32_t odd[32];       /* odd-power-of-two zeros operator */

    /* put operator for one zero bit in odd */
    odd[0] = POLY;              /* CRC-32C polynomial */
    row = 1;
    for (n = 1; n < 32; n++) {
        odd[n] = row;
        row <<= 1;
    }

    /* put operator for two zero bits in even */
    gf2_matrix_square(even, odd);

    /* put operator for four zero bits in odd */
    gf2_matrix_square(odd, even);

    /* first square will put the operator for one zero byte (eight zero bits),
       in even -- next square puts operator for two zero bytes in odd, and so
       on, until len has been rotated down to zero */
    do {
        gf2_matrix_square(even, odd);
        len >>= 1;
        if (len == 0)
            return;
        gf2_matrix_square(odd, even);
        len >>= 1;
    } while (len);

    /* answer ended up in odd -- copy to even */
    for (n = 0; n < 32; n++)
        even[n] = odd[n];
}

/* Take a length and build four lookup tables for applying the zeros operator
   for that length, byte-by-byte on the operand. */
static void crc32c_zeros(uint32_t zeros[][256], size_t len)
{
    uint32_t n;
    uint32_t op[32];

    crc32c_zeros_op(op, len);
    for (n = 0; n < 256; n++) {
        zeros[0][n] = gf2_matrix_times(op, n);
        zeros[1][n] = gf2_matrix_times(op, n << 8);
        zeros[2][n] = gf2_matrix_times(op, n << 16);
        zeros[3][n] = gf2_matrix_times(op, n << 24);
    }
}

/* Apply the zeros operator table to crc. */
static inline uint32_t crc32c_shift(uint32_t zeros[][256], uint32_t crc)
{
    return zeros[0][crc & 0xff] ^ zeros[1][(crc >> 8) & 0xff] ^
           zeros[2][(crc >> 16) & 0xff] ^ zeros[3][crc >> 24];
}

/* Block sizes for three-way parallel crc computation.  LONG and SHORT must
   both be powers of two.  The associated string constants must be set
   accordingly, for use in constructing the assembler instructions. */
#define LONG 8192
#define LONGx1 "8192"
#define LONGx2 "16384"
#define SHORT 256
#define SHORTx1 "256"
#define SHORTx2 "512"

/* Tables for hardware crc that shift a crc by LONG and SHORT zeros. */
static pthread_once_t crc32c_once_hw = PTHREAD_ONCE_INIT;
static uint32_t crc32c_long[4][256];
static uint32_t crc32c_short[4][256];

/* Initialize tables for shifting crcs. */
static void crc32c_init_hw(void)
{
    crc32c_zeros(crc32c_long, LONG);
    crc32c_zeros(crc32c_short, SHORT);
}

#if defined(_WIN64)

/* Compute CRC-32C using the Intel hardware instruction. */
static uint32_t crc32c_hw(uint32_t crc, const unsigned char *buf, size_t len)
{
    const unsigned char *next = buf;
    const unsigned char *end;
    uint64_t crc0, crc1, crc2;      /* need to be 64 bits for crc32q */

    /* populate shift tables the first time through */
    pthread_once(&crc32c_once_hw, crc32c_init_hw);

    /* pre-process the crc */
    crc0 = crc ^ 0xffffffff;

    /* compute the crc for up to seven leading bytes to bring the data pointer
       to an eight-byte boundary */
    while (len && ((uintptr_t)next & 7) != 0) {
        __asm__("crc32b\t" "(%1), %0"
                : "=r"(crc0)
                : "r"(next), "0"(crc0));
        next++;
        len--;
    }

    /* compute the crc on sets of LONG*3 bytes, executing three independent crc
       instructions, each on LONG bytes -- this is optimized for the Nehalem,
       Westmere, Sandy Bridge, and Ivy Bridge architectures, which have a
       throughput of one crc per cycle, but a latency of three cycles */
    while (len >= LONG*3) {
        crc1 = 0;
        crc2 = 0;
        end = next + LONG;
        do {
            __asm__("crc32q\t" "(%3), %0\n\t"
                    "crc32q\t" LONGx1 "(%3), %1\n\t"
                    "crc32q\t" LONGx2 "(%3), %2"
                    : "=r"(crc0), "=r"(crc1), "=r"(crc2)
                    : "r"(next), "0"(crc0), "1"(crc1), "2"(crc2));
            next += 8;
        } while (next < end);
        crc0 = crc32c_shift(crc32c_long, crc0) ^ crc1;
        crc0 = crc32c_shift(crc32c_long, crc0) ^ crc2;
        next += LONG*2;
        len -= LONG*3;
    }

    /* do the same thing, but now on SHORT*3 blocks for the remaining data less
       than a LONG*3 block */
    while (len >= SHORT*3) {
        crc1 = 0;
        crc2 = 0;
        end = next + SHORT;
        do {
            __asm__("crc32q\t" "(%3), %0\n\t"
                    "crc32q\t" SHORTx1 "(%3), %1\n\t"
                    "crc32q\t" SHORTx2 "(%3), %2"
                    : "=r"(crc0), "=r"(crc1), "=r"(crc2)
                    : "r"(next), "0"(crc0), "1"(crc1), "2"(crc2));
            next += 8;
        } while (next < end);
        crc0 = crc32c_shift(crc32c_short, crc0) ^ crc1;
        crc0 = crc32c_shift(crc32c_short, crc0) ^ crc2;
        next += SHORT*2;
        len -= SHORT*3;
    }

    /* compute the crc on the remaining eight-byte units less than a SHORT*3
       block */
    end = next + (len - (len & 7));
    while (next < end) {
        __asm__("crc32q\t" "(%1), %0"
                : "=r"(crc0)
                : "r"(next), "0"(crc0));
        next += 8;
    }
    len &= 7;

    /* compute the crc for up to seven trailing bytes */
    while (len) {
        __asm__("crc32b\t" "(%1), %0"
                : "=r"(crc0)
                : "r"(next), "0"(crc0));
        next++;
        len--;
    }

    /* return a post-processed crc */
    return (uint32_t)crc0 ^ 0xffffffff;
}

/* Check for SSE 4.2.  SSE 4.2 was first supported in Nehalem processors
   introduced in November, 2008.  This does not check for the existence of the
   cpuid instruction itself, which was introduced on the 486SL in 1992, so this
   will fail on earlier x86 processors.  cpuid works on all Pentium and later
   processors. */
#define SSE42(have) \
    do { \
        uint32_t eax, ecx; \
        eax = 1; \
        __asm__("cpuid" \
                : "=c"(ecx) \
                : "a"(eax) \
                : "%ebx", "%edx"); \
        (have) = (ecx >> 20) & 1; \
    } while (0)

/* Compute a CRC-32C.  If the crc32 instruction is available, use the hardware
   version.  Otherwise, use the software version. */
#endif

uint32_t crc32c(uint32_t crc, const unsigned char *buf, size_t len)
{
#if  defined(_WIN64)
    int sse42;
    SSE42(sse42);
	//printf("SSE42 %d\n",sse42);
    return sse42 ? crc32c_hw(crc, buf, len) : crc32c_sw(crc, buf, len);
#else
    return crc32c_sw(crc, buf, len);
#endif

}


void myreplaceall(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}


bool fileexists(const string& i_filename) 
{
#ifdef unix
// true even for dirs no S_ISDIR
  struct stat buffer;   
  return (stat(i_filename.c_str(),&buffer)==0); 
#endif

#ifdef _WIN32
	HANDLE	myhandle;
	WIN32_FIND_DATA findfiledata;
	std::wstring wpattern=utow(i_filename.c_str());
	myhandle=FindFirstFile(wpattern.c_str(),&findfiledata);
	if (myhandle!=INVALID_HANDLE_VALUE)
	{
		FindClose(myhandle);
		return true;
	}
	return false;
#endif

	return false;
}


void xcommand(string i_command,const string& i_parameter)
{
	if (i_command=="")
		return;
	if (!fileexists(i_command))
		return;
	if (i_parameter=="")
		system(i_command.c_str());
	else
	{
		string mycommand=i_command+" \""+i_parameter+"\"";
		system(mycommand.c_str());
	}
	
}


///////////// support functions
string mytrim(const string& i_str)
{
	size_t first = i_str.find_first_not_of(' ');
	if (string::npos == first)
		return i_str;
	size_t last = i_str.find_last_not_of(' ');
	return i_str.substr(first, (last - first + 1));
}

#if defined(_WIN32) || defined(_WIN64)



/// something to get VSS done via batch file
string g_gettempdirectory()
{
	
	string temppath="";
	wchar_t charpath[MAX_PATH];
	if (GetTempPathW(MAX_PATH, charpath))
	{
		wstring ws(charpath);
		string str(ws.begin(), ws.end());	
		return str;
	}
	return temppath;
}

void waitexecute(string i_filename,string i_parameters,int i_show)
{
	SHELLEXECUTEINFOA ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = i_filename.c_str();
	ShExecInfo.lpParameters = i_parameters.c_str();   
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = i_show;
	ShExecInfo.hInstApp = NULL; 
	ShellExecuteExA(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
	CloseHandle(ShExecInfo.hProcess);
}

bool isadmin()
{
	BOOL fIsElevated = FALSE;
	HANDLE hToken = NULL;
	TOKEN_ELEVATION elevation;
	DWORD dwSize;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		printf("\n Failed to get Process Token\n");
		goto Cleanup;  // if Failed, we treat as False
	}


	if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
	{	
		printf("\nFailed to get Token Information\n");
		goto Cleanup;// if Failed, we treat as False
	}

	fIsElevated = elevation.TokenIsElevated;

Cleanup:
	if (hToken)
	{
		CloseHandle(hToken);
		hToken = NULL;
	}
	return fIsElevated; 
}
#endif

int mypos(const string& i_substring,const string& i_string) 
{
    size_t start_pos = i_string.find(i_substring);
    if	(start_pos==std::string::npos)
        return -1;
	else
		return start_pos;
}


bool myreplace(string& i_str, const string& i_from, const string& i_to) 
{
    size_t start_pos = i_str.find(i_from);
    if(start_pos == std::string::npos)
        return false;
    i_str.replace(start_pos, i_from.length(), i_to);
    return true;
}

// Return true if a file or directory (UTF-8 without trailing /) exists.
bool exists(string filename) {
  int len=filename.size();
  if (len<1) return false;
  if (filename[len-1]=='/') filename=filename.substr(0, len-1);
#ifdef unix
  struct stat sb;
  return !lstat(filename.c_str(), &sb);
#else
  return GetFileAttributes(utow(filename.c_str()).c_str())
         !=INVALID_FILE_ATTRIBUTES;
#endif
}

// Delete a file, return true if successful
bool delete_file(const char* filename) {
#ifdef unix
	return remove(filename)==0;
#else

	if (!fileexists(filename))
		return true;
	SetFileAttributes(utow(filename).c_str(),FILE_ATTRIBUTE_NORMAL);
	return DeleteFile(utow(filename).c_str());
#endif
}

bool delete_dir(const char* i_directory) {
#ifdef unix
  return remove(i_directory)==0;
#else
  return RemoveDirectory(utow(i_directory).c_str());
#endif
}

long long fsbtoblk(int64_t num, uint64_t fsbs, u_long bs)
{
	return (num * (intmax_t) fsbs / (int64_t) bs);
}

/// it is not easy, at all, to take *nix free filesystem space
uint64_t getfreespace(const char* i_path)
{
	
#ifdef BSD
	struct statfs stat;
 
	if (statfs(i_path, &stat) != 0) 
	{
		// error happens, just quits here
		return 0;
	}
///	long long used = stat.f_blocks - stat.f_bfree;
///	long long availblks = stat.f_bavail + used;

	static long blocksize = 0;
	int dummy;

	if (blocksize == 0)
		getbsize(&dummy, &blocksize);
	return  fsbtoblk(stat.f_bavail,
	stat.f_bsize, blocksize)*1024;
#else
#ifdef unix //this sould be Linux
	return 0;
#else
	uint64_t spazio=0;
	
	BOOL  fResult;
	unsigned __int64 i64FreeBytesToCaller,i64TotalBytes,i64FreeBytes;
	WCHAR  *pszDrive  = NULL, szDrive[4];

	const size_t WCHARBUF = 512;
	wchar_t  wszDest[WCHARBUF];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, i_path, -1, wszDest, WCHARBUF);

	pszDrive = wszDest;
	if (i_path[1] == ':')
	{
		szDrive[0] = pszDrive[0];
		szDrive[1] = ':';
        szDrive[2] = '\\';
        szDrive[3] = '\0';
		pszDrive = szDrive;
	}
	fResult = GetDiskFreeSpaceEx ((LPCTSTR)pszDrive,
                                 (PULARGE_INTEGER)&i64FreeBytesToCaller,
                                 (PULARGE_INTEGER)&i64TotalBytes,
                                 (PULARGE_INTEGER)&i64FreeBytes);
	if (fResult)
		spazio=i64FreeBytes;
	return spazio; // Windows
#endif

#endif


}

bool direxists(string& i_directory) 
{
#ifdef unix
	struct stat sb;
    return ((stat(i_directory.c_str(), &sb) == 0) && S_ISDIR(sb.st_mode)); 	
#endif

#ifdef _WIN32
	HANDLE	myhandle;
	WIN32_FIND_DATA findfiledata;
	if (!isdirectory(i_directory))
		i_directory+="/";
	std::string pattern=i_directory+"*.*";
	std::wstring wpattern=utow(pattern.c_str());
	myhandle=FindFirstFile(wpattern.c_str(),&findfiledata);
	if (myhandle!=INVALID_HANDLE_VALUE)
	{
		FindClose(myhandle);
		return true;
	}
	return false;
#endif

	return false;
}


#ifdef unix

// Print last error message
void printerr(const char* i_where,const char* filename) 
{
  perror(filename);
}

#else

// Print last error message
void printerr(const char* i_where,const char* filename) 
{
  fflush(stdout);
  int err=GetLastError();
	fprintf(stderr,"\n");
  printUTF8(filename, stderr);
  
  
  if (err==ERROR_FILE_NOT_FOUND)
    g_exec_text=": error file not found";
  else if (err==ERROR_PATH_NOT_FOUND)
   g_exec_text=": error path not found";
  else if (err==ERROR_ACCESS_DENIED)
    g_exec_text=": error access denied";
  else if (err==ERROR_SHARING_VIOLATION)
    g_exec_text=": error sharing violation";
  else if (err==ERROR_BAD_PATHNAME)
    g_exec_text=": error bad pathname";
  else if (err==ERROR_INVALID_NAME)
    g_exec_text=": error invalid name";
  else if (err==ERROR_NETNAME_DELETED)
      g_exec_text=": error network name no longer available";
  else if (err==ERROR_ALREADY_EXISTS)
      g_exec_text=": cannot write file already exists";
  else
  {
	char buffer[100];
	sprintf(buffer,": Windows error # %d\n",err);
	
  g_exec_text=buffer;
  }
	fprintf(stderr, g_exec_text.c_str());
	fprintf(stderr,"\n");
	g_exec_text=filename+g_exec_text;
	
	uint64_t spazio=getfreespace(filename);
	if (spazio<16384)
	{
		string mypath=filename;
		if (direxists(mypath))
			printf("\n\nMAYBE OUT OF FREE SPACE?\n");
		else
			printf("\n\nMAYBE OUT OF FREE SPACE OR INVALID PATH?\n");
	}
//	exit(0);
}

#endif

// Close fp if open. Set date and attributes unless 0
void close(const char* filename, int64_t date, int64_t attr, FP fp=FPNULL) {
  assert(filename);
#ifdef unix
  if (fp!=FPNULL) fclose(fp);
  if (date>0) {
    struct utimbuf ub;
    ub.actime=time(NULL);
    ub.modtime=unix_time(date);
    utime(filename, &ub);
  }
  if ((attr&255)=='u')
    chmod(filename, attr>>8);
#else
  const bool ads=strstr(filename, ":$DATA")!=0;  // alternate data stream?
  if (date>0 && !ads) {
    if (fp==FPNULL)
      fp=CreateFile(utow(filename).c_str(),
                    FILE_WRITE_ATTRIBUTES,
                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                    NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (fp!=FPNULL) {
      SYSTEMTIME st;
      st.wYear=date/10000000000LL%10000;
      st.wMonth=date/100000000%100;
      st.wDayOfWeek=0;  // ignored
      st.wDay=date/1000000%100;
      st.wHour=date/10000%100;
      st.wMinute=date/100%100;
      st.wSecond=date%100;
      st.wMilliseconds=0;
      FILETIME ft;
      SystemTimeToFileTime(&st, &ft);
      SetFileTime(fp, NULL, NULL, &ft);
    }
  }
  if (fp!=FPNULL) CloseHandle(fp);
  if ((attr&255)=='w' && !ads)
    SetFileAttributes(utow(filename).c_str(), attr>>8);
#endif
}

// Print file open error and throw exception
void ioerr(const char* msg) {
  printerr("11896",msg);
  throw std::runtime_error(msg);
}

// Create directories as needed. For example if path="/tmp/foo/bar"
// then create directories /, /tmp, and /tmp/foo unless they exist.
// Set date and attributes if not 0.
void makepath(string path, int64_t date=0, int64_t attr=0) {
  for (unsigned i=0; i<path.size(); ++i) {
    if (path[i]=='\\' || path[i]=='/') {
      path[i]=0;
#ifdef unix
      mkdir(path.c_str(), 0777);
#else
      CreateDirectory(utow(path.c_str()).c_str(), 0);
#endif
      path[i]='/';
    }
  }

  // Set date and attributes
  string filename=path;
  if (filename!="" && filename[filename.size()-1]=='/')
    filename=filename.substr(0, filename.size()-1);  // remove trailing slash
  close(filename.c_str(), date, attr);
}

#ifndef unix

// Truncate filename to length. Return -1 if error, else 0.
int truncate(const char* filename, int64_t length) {
  std::wstring w=utow(filename);
  HANDLE out=CreateFile(w.c_str(), GENERIC_READ | GENERIC_WRITE,
                        0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (out!=INVALID_HANDLE_VALUE) {
    //// LONG
	long hi=length>>32;
    if (SetFilePointer(out, length, &hi, FILE_BEGIN)
             !=INVALID_SET_FILE_POINTER
        && SetEndOfFile(out)
        && CloseHandle(out))
      return 0;
  }
  return -1;
}
#endif

/////////////////////////////// Archive ///////////////////////////////

// Convert non-negative decimal number x to string of at least n digits
string itos(int64_t x, int n=1) {
  assert(x>=0);
  assert(n>=0);
  string r;
  for (; x || n>0; x/=10, --n) r=string(1, '0'+x%10)+r;
  return r;
}

// Replace * and ? in fn with part or digits of part
string subpart(string fn, int part) {
  for (int j=fn.size()-1; j>=0; --j) {
    if (fn[j]=='?')
      fn[j]='0'+part%10, part/=10;
    else if (fn[j]=='*')
      fn=fn.substr(0, j)+itos(part)+fn.substr(j+1), part=0;
  }
  return fn;
}

// Base of InputArchive and OutputArchive
class ArchiveBase {
protected:
  libzpaq::AES_CTR* aes;  // NULL if not encrypted
  FP fp;          // currently open file or FPNULL
public:
  ArchiveBase(): aes(0), fp(FPNULL) {}
  ~ArchiveBase() {
    if (aes) delete aes;
    if (fp!=FPNULL) fclose(fp);
  }  
  bool isopen() {return fp!=FPNULL;}
};

// An InputArchive supports encrypted reading
class InputArchive: public ArchiveBase, public libzpaq::Reader {
  vector<int64_t> sz;  // part sizes
  int64_t off;  // current offset
  string fn;  // filename, possibly multi-part with wildcards
public:

  // Open filename. If password then decrypt input.
  InputArchive(const char* filename, const char* password=0);

  // Read and return 1 byte or -1 (EOF)
  int get() {
    error("get() not implemented");
    return -1;
  }

  // Read up to len bytes into obuf at current offset. Return 0..len bytes
  // actually read. 0 indicates EOF.
  int read(char* obuf, int len) {
    int nr=fread(obuf, 1, len, fp);
    if (nr==0) {
      seek(0, SEEK_CUR);
      nr=fread(obuf, 1, len, fp);
    }
    if (nr==0) return 0;
    if (aes) aes->encrypt(obuf, nr, off);
    off+=nr;
    return nr;
  }

  // Like fseeko()
  void seek(int64_t p, int whence);

  // Like ftello()
  int64_t tell() {
    return off;
  }
};

// Like fseeko. If p is out of range then close file.
void InputArchive::seek(int64_t p, int whence) {
  if (!isopen()) return;

  // Compute new offset
  if (whence==SEEK_SET) off=p;
  else if (whence==SEEK_CUR) off+=p;
  else if (whence==SEEK_END) {
    off=p;
    for (unsigned i=0; i<sz.size(); ++i) off+=sz[i];
  }

  // Optimization for single file to avoid close and reopen
  if (sz.size()==1) {
    fseeko(fp, off, SEEK_SET);
    return;
  }

  // Seek across multiple files
  assert(sz.size()>1);
  int64_t sum=0;
  unsigned i;
  for (i=0;; ++i) {
    sum+=sz[i];
    if (sum>off || i+1>=sz.size()) break;
  }
  const string next=subpart(fn, i+1);
  fclose(fp);
  fp=fopen(next.c_str(), RB);
  if (fp==FPNULL) ioerr(next.c_str());
  fseeko(fp, off-sum, SEEK_END);
}

// Open for input. Decrypt with password and using the salt in the
// first 32 bytes. If filename has wildcards then assume multi-part
// and read their concatenation.

InputArchive::InputArchive(const char* filename, const char* password):
    /*off(0),*/ fn(filename) {
  assert(filename);
  off=0;
 

  // Get file sizes
  const string part0=subpart(filename, 0);
  for (unsigned i=1; ; ++i) {
    const string parti=subpart(filename, i);
    if (i>1 && parti==part0) break;
    fp=fopen(parti.c_str(), RB);
    if (fp==FPNULL) break;
    fseeko(fp, 0, SEEK_END);
    sz.push_back(ftello(fp));
    fclose(fp);
  }

  // Open first part
  const string part1=subpart(filename, 1);
  fp=fopen(part1.c_str(), RB);
  if (!isopen()) ioerr(part1.c_str());
  assert(fp!=FPNULL);

  // Get encryption salt
  if (password) {
    char salt[32], key[32];
    if (fread(salt, 1, 32, fp)!=32) error("cannot read salt");
    libzpaq::stretchKey(key, password, salt);
    aes=new libzpaq::AES_CTR(key, 32, salt);
    off=32;
  }
}

// An Archive is a file supporting encryption
class OutputArchive: public ArchiveBase, public libzpaq::Writer {
  int64_t off;    // preceding multi-part bytes
  unsigned ptr;   // write pointer in buf: 0 <= ptr <= BUFSIZE
  enum {BUFSIZE=1<<16};
  char buf[BUFSIZE];  // I/O buffer
public:

  // Open. If password then encrypt output.
  OutputArchive(const char* filename, const char* password=0,
                const char* salt_=0, int64_t off_=0);

  // Write pending output
  void flush() {
    assert(fp!=FPNULL);
    if (aes) aes->encrypt(buf, ptr, ftello(fp)+off);
    fwrite(buf, 1, ptr, fp);
    ptr=0;
  }

  // Position the next read or write offset to p.
  void seek(int64_t p, int whence) {
    if (fp!=FPNULL) {
      flush();
      fseeko(fp, p, whence);
    }
    else if (whence==SEEK_SET) off=p;
    else off+=p;  // assume at end
  }

  // Return current file offset.
  int64_t tell() const {
    if (fp!=FPNULL) return ftello(fp)+ptr;
    else return off;
  }

  // Write one byte
  void put(int c) {
    if (fp==FPNULL) ++off;
    else {
      if (ptr>=BUFSIZE) flush();
      buf[ptr++]=c;
    }
  }

  // Write buf[0..n-1]
  void write(const char* ibuf, int len) {
    if (fp==FPNULL) off+=len;
    else while (len-->0) put(*ibuf++);
  }

  // Flush output and close
  void close() {
    if (fp!=FPNULL) {
      flush();
      fclose(fp);
    }
    fp=FPNULL;
  }
};

// Create or update an existing archive or part. If filename is ""
// then keep track of position in off but do not write to disk. Otherwise
// open and encrypt with password if not 0. If the file exists then
// read the salt from the first 32 bytes and off_ must be 0. Otherwise
// encrypt assuming off_ previous bytes, of which the first 32 are salt_.
// If off_ is 0 then write salt_ to the first 32 bytes.

OutputArchive::OutputArchive(const char* filename, const char* password,
    const char* salt_, int64_t off_): /*off(off_), */ptr(0) {
  assert(filename);
  off=off_;
  if (!*filename) return;

  // Open existing file
  char salt[32]={0};
  fp=fopen(filename, RBPLUS);
  if (isopen()) {
    if (off!=0) error("file exists and off > 0");
    if (password) {
      if (fread(salt, 1, 32, fp)!=32) error("cannot read salt");
      if (salt_ && memcmp(salt, salt_, 32)) error("salt mismatch");
    }
    seek(0, SEEK_END);
  }

  // Create new file
  else {
    fp=fopen(filename, WB);
    if (!isopen()) ioerr(filename);
    if (password) {
      if (!salt_) error("salt not specified");
      memcpy(salt, salt_, 32);
      if (off==0 && fwrite(salt, 1, 32, fp)!=32) ioerr(filename);
    }
  }

  // Set up encryption
  if (password) {
    char key[32];
    libzpaq::stretchKey(key, password, salt);
    aes=new libzpaq::AES_CTR(key, 32, salt);
  }
}

///////////////////////// System info /////////////////////////////////

// Guess number of cores. In 32 bit mode, max is 2.
int numberOfProcessors() {
  int rc=0;  // result
#ifdef unix
#ifdef BSD  // BSD or Mac OS/X
  size_t rclen=sizeof(rc);
  int mib[2]={CTL_HW, HW_NCPU};
  if (sysctl(mib, 2, &rc, &rclen, 0, 0)!=0)
    perror("sysctl");

#else  // Linux
  // Count lines of the form "processor\t: %d\n" in /proc/cpuinfo
  // where %d is 0, 1, 2,..., rc-1
  FILE *in=fopen("/proc/cpuinfo", "r");
  if (!in) return 1;
  std::string s;
  int c;
  while ((c=getc(in))!=EOF) {
    if (c>='A' && c<='Z') c+='a'-'A';  // convert to lowercase
    if (c>' ') s+=c;  // remove white space
    if (c=='\n') {  // end of line?
      if (s.size()>10 && s.substr(0, 10)=="processor:") {
        c=atoi(s.c_str()+10);
        if (c==rc) ++rc;
      }
      s="";
    }
  }
  fclose(in);
#endif
#else

  // In Windows return %NUMBER_OF_PROCESSORS%
  //const char* p=getenv("NUMBER_OF_PROCESSORS");
  //if (p) rc=atoi(p);
  SYSTEM_INFO si={0};
  GetSystemInfo(&si);
  rc=si.dwNumberOfProcessors;
#endif
  if (rc<1) rc=1; /// numero massimo core 32bit
  if (sizeof(char*)==4 && rc>2) rc=2;
  return rc;
}

////////////////////////////// misc ///////////////////////////////////
//zpaq
// For libzpaq output to a string less than 64K chars
struct StringWriter: public libzpaq::Writer {
  string s;
  void put(int c) {
    if (s.size()>=65535) error("string too long");
    s+=char(c);
  }
};

// In Windows convert upper case to lower case.
inline int tolowerW(int c) {
#ifndef unix
  if (c>='A' && c<='Z') return c-'A'+'a';
#endif
  return c;
}

// Return true if strings a == b or a+"/" is a prefix of b
// or a ends in "/" and is a prefix of b.
// Match ? in a to any char in b.
// Match * in a to any string in b.
// In Windows, not case sensitive.
bool ispath(const char* a, const char* b) {
  
  /*
  printf("ZEKEa %s\n",a);
  printf("ZEKEb %s\n",b);
  */
  for (; *a; ++a, ++b) {
    const int ca=tolowerW(*a);
    const int cb=tolowerW(*b);
    if (ca=='*') {
      while (true) {
        if (ispath(a+1, b)) return true;
        if (!*b) return false;
        ++b;
      }
    }
    else if (ca=='?') {
      if (*b==0) return false;
    }
    else if (ca==cb && ca=='/' && a[1]==0)
      return true;
    else if (ca!=cb)
      return false;
  }
  return *b==0 || *b=='/';
}

// Read 4 byte little-endian int and advance s
unsigned btoi(const char* &s) {
  s+=4;
  return (s[-4]&255)|((s[-3]&255)<<8)|((s[-2]&255)<<16)|((s[-1]&255)<<24);
}

// Read 8 byte little-endian int and advance s
int64_t btol(const char* &s) {
  uint64_t r=btoi(s);
  return r+(uint64_t(btoi(s))<<32);
}

/////////////////////////////// Jidac /////////////////////////////////

// A Jidac object represents an archive contents: a list of file
// fragments with hash, size, and archive offset, and a list of
// files with date, attributes, and list of fragment pointers.
// Methods add to, extract from, compare, and list the archive.

// enum for version

static const int64_t HT_BAD=   -0x7FFFFFFFFFFFFFFALL;  // no such frag
static const int64_t DEFAULT_VERSION=99999999999999LL; // unless -until


// fragment hash table entry
struct HT {
	uint32_t crc32;			// new: take the CRC-32 of the fragment
	uint32_t crc32size;

	unsigned char sha1[20];  // fragment hash
	int usize;      // uncompressed size, -1 if unknown, -2 if not init
	int64_t csize;  // if >=0 then block offset else -fragment number
  
	HT(const char* s=0, int u=-2) 
	{
		crc32=0;
		crc32size=0;
		csize=0;
		if (s) 
			memcpy(sha1, s, 20);
		else 
			memset(sha1, 0, 20);
		usize=u;
	}
	HT(string hello,const char* s=0, int u=-1, int64_t c=HT_BAD) 
	{
		crc32=0;
		crc32size=0;
		if (s) 
			memcpy(sha1, s, 20);
		else 
			memset(sha1, 0, 20);
		usize=u; 
		csize=c;
	}
};



struct DTV {
  int64_t date;          // decimal YYYYMMDDHHMMSS (UT) or 0 if deleted
  int64_t size;          // size or -1 if unknown
  int64_t attr;          // first 8 attribute bytes
  double csize;          // approximate compressed size
  vector<unsigned> ptr;  // list of fragment indexes to HT
  int version;           // which transaction was it added?
  
  DTV(): date(0), size(0), attr(0), csize(0), version(0) {}
};

// filename entry
struct DT {
  int64_t date;          // decimal YYYYMMDDHHMMSS (UT) or 0 if deleted
  int64_t size;          // size or -1 if unknown
  int64_t attr;          // first 8 attribute bytes
  int64_t data;          // sort key or frags written. -1 = do not write
  vector<unsigned> ptr;  // fragment list
  char sha1hex[66];		 // 1+32+32 (SHA256)+ zero. In fact 40 (SHA1)+zero+8 (CRC32)+zero 
  vector<DTV> dtv;       // list of versions
	int written;           // 0..ptr.size() = fragments output. -1=ignore
  uint32_t crc32;
  string hexhash;
  DT(): date(0), size(0), attr(0), data(0),written(-1),crc32(0) {memset(sha1hex,0,sizeof(sha1hex));hexhash="";}
};
typedef map<string, DT> DTMap;

typedef map<int, string> MAPPACOMMENTI;
typedef map<string, string> MAPPAFILEHASH;
	


////// calculate CRC32 by blocks and not by file. Need to sort before combine
////// list of CRC32 for every file  
struct s_crc32block
{
	string	filename;
	uint64_t crc32start;
	uint64_t crc32size;
	uint32_t crc32;
	unsigned char sha1[20];  // fragment hash
	char sha1hex[40];
	s_crc32block(): crc32start(0),crc32size(0),crc32(0) {memset(sha1,0,sizeof(sha1));memset(sha1hex,0,sizeof(sha1hex));}
};
vector <s_crc32block> g_crc32;

///// sort by offset, then by filename
///// use sprintf for debug, not very fast.
bool comparecrc32block(s_crc32block a, s_crc32block b)
{
	char a_start[40];
	char b_start[40];
	sprintf(a_start,"%014lld",(long long)a.crc32start);
	sprintf(b_start,"%014lld",(long long)b.crc32start);
	return a.filename+a_start<b.filename+b_start;
}


// list of blocks to extract
struct Block {
  int64_t offset;       // location in archive
  int64_t usize;        // uncompressed size, -1 if unknown (streaming)
  int64_t bsize;        // compressed size
  vector<DTMap::iterator> files;  // list of files pointing here
  unsigned start;       // index in ht of first fragment
  unsigned size;        // number of fragments to decompress
  unsigned frags;       // number of fragments in block
  unsigned extracted;   // number of fragments decompressed OK
  enum {READY, WORKING, GOOD, BAD} state;
  Block(unsigned s, int64_t o): offset(o), usize(-1), bsize(0), start(s),
      size(0), frags(0), extracted(0), state(READY) {}
};

// Version info
struct VER {
  int64_t date;          // Date of C block, 0 if streaming
  int64_t lastdate;      // Latest date of any block
  int64_t offset;        // start of transaction C block
  int64_t data_offset;   // start of first D block
  int64_t csize;         // size of compressed data, -1 = no index
  int updates;           // file updates
  int deletes;           // file deletions
  unsigned firstFragment;// first fragment ID
  int64_t usize;         // uncompressed size of files
  
  VER() {memset(this, 0, sizeof(*this));}
};

// Windows API functions not in Windows XP to be dynamically loaded
#ifndef unix
typedef HANDLE (WINAPI* FindFirstStreamW_t)
                   (LPCWSTR, STREAM_INFO_LEVELS, LPVOID, DWORD);
FindFirstStreamW_t findFirstStreamW=0;
typedef BOOL (WINAPI* FindNextStreamW_t)(HANDLE, LPVOID);
FindNextStreamW_t findNextStreamW=0;
#endif

class CompressJob;

// Do everything
class Jidac {
public:
  int doCommand(int argc, const char** argv);
  friend ThreadReturn decompressThread(void* arg);
  friend ThreadReturn testThread(void* arg);
  friend struct ExtractJob;
private:

  // Command line arguments
  uint64_t minsize;
  uint64_t maxsize;
  uint32_t filelength;
  uint32_t dirlength;
  
  unsigned int menoenne;
  string versioncomment;
  bool flagcomment;
  char command;             // command 'a', 'x', or 'l'
  string archive;           // archive name
  vector<string> files;     // filename args
  vector<uint64_t> files_size;     // filename args
  vector<uint64_t> files_count;     // filename args
  vector<uint64_t> files_time;     // filename args
  vector<DTMap> files_edt;
  
  int all;                  // -all option
  bool flagforce;               // -force option
  int fragment;             // -fragment option
  const char* index;        // index option
  char password_string[32]; // hash of -key argument
  const char* password;     // points to password_string or NULL
  string method;            // default "1"
  bool flagnoattributes;        // -noattributes option
  vector<string> notfiles;  // list of prefixes to exclude
  string nottype;           // -not =...
  vector<string> onlyfiles; // list of prefixes to include
  vector<string> alwaysfiles; // list of prefixes to pack ALWAYS
  const char* repack;       // -repack output file
  char new_password_string[32]; // -repack hashed password
  const char* new_password; // points to new_password_string or NULL
  int summary;              // do summary if >0, else brief. OLD 7.15 summary option if > 0, detailed if -1
  
  bool flagtest;              // -test option
  bool flagskipzfs;				// -zfs option (hard-core skip of .zfs)
  bool flagnoqnap;				// exclude @Recently-Snapshot and @Recycle
  bool flagforcewindows;        // force alterate data stream $DATA$, system volume information
  bool flagdonotforcexls;       // do not always include xls
  bool flagnopath;				// do not store path
  bool flagnosort;              // do not sort files std::sort(vf.begin(), vf.end(), compareFilename);
  bool flagchecksum;			// get  checksum for every file (default SHA1)
  string searchfrom;		// search something in path
  string replaceto;			// replace (something like awk)
  int howmanythreads;              // default is number of cores
  vector<string> tofiles;   // -to option
  int64_t date;             // now as decimal YYYYMMDDHHMMSS (UT)
  int64_t version;          // version number or 14 digit date
  // Archive state
  int64_t dhsize;           // total size of D blocks according to H blocks
  int64_t dcsize;           // total size of D blocks according to C blocks
  vector<HT> ht;            // list of fragments
  DTMap dt;                 // set of files in archive
  DTMap edt;                // set of external files to add or compare
  vector<Block> block;      // list of data blocks to extract
  vector<VER> ver;          // version info

  map<int, string> mappacommenti;

  bool getchecksum(string i_filename, char* o_sha1); //get SHA1 AND CRC32 of a file

  // Commands
	int add();                				// add, return 1 if error else 0
	int extract();            				// extract, return 1 if error else 0
	int info();
	int list();               				// list (one parameter) / check (more than one)
	int verify();
	int kill();
	int utf();
	int test();           					// test, return 1 if error else 0
	int consolidate(string i_archive);

	int summa();							// get hash / or sum
	int deduplicate();							// get hash / or sum
	int paranoid();							// paranoid test by unz. Really slow & lot of RAM
	int fillami();							// media check
	void usage();        					// help
	int dir();								// Windows-like dir command
	int robocopy();
	int zero();
	int dircompare(bool i_flagonlysize,bool i_flagrobocopy);
	void printsanitizeflags();

  // Support functions
  string rename(string name);           // rename from -to
  int64_t read_archive(const char* arc, int *errors=0, int i_myappend=0);  // read arc
  bool isselected(const char* filename, bool rn,int64_t i_size);// files, -only, -not
  void scandir(bool i_checkifselected,DTMap& i_edt,string filename, bool i_recursive=true);        // scan dirs to dt
  void addfile(bool i_checkifselected,DTMap& i_edt,string filename, int64_t edate, int64_t esize,int64_t eattr);          // add external file to dt
			   
	int64_t franzparallelscandir(bool i_flaghash);

  bool equal(DTMap::const_iterator p, const char* filename, uint32_t &o_crc32);
             // compare file contents with p
	
	///string mygetalgo();					// checksum choosed
	///int flag2algo();

	void usagefull();         			// verbose help
	void examples();						// some examples
	void differences();					// differences from 7.15
	void write715attr(libzpaq::StringBuffer& i_sb, uint64_t i_data, unsigned int i_quanti);
	void writefranzattr(libzpaq::StringBuffer& i_sb, uint64_t i_data, unsigned int i_quanti,bool i_checksum,string i_filename,char *i_sha1,uint32_t i_crc32);
	int enumeratecomments();
	int searchcomments(string i_testo,vector<DTMap::iterator> &filelist);
	

 };

int terminalwidth() 
{
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return (int) csbi.srWindow.Right - csbi.srWindow.Left + 1;
   /// return (int)(csbi.dwSize.X);
#else
    struct winsize w;
    ioctl(fileno(stdout), TIOCGWINSZ, &w);
    return (int)(w.ws_col);
#endif 
}
int terminalheight()
{
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
//    return (int)(csbi.dwSize.Y);
	return (int)csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize w;
    ioctl(fileno(stdout), TIOCGWINSZ, &w);
    return (int)(w.ws_row);
#endif 
}

void mygetch()
{
	int mychar=0;
#if defined(_WIN32)
	mychar=getch();
#endif


#ifdef unix
/// BSD Unix
	struct termios oldt, newt;
	tcgetattr ( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr ( STDIN_FILENO, TCSANOW, &newt );
	mychar = getchar();
	tcsetattr ( STDIN_FILENO, TCSANOW, &oldt );
#endif



	if ((mychar==113) || (mychar==81) || (mychar==3))  /// q, Q, control-C
	{
#ifdef unix
		printf("\n\n");
#endif
	exit(0);
	}
}
void moreprint(const char* i_stringa)
{
	
	 int larghezzaconsole=terminalwidth()-2;
	 int altezzaconsole=terminalheight();
	 static int righestampate=0;
	if (!i_stringa)
		return;
		
		
	if ((larghezzaconsole<0) || (altezzaconsole<0))
	{
		printf("%s\n",i_stringa);
		return;
	}

	if (!strcmp(i_stringa,"\n"))
	{
		printf("\n");
		righestampate++;
		if (righestampate>(altezzaconsole-2))
		{
			printf("-- More (q, Q or control C to exit) --\r");
			mygetch();
			for (int i=0;i<altezzaconsole;i++)
				printf("\n");
			righestampate=0;
		}
		return;
	}
	 int lunghezzastringa=strlen(i_stringa);
	
	if (!larghezzaconsole)
		return;

	 int righe=(lunghezzastringa/larghezzaconsole)+1;
	 int massimo=lunghezzastringa-(larghezzaconsole*(righe-1));
	
	for (int riga=1; riga<=righe;riga++)
	{
		
		int currentmax=larghezzaconsole;
		
		if (riga==righe)
			currentmax=massimo;

		int startcarattere=(riga-1)*larghezzaconsole;
		for (int i=startcarattere;i<startcarattere+currentmax;i++)
			printf("%c",i_stringa[i]);
		printf("\n");
		
		righestampate++;
		if (righestampate>(altezzaconsole-2))
		{
				printf("-- More (q, Q or control C to exit) --\r");
				mygetch();
			for (int i=0;i<altezzaconsole;i++)
				printf("\n");
			righestampate=0;
		}
	}	
}
void Jidac::differences() 
{
	
	int twidth=terminalwidth();
	if (twidth<10)
		twidth=10;
	
	char barbuffer[twidth+10];
	barbuffer[0]=0;
	for (int i=0;i<twidth-4;i++)
		sprintf(barbuffer+i,"-");
			
			
	
moreprint("@2021-06-21: Key differences from ZPAQ 7.15 and zpaqfranz");
moreprint("");
moreprint("Doveryay, no proveryay (trust, but verify; fidarsi e' bene, non fidarsi e' meglio).");
moreprint("'Verify' is the goal of this zpaq fork, without breaking 7.15 compatibility.");
moreprint("");
moreprint("Pack everything needed for a storage manager (dir compare, hashing, deduplication,");
moreprint("fix utf-8 filenames, wiping etc) on Win/*Nix in a single executable.");
moreprint("");
moreprint("'Smart' support for non-latin charsets,Windows and non-Windows, smooth with ZFS.");
moreprint(barbuffer);
moreprint("CMD   a (add)");
moreprint("DIFF: zpaqfranz (by default) store CRC-32s of the files, while zpaq cannot by design.");
moreprint("      Any SHA-1 collisions are detected by zpaqfranz, not by zpaq.");
moreprint("      Additional computation (can be disabled by -crc32 or -715), on modern CPU,");
moreprint("      does not significantly slow down.");
moreprint("PLUS: -checksum store hash code of each file (slower, but almost 100% sure).");
moreprint("DIFF: by default do NOT store ADSs on Windows (essentially useless).");
moreprint("DIFF: By default every .XLS file is forcibily added (old Excel change metafiles).");
moreprint("PLUS: -forcewindows, -forcezfs, -xls and -715 to mimic old behaviour.");
moreprint("PLUS: freezing many zfs snapshot in one archive by -timestamp X, setting version datetime");
moreprint("      @X, ex 2021-12-30_01:03:04 Must be monotonic. increasing (v[i+1].date>v[i]+date)");
moreprint("PLUS: -test do a post-add test (doveryay, no proveryay).");
moreprint("PLUS: -vss Volume Shadow Copies (Win with admin rights) to backup files from %users%.");
moreprint("DIFF: Progress output with switches -noeta, -verbose, -pakka, -debug, summary, -n K");
moreprint("PLUS: ASCII comment for versions with -comment sometext.");
moreprint(barbuffer);
moreprint("CMD   x (extract)");
moreprint("PLUS: During extraction,if CRC-32s are present, the codes are checked.");
moreprint("PLUS: -checksum force a full hash-code verify (if added with -checksum)");
moreprint("PLUS: -kill extract to dummy, 0-length files. Da a empty-full restore.");
moreprint("PLUS: -utf change everything non latin to latin (Linux/*Nix => NTFS compatibility)");
moreprint("PLUS: -fix255 shrink max file name, avoid different case collision (Linux => NTFS)");
moreprint("      (pippo.txt and PIPPO.txt are be silently overwritten by 7.15).");
moreprint("PLUS: -fixeml compress .eml filenames.");
moreprint("PLUS: -flat emergency restore of everything into a single folder (Linux => NTFS)");
moreprint(barbuffer);
moreprint("CMD   l (list)");
moreprint("PLUS: If source folders are specified, do a compare of the archive's content.");
moreprint("      In fact this is a 'verify' more than a 'list', much faster than the standard,");
moreprint("      as it performs a block calculation of the SHA1 codes of source files, but not of");
moreprint("      the archived ones; also checks the CRC-32, to intercept any SHA1 collisions.");
moreprint("      Use the exact same parameters of add(), just use l instead of a.");
moreprint("PLUS: Filtering by by -comment something, -find pippo (just like |grep -i pippo),");
moreprint("      -replace pluto => replace 'pippo' with 'pluto' in the output.");
moreprint(barbuffer);
moreprint("CMD   i (info)");
moreprint("PLUS: Directly shows the versions into the archive, with size (and comments, if any)");
moreprint(barbuffer);
moreprint("CMD   t (test)");
moreprint("PLUS: Compared to 7.15, check that all block are OK, and the CRC-32s of the individual ");
moreprint("      files corresponds to what would be generated by actually extracting.");
moreprint("PLUS: -verify do a filesystem post-check: STORED CRC==DECOMPRESSED==FROM FILE.");
moreprint(barbuffer);
moreprint("CMD   p (paranoid test)");
moreprint("PLUS: Test the archive (** does not work for multipart **) in a very paranoid fashion.");
moreprint("      ZPAQ reference decompressor is used to extract various blocks in RAM and check them.");
moreprint("      In fact runs a unzpaq instead of 7.15 extract: double check to avoid the risk of");
moreprint("      'silent' bugs. Just about extracting the entire archive in RAM, amount needed can");
moreprint("      quickly become unmanageable (warn: be very careful with 32bit versions).");
moreprint("PLUS: Hard-hash check on archive created with a -checksum");
moreprint("PLUS: -verify next level (mine) of paranoia: check SHA-11 against a re-read from filesystem. ");
moreprint("      Essentially equivalent to extracting thearchive in a temporary folder and check");
moreprint("      against initial folders. For very paranoid people, or debug reason.");
moreprint(barbuffer);
moreprint("CMD   c (compare dirs)");
moreprint("PLUS: Compare a 'master' directory (d0) against N 'slaves' (d1, d2... dN).");
moreprint("      Typically the slaves are made by zpaq restores, or rsync, robocopy, zfs replica.");
moreprint("      It is not easy to check on different filesystems (ex. NAS-Linux, NTFS) the charsets");
moreprint("      By default check file name and file size (excluding .zfs), not the content.");
moreprint("PLUS: -verify run a hash 'hard' check, suggested -xxhash, fast and reliable.");
moreprint("PLUS: -all N concurrent threads will be created, each scan a slave dir (-t K to limit).");
moreprint("      NOT good for single spinning drives, good for multiple slaves on different media.");
moreprint(barbuffer);
moreprint("CMD   s (size)");
moreprint("PLUS: Cumulative size of N directory, and an estimate of the free space on its filesystem.");
moreprint("      Everything containing .zfs and :$DATA (Windows's ADS) ignored by default");
moreprint("      Very quick-and-dirty check of rsynced folders against the master");
moreprint("PLUS: -all for multithreaded executions (warning for single spinning drive)");
moreprint(barbuffer);
moreprint("CMD   r ('robocopy')");
moreprint("PLUS: Mirror a master folder in K slaves, just like robocopy /mir or rsync -a --delete");
moreprint("      Ignore .zfs and ADS by default (-715 or -forcezfs to enable)");
moreprint("PLUS: -kill wet run (default: dry-run");
moreprint("PLUS: -all run one thread for folder");
moreprint("PLUS: -verify after copy quick check if OK (only filename and size)");
moreprint("PLUS: -checksum heavy (hash) test of equality. Suggest: -xxhash fast and reliable.");
moreprint(barbuffer);
moreprint("CMD   z (delete empty directories, zero length)");
moreprint("PLUS: Remove empty directories in d0...dK folders. Conservative (ex hidden Thumbs.db)");
moreprint("PLUS: -kill Do a wet run (default dry run)");
moreprint("PLUS: -all  Multithread scan");
moreprint("PLUS: -verbose Show infos");
moreprint(barbuffer);
moreprint("CMD   m (merge, consolidate)");
moreprint("PLUS: Merge a splitted (multipart) archive into a single one, just like a concatenated cat");
moreprint("PLUS: -force Overwrite existing output, ignore lack of free space");
moreprint("PLUS: -verify do a double-check (XXH3 hash test)");
moreprint(barbuffer);
moreprint("CMD   d (deduplicate)");
moreprint("PLUS: Deduplicate a single folder WITHOUT ASKING ANYTHING!");
moreprint("PLUS: -force wet run (default: dry-run)");
moreprint("PLUS: -verbose show duplicated files");
moreprint("PLUS: -sha256 use sha256 instead of XXH3 for detection (slower, the most reliable)");
moreprint(barbuffer);
moreprint("CMD   f (fill, or wipe)");
moreprint("PLUS: Fill (wipe) 9%% of free disk space, checking that data is well written, in 500MB chunks");
moreprint("      Check if disk-controller-system-RAM-cache-cables are working fine");
moreprint("PLUS: -verbose show write speed (useful to check speed consistency)");
moreprint("PLUS: -kill delete (after run) the temporary filename. By default do NOT erase temporary files");
moreprint(barbuffer);
moreprint("CMD   utf (deal with strange filenames)");
moreprint("PLUS: Check (or sanitize) paths with non-latin chars and/or >260 length and/or case.");
moreprint("      Can become a real problem extracting on different filesystems (ex. *nix => NTFS)");
moreprint("PLUS: -kill (wet run, default dry run)");
moreprint("PLUS: -utf (sanitize filenames)");
moreprint("PLUS: -fix255 (sanitize file length and filecase collisions pippo.txt PIPPO.txt)");
moreprint("PLUS: -fixeml (sanitize .eml filenames)");
moreprint("PLUS: -dirlength X (set the 'fix')");
moreprint("PLUS: -filelength Y ");
moreprint(barbuffer);
moreprint("CMD   sha1 (hashes: named for historical reasons, 7.15 always uses SHA-1 only)");
moreprint("PLUS: Calculate hashe/checksum of files/directories, duplicates, and cumulative GLOBAL SHA256");
moreprint("      (If two directories have the same GLOBAL SHA256 they are ==)");
moreprint("      With no switches, by default, use SHA-1 (reliable, but not very fast)");
moreprint("PLUS: -xxhash very fast hash algorithm (XXH3)");
moreprint("PLUS: -crc32  very fast checksum");
moreprint("PLUS: -crc32c very fast hardware-accelerated CRC-32c");
moreprint("PLUS: -sha256 slow but very reliable, legal level (in Italy)");
moreprint("PLUS: -all make N thread (do not use with spinning HDDs, but SSDs and NVMes)");
moreprint("PLUS: -kill show the files to be deleted to manually deduplicate");
moreprint("PLUS: -checksum get a 1-level checksum, for comparing hierarchically user-organized folders.");
moreprint("PLUS: -summary show only GLOBAL (fast manual compare of directories)");
moreprint("PLUS: -forcezfs force .zfs path (DEFAULT: skip)");
moreprint("PLUS: -kill -force => runs a deduplication without ask anything!");
moreprint(barbuffer);
moreprint("CMD   dir (yes, dir as in Windows).");
moreprint("PLUS: I really,really hate the ls command, which does not show the cumulative filesize (!)");
moreprint("      This is a 'mini clone' of Windows's dir command, with the main switches");
moreprint("      /s, /os, /on, /a and (yessss!)      -n X => like |tail -X");
moreprint("EX:   Largest file in c:\\z ?             zpaqfranz dir c:\\z /os -n 1");
moreprint("EX:   10 biggest files in c:\\?           zpaqfranz dir c:\\z /s /os -n 10");
moreprint("EX:   How big is c:\\z, with subdirs?     zpaqfranz dir c:\\z /s  -n 1");
moreprint("EX:   100 biggest dups in c:\\z?          zpaqfranz dir c:\\z /s -crc32 -n 100");
moreprint(barbuffer);
moreprint("CMD   k (kill, risky!)");
moreprint("PLUS: kill (delete) all files and directories that arent in an archivem removing excess files");
moreprint("EX:   create an archive                   zpaqfranz a z:\\1.zpaq c:\\z");
moreprint("      extract into z:\\kbn                zpaqfranz x z:\\1.zpaq c:\\z -to z:\\knb");
moreprint("      ... something happens (change) in z:\\knb and we want to turn back to archive ...");
moreprint("      ... WITHOUT delete everything and extract again (maybe it's huge) ...");
moreprint("                                          zpaqfranz k z:\\1.zpaq c:\\z -to z:\\knb");
moreprint(barbuffer);
moreprint("New switches for automation (launched after the execution)");
moreprint("PLUS: During script execution it is possible to check the result (0=OK, 1=WARN, 2=ERROR)");
moreprint("      But it can be inconvenient to intercept the result (e.g. cron), so it is possible to");
moreprint("      run a script or program directly");
moreprint("EX:   -exec_ok    c:\\nz\\fo.bat     => After OK launch c:\\nz\\fo.bat");
moreprint("EX:   -exec_error c:\\nz\\kaputt.bat => After NOT OK launch c:\\nz\\kaputt.bat");
}


//// some examples (to be completed)
void Jidac::examples() 
{
	int twidth=terminalwidth();
	if (twidth<10)
		twidth=10;
	
	char barbuffer[twidth+10];
	barbuffer[0]=0;
	for (int i=0;i<twidth-4;i++)
		sprintf(barbuffer+i,"-");
			
			
	moreprint(barbuffer);
	moreprint("   ADDING FILES TO ARCHIVE");
	moreprint("Add two folders to archive:          a z:\\1.zpaq c:\\data\\* d:\\pippo\\*");
	moreprint("Add folder, storing full hash        a z:\\2.zpaq c:\\nz\\ -checksum");
	moreprint("Add folder, then verification:       a z:\\3.zpaq c:\\nz\\ -test");
	moreprint("Add two files:                       a z:\\4.zpaq.zpaq c:\\vecchio.sql r:\\1.txt");
	moreprint("Add as zpaq 7.15 (no checksum):      a z:\\5.zpaq c:\\audio -715");
	moreprint("Add and mark version:                a z:\\6.paq c:\\data\\* -comment first_copy");
#if defined(_WIN32) || defined(_WIN64)
	moreprint("Add by a VSS (Windows admin):        a z:\\7.zpaq c:\\users\\utente\\* -vss");
#endif
	moreprint("Add folder with timestamping (zfs):  a z:\\8.zpaq c:\\data\\* -timestamp 2021-12-30_01:03:04");
	moreprint("Create multipart archive:            a \"z:\\9_????.zpaq\" c:\\data\\");
	moreprint("Create indexed multipart archive:    a \"z:\\a_???.zpaq\" c:\\data\\ -index z:\\a_000.zpaq");
	moreprint("Add folder, with encryption:         a z:\\b.zpaq c:\\nz\\ -key mygoodpassword");
	moreprint("Add folder, maximum compress         a z:\\c.zpaq c:\\nz\\ -m 5");
	moreprint("\n");
	moreprint(barbuffer);
	moreprint("   LISTING/GETTING INFOS");
	moreprint("Last version:                        l z:\\1.zpaq");
	moreprint("All (every) version:                 l z:\\1.zpaq -all");
	moreprint("Version comments (if any):           l z:\\1.zpaq -comment");
	moreprint("V.comments verbose (if any):         l z:\\1.zpaq -comment -all");
	moreprint("Only stored 'zpaq.cpp':              l z:\\1.zpaq -find zpaq.cpp -pakka");
	moreprint("Find-and-replace (like awk or sed)   l r:\\1.zpaq -find c:/biz/ -replace z:\\mydir\\");
	moreprint("List the 10 greatest file            l z:\\1.zpaq -n 10");
	moreprint("Show versions info (command i):      i z:\\1.zpaq");
	moreprint("\n");
	moreprint(barbuffer);
	moreprint("   TESTING ARCHIVE");
	moreprint("Last version:                        t z:\\1.zpaq");
	moreprint("All versions:                        t z:\\1.zpaq -all");
	moreprint("Compare against filesystem:          t z:\\1.zpaq -verify");
	moreprint("Highly reliable (nz the source dir): l z:\\1.zpaq c:\\nz");
	moreprint("Paranoid, use lots of RAM:           p z:\\1.zpaq");
	moreprint("Very paranoid, use lots of RAM:      p z:\\1.zpaq -verify");
	moreprint("\n");
	moreprint(barbuffer);
	moreprint("    EXTRACTING");
	moreprint("Extract into folder muz7:            x z:\\1.zpaq -to z:\\muz7\\");
	moreprint("0-bytes files (check restoration):   x z:\\1.zpaq -to z:\\muz7\\ -kill");
	moreprint("Extract into single directory:       x z:\\1.zpaq -to z:\\muz7\\ -flat");
	moreprint("Extract without utf,<255,.eml:       x z:\\1.zpaq -to z:\\muz7\\ -utf -fix255 -fixeml");
	moreprint("Extract forcing overwrite:           x z:\\1.zpaq -to z:\\muz7\\ -force");
	moreprint("Extract version K:                   x z:\\1.zpaq -to z:\\muz7\\ -until K");
	moreprint("Extract ALL versions of multipart    x \"z:\\a_???\" -to z:\\ugo -all");
	moreprint("\n");
	moreprint(barbuffer);
	moreprint("    SUPPORT FUNCTIONS FOR DIRS");
	moreprint("Dir cumulative size (no .zfs/NTFS):  s r:\\vbox s:\\uno");
	moreprint("Multithreaded size:                  s r:\\vbox s:\\uno -all");
	moreprint(barbuffer);
	moreprint("Compare master d0 against d1,d2,d3:  c c:\\d0 k:\\d1 j:\\d2 p:\\d3");
	moreprint("Multithread compare:                 c c:\\d0 k:\\d1 j:\\d2 p:\\d3 -all");
	moreprint("Hashed compare d0 against d1,d2,d3:  c c:\\d0 k:\\d1 j:\\d2 p:\\d3 -verify");
	moreprint(barbuffer);
	moreprint("Robocopy d0 in d1,d2 (dry run):      r c:\\d0 k:\\d1 j:\\d2 p:\\d3");
	moreprint("Robocopy d0 in d1,d2 (WET run):      r c:\\d0 k:\\d1 j:\\d2 p:\\d3 -kill");
	moreprint("Robocopy with verify (WET run):      r c:\\d0 k:\\d1 j:\\d2 p:\\d3 -kill -verify");
	moreprint("Robocopy with hash verify (WET run): r c:\\d0 k:\\d1 j:\\d2 p:\\d3 -kill -verify -checksum");
	moreprint(barbuffer);
	moreprint("Delete empty dirs in d0,d1(dry run): z c:\\d0 k:\\d1");
	moreprint("Delete empty dirs in d0,d1(WET run): z c:\\d0 k:\\d1 -kill");
	moreprint(barbuffer);
	moreprint("Deduplicate d0 WITHOUT MERCY:        d c:\\d0");
	moreprint("Deduplicate d0 WITHOUT MERCY (wet):  d c:\\d0 -force");
	moreprint("Dedup WITHOUT MERCY (wet run,sha256):d c:\\d0 -force -sha256");
	moreprint(barbuffer);
	moreprint("Windows-dir command clone:           dir /root/script /od");
	moreprint("Show the 10 largest .mp4 file in c:\\:dir c:\\ /s /os -n 10 -find .mp4");
	moreprint("Find .mp4 duplicate in C:\\:          dir c:\\ /s -crc32 -find .mp4");
	moreprint("How big is c:\\z,with subdirs?:       dir c:\\z /s -n 1");
	moreprint("100 biggest dup. files in c:\\z?:     dir c:\\z /s -crc32 -n 100");
	moreprint("\n");
	moreprint(barbuffer);
	moreprint("   SUPPORT FUNCTIONS FOR FILES");
	moreprint("SHA1 of all files (and duplicated):  sha1 z:\\knb");
	moreprint("SHA1 multithread, only summary:      sha1 z:\\knb -all -summary");
	moreprint("XXH3 multithread:                    sha1 z:\\knb -all -xxhash");
	moreprint("CRC-32c HW accelerated:              sha1 z:\\knb -crc32c -pakka -noeta");
	moreprint("Hashes to be compared (dir1):        sha1 c:\\nz  -pakka -noeta -nosort -crc32c -find c:\\nz  -replace bakdir >1.txt");
	moreprint("Hashes to be compared (dir2):        sha1 z:\\knb -pakka -noeta -nosort -crc32c -find z:\\knb -replace bakdir >2.txt");
	moreprint("Duplicated files with sha256:        sha1 z:\\knb -kill -sha256");
	moreprint("Duplicated files minsize 1000000:    sha1 z:\\knb -kill -all -minsize 1000000");
	moreprint("MAGIC cumulative hashes of 1-level:  sha1 p:\\staff -xxhash -checksum");
	moreprint(barbuffer);
	moreprint("Fill (wipe) almost all free space:   f z:\\");
	moreprint("Fill (wipe), free space after run:   f z:\\ -kill -verbose");
	moreprint(barbuffer);
	moreprint("Check UTF-filenames (dry run):       utf z:\\knb");
	moreprint("Sanitize UTF-filenames (wet run):    utf z:\\knb -kill");
	moreprint(barbuffer);
	moreprint("Check >255 and case collisions:      utf z:\\knb -fix255");
	moreprint("Fix .eml filenames (dry run):        utf z:\\knb -fixeml");
	moreprint(barbuffer);
	moreprint("   RISKY COMMAND: DO NOT USE IF YOU DO NOT UNDERSTAND!");
	moreprint("Kill everything not in archive:      a z:\\1.zpaq c:\\z");
	moreprint("Extract into Z                       x z:\\1.zpaq c:\\z -to z:\\knb");
	moreprint("... something change z:\\knb:         k z:\\1.zpaq c:\\z -to z:\\knb");
}

//// cut a too long filename 
string mymaxfile(string i_filename,const unsigned int i_lunghezza)
{
	if (i_lunghezza==0)
		return "";
	if (i_filename.length()<=i_lunghezza)
		return i_filename;
	
	if (i_lunghezza>10)
	{
		if (i_filename.length()>10)
		{
			unsigned int intestazione=i_lunghezza-10;
			return i_filename.substr(0,intestazione)+"(...)"+i_filename.substr(i_filename.length()-5,5);
		}
		else
		return i_filename.substr(0,i_lunghezza);
			
	}
	else
		return i_filename.substr(0,i_lunghezza);
}

//// default help
void Jidac::usage() 
{
	moreprint("Usage: zpaqfranz command archive[.zpaq] files|directory... -switches...");
	moreprint("Multi-part archive name               | \?\?\?\?\? => \"test_?????.zpaq\"");
///	moreprint("       Archive:");
	moreprint("             a: Append files          |          t: Fast test (-all)");
	moreprint("             x: Extract versions      |          l: list files (-all)");
	moreprint(" l d0 d1 d2...: Compare (test) content|          i: show archive's versions");
	moreprint("                                   Various");
///	moreprint("--------------------------------------|--------------------------------------");
	moreprint(" c d0 d1 d2...: Compare d0 to d1,d2...| s d0 d1 d2: cumulative size of d0,d1,d2");
	moreprint(" r d0 d1 d2...: Mirror d0 in d1...    |       d d0: deduplicate d0 WITHOUT MERCY");
	moreprint(" z d0 d1 d2...: Delete empty dirs     |        m X: merge multipart archive");
	moreprint("          f d0: Fill (wipe) free space|     utf d0: Detox filenames in d0");
	moreprint(" sha1 d0 d1...: Hashing/deduplication |     dir d0: Win dir (/s /a /os /od -crc32)");
	moreprint("                                Main switches");
	moreprint("      -all [N]: All versions N digit  |     -key X: archive password X");
	moreprint(" -mN -method N: 0..5= faster..better  |     -force: always overwrite");
	moreprint("         -test: Verify (extract/add)  |      -kill: allow destructive operations");
	moreprint("    -to out...: Prefix files to out   |   -until N: Roll back to N'th version");
	moreprint("                               Getting help");
	moreprint(" -h: Long help (-?)     -diff: against zpaq 715     -examples: common examples");
	//moreprint("            -h: Long help (-?)        |  -examples: Usage examples");
	///moreprint("--------------------------------------|--------------------------------------");
}


//// print a lot more
void Jidac::usagefull() 
{

  time_t now=time(NULL);
  tm* t=gmtime(&now);
  date=(t->tm_year+1900)*10000000000LL+(t->tm_mon+1)*100000000LL
      +t->tm_mday*1000000+t->tm_hour*10000+t->tm_min*100+t->tm_sec;

usage();
char buffer[200];
moreprint("Various:");
moreprint("   c d0 d1 d2... Compare dir d0 to d1,d2... (-all M/T scan -checksum -xxhash...)");
moreprint("   s d0 d1 d2... Cumulative size (excluding .zfs) of d0,d1,d2 (-all M/T -715)");
moreprint("   r d0 d1 d2... Mirror d0 in d1... (-kill -verify -checksum -all -xxhash...");
moreprint("   d d0          Deduplicate folder d0 WITHOUT MERCY (-force -verbose -sha256)");
moreprint("   z d0 d1 d2... Delete empty directories in d0, d1, d2 (-kill -verbose -all)");
moreprint("   m archive out Merge (consolidate) multi-part archive into one (-force -verify)");
moreprint("   f             Fill (wipe) almost all disk space and check (-verbose -kill)");
moreprint("   utf d0        Check/detox filenames in d0 (-dirlength X -filelength Y -kill)");
moreprint("   sha1          Hash/dup (#HASH -all -kill -force -checksum -verify -summary)");
moreprint("                 #HASH can be (nothing=>SHA1) -crc32 -crc32c -xxhash -sha256");
moreprint("   dir           Windows dir-like (/s /a /os /od) -crc32 show duplicates");
moreprint("\n");
moreprint("Extended switches:");
sprintf(buffer,"  -until %s   Set date, roll back (UT, default time: 235959)",dateToString(date).c_str());
moreprint(buffer);
moreprint("  -not files...   Exclude. * and ? match any string or char");
moreprint("       =[+-#^?]   List: exclude by comparison result");
moreprint("  -only files...  Include only matches (default: *) example *pippo*.mp4");
moreprint("  -always files   Always (force) adding some file");
moreprint("  -noattributes   Ignore/don't save file attributes or permissions");
moreprint("  -index F        Extract: create index F for archive");
moreprint("                  Add: create suffix for archive indexed by F, update F");
moreprint("  -sN -summary N  If >0 show only summary (sha1())");
moreprint("\n");
moreprint("zpaqfranz switches:");
moreprint("  -715            Works just about like v7.15");
moreprint("  -checksum       Store SHA1+CRC32 for every file");
moreprint("  -verify         Force re-read of file during t (test command) or c");
moreprint("  -noeta          Do not show ETA");
moreprint("  -pakka          Output for PAKKA (briefly)");
moreprint("  -verbose        Show excluded file");
moreprint("  -zfs            Skip paths including .zfs");
moreprint("  -forcezfs       Force paths including .zfs");
moreprint("  -noqnap         Skip path including @Recently-Snapshot and @Recycle");
moreprint("  -forcewindows   Take $DATA$ and System Volume Information");
moreprint("  -xls            Do NOT always force XLS");
moreprint("  -nopath         Do not store path");
moreprint("  -nosort         Do not sort file when adding or listing");
moreprint("  -find       X   Search for X in full filename (ex. list)");
moreprint("  -replace    Y   Replace X with Y in full filename (ex. list)");
moreprint("  -n          X   Only print last X lines in dir (like tail)/first X (list)");
moreprint("  -limit      X   (like -n)");
moreprint("  -minsize    X   Skip files by length (add(), list(), dir())");
moreprint("  -maxsize    X   Skip files by length (add(), list(), dir())");
moreprint("  -filelength X   Utf command: find file with length>X, extract maxfilelen");
moreprint("  -dirlength  X   Utf command: find dirs with length>X, extract maxdirlen");
moreprint("  -comment foo    Add/find ASCII comment string to versions");
#if defined(_WIN32) || defined(_WIN64)
moreprint("  -vss            Do a VSS for drive C: (Windows with administrative rights)");
#endif
moreprint("  -crc32c         In sha1 command use CRC32c instead of SHA1");
moreprint("  -crc32          In sha1 command use CRC32");
moreprint("  -xxhash         In sha1 command use XXH3");
moreprint("  -sha256         In sha1 command use SHA256");
moreprint("  -exec_ok fo.bat After OK launch fo.bat");
moreprint("  -exec_error kz  After NOT OK launch kz");
moreprint("  -kill           Show 'script-ready' log of dup files");
moreprint("  -kill           In extraction write 0-bytes file instead of data");
moreprint("  -utf            Remove non-utf8 chars");
moreprint("  -utf8           Like -utf");
moreprint("  -fix255         Shrink total file length and case collisions (NTFS)");
moreprint("  -fixeml         Heuristically compress .eml filenames (Fwd Fwd Fwd =>Fwd)");
moreprint("  -flat           Everything in single path (emergency extract of strange files)");
moreprint("  -debug          Show lot of infos (superverbose)");
moreprint("  -timestamp X    Set version datetime@X 14 digit (2021-12-30_01:03:04)"); 	// force the timestamp
moreprint("\n");
moreprint("Advanced commands:");
moreprint("   p              Paranoid test. Use lots (LOTS!) of RAM (-verify -verbose)");
moreprint("   k              file.zpaq sourcepath -to foo");
moreprint("\n");
moreprint("Voodoo switches:");
moreprint("  -repack F [X]   Extract to new archive F with key X (default: none)");
sprintf(buffer,"  -tN -threads N  Use N threads (default: 0 = %d cores.",howmanythreads);
moreprint(buffer);
moreprint("  -fragment N     Use 2^N KiB average fragment size (default: 6)");
moreprint("  -mNB -method NB Use 2^B MiB blocks (0..11, default: 04, 14, 26..56)");
moreprint("  -method {xs}B[,N2]...[{ciawmst}[N1[,N2]...]]...  Advanced:");
moreprint("  x=journaling (default). s=streaming (no dedupe)");
moreprint("    N2: 0=no pre/post. 1,2=packed,byte LZ77. 3=BWT. 4..7=0..3 with E8E9");
moreprint("    N3=LZ77 min match. N4=longer match to try first (0=none). 2^N5=search");
moreprint("    depth. 2^N6=hash table size (N6=B+21: suffix array). N7=lookahead");
moreprint("    Context modeling defaults shown below:");
moreprint("  c0,0,0: context model. N1: 0=ICM, 1..256=CM max count. 1000..1256 halves");
moreprint("    memory. N2: 1..255=offset mod N2, 1000..1255=offset from N2-1000 byte");
moreprint("    N3...: order 0... context masks (0..255). 256..511=mask+byte LZ77");
moreprint("    parse state, >1000: gap of N3-1000 zeros");
moreprint("  i: ISSE chain. N1=context order. N2...=order increment");
moreprint("  a24,0,0: MATCH: N1=hash multiplier. N2=halve buffer. N3=halve hash tab");
moreprint("  w1,65,26,223,20,0: Order 0..N1-1 word ISSE chain. A word is bytes");
moreprint("    N2..N2+N3-1 ANDed with N4, hash mulitpiler N5, memory halved by N6");
moreprint("  m8,24: MIX all previous models, N1 context bits, learning rate N2");
moreprint("  s8,32,255: SSE last model. N1 context bits, count range N2..N3");
moreprint("  t8,24: MIX2 last 2 models, N1 context bits, learning rate N2");
moreprint("\n");
differences();
exit(1);
}


// return a/b such that there is exactly one "/" in between, and
// in Windows, any drive letter in b the : is removed and there
// is a "/" after.
string append_path(string a, string b) {
  int na=a.size();
  int nb=b.size();
#ifndef unix
  if (nb>1 && b[1]==':') {  // remove : from drive letter
    if (nb>2 && b[2]!='/') b[1]='/';
    else b=b[0]+b.substr(2), --nb;
  }
#endif
  if (nb>0 && b[0]=='/') b=b.substr(1);
  if (na>0 && a[na-1]=='/') a=a.substr(0, na-1);
  return a+"/"+b;
}


#ifdef _WIN32

	uint32_t convert_unicode_to_ansi_string(std::string& ansi,const wchar_t* unicode,const size_t unicode_size) 
	{
		uint32_t error = 0;
		do 
		{
			if ((nullptr == unicode) || (0 == unicode_size)) 
			{
				error = ERROR_INVALID_PARAMETER;
				break;
			}
			ansi.clear();
			int required_cch=::WideCharToMultiByte(
									CP_ACP,
									0,
									unicode, static_cast<int>(unicode_size),
									nullptr, 0,
									nullptr, nullptr
									);

			if (required_cch==0) 
			{
				error=::GetLastError();
				break;
			}
			ansi.resize(required_cch);
			if (0 == ::WideCharToMultiByte(
						CP_ACP,
						0,
						unicode, static_cast<int>(unicode_size),
						const_cast<char*>(ansi.c_str()), static_cast<int>(ansi.size()),
						nullptr, nullptr
						)) 
			{
				error =::GetLastError();
				break;
			}
		} 
		while (false);

		return error;
	}

	uint32_t convert_utf8_to_unicode_string(std::wstring& unicode,const char* utf8,const size_t utf8_size)
	{
		uint32_t error = 0;
		do 
		{
			if ((nullptr == utf8) || (0 == utf8_size)) 
			{
				error = ERROR_INVALID_PARAMETER;
				break;
			}
			unicode.clear();
			int required_cch = ::MultiByteToWideChar(
				CP_UTF8,
				MB_ERR_INVALID_CHARS,
				utf8, static_cast<int>(utf8_size),
				nullptr, 0
			);
			if (required_cch==0) 
			{
				error = ::GetLastError();
				break;
			}
			unicode.resize(required_cch);
			if (0 == ::MultiByteToWideChar(
						CP_UTF8,
						MB_ERR_INVALID_CHARS,
						utf8, static_cast<int>(utf8_size),
						const_cast<wchar_t*>(unicode.c_str()), static_cast<int>(unicode.size())
						)) 
			{
				error=::GetLastError();
				break;
			}
		} 
		while (false);

		return error;
	}
// Windows: double converison
	std::string utf8toansi(const std::string & utf8)
	{
		std::wstring unicode = L"";
		convert_utf8_to_unicode_string(unicode, utf8.c_str(), utf8.size());
		std::string ansi = "";
		convert_unicode_to_ansi_string(ansi, unicode.c_str(), unicode.size());
		return ansi;
	}

#else

/// Unix & Linux

	std::string utf8toansi(const std::string & utf8)
	{
		return utf8;
	}

#endif

const std::string WHITESPACE = " \n\r\t\f\v";
 
std::string myltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}
 
std::string myrtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
 
std::string mytrim2(const std::string &s) {
    return myrtrim(myltrim(s));
}

string extractfilename(const string& i_string) 
{
	size_t i = i_string.rfind('/', i_string.length());
	if (i != string::npos) 
		return(i_string.substr(i+1, i_string.length() - i));
	return(i_string);
}
string prendiestensione(const string& s) 
{
	if (isdirectory(s))
		return ("");
	string nomefile=extractfilename(s);
	size_t i = nomefile.rfind('.', nomefile.length());
	if (i != string::npos) 
	{
		
		int lunghezzaestensione=nomefile.length() - i;
		if (lunghezzaestensione>20)
			return("");
		return(nomefile.substr(i+1, lunghezzaestensione));
   }
   return("");
}

string extractfilepath(const string& i_string) 
{
	size_t i = i_string.rfind('/', i_string.length());
	if (i != string::npos) 
		return(i_string.substr(0, i+1));
	return("");
}


string prendinomefileebasta(const string& s) 
{
	string nomefile=extractfilename(s);
	size_t i = nomefile.rfind('.', nomefile.length());
	if (i != string::npos) 
		return(nomefile.substr(0,i));
	return(nomefile);
}

string nomefileseesistegia(string i_nomefile)
{
	if (!fileexists(i_nomefile))
		return i_nomefile;
	string percorso=extractfilepath(i_nomefile);
	string estensione=prendiestensione(i_nomefile);
	string nomefile=prendinomefileebasta(i_nomefile);
	char	numero[10];
	for (int i=1;i<99999;i++)
	{
		sprintf(numero,"%05d",i);
		string snumero=numero;
		string candidato=percorso+nomefile+"_"+snumero+"."+estensione;
		if (!fileexists(candidato))
			return candidato;
	}
	return ("");
}

string padleft(std::string str, const size_t num, const char paddingChar = ' ')
{
    if(num > str.size())
	{
        string tmpstring=str;
		tmpstring.insert(0, num - tmpstring.size(), paddingChar);
		return tmpstring;
	}
	else
	return("");
}


string purgeansi(string i_string,bool i_keeppath=false)
{
	if (i_string=="")
		return ("");

	string purged;
	for (unsigned int i=0;i<i_string.length();i++)
	{
		if (i_keeppath)
		{
			if ((i_string[i]==':') || (i_string[i]=='/') || (i_string[i]=='\\'))
			{
				purged+=i_string[i];
				continue;
			}
		}
		if (isalnum(i_string[i]))
			purged+=i_string[i];
		else
		{
			switch (i_string[i]) 
			{
/*
very forbiden
< (less than)
> (greater than)
: (colon)
" (double quote)
/ (forward slash)
\ (backslash)
| (vertical bar or pipe)
? (question mark)
* (asterisk)
*/


				case ' ':
				case '-':
				case '#':
				case '~':
				case '%':
				case '^':
				case '_':
				case '.':
				case '+':
				case '=':
				purged+=i_string[i];
				break;

				case '&':
				purged+="_and_";
				break;

				case ',':
				case '`':
//				case '!':
				case '@':
				case '$':
				case '*':
	///			case '\\':
				case '|':
				case ':':
				case ';':
				case '"':
				case '\'':
				case '<':
				case '>':
	///			case '/':
				case '\n':
				case '\r':
				case '\t':
				purged+='_';
				break;

				case '(':
	///			case '[':
				case '{':
				purged+='(';
				break;

				case ')':
		///		case ']':
				case '}':
				purged+=')';
				break;
			}
		}
	}

	return purged;
}

string forcelatinansi(string i_string)
{
	if (i_string=="")
		return ("");

	for (unsigned int j=0;j<i_string.length();j++)
	{
		if (i_string[j]<0)
		{
			if (i_string[j]==-1) // °
			i_string[j]='u';
				else
				if (i_string[j]==-2) // °
					i_string[j]='u';
				else
				if (i_string[j]==-3) // °
					i_string[j]='u';
				else
				if (i_string[j]==-4) // °
					i_string[j]='u';
				else
				if (i_string[j]==-5) // °
					i_string[j]='u';
				else
				if (i_string[j]==-6) // °
					i_string[j]='u';
				else
				if (i_string[j]==-7) // °
					i_string[j]='u';
				else
				if (i_string[j]==-8) // °
					i_string[j]='o';
				else
				if (i_string[j]==-9) // °
					i_string[j]='o';
				else
				if (i_string[j]==-10) //ò
					i_string[j]='o';
				else
				if (i_string[j]==-11) //ò
					i_string[j]='o';
				else
				if (i_string[j]==-12) //ò
					i_string[j]='o';
				else
				if (i_string[j]==-13) //ò
					i_string[j]='o';
				else
				if (i_string[j]==-14) //ò
					i_string[j]='o';
				else
				if (i_string[j]==-15) //ò
					i_string[j]='o';
				else
				if (i_string[j]==-17) //ò
					i_string[j]='i';
				else
				if (i_string[j]==-18) //ò
					i_string[j]='i';
				else
				if (i_string[j]==-19) //ò
					i_string[j]='i';
				else
				if (i_string[j]==-20) //ò
					i_string[j]='i';
				else
				if (i_string[j]==-21) //è
					i_string[j]='e';
				else
				if (i_string[j]==-22) //è
					i_string[j]='e';
				else
				if (i_string[j]==-23) //è
					i_string[j]='e';
				else
				if (i_string[j]==-24) //è
					i_string[j]='e';
				else
				if (i_string[j]==-25) //è
					i_string[j]='a';
				else
				if (i_string[j]==-26) //è
					i_string[j]='a';
				else
				if (i_string[j]==-27) //a
					i_string[j]='a';
				else
				if (i_string[j]==-28) //a
					i_string[j]='a';
				else
				if (i_string[j]==-29) //a
					i_string[j]='a';
				else
				if (i_string[j]==-30) //a
					i_string[j]='e';
				else
				if (i_string[j]==-31) //a
					i_string[j]='a';
				else
				if (i_string[j]==-32) //a
					i_string[j]='a';
				else
				if (i_string[j]==-33) //a
					i_string[j]='u';
				else
				if (i_string[j]==-34) //a
					i_string[j]='u';
				else
				if (i_string[j]==-35) //a
					i_string[j]='u';
				else
				if (i_string[j]==-36) //a
					i_string[j]='u';
				else
				if (i_string[j]==-37) //a
					i_string[j]='u';
				else
				if (i_string[j]==-38) //a
					i_string[j]='u';
				else
				if (i_string[j]==-39) //a
					i_string[j]='U';
				else
				if (i_string[j]==-40) //a
					i_string[j]='O';
				else
				if (i_string[j]==-41) //a
					i_string[j]='x';
				else
				if (i_string[j]==-42) //ò
					i_string[j]='O';
				else
				if (i_string[j]==-45) //ò
					i_string[j]='O';
				else
				if (i_string[j]==-46) //ò
					i_string[j]='O';
				else
				if (i_string[j]==-47) //ò
					i_string[j]='N';
				else
				if (i_string[j]==-49) //ò
					i_string[j]='I';
				else
				if (i_string[j]==-50) //ò
					i_string[j]='I';
				else
				if (i_string[j]==-51) //ò
					i_string[j]='I';
				else
				if (i_string[j]==-52) //ò
					i_string[j]='I';
				else
				if (i_string[j]==-53) //ò
					i_string[j]='E';
				else
				if (i_string[j]==-54) //ò
					i_string[j]='E';
				else
				if (i_string[j]==-55) //ò
					i_string[j]='E';
				else
				if (i_string[j]==-56) //ò
					i_string[j]='E';
				else
				if (i_string[j]==-57) //ò
					i_string[j]='E';
				else		
				if (i_string[j]==-60) //ò
					i_string[j]='A';
				else
				if (i_string[j]==-61) //ò
					i_string[j]='A';
				else
				if (i_string[j]==-62) //ò
					i_string[j]='A';
				else
				if (i_string[j]==-63) //ò
					i_string[j]='A';
				else
				if (i_string[j]==-64) //ò
					i_string[j]='A';
				else
				if (i_string[j]==-66) //ò
					i_string[j]='3';
				else
				if (i_string[j]==-68) //ò
					i_string[j]='3';
				else
				if (i_string[j]==-69) //ò
					i_string[j]='>';
				else
				if (i_string[j]==-70) //ò
					i_string[j]='o';
				else
				if (i_string[j]==-72) //ò
					i_string[j]='.';
				else
				if (i_string[j]==-74) //ò
					i_string[j]=' ';
				else
				if (i_string[j]==-75) //ò
					i_string[j]=' ';
				else
				if (i_string[j]==-76) //ò
					i_string[j]=' ';
				else
				if (i_string[j]==-78) //ò
					i_string[j]='2';
				else
				if (i_string[j]==-80) // °
					i_string[j]='o';
				else
				if (i_string[j]==-81) // °
					i_string[j]='-';
				else
				if (i_string[j]==-82) // °
					i_string[j]='-';
				else
				if (i_string[j]==-83) // °
					i_string[j]='-';
				else
				if (i_string[j]==-84) // °
					i_string[j]='-';
				else
				if (i_string[j]==-85) // °
					i_string[j]='<';
				else
				if (i_string[j]==-89) // °
					i_string[j]='S';
				else
				if (i_string[j]==-90) // °
					i_string[j]=' ';
				else
				if (i_string[j]==-93) // °
					i_string[j]=' ';
				else
				if (i_string[j]==-94) // °
					i_string[j]=' ';
				else
				if (i_string[j]==-95) // °
					i_string[j]=' ';
				else
				if (i_string[j]==-96) // °
					i_string[j]=' ';
				else
				if (i_string[j]==-98) // °
					i_string[j]=' ';
				else
				if (i_string[j]==-102) //-
					i_string[j]='s';
				else
				if (i_string[j]==-103) //-
					i_string[j]='a';
				else
				if (i_string[j]==-106) //-
					i_string[j]='-';
				else
				if (i_string[j]==-107) //-
					i_string[j]='.';
				else
				if (i_string[j]==-108) //-
					i_string[j]='"';
				else
				if (i_string[j]==-109) //-
					i_string[j]='"';
				else
				if (i_string[j]==-110) //-
					i_string[j]=' ';
				else
				if (i_string[j]==-111) //-
					i_string[j]=' ';
				else
				if (i_string[j]==-112) //-
					i_string[j]=' ';
				else
				if (i_string[j]==-112) //-
					i_string[j]=' ';
				else
				if (i_string[j]==-113) //-
					i_string[j]=' ';
				else
				if (i_string[j]==-114) //-
					i_string[j]=' ';
				else
				if (i_string[j]==-115)//-
					i_string[j]=' ';
				else
				if (i_string[j]==-116) //-
					i_string[j]=' ';
				else
				if (i_string[j]==-118) //-
					i_string[j]='S';
				else
				if (i_string[j]==-123) //-
					i_string[j]='d';
				else
				if (i_string[j]==-128) //-
					i_string[j]='E';
				else
					i_string[j]=' ';
			
			
		}
	}
		
	return i_string;
}

string purgedouble(const string& i_string,const string& i_from,const string& i_to)
{
	if (i_string=="")
		return("");
	if (i_from=="")
		return("");
	if (i_to=="")
		return("");
	string purged=i_string;
	myreplaceall(purged,i_from,i_to);
		
	return purged;
}

string compressemlfilename(const string& i_string)
{
	if (i_string=="")
		return("");
	string uniqfilename=extractfilename(i_string);
	string percorso=extractfilepath(i_string);
		
	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"  "," ");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"..",".");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"Fw ","Fwd ");
	
	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"Fwd Fwd ","Fwd ");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename," R "," Re ");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"R Fwd ","Re Fwd");
	
	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename," RE "," Re ");
	
	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"Re Re ","Re ");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"Fwd Re Fwd Re ","Fwd Re ");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"Re Fwd Re Fwd ","Re Fwd ");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename," SV SV "," SV ");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"Fwd FW ","Fwd ");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"Fwd I ","Fwd ");
	
	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"I Fwd ","Fwd ");

	for (int k=0;k<10;k++)
			uniqfilename=purgedouble(uniqfilename,"R Re ","Re ");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"__","_");

	for (int k=0;k<10;k++)
	uniqfilename=purgedouble(uniqfilename," _ ","_");

	for (int k=0;k<10;k++)
		uniqfilename=purgedouble(uniqfilename,"  "," ");

	for (int k=uniqfilename.length()-1;k>0;k--)
	{
		if ((uniqfilename[k]=='-') || (uniqfilename[k]=='.') || (uniqfilename[k]==' '))
		{
			uniqfilename.pop_back();
		}
		else
		{
			break;
		}
	}
	uniqfilename=mytrim2(uniqfilename);
	
	uniqfilename=percorso+uniqfilename;
	
	return uniqfilename;
}

int64_t prendidimensionefile(const char* i_filename)
{
	if (!i_filename)
		return 0;
	FILE* myfile = freadopen(i_filename);
	if (myfile)
    {
		fseeko(myfile, 0, SEEK_END);
		int64_t dimensione=ftello(myfile);
		fclose(myfile);
		return dimensione;
	}
	else
	return 0;
}

// Rename name using tofiles[]
string Jidac::rename(string name) 
{
	if (flagnopath)
	{
		string myname=name;
		for (unsigned i=0; i<files.size(); ++i) 
		{
			string pathwithbar=files[i];
			if (pathwithbar[pathwithbar.size()-1]!='/')
				pathwithbar+="/";

			  if (strstr(name.c_str(), pathwithbar.c_str())!=0)
			  {
				myreplace(myname,pathwithbar,"");
				if (flagverbose)
					printf("Cutting path <<%s>> to <<%s>>\n",files[i].c_str(),myname.c_str());
				return myname;
			  }

		}
		return myname;
	}
	
  if (files.size()==0 && tofiles.size()>0)  // append prefix tofiles[0]
    name=append_path(tofiles[0], name);
  else {  // replace prefix files[i] with tofiles[i]
    const int n=name.size();
    for (unsigned i=0; i<files.size() && i<tofiles.size(); ++i) {
      const int fn=files[i].size();
      if (fn<=n && files[i]==name.substr(0, fn))
///        return tofiles[i]+"/"+name.substr(fn);
        return tofiles[i]+name.substr(fn);
    }
  }
  return name;
}

//// some support functions to string compare case insensitive
bool comparechar(char c1, char c2)
{
    if (c1 == c2)
        return true;
    else if (std::toupper(c1) == std::toupper(c2))
        return true;
    return false;
}

bool stringcomparei(std::string str1, std::string str2)
{
    return ( (str1.size() == str2.size() ) &&
             std::equal(str1.begin(), str1.end(), str2.begin(), &comparechar) );
}
inline char *  migliaia(uint64_t n)
{
	static char retbuf[30];
	char *p = &retbuf[sizeof(retbuf)-1];
	unsigned int i = 0;
	*p = '\0';
	do 
	{
		if(i%3 == 0 && i != 0)
			*--p = '.';
		*--p = '0' + n % 10;
		n /= 10;
		i++;
		} while(n != 0);
	return p;
}
inline char *  migliaia2(uint64_t n)
{
	static char retbuf[30];
	char *p = &retbuf[sizeof(retbuf)-1];
	unsigned int i = 0;
	*p = '\0';
	do 
	{
		if(i%3 == 0 && i != 0)
			*--p = '.';
		*--p = '0' + n % 10;
		n /= 10;
		i++;
		} while(n != 0);
	return p;
}
inline char *  migliaia3(uint64_t n)
{
	static char retbuf[30];
	char *p = &retbuf[sizeof(retbuf)-1];
	unsigned int i = 0;
	*p = '\0';
	do 
	{
		if(i%3 == 0 && i != 0)
			*--p = '.';
		*--p = '0' + n % 10;
		n /= 10;
		i++;
		} while(n != 0);
	return p;
}
inline char *  migliaia4(uint64_t n)
{
	static char retbuf[30];
	char *p = &retbuf[sizeof(retbuf)-1];
	unsigned int i = 0;
	*p = '\0';
	do 
	{
		if(i%3 == 0 && i != 0)
			*--p = '.';
		*--p = '0' + n % 10;
		n /= 10;
		i++;
		} while(n != 0);
	return p;
}
inline char *  migliaia5(uint64_t n)
{
	static char retbuf[30];
	char *p = &retbuf[sizeof(retbuf)-1];
	unsigned int i = 0;
	*p = '\0';
	do 
	{
		if(i%3 == 0 && i != 0)
			*--p = '.';
		*--p = '0' + n % 10;
		n /= 10;
		i++;
		} while(n != 0);
	return p;
}
char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}
inline char* tohuman(uint64_t i_bytes)
{
	static char io_buf[30];
	
	char const *myappend[] = {"B","KB","MB","GB","TB","PB"};
	char length = sizeof(myappend)/sizeof(myappend[0]);
	double mybytes=i_bytes;
	int i=0;
	if (i_bytes > 1024) 
		for (i=0;(i_bytes / 1024) > 0 && i<length-1; i++, i_bytes /= 1024)
			mybytes = i_bytes / 1024.0;
	sprintf(io_buf, "%.02lf %s",mybytes,myappend[i]);
	return io_buf;
}
inline char* tohuman2(uint64_t i_bytes)
{
	static char io_buf[30];
	
	char const *myappend[] = {"B","KB","MB","GB","TB","PB"};
	char length = sizeof(myappend)/sizeof(myappend[0]);
	double mybytes=i_bytes;
	int i=0;
	if (i_bytes > 1024) 
		for (i=0;(i_bytes / 1024) > 0 && i<length-1; i++, i_bytes /= 1024)
			mybytes = i_bytes / 1024.0;
	sprintf(io_buf, "%.02lf %s",mybytes,myappend[i]);
	return io_buf;
}
inline char* tohuman3(uint64_t i_bytes)
{
	static char io_buf[30];
	
	char const *myappend[] = {"B","KB","MB","GB","TB","PB"};
	char length = sizeof(myappend)/sizeof(myappend[0]);
	double mybytes=i_bytes;
	int i=0;
	if (i_bytes > 1024) 
		for (i=0;(i_bytes / 1024) > 0 && i<length-1; i++, i_bytes /= 1024)
			mybytes = i_bytes / 1024.0;
	sprintf(io_buf, "%.02lf %s",mybytes,myappend[i]);
	return io_buf;
}
inline char* tohuman4(uint64_t i_bytes)
{
	static char io_buf[30];
	
	char const *myappend[] = {"B","KB","MB","GB","TB","PB"};
	char length = sizeof(myappend)/sizeof(myappend[0]);
	double mybytes=i_bytes;
	int i=0;
	if (i_bytes > 1024) 
		for (i=0;(i_bytes / 1024) > 0 && i<length-1; i++, i_bytes /= 1024)
			mybytes = i_bytes / 1024.0;
	sprintf(io_buf, "%.02lf %s",mybytes,myappend[i]);
	return io_buf;
}


int64_t encodestringdate(string i_date)
{
	string purged;
	for (unsigned int i=0;i<i_date.length();i++)
		if (isdigit(i_date[i]))
			purged+=i_date[i];
	
	if (purged.length()!=14)
	{
		printf("106: datelength !=14\n");
		return -1;
	}
	for (int i=0;i<=13;i++)
		if (!isdigit(purged[i]))
		{
			printf("107: date[%d] not idigit\n",i);
			return -1;
		}
		
	int year=std::stoi(purged.substr(0,4));
	int month=std::stoi(purged.substr(4,2));
	int day=std::stoi(purged.substr(6,2));
	
	int hour=std::stoi(purged.substr(8,2));
	int minute=std::stoi(purged.substr(10,2));
	int second=std::stoi(purged.substr(12,2));
	if (flagdebug)
		printf("14669: date   %04d-%02d-%02d %02d:%02d:%02d\n",year,month,day,hour,minute,second);
	if ((year<1970) || (year>2070))
	{
		printf("108: year not from 1970 to 2070\n");
		return -1;
	}
	if ((month<1) || (month>12))
	{
		printf("136: month not in 1 to 12\n");
		return -1;
	}
	if ((day<1) || (day>31))
	{
		printf("141: day not in 1 to 31\n");
		return -1;
	}
	
	if (hour>24)
	{
		printf("147: hour >24\n");
		return -1;
	}
	if (minute>60)
	{
		printf("152: minute >60\n");
		return -1;
	}	
	if (second>60)
	{
		printf("157: second >60\n");
		return -1;
	}	
		
	bool isleap= (((year % 4 == 0) &&
         (year % 100 != 0)) ||
         (year % 400 == 0));
		

    if (month == 2)
    {
        if (isleap)
		{
			if (!(day <=29))
			{
				printf("180: leap year, feb must be <=29\n");
				return -1;
			}
		}
        else
			if (!(day <=28))
			{
				printf("187: NO leap year, feb must be <=28\n");
				return -1;
			}
    }
 
    if ((month==4) || (month==6) || (month==9) || (month==11))
		if (!(day <= 30))
		{
			printf("195: month cannot have more than 30 days\n");
			return -1;
		}
	return year*10000000000LL
		+month*100000000LL //mese
		+day*1000000				// giorno
	  
		+hour*10000 // ore
		+minute*100	// minuti
		+second;		// secondi
}



// Parse the command line. Return 1 if error else 0.
int Jidac::doCommand(int argc, const char** argv) 
{
  // Initialize options to default values
  // Why some variables out of Jidac? Because of pthread: does not like very much C++ objects
  // quick and dirty.
  
	g_exec_error="";
	g_exec_ok="";
	g_exec_text="";
	command=0;
	flagforce=false;
	fragment=6;
	minsize=0;
	maxsize=0;
	dirlength=0;
	filelength=0;
	all=0;
	password=0;  // no password
	index=0;
	method="";  // 0..5
	flagnoattributes=false;
	repack=0;
	new_password=0;
	summary=-1; 
	menoenne=0;
	versioncomment="";
	flagdebug=false;
	flagtest=false;
	flagskipzfs=false; 
	flagverbose=false;
	flagnoqnap=false;
	flagforcewindows=false;
	flagnopath=false;
	flagnoeta=false;
	flagpakka=false;
	flagvss=false;
	flagnosort=false;
	flagchecksum=false;
	flagcrc32c=false;
	flagverify=false;
	flagkill=false;
	flagutf=false;
	flagfix255=false;
	flagfixeml=false;
	flagflat=false;
	flagxxhash=false;
	flagcrc32=false;
	flagsha256=false;
	flagwyhash=false;
	flagdonotforcexls=false;
	flagcomment=false;
	flag715=false;
	searchfrom="";
	replaceto="";
 
	howmanythreads=0; // 0 = auto-detect
	version=DEFAULT_VERSION;
	date=0;
 
  /// if we have a -pakka, set the flag early
	for (int i=0; i<argc; i++)
	{
		const string parametro=argv[i];
		if (stringcomparei(parametro,"-pakka"))
		{
				flagpakka=true;
				break;
		}
	}
  
	if (!flagpakka)
		moreprint("zpaqfranz v" ZPAQ_VERSION " snapshot archiver, compiled " __DATE__);
	
/// check some magic to show help in heuristic way
/// I know, it's bad, but help is always needed!

	if (argc==1) // zero parameters
	{
		usage();
		exit(0);
	}

	if (argc==2) // only ONE 
	{
		const string parametro=argv[1];
		if  ((stringcomparei(parametro,"help"))
			||
			(stringcomparei(parametro,"-h"))
			||
			(stringcomparei(parametro,"?"))
			||
			(stringcomparei(parametro,"-?"))
			||
			(stringcomparei(parametro,"--help"))
			||
			(stringcomparei(parametro,"-help")))
			{
				usagefull();
				exit(0);
			}
		if ((stringcomparei(parametro,"-he"))
			||
			(stringcomparei(parametro,"-helpe"))
			||
			(stringcomparei(parametro,"-example"))
			||
			(stringcomparei(parametro,"-examples")))
			{
				examples();
				exit(0);
			}
		if (stringcomparei(parametro,"-diff"))
		{
			differences();
			exit(0);
		}
	}

	
  // Init archive state
	ht.resize(1);  // element 0 not used
	ver.resize(1); // version 0
	dhsize=dcsize=0;
	
	// Get date
	time_t now=time(NULL);
	tm* t=gmtime(&now);
	date=(t->tm_year+1900)*10000000000LL+(t->tm_mon+1)*100000000LL
      +t->tm_mday*1000000+t->tm_hour*10000+t->tm_min*100+t->tm_sec;

	// Get optional options
	for (int i=1; i<argc; ++i) 
	{
		const string opt=argv[i];  // read command
		if ((
		opt=="add" || 
		opt=="extract" || 
		opt=="list" || 
		opt=="k" || 
		opt=="a"  || 
		opt=="x" || 
		opt=="p" || 
		opt=="t" ||
		opt=="test" || 
		opt=="l" || 
		opt=="i" || 
		opt=="m")
        && i<argc-1 && argv[i+1][0]!='-' && command==0) 
		{
			command=opt[0];
			if (opt=="extract") 
				command='x';
			if (opt=="test") 
				command='t';
			
			archive=argv[++i];  // append ".zpaq" to archive if no extension
			const char* slash=strrchr(argv[i], '/');
			const char* dot=strrchr(slash ? slash : argv[i], '.');
			if (!dot && archive!="") 
				archive+=".zpaq";
			while (++i<argc && argv[i][0]!='-')  // read filename args
				files.push_back(argv[i]);
			--i;
		}
		else if (opt=="sha1")
		{
			command=opt[3]; /// note!
			while (++i<argc && argv[i][0]!='-')  // read filename args
				files.push_back(argv[i]);
			i--;
		}
		else if (opt=="d")
		{
			command='d';
			while (++i<argc && argv[i][0]!='-')  // read filename args
				files.push_back(argv[i]);
			i--;
		}
		else if (opt=="dir")
		{
			command='b'; // YES, B
			while (++i<argc && argv[i][0]!='-')  // read filename args
				files.push_back(argv[i]);
			i--;
		}
		else if (
		(opt=="f") || 
		(opt=="c") || 
		(opt=="s") || 
		(opt=="utf") || 
		(opt=="r") || 
		(opt=="robocopy") || 
		opt=="z")
		{
			command=opt[0];
			while (++i<argc && argv[i][0]!='-')
			{
	//	a very ugly fix for last \ in Windows
				string candidate=argv[i];
				cutdoublequote(candidate);
				if (havedoublequote(candidate))
				{
					printf("WARNING: double quote founded in command line and cutted\n");
					candidate.pop_back();
					printf("||%s||\n",candidate.c_str());
				}
				files.push_back(candidate);
			}
			i--;
		}

		else if (opt.size()<2 || opt[0]!='-') usage();
		else 
		if (opt=="-all") 
		{
			all=4;
			if (i<argc-1 && isdigit(argv[i+1][0])) 	all				=atoi(argv[++i]);
		}
		else if (opt=="-fragment" 	&& i<argc-1)	fragment		=atoi(argv[++i]);
		else if (opt=="-minsize" 	&& i<argc-1) 	minsize			=atoi(argv[++i]);
		else if (opt=="-maxsize" 	&& i<argc-1) 	maxsize			=atoi(argv[++i]);
		else if (opt=="-filelength" && i<argc-1) 	filelength		=atoi(argv[++i]);
		else if (opt=="-dirlength" 	&& i<argc-1) 	dirlength		=atoi(argv[++i]);
		else if (opt=="-n" 			&& i<argc-1) 	menoenne		=atoi(argv[++i]);
		else if (opt=="-limit" 		&& i<argc-1) 	menoenne		=atoi(argv[++i]);
		else if (opt=="-index" 		&& i<argc-1) 	index			=argv[++i];
		else if (opt=="-method" 	&& i<argc-1) 	method			=argv[++i];
		else if (opt[1]=='m') 						method			=argv[i]+2;
		else if (opt[1]=='s') 						summary			=atoi(argv[i]+2);
		else if (opt[1]=='t') 						howmanythreads	=atoi(argv[i]+2);
		else if (opt=="-threads" && i<argc-1) 		howmanythreads	=atoi(argv[++i]);
		
		else if (opt=="-summary") // retained for backwards compatibility
		{
			summary=1;
			if (i<argc-1) 
				summary=atoi(argv[++i]);
		}		
		else if (opt=="-find")
		{
			if (searchfrom=="" && strlen(argv[i+1])>=1)
			{
				searchfrom=argv[i+1];
				i++;
			}
		}
		else if (opt=="-replace")
		{
			if (replaceto=="" && strlen(argv[i+1])>=1)
			{
				replaceto=argv[i+1];
				i++;
			}
		}
		else if (opt=="-exec_error")
		{
			if (g_exec_error=="")
			{
				if (++i<argc && argv[i][0]!='-')  
					g_exec_error=argv[i];
				else
					i--;
			}
		}
		else if (opt=="-exec_ok")
		{
			if (g_exec_ok=="")
			{
				if (++i<argc && argv[i][0]!='-')  
					g_exec_ok=argv[i];
				else
					i--;
			}
		}
		else if (opt=="-comment")
		{
			flagcomment=true;
			if (++i<argc && argv[i][0]!='-')  
				versioncomment=argv[i];
			else
				i--;
		}
		else if (opt=="-timestamp") 	// force the timestamp: zfs freezing
		{
			if (i+1<argc)
				if (strlen(argv[i+1])>=1)
				{
					string mytimestamp=argv[i+1];
					i++;
					int64_t newdate=encodestringdate(mytimestamp);
					if (newdate!=-1)
					{
						printf("-timestamp change from %s => %s\n",dateToString(date).c_str(),dateToString(newdate).c_str());
						date=newdate;
					}
				}
		}
		
		else if (opt=="-force" || opt=="-f") 		flagforce			=true;
		else if (opt=="-noattributes") 				flagnoattributes	=true;
		else if (opt=="-test") 						flagtest			=true;
		else if (opt=="-zfs") 						flagskipzfs			=true;
		else if (opt=="-forcezfs") 					flagforcezfs		=true;
		else if (opt=="-715") 						flag715				=true;
		else if (opt=="-xls") 						flagdonotforcexls	=true;
		else if (opt=="-verbose") 					flagverbose			=true;
		else if (opt=="-debug") 					flagdebug			=true;
		else if (opt=="-noqnap") 					flagnoqnap			=true;
		else if (opt=="-nopath") 					flagnopath			=true;
		else if (opt=="-nosort") 					flagnosort			=true;
		else if (opt=="-checksum") 					flagchecksum		=true;
		else if (opt=="-crc32c") 					flagcrc32c			=true;
		else if (opt=="-xxhash") 					flagxxhash			=true;
		else if (opt=="-sha256") 					flagsha256			=true;
		else if (opt=="-wyhash") 					flagwyhash			=true;
		else if (opt=="-crc32") 					flagcrc32			=true;
		else if (opt=="-verify") 					flagverify			=true;
		else if (opt=="-kill") 						flagkill			=true;
		else if (opt=="-utf") 						flagutf				=true;
		else if (opt=="-utf8") 						flagutf				=true;//alias
		else if (opt=="-fix255") 					flagfix255			=true;
		else if (opt=="-fixeml") 					flagfixeml			=true;
		else if (opt=="-flat") 						flagflat			=true;
		else if (opt=="-noeta") 					flagnoeta			=true;
		else if (opt=="-pakka") 					flagpakka			=true;  // we already have pakka flag, but this is for remember
		else if (opt=="-vss") 						flagvss				=true;
		else if (opt=="-forcewindows") 				flagforcewindows	=true;
		else if (opt=="-not") 
		{  // read notfiles
			while (++i<argc && argv[i][0]!='-') 
			{
				if (argv[i][0]=='=')
					nottype=argv[i];
				else
					notfiles.push_back(argv[i]);
			}
			--i;
		}
		else if (opt=="-only") 
		{  // read onlyfiles
			while (++i<argc && argv[i][0]!='-')
				onlyfiles.push_back(argv[i]);
			--i;
		}
		else if (opt=="-always") 
		{  // read alwaysfiles
			while (++i<argc && argv[i][0]!='-')
				alwaysfiles.push_back(argv[i]);
			--i;
		}
		else if (opt=="-key" && i<argc-1) 
		{
			libzpaq::SHA256 sha256;
			for (const char* p=argv[++i]; *p; ++p)
				sha256.put(*p);
			memcpy(password_string, sha256.result(), 32);
			password=password_string;
		}
		else
		if (opt=="-repack" && i<argc-1) 
		{
			repack=argv[++i];
			if (i<argc-1 && argv[i+1][0]!='-') 
			{
				libzpaq::SHA256 sha256;
				for (const char* p=argv[++i]; *p; ++p) 
					sha256.put(*p);
				memcpy(new_password_string, sha256.result(), 32);
				new_password=new_password_string;
			}
		}
		else if (opt=="-to") 
		{  // read tofiles
			while (++i<argc && argv[i][0]!='-')
			{
	// fix to (on windows) -to "z:\pippo pluto" going to a mess
				string mytemp=mytrim(argv[i]);
				myreplaceall(mytemp,"\"","");
				tofiles.push_back(mytemp);
			}
			if (tofiles.size()==0)
				tofiles.push_back("");
			--i;
		}
		else 
		if (opt=="-until" && i+1<argc) 
		{  // read date

		  // Read digits from multiple args and fill in leading zeros
			version=0;
			int digits=0;
			if (argv[i+1][0]=='-')
			{  // negative version
				version=atol(argv[i+1]);
				if (version>-1) usage();
				++i;
			}
			else 
			{  // positive version or date
				while (++i<argc && argv[i][0]!='-') 
				{
					for (int j=0; ; ++j) 
					{
						if (isdigit(argv[i][j])) 
						{
							version=version*10+argv[i][j]-'0';
							++digits;
						}
						else 
						{
							if (digits==1)
								version=version/10*100+version%10;
							digits=0;
							if (argv[i][j]==0)
								break;
						}
					}
				}
				--i;
			}

		  // Append default time
			if (version>=19000000LL     && version<=29991231LL)
				version=version*100+23;
			if (version>=1900000000LL   && version<=2999123123LL)
				version=version*100+59;
			if (version>=190000000000LL && version<=299912312359LL)
				version=version*100+59;
			if (version>9999999) 
			{
				if (version<19000101000000LL || version>29991231235959LL) 
				{
					fflush(stdout);
					fprintf(stderr,"Version date %1.0f must be 19000101000000 to 29991231235959\n",double(version));
					g_exec_text="Version date must be 19000101000000 to 29991231235959";
					exit(1);
				}
				date=version;
			}
		}
		else 
		{
		printf("Unknown option ignored: %s\n", argv[i]);
		}
	}
  

//	write franzs' parameters (if not very briefly)
	if (!flagpakka)
	{
		string franzparameters="";
			
		if (flagforcezfs)
		{
			franzparameters+="force ZFS on (-forcezfs) ";
			flagskipzfs=false; // win over skip
		}
		if (flagskipzfs)
			franzparameters+="SKIP ZFS on (-zfs) ";
		
		if (flag715)
			franzparameters+="mode 715 activated (-715) " ;
		
		if (flagnoqnap)
			franzparameters+="Exclude QNAP snap & trash (-noqnap) ";

		if (flagforcewindows)
			franzparameters+="Force Windows stuff (-forcewindows) ";

		if (flagnopath)
			franzparameters+="Do not store path (-nopath) ";

		if (flagnosort)
			franzparameters+="Do not sort (-nosort) ";

		if (flagdonotforcexls)
			franzparameters+="Do not force XLS (-xls) ";
		
		if (flagverbose)
			franzparameters+="VERBOSE on (-verbose) ";
		
		if (flagchecksum)
			franzparameters+="checksumming (-checksum) ";
		
		if (flagcrc32c)
			franzparameters+="CRC-32C (-crc32c) ";
		
		if (flagcrc32)
			franzparameters+="CRC-32 (-crc32) ";
				
		if (flagxxhash)
			franzparameters+="XXH3 (-xxhash) ";
		
		if (flagsha256)
			franzparameters+="SHA256 (-sha256) ";
				/*
		if (flagwyhash)
				printf("franz:wyhash\n");
		*/
		if (flagverify)
			franzparameters+="VERIFY (-verify) ";
				
		if (flagkill)
			franzparameters+="do a wet run! (-kill) ";
				
		if (flagutf)
			franzparameters+="flagutf (-utf / -utf8) ";
		
		if (flagfix255)
			franzparameters+="fix255 (-fix255) ";
				
		if (flagfixeml)
			franzparameters+="Fix eml filenames (-fixeml) ";
		
		if (flagflat)
			franzparameters+="Flat filenames (-flat) ";
				
		if (flagdebug)
			franzparameters+="DEBUG very verbose (-debug) ";

		if (flagcomment)
			franzparameters+="Use comment (-comment) ";

	#if defined(_WIN32) || defined(_WIN64)
		if (flagvss)
		{
			franzparameters+=" VSS Volume Shadow Copy (-vss) ";
			if (!isadmin())
			{
				printf("\nImpossible to run VSS: admin rights required\n");
				return 2;
			}	
		}
	#endif	
		if (franzparameters!="")
			printf("franz:%s\n",franzparameters.c_str());

///	extended (switch with options)

		if (searchfrom!="")
			printf("franz:find       <<%s>>\n",searchfrom.c_str());

		if (replaceto!="")
			printf("franz:replace    <<%s>>\n",replaceto.c_str());
		
		if (g_exec_error!="")
			printf("franz:exec_error <<%s>>\n",g_exec_error.c_str());
		
		if (g_exec_ok!="")
			printf("franz:exec_ok    <<%s>>\n",g_exec_ok.c_str());
		
		if (minsize)
			printf("franz:minsize    %s\n",migliaia(minsize));
		
		if (maxsize)
			printf("franz:maxsize    %s\n",migliaia(maxsize));
		
		if (dirlength)
			printf("franz:dirlength  %s\n",migliaia(dirlength));
		
		if (filelength)
			printf("franz:filelength %s\n",migliaia(filelength));
		
		if (summary>0)
			printf("franz:summary    %d\n",summary);
		
		if (menoenne)
			printf("franz:-n -limit  %u\n",menoenne);
	}
	
	if (howmanythreads<1) 
		howmanythreads=numberOfProcessors();

#ifdef _OPENMP
  omp_set_dynamic(howmanythreads);
#endif

  // Test date
	if (now==-1 || date<19000000000000LL || date>30000000000000LL)
		error("date is incorrect, use -until YYYY-MM-DD HH:MM:SS to set");

  // Adjust negative version
	if (version<0) 
	{
		Jidac jidac(*this);
		jidac.version=DEFAULT_VERSION;
		jidac.read_archive(archive.c_str());
		version+=jidac.ver.size()-1;
		printf("Version %1.0f\n", version+.0);
	}

// Load dynamic functions in Windows Vista and later
#ifndef unix
	HMODULE h=GetModuleHandle(TEXT("kernel32.dll"));
	if (h==NULL) 
		printerr("14717","GetModuleHandle");
	else 
	{
		findFirstStreamW=(FindFirstStreamW_t)GetProcAddress(h, "FindFirstStreamW");
		findNextStreamW=(FindNextStreamW_t)GetProcAddress(h, "FindNextStreamW");
	}
///  if (!findFirstStreamW || !findNextStreamW)
///broken XP parser    printf("Alternate streams not supported in Windows XP.\n");
#endif

	if (flag715)
	{
		printf("**** Activated V7.15 mode ****\n");
		flagforcewindows	=true;
		flagcrc32			=false;
		flagchecksum		=false;
		flagforcezfs		=true;
		flagdonotforcexls	=true;
	}

  // Execute command
	if (command=='a' && files.size()>0) // enforce: we do not want to change anything when adding
	{
		flagfixeml			=false;
		flagfix255			=false;
		flagutf				=false;
		flagflat			=false;
		return add();
	}
	else if (command=='1') return summa();
	else if (command=='b') return dir();
	else if (command=='c') return dircompare(false,false);
	else if (command=='d') return deduplicate();
	else if (command=='f') return fillami();
	else if (command=='i') return info();
	else if (command=='k') return kill();
	else if (command=='l') return list();
	else if (command=='m') return consolidate(archive);
	else if (command=='p') return paranoid();
	else if (command=='r') return robocopy();
	else if (command=='s') return dircompare(true,false); 
	else if (command=='t') return test();
	else if (command=='x') return extract();
	else if (command=='u') return utf();
	else if (command=='z') return zero();
	else usage();
	return 0;
}




/////////////////////////// read_archive //////////////////////////////

// Read arc up to -date into ht, dt, ver. Return place to
// append. If errors is not NULL then set it to number of errors found.
int64_t Jidac::read_archive(const char* arc, int *errors, int i_myappend) {
  if (errors) *errors=0;
  dcsize=dhsize=0;
  assert(ver.size()==1);
  unsigned files=0;  // count
///printf("ZA1\n");

  // Open archive
  InputArchive in(arc, password);
///printf("ZA2\n");

  if (!in.isopen()) {
    if (command!='a') {
      fflush(stdout);
      printUTF8(arc, stderr);
      fprintf(stderr, " not found.\n");
	  g_exec_text=arc;
	  g_exec_text+=" not found";
      if (errors) ++*errors;
    }
    return 0;
  }
  if (!flagpakka)
  {
  printUTF8(arc);
  if (version==DEFAULT_VERSION) printf(": ");
  else printf(" -until %1.0f: ", version+0.0);
  fflush(stdout);
  }
  // Test password
  {
    char s[4]={0};
    const int nr=in.read(s, 4);
    if (nr>0 && memcmp(s, "7kSt", 4) && (memcmp(s, "zPQ", 3) || s[3]<1))
	{
      printf("zpaqfranz error:password incorrect\n");
	  error("password incorrect");
	}
    in.seek(-nr, SEEK_CUR);
  }
///printf("ZA3\n");

  // Scan archive contents
  string lastfile=archive; // last named file in streaming format
  if (lastfile.size()>5 && lastfile.substr(lastfile.size()-5)==".zpaq")
    lastfile=lastfile.substr(0, lastfile.size()-5); // drop .zpaq
  int64_t block_offset=32*(password!=0);  // start of last block of any type
  int64_t data_offset=block_offset;    // start of last block of d fragments
  bool found_data=false;   // exit if nothing found
  bool first=true;         // first segment in archive?
  StringBuffer os(32832);  // decompressed block
  const bool renamed=command=='l' || command=='a';

	int toolongfilenames=0;
	int adsfilenames=0;
	int utf8names=0;
	int casecollision=0;
	/*
	struct insensitivecompare { 
    bool operator() (const std::string& a, const std::string& b) const {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};


	std::set<std::string, insensitivecompare> collisionset;
	*/
	///std::set<std::string> collisionset;
	uint64_t parts=0;
  // Detect archive format and read the filenames, fragment sizes,
  // and hashes. In JIDAC format, these are in the index blocks, allowing
  // data to be skipped. Otherwise the whole archive is scanned to get
  // this information from the segment headers and trailers.
  bool done=false;
  printf("\n");
  uint64_t startblock=mtime();
  while (!done) {
    libzpaq::Decompresser d;
	///printf("New iteration\r");
    try {
      d.setInput(&in);
      double mem=0;
      while (d.findBlock(&mem)) {
        found_data=true;

        // Read the segments in the current block
        StringWriter filename, comment;
        int segs=0;  // segments in block
        bool skip=false;  // skip decompression?
        while (d.findFilename(&filename)) {
          if (filename.s.size()) {
            for (unsigned i=0; i<filename.s.size(); ++i)
              if (filename.s[i]=='\\') filename.s[i]='/';
            lastfile=filename.s.c_str();
          }
          comment.s="";
          d.readComment(&comment);

          // Test for JIDAC format. Filename is jDC<fdate>[cdhi]<num>
          // and comment ends with " jDC\x01". Skip d (data) blocks.
          if (comment.s.size()>=4
              && comment.s.substr(comment.s.size()-4)=="jDC\x01") {
            if (filename.s.size()!=28 || filename.s.substr(0, 3)!="jDC")
              error("bad journaling block name");
            if (skip) error("mixed journaling and streaming block");

            // Read uncompressed size from comment
            int64_t usize=0;
            unsigned i;
            for (i=0; i<comment.s.size() && isdigit(comment.s[i]); ++i) {
              usize=usize*10+comment.s[i]-'0';
              if (usize>0xffffffff) error("journaling block too big");
            }

            // Read the date and number in the filename
            int64_t fdate=0, num=0;
            for (i=3; i<17 && isdigit(filename.s[i]); ++i)
              fdate=fdate*10+filename.s[i]-'0';
            if (i!=17 || fdate<19000000000000LL || fdate>=30000000000000LL)
              error("bad date");
            for (i=18; i<28 && isdigit(filename.s[i]); ++i)
              num=num*10+filename.s[i]-'0';
            if (i!=28 || num>0xffffffff) error("bad fragment");


            // Decompress the block.
            os.resize(0);
            os.setLimit(usize);
            d.setOutput(&os);
            libzpaq::SHA1 sha1;
            d.setSHA1(&sha1);
            if (strchr("chi", filename.s[17])) {
              if (mem>1.5e9) error("index block requires too much memory");
              d.decompress();
              char sha1result[21]={0};
              d.readSegmentEnd(sha1result);
              if ((int64_t)os.size()!=usize) error("bad block size");
              if (usize!=int64_t(sha1.usize())) error("bad checksum size");
              if (sha1result[0] && memcmp(sha1result+1, sha1.result(), 20))
                error("bad checksum");
			if ( ++parts % 1000 ==0)
				if (!flagnoeta)
				printf("Block %10s K %12s (block/s)\r",migliaia(parts/1000),migliaia2(parts/((mtime()-startblock+1)/1000.0)));
            }
            else
              d.readSegmentEnd();

            // Transaction header (type c).
            // If in the future then stop here, else read 8 byte data size
            // from input and jump over it.
            if (filename.s[17]=='c') {
              if (os.size()<8) error("c block too small");
              data_offset=in.tell()+1-d.buffered();
              const char* s=os.c_str();
              int64_t jmp=btol(s);
              if (jmp<0) printf("Incomplete transaction ignored\n");
              if (jmp<0
                  || (version<19000000000000LL && int64_t(ver.size())>version)
                  || (version>=19000000000000LL && version<fdate)) {
                done=true;  // roll back to here
                goto endblock;
              }
              else {
                dcsize+=jmp;
                if (jmp) in.seek(data_offset+jmp, SEEK_SET);
                ver.push_back(VER());
                ver.back().firstFragment=ht.size();
                ver.back().offset=block_offset;
                ver.back().data_offset=data_offset;
                ver.back().date=ver.back().lastdate=fdate;
                ver.back().csize=jmp;
                if (all) {
                  string fn=itos(ver.size()-1, all)+"/"; ///peusa1 versioni
                  if (renamed) fn=rename(fn);
                  if (isselected(fn.c_str(), false,-1))
                    dt[fn].date=fdate;
                }
                if (jmp) goto endblock;
              }
            }

            // Fragment table (type h).
            // Contents is bsize[4] (sha1[20] usize[4])... for fragment N...
            // where bsize is the compressed block size.
            // Store in ht[].{sha1,usize}. Set ht[].csize to block offset
            // assuming N in ascending order.
            else if (filename.s[17]=='h') {
              assert(ver.size()>0);
              if (fdate>ver.back().lastdate) ver.back().lastdate=fdate;
              if (os.size()%24!=4) error("bad h block size");
              const unsigned n=(os.size()-4)/24;
              if (num<1 || num+n>0xffffffff) error("bad h fragment");
              const char* s=os.c_str();
              const unsigned bsize=btoi(s);
              dhsize+=bsize;
              assert(ver.size()>0);
              if (int64_t(ht.size())>num) {
                fflush(stdout);
                fprintf(stderr,
                  "Unordered fragment tables: expected >= %d found %1.0f\n",
                  int(ht.size()), double(num));
				 g_exec_text="Unordered fragment tables";
              }
              for (unsigned i=0; i<n; ++i) {
                if (i==0) {
                  block.push_back(Block(num, data_offset));
                  block.back().usize=8;
                  block.back().bsize=bsize;
                  block.back().frags=os.size()/24;
                }
                while (int64_t(ht.size())<=num+i) ht.push_back(HT());
                memcpy(ht[num+i].sha1, s, 20);
                s+=20;
                assert(block.size()>0);
                unsigned f=btoi(s);
                if (f>0x7fffffff) error("fragment too big");
                block.back().usize+=(ht[num+i].usize=f)+4u;
              }
              data_offset+=bsize;
            }

            // Index (type i)
            // Contents is: 0[8] filename 0 (deletion)
            // or:       date[8] filename 0 na[4] attr[na] ni[4] ptr[ni][4]
            // Read into DT
            else if (filename.s[17]=='i') 
			{
				
              assert(ver.size()>0);
              if (fdate>ver.back().lastdate) ver.back().lastdate=fdate;
              const char* s=os.c_str();
              const char* const end=s+os.size();
			
				while (s+9<=end) 
				{
					DT dtr;
					dtr.date=btol(s);  // date
					if (dtr.date) 
						++ver.back().updates;
					else 
						++ver.back().deletes;
					const int64_t len=strlen(s);
					if (len>65535) 
						error("filename too long");
					
					string fn=s;  // filename renamed
					if (all) 
					{
						if (i_myappend)
							fn=itos(ver.size()-1, all)+"|$1"+fn;
						else
							fn=append_path(itos(ver.size()-1, all), fn);
					}
/// OK let's do some check on filenames (slow down, but sometimes save the day)

		
					if (strstr(fn.c_str(), ":$DATA"))
						adsfilenames++;
		
					if (fn.length()>255)
						toolongfilenames++;
					
					if (fn!=utf8toansi(fn))
						utf8names++;
					else
					{/*
							if (!isdirectory(fn))
							{
								string candidato=extractfilename(fn);
								std::for_each(candidato.begin(), candidato.end(), [](char & c)
								{
									c = ::tolower(c);	
								});
								candidato=extractfilepath(fn)+candidato;
								
			///		printf("Cerco %s\n",candidato.c_str());
					if (collisionset.find(candidato) != collisionset.end())
					{
						printf("KOLLISIONE %s\n",fn.c_str());
						printf("kollisione %s\n\n\n\n",candidato.c_str());
						casecollision++;
					}
					else
						collisionset.insert(candidato);
							}
							*/
					}
		
					const bool issel=isselected(fn.c_str(), renamed,-1);
					s+=len+1;  // skip filename
					if (s>end) 
						error("filename too long");
					if (dtr.date) 
					{
						++files;
						if (s+4>end) 
							error("missing attr");
						unsigned na=0;
	
						na=btoi(s);  // attr bytes
				  
						if (s+na>end || na>65535) 
							error("attr too long");
				  
						if (na>FRANZOFFSET) //this is the -checksum options. Get SHA1 0x0 CRC32 0x0
						{
							for (unsigned int i=0;i<50;i++)
								dtr.sha1hex[i]=*(s+(na-FRANZOFFSET)+i);
							dtr.sha1hex[50]=0x0;
						}
				
						for (unsigned i=0; i<na; ++i, ++s)  // read attr
							if (i<8) 
								dtr.attr+=int64_t(*s&255)<<(i*8);
					
						if (flagnoattributes) 
							dtr.attr=0;
						if (s+4>end) 
							error("missing ptr");
						unsigned ni=btoi(s);  // ptr list size
				  
						if (ni>(end-s)/4u) 
							error("ptr list too long");
						if (issel) 
							dtr.ptr.resize(ni);
						for (unsigned i=0; i<ni; ++i) 
						{  // read ptr
							const unsigned j=btoi(s);
							if (issel) 
								dtr.ptr[i]=j;
						}
					}
					if (issel) 
					dt[fn]=dtr;
				}  // end while more files
            }  // end if 'i'
            else 
			{
				printf("Skipping %s %s\n",filename.s.c_str(), comment.s.c_str());
				error("Unexpected journaling block");
            }
          }  // end if journaling

          // Streaming format
          else 
		  {

            // If previous version does not exist, start a new one
            if (ver.size()==1) 
			{
				if (version<1) 
				{
					done=true;
					goto endblock;
				}
				ver.push_back(VER());
				ver.back().firstFragment=ht.size();
				ver.back().offset=block_offset;
				ver.back().csize=-1;
			}

			char sha1result[21]={0};
			d.readSegmentEnd(sha1result);
            skip=true;
            string fn=lastfile;
            if (all) fn=append_path(itos(ver.size()-1, all), fn); ///peusa3
            if (isselected(fn.c_str(), renamed,-1)) {
              DT& dtr=dt[fn];
              if (filename.s.size()>0 || first) {
                ++files;
                dtr.date=date;
                dtr.attr=0;
                dtr.ptr.resize(0);
                ++ver.back().updates;
              }
              dtr.ptr.push_back(ht.size());
            }
            assert(ver.size()>0);
            if (segs==0 || block.size()==0)
              block.push_back(Block(ht.size(), block_offset));
            assert(block.size()>0);
            ht.push_back(HT(sha1result+1, -1));
          }  // end else streaming
          ++segs;
          filename.s="";
          first=false;
        }  // end while findFilename
        if (!done) block_offset=in.tell()-d.buffered();
      }  // end while findBlock
      done=true;
    }  // end try
    catch (std::exception& e) {
      in.seek(-d.buffered(), SEEK_CUR);
      fflush(stdout);
      fprintf(stderr, "Skipping block at %1.0f: %s\n", double(block_offset),
              e.what());
		g_exec_text="Skipping block";
      if (errors) ++*errors;
    }
endblock:;
  }  // end while !done
  if (in.tell()>32*(password!=0) && !found_data)
    error("archive contains no data");
	if (!flagpakka)
	printf("%d versions, %s files, %s fragments, %s bytes (%s)\n", 
      int(ver.size()-1), migliaia2(files), migliaia3(unsigned(ht.size())-1),
      migliaia(block_offset),tohuman(block_offset));

	if (casecollision>0)
		printf("Case collisions       %9s (-fix255)\n",migliaia(casecollision));
	if (toolongfilenames)
	{
#ifdef _WIN32	
	printf("Long filenames (>255) %9s *** WARNING *** (-fix255)\n",migliaia(toolongfilenames));
#else
	printf("Long filenames (>255) %9s\n",migliaia(toolongfilenames));
#endif

	}
	if (utf8names)
		printf("Non-latin (UTF-8)     %9s\n",migliaia(utf8names));
	if (adsfilenames)
		printf("ADS ($:DATA)          %9s\n",migliaia(adsfilenames));
	
  // Calculate file sizes
	for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) 
	{
		for (unsigned i=0; i<p->second.ptr.size(); ++i) 
		{
			unsigned j=p->second.ptr[i];
			if (j>0 && j<ht.size() && p->second.size>=0) 
			{
				if (ht[j].usize>=0) 
					p->second.size+=ht[j].usize;
				else 
					p->second.size=-1;  // unknown size
			}
		}
	}
  
	return block_offset;
}

// Test whether filename and attributes are selected by files, -only, and -not
// If rn then test renamed filename.
// if i_size >0 check file size too (minsize,maxsize)

bool Jidac::isselected(const char* filename, bool rn,int64_t i_size) 
{
	bool matched=true;
	
	if (!flagforcezfs)
		if (flagskipzfs) //this is an "automagically" exclude for zfs' snapshots
			if (iszfs(filename))
			{
				if (flagverbose)
					printf("Verbose: Skip .zfs %s\n",filename);
				return false;
			}

	if (flagnoqnap) //this is an "automagically" exclude for qnap's snapshots
		if (
			(strstr(filename, "@Recently-Snapshot")) 
			||
			(strstr(filename, "@Recycle")) 
			)
			{
				if (flagverbose)
					printf("Verbose: Skip qnap %s\n",filename);
				return false;
			}
			
	if (!flagforcewindows) //this "automagically" exclude anything with System Volume Information
	{
		if (strstr(filename, "System Volume Information")) 
		{
			if (flagverbose)
				printf("Verbose: Skip System Volume Information %s\n",filename);
			return false;
		}
		if (strstr(filename, "$RECYCLE.BIN")) 
		{
			if (flagverbose)
				printf("Verbose: Skip trash %s\n",filename);
			return false;
		}
	}			
	
	if  (!flagforcewindows)
		if (isads(filename))
			return false;
	
	if (i_size>0)
	{
		if (maxsize)
			if (maxsize<(uint64_t)i_size)
			{
				if (flagverbose)
					printf("Verbose: (-maxsize) too big   %19s %s\n",migliaia(i_size),filename);
				return false;
			}
		if (minsize)
			if (minsize>(uint64_t)i_size)
			{
				if (flagverbose)
					printf("Verbose: (-minsize) too small %19s %s\n",migliaia(i_size),filename);
				return false;
			}
	}
	
  if (files.size()>0) {
    matched=false;
    for (unsigned i=0; i<files.size() && !matched; ++i) {
      if (rn && i<tofiles.size()) {
        if (ispath(tofiles[i].c_str(), filename)) matched=true;
      }
      else if (ispath(files[i].c_str(), filename)) matched=true;
    }
  }
  
  if (!matched) return false;
  
  if (onlyfiles.size()>0) 
  {
	matched=false;
    for (unsigned i=0; i<onlyfiles.size() && !matched; ++i)
	{
		///printf("ZEKE:Testo %s su %s\n",onlyfiles[i].c_str(),filename);
	 if (ispath(onlyfiles[i].c_str(), filename))
	 {
        matched=true;
		///printf("MAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
	 }
	}
  }
  
  if (!matched) return false;
  for (unsigned i=0; i<notfiles.size(); ++i) {
    if (ispath(notfiles[i].c_str(), filename))
      return false;
  }
  
  return true;
}

// Return the part of fn up to the last slash
string path(const string& fn) {
  int n=0;
  for (int i=0; fn[i]; ++i)
    if (fn[i]=='/' || fn[i]=='\\') n=i+1;
  return fn.substr(0, n);
}

// Insert external filename (UTF-8 with "/") into dt if selected
// by files, onlyfiles, and notfiles. If filename
// is a directory then also insert its contents.
// In Windows, filename might have wildcards like "file.*" or "dir/*"
// In zpaqfranz, sometimes, we DO not want to recurse
void Jidac::scandir(bool i_checkifselected,DTMap& i_edt,string filename, bool i_recursive)
{

  // Don't scan diretories excluded by -not
  for (unsigned i=0; i<notfiles.size(); ++i)
    if (ispath(notfiles[i].c_str(), filename.c_str()))
      return;

	if (!flagforcezfs)
		if (flagskipzfs)
			if (iszfs(filename))
			{
				if (flagverbose)
					printf("Verbose: Skip .zfs ----> %s\n",filename.c_str());
				return;
			}
	if (flagnoqnap)
	{
		if (filename.find("@Recently-Snapshot")!=std::string::npos)
		{
			if (flagverbose)
				printf("Verbose: Skip qnap snapshot ----> %s\n",filename.c_str());

			return;
		}
		if (filename.find("@Recycle")!=std::string::npos)
		{
			if (flagverbose)
				printf("Verbose: Skip qnap recycle ----> %s\n",filename.c_str());
			return;
		}
	}
	
#ifdef unix
// Add regular files and directories
	while (filename.size()>1 && filename[filename.size()-1]=='/')
		filename=filename.substr(0, filename.size()-1);  // remove trailing /
	
	struct stat sb;
	if (!lstat(filename.c_str(), &sb)) 
	{
		if (S_ISREG(sb.st_mode))
			addfile(i_checkifselected,i_edt,filename, decimal_time(sb.st_mtime), sb.st_size,'u'+(sb.st_mode<<8));

    // Traverse directory
		if (S_ISDIR(sb.st_mode)) 
		{
			addfile(i_checkifselected,i_edt,filename=="/" ? "/" : filename+"/", decimal_time(sb.st_mtime),0, 'u'+(int64_t(sb.st_mode)<<8));
			DIR* dirp=opendir(filename.c_str());
			if (dirp) 
			{
				for (dirent* dp=readdir(dirp); dp; dp=readdir(dirp)) 
				{
					if (strcmp(".", dp->d_name) && strcmp("..", dp->d_name)) 
					{
						string s=filename;
						if (s!="/") s+="/";
						s+=dp->d_name;
						if (i_recursive)        
							scandir(i_checkifselected,i_edt,s);
						else
						{
							if (!lstat(s.c_str(), &sb)) 
							{
								if (S_ISREG(sb.st_mode))
									addfile(i_checkifselected,i_edt,s, decimal_time(sb.st_mtime), sb.st_size,'u'+(sb.st_mode<<8));
								if (S_ISDIR(sb.st_mode)) 
									addfile(i_checkifselected,i_edt,s=="/" ? "/" :s+"/", decimal_time(sb.st_mtime),0, 'u'+(int64_t(sb.st_mode)<<8));
							}
						}          			
					}
				}
				closedir(dirp);
			}
			else
				perror(filename.c_str());
		}
	}
	else
		perror(filename.c_str());

#else  // Windows: expand wildcards in filename

  // Expand wildcards
  WIN32_FIND_DATA ffd;
  string t=filename;
  if (t.size()>0 && t[t.size()-1]=='/') t+="*";
  HANDLE h=FindFirstFile(utow(t.c_str()).c_str(), &ffd);
  if (h==INVALID_HANDLE_VALUE
      && GetLastError()!=ERROR_FILE_NOT_FOUND
      && GetLastError()!=ERROR_PATH_NOT_FOUND)
    printerr("15367",t.c_str());
  while (h!=INVALID_HANDLE_VALUE) {

    // For each file, get name, date, size, attributes
    SYSTEMTIME st;
    int64_t edate=0;
    if (FileTimeToSystemTime(&ffd.ftLastWriteTime, &st))
      edate=st.wYear*10000000000LL+st.wMonth*100000000LL+st.wDay*1000000
            +st.wHour*10000+st.wMinute*100+st.wSecond;
    const int64_t esize=ffd.nFileSizeLow+(int64_t(ffd.nFileSizeHigh)<<32);
    const int64_t eattr='w'+(int64_t(ffd.dwFileAttributes)<<8);

    // Ignore links, the names "." and ".." or any unselected file
    t=wtou(ffd.cFileName);
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT
        || t=="." || t=="..") edate=0;  // don't add
    string fn=path(filename)+t;

    // Save directory names with a trailing / and scan their contents
    // Otherwise, save plain files
    if (edate) 
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) fn+="/";
		addfile(i_checkifselected,i_edt,fn, edate, esize, eattr);
		
		if (i_recursive)
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
			{
				fn+="*";
				scandir(i_checkifselected,i_edt,fn);
			}
      // enumerate alternate streams (Win2003/Vista or later)
      else 
	  if (findFirstStreamW && findNextStreamW) 
	  {
		if (flagforcewindows)
		{
			WIN32_FIND_STREAM_DATA fsd;
			HANDLE ah=findFirstStreamW(utow(fn.c_str()).c_str(),
				FindStreamInfoStandard, &fsd, 0);
			while (ah!=INVALID_HANDLE_VALUE && findNextStreamW(ah, &fsd))
				addfile(i_checkifselected,i_edt,fn+wtou(fsd.cStreamName), edate,
              fsd.StreamSize.QuadPart, eattr);
			if (ah!=INVALID_HANDLE_VALUE) FindClose(ah);
		}
      }
	  }
    }
    if (!FindNextFile(h, &ffd)) {
      if (GetLastError()!=ERROR_NO_MORE_FILES) printerr("15417",fn.c_str());
      break;
    }
  }
  FindClose(h);
#endif
}

	
	
// Add external file and its date, size, and attributes to dt
void Jidac::addfile(bool i_checkifselected,DTMap& i_edt,string filename, int64_t edate,
                    int64_t esize, int64_t eattr) {
				
	if (i_checkifselected)
		if (!isselected(filename.c_str(), false,esize)) 
			return;
  
  DT& d=i_edt[filename];
  d.date=edate;
  d.size=esize;
  d.attr=flagnoattributes?0:eattr;
  d.data=0;
  g_bytescanned+=esize;
  g_filescanned++;
 if (flagnoeta==false)
 { 
  if (!(i_edt.size() % 1000))
  {
    double scantime=mtime()-global_start+1;
	printf("Scanning %s %2.2fs %d file/s (%s)\r",migliaia(i_edt.size()),scantime/1000.0,(int)(i_edt.size()/(scantime/1000.0)),migliaia2(g_bytescanned));
	fflush(stdout);
 }
 
 }
}

//////////////////////////////// add //////////////////////////////////

// Append n bytes of x to sb in LSB order
inline void puti(libzpaq::StringBuffer& sb, uint64_t x, int n) {
  for (; n>0; --n) sb.put(x&255), x>>=8;
}

// Print percent done (td/ts) and estimated time remaining
// two modes: "normal" (old zpaqfranz) "pakka" (new)
void print_progress(int64_t ts, int64_t td,int64_t i_scritti) 
{
	static int ultimapercentuale=0;
	static int ultimaeta=0;
	
	if (flagnoeta==true)
		return;
		
	if (td>ts) 
		td=ts;
	
	if (td<1000000)
		return;
		
	double eta=0.001*(mtime()-global_start)*(ts-td)/(td+1.0);
	int secondi=(mtime()-global_start)/1000;
	if (secondi==0)
		secondi=1;
	
	int percentuale=int(td*100.0/(ts+0.5));

///	printf("%d %d\n",int(eta),ultimaeta);
	

	if (flagpakka)
	{
		if (eta<350000)
		if (((percentuale%10)==0) ||(percentuale==1))
			if ((percentuale!=ultimapercentuale) || (percentuale==1))
			{
				ultimapercentuale=percentuale;
				printf("%03d%% %02d:%02d:%02d %20s of %20s %s/sec\r", percentuale,
					int(eta/3600), int(eta/60)%60, int(eta)%60, migliaia(td), migliaia2(ts),migliaia3(td/secondi));
			}
	}
	else 
	{
		if (int(eta)!=ultimaeta)
		if (eta<350000)
		{
			ultimaeta=int(eta);
			if (i_scritti>0)
			printf("%6.2f%% %02d:%02d:%02d (%10s) -> (%10s) of (%10s) %10s/sec\r", td*100.0/(ts+0.5),
			int(eta/3600), int(eta/60)%60, int(eta)%60, tohuman(td),tohuman2(i_scritti),tohuman3(ts),tohuman4(td/secondi));
			else
			printf("%6.2f%% %02d:%02d:%02d (%10s) of (%10s) %10s/sec\r", td*100.0/(ts+0.5),
			int(eta/3600), int(eta/60)%60, int(eta)%60, tohuman(td),tohuman2(ts),tohuman3(td/secondi));
			
			
			
		}
	}				
}

// A CompressJob is a queue of blocks to compress and write to the archive.
// Each block cycles through states EMPTY, FILLING, FULL, COMPRESSING,
// COMPRESSED, WRITING. The main thread waits for EMPTY buffers and
// fills them. A set of compressThreads waits for FULL threads and compresses
// them. A writeThread waits for COMPRESSED buffers at the front
// of the queue and writes and removes them.

// Buffer queue element
struct CJ {
  enum {EMPTY, FULL, COMPRESSING, COMPRESSED, WRITING} state;
  StringBuffer in;       // uncompressed input
  StringBuffer out;      // compressed output
  string filename;       // to write in filename field
  string comment;        // if "" use default
  string method;         // compression level or "" to mark end of data
  Semaphore full;        // 1 if in is FULL of data ready to compress
  Semaphore compressed;  // 1 if out contains COMPRESSED data
  CJ(): state(EMPTY) {}
};

// Instructions to a compression job
class CompressJob {
public:
  Mutex mutex;           // protects state changes
private:
  int job;               // number of jobs
  CJ* q;                 // buffer queue
  unsigned qsize;        // number of elements in q
  int front;             // next to remove from queue
  libzpaq::Writer* out;  // archive
  Semaphore empty;       // number of empty buffers ready to fill
  Semaphore compressors; // number of compressors available to run
public:
  friend ThreadReturn compressThread(void* arg);
  friend ThreadReturn writeThread(void* arg);
  CompressJob(int threads, int buffers, libzpaq::Writer* f):
      job(0), q(0), qsize(buffers), front(0), out(f) {
    q=new CJ[buffers];
    if (!q) throw std::bad_alloc();
    init_mutex(mutex);
    empty.init(buffers);
    compressors.init(threads);
    for (int i=0; i<buffers; ++i) {
      q[i].full.init(0);
      q[i].compressed.init(0);
    }
  }
  ~CompressJob() {
    for (int i=qsize-1; i>=0; --i) {
      q[i].compressed.destroy();
      q[i].full.destroy();
    }
    compressors.destroy();
    empty.destroy();
    destroy_mutex(mutex);
    delete[] q;
  }      
  void write(StringBuffer& s, const char* filename, string method,
             const char* comment=0);
  vector<int> csize;  // compressed block sizes
};

// Write s at the back of the queue. Signal end of input with method=""
void CompressJob::write(StringBuffer& s, const char* fn, string method,
                        const char* comment) {
  for (unsigned k=(method=="")?qsize:1; k>0; --k) {
    empty.wait();
    lock(mutex);
    unsigned i, j;
    for (i=0; i<qsize; ++i) {
      if (q[j=(i+front)%qsize].state==CJ::EMPTY) {
        q[j].filename=fn?fn:"";
        q[j].comment=comment?comment:"jDC\x01";
        q[j].method=method;
        q[j].in.resize(0);
        q[j].in.swap(s);
        q[j].state=CJ::FULL;
        q[j].full.signal();
        break;
      }
    }
    release(mutex);
    assert(i<qsize);  // queue should not be full
  }
}

// Compress data in the background, one per buffer
ThreadReturn compressThread(void* arg) {
  CompressJob& job=*(CompressJob*)arg;
  int jobNumber=0;
  try {

    // Get job number = assigned position in queue
    lock(job.mutex);
    jobNumber=job.job++;
    assert(jobNumber>=0 && jobNumber<int(job.qsize));
    CJ& cj=job.q[jobNumber];
    release(job.mutex);

    // Work until done
    while (true) {
      cj.full.wait();
      lock(job.mutex);

      // Check for end of input
      if (cj.method=="") {
        cj.compressed.signal();
        release(job.mutex);
        return 0;
      }

      // Compress
      assert(cj.state==CJ::FULL);
      cj.state=CJ::COMPRESSING;
      release(job.mutex);
      job.compressors.wait();
	       libzpaq::compressBlock(&cj.in, &cj.out, cj.method.c_str(),
          cj.filename.c_str(), cj.comment=="" ? 0 : cj.comment.c_str());
      cj.in.resize(0);
      lock(job.mutex);

	///	printf("\nFarei qualcosa <<%s>> %08X size %ld\n",myblock.filename.c_str(),myblock.crc32,myblock.crc32size);

	  cj.state=CJ::COMPRESSED;
      cj.compressed.signal();
      job.compressors.signal();
      release(job.mutex);
    }
  }
  catch (std::exception& e) {
    lock(job.mutex);
    fflush(stdout);
    fprintf(stderr, "job %d: %s\n", jobNumber+1, e.what());
	g_exec_text="job error";
    release(job.mutex);
    exit(1);
  }
  return 0;
}

// Write compressed data to the archive in the background
ThreadReturn writeThread(void* arg) {
  CompressJob& job=*(CompressJob*)arg;
  try {

    // work until done
    while (true) {

      // wait for something to write
      CJ& cj=job.q[job.front];  // no other threads move front
      cj.compressed.wait();

      // Quit if end of input
      lock(job.mutex);
      if (cj.method=="") {
        release(job.mutex);
        return 0;
      }

      // Write to archive
      assert(cj.state==CJ::COMPRESSED);
      cj.state=CJ::WRITING;
      job.csize.push_back(cj.out.size());
      if (job.out && cj.out.size()>0) {
        release(job.mutex);
        assert(cj.out.c_str());
        const char* p=cj.out.c_str();
        int64_t n=cj.out.size();
        
		g_scritti+=n; // very rude
		const int64_t N=1<<30;
        while (n>N) {
          job.out->write(p, N);
          p+=N;
          n-=N;
        }
        job.out->write(p, n);
        lock(job.mutex);
      }
      cj.out.resize(0);
      cj.state=CJ::EMPTY;
      job.front=(job.front+1)%job.qsize;
      job.empty.signal();
      release(job.mutex);
    }
  }
  catch (std::exception& e) {
    fflush(stdout);
    fprintf(stderr, "zpaqfranz exiting from writeThread: %s\n", e.what());
	g_exec_text="exiting from writethread";
    exit(1);
  }
  return 0;
}

// Write a ZPAQ compressed JIDAC block header. Output size should not
// depend on input data.
void writeJidacHeader(libzpaq::Writer *out, int64_t date,
                      int64_t cdata, unsigned htsize) {
  if (!out) return;
  assert(date>=19000000000000LL && date<30000000000000LL);
  StringBuffer is;
  puti(is, cdata, 8);
  libzpaq::compressBlock(&is, out, "0",
      ("jDC"+itos(date, 14)+"c"+itos(htsize, 10)).c_str(), "jDC\x01");
}

// Maps sha1 -> fragment ID in ht with known size
class HTIndex {
  vector<HT>& htr;  // reference to ht
  libzpaq::Array<unsigned> t;  // sha1 prefix -> index into ht
  unsigned htsize;  // number of IDs in t

  // Compuate a hash index for sha1[20]
  unsigned hash(const char* sha1) {
    return (*(const unsigned*)sha1)&(t.size()-1);
  }

public:
  // r = ht, sz = estimated number of fragments needed
  HTIndex(vector<HT>& r, size_t sz): htr(r), t(0), htsize(1) {
    int b;
    for (b=1; sz*3>>b; ++b);
    t.resize(1, b-1);
    update();
  }

  // Find sha1 in ht. Return its index or 0 if not found.
  unsigned find(const char* sha1) {
    unsigned h=hash(sha1);
    for (unsigned i=0; i<t.size(); ++i) {
      if (t[h^i]==0) return 0;
      if (memcmp(sha1, htr[t[h^i]].sha1, 20)==0) return t[h^i];
    }
    return 0;
  }

  // Update index of ht. Do not index if fragment size is unknown.
  void update() {
    char zero[20]={0};
    while (htsize<htr.size()) {
      if (htsize>=t.size()/4*3) {
        t.resize(t.size(), 1);
        htsize=1;
      }
      if (htr[htsize].usize>=0 && memcmp(htr[htsize].sha1, zero, 20)!=0) {
        unsigned h=hash((const char*)htr[htsize].sha1);
        for (unsigned i=0; i<t.size(); ++i) {
          if (t[h^i]==0) {
            t[h^i]=htsize;
            break;
          }
        }
      }
      ++htsize;
    }
  }    
};

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

// Sort by sortkey, then by full path
bool compareFilename(DTMap::iterator ap, DTMap::iterator bp) {
  if (ap->second.data!=bp->second.data)
    return ap->second.data<bp->second.data;
  return ap->first<bp->first;
}
/*
bool comparefilesize(DTMap::iterator ap, DTMap::iterator bp) {
  if (ap->second.size!=bp->second.size)
    return ap->second.size<bp->second.size;
  return ap->first<bp->first;
}
*/
// sort by sha1hex, without checks
/*
bool comparesha1hex(DTMap::iterator i_primo, DTMap::iterator i_secondo) 
{
	return (strcmp(i_primo->second.sha1hex,i_secondo->second.sha1hex)<0);
}
*/

// For writing to two archives at once
struct WriterPair: public libzpaq::Writer {
  OutputArchive *a, *b;
  void put(int c) {
    if (a) a->put(c);
    if (b) b->put(c);
  }
  void write(const char* buf, int n) {
    if (a) a->write(buf, n);
    if (b) b->write(buf, n);
  }
  WriterPair(): a(0), b(0) {}
};

#if defined(_WIN32) || defined(_WIN64)
//// VSS on Windows by... batchfile
//// delete all kind of shadows copies (if any)
void vss_deleteshadows()
{
	if (flagvss)
	{
		string	filebatch	=g_gettempdirectory()+"vsz.bat";
		
		printf("Starting delete VSS shadows\n");
    		
		if (fileexists(filebatch))
			if (remove(filebatch.c_str())!=0)
			{
				printf("Highlander batch  %s\n", filebatch.c_str());
				return;
			}
		
		FILE* batch=fopen(filebatch.c_str(), "wb");
		fprintf(batch,"@echo OFF\n");
		fprintf(batch,"@wmic shadowcopy delete /nointeractive\n");
		fclose(batch);
	
		waitexecute(filebatch,"",SW_HIDE);
	
		printf("End VSS delete shadows\n");
	}

}
#endif

/// work with a batch job
void avanzamento(int64_t i_lavorati,int64_t i_totali,int64_t i_inizio)
{
	static int ultimapercentuale=0;
	
	if (flagnoeta==true)
		return;
	
	int percentuale=int(i_lavorati*100.0/(i_totali+0.5));
	
	if (percentuale>0)
		if (((percentuale%10)==0)  || (percentuale==1))
	//if ((((percentuale%10)==0) && (percentuale>0)) || (percentuale==1))
	if (percentuale!=ultimapercentuale)
	{
		ultimapercentuale=percentuale;
				
		double eta=0.001*(mtime()-i_inizio)*(i_totali-i_lavorati)/(i_lavorati+1.0);
		int secondi=(mtime()-i_inizio)/1000;
		if (secondi==0)
			secondi=1;
		if (eta<356000)
		printf("%03d%% %02d:%02d:%02d (%10s) of (%10s) %20s /sec\n", percentuale,
		int(eta/3600), int(eta/60)%60, int(eta)%60, tohuman(i_lavorati), tohuman2(i_totali),migliaia3(i_lavorati/secondi));
		fflush(stdout);
	}
}

std::string sha1_calc_file(const char * i_filename,const int64_t i_inizio,const int64_t i_totali,int64_t& io_lavorati)
{
	std::string risultato="";
	FILE* myfile = freadopen(i_filename);
	if(myfile==NULL )
 		return risultato;
	
	const int BUFSIZE	=65536*8;
	char 				unzBuf[BUFSIZE];
	int 				n=BUFSIZE;
				
	libzpaq::SHA1 sha1;
	while (1)
	{
		int r=fread(unzBuf, 1, n, myfile);
		
		for (int i=0;i<r;i++)
			sha1.put(*(unzBuf+i));
 		if (r!=n) 
			break;
		io_lavorati+=r;
		if ((flagnoeta==false) && (i_inizio>0))
			avanzamento(io_lavorati,i_totali,i_inizio);
	}
	fclose(myfile);
	
	char sha1result[20];
	memcpy(sha1result, sha1.result(), 20);
	char myhex[4];
	risultato="";
	for (int j=0; j <20; j++)
	{
		sprintf(myhex,"%02X", (unsigned char)sha1result[j]);
		risultato.push_back(myhex[0]);
		risultato.push_back(myhex[1]);
	}
	return risultato;
}

//// get CRC32 of a file

uint32_t crc32_calc_file(const char * i_filename,const int64_t i_inizio,const int64_t i_totali,int64_t& io_lavorati)
{
	FILE* myfile = freadopen(i_filename);
	if(myfile==NULL )
		return 0;
		
	char data[65536*16];
    uint32_t crc=0;
	int got=0;
	
	while ((got=fread(data,sizeof(char),sizeof(data),myfile)) > 0) 
	{
		crc=crc32_16bytes (data, got, crc);
		io_lavorati+=got;	
		if ((flagnoeta==false) && (i_inizio>0))
			avanzamento(io_lavorati,i_totali,i_inizio);
	}
	fclose(myfile);
	return crc;
}


/// special function: get SHA1 AND CRC32 of a file
/// return a special string (HEX ASCII)
/// SHA1 (zero) CRC32 (zero)
/// output[40] and output[50] will be set to zero
/// a waste of space, but I do not care at all

/// note: take CRC-32 to a quick verify against CRC-32 of fragments
bool Jidac::getchecksum(string i_filename, char* o_sha1)
{
		
	if (i_filename=="")
		return false;
	
	if (!o_sha1)
		return false;
	if (isdirectory(i_filename))
		return false;
		
	
///	houston, we have a directory	
	if (isdirectory(i_filename))
	{
///	                    1234567890123456789012345678901234567890 12345678
//			sprintf(o_sha1,"THIS IS A DIRECTORY                    Z MYZZEKAN");
		sprintf(o_sha1,"THIS IS A DIRECTORY                      DIRECTOR");
		o_sha1[40]=0x0;
		o_sha1[50]=0x0;
		return true;
	}
	
	int64_t total_size		=0;  
	int64_t lavorati		=0;

	const int BUFSIZE	=65536*16;
	char 				buf[BUFSIZE];
	int 				n=BUFSIZE;

	uint32_t crc=0;
	
	FP myin=fopen(i_filename.c_str(), RB);
	if (myin==FPNULL)
	{
///		return false;
		ioerr(i_filename.c_str());
	
	}
    fseeko(myin, 0, SEEK_END);
    total_size=ftello(myin);
	fseeko(myin, 0, SEEK_SET);

	string nomecorto=mymaxfile(i_filename,40); /// cut filename to max 40 chars
	printf("  Checksumming (%19s) %40s",migliaia(total_size),nomecorto.c_str());

	int64_t blocco = (total_size / 8)+1;
	libzpaq::SHA1 sha1;
	unsigned int ultimapercentuale=200;
	char simboli[]= {'|','/','-','\\','|','/','-','\\'};
	while (1)
	{
		int r=fread(buf, 1, n, myin);
		sha1.write(buf, r);					// double tap: first SHA1 (I like it)
		crc=crc32_16bytes (buf, r, crc);	// than CRC32 (for checksum)

		if (r!=n) 
			break;
					
		lavorati+=r;
		unsigned int rapporto=lavorati/blocco;
		if (rapporto!=ultimapercentuale)
		{
			if (rapporto<=sizeof(simboli)) // just to be sure
				printf("\r%c",simboli[rapporto]); //,migliaia(total_size),nomecorto.c_str());
			ultimapercentuale=lavorati/blocco;
		}
	}
	fclose(myin);
	
	char sha1result[20];
	memcpy(sha1result, sha1.result(), 20);
	for (int j=0; j <= 19; j++)
		sprintf(o_sha1+j*2,"%02X", (unsigned char)sha1result[j]);
	o_sha1[40]=0x0;

	sprintf(o_sha1+41,"%08X",crc);
	o_sha1[50]=0x0;
	
	// example output 	96298A42DB2377092E75F5E649082758F2E0718F 
	//				  	(zero)
	//					3953EFF0
	//					(zero)
	printf("\r+\r");
		
	return true;
}



void Jidac::write715attr(libzpaq::StringBuffer& i_sb, uint64_t i_data, unsigned int i_quanti)
{
	assert(i_sb);
	assert(i_quanti<=8);
	puti(i_sb, i_quanti, 4);
	puti(i_sb, i_data, i_quanti);
}
void Jidac::writefranzattr(libzpaq::StringBuffer& i_sb, uint64_t i_data, unsigned int i_quanti,bool i_checksum,string i_filename,char *i_sha1,uint32_t i_crc32)
{
	
///	experimental fix: pad to 8 bytes (with zeros) for 7.15 enhanced compatibility

	assert(i_sb);
	if (i_checksum)
	{
//	ok, we are paranoid.

		assert(i_sha1);
		assert(i_filename.length()>0); // I do not like empty()
		assert(i_quanti<8);	//just to be sure at least 1 zero pad, so < and not <=

//	getchecksum return SHA1 (zero) CRC32.
		
		if (getchecksum(i_filename,i_sha1))
		{
//	we are paranoid: check if CRC-32, calculated from fragments during compression,
//	is equal to the CRC-32 taken from a post-read of the file
//	in other words we are checking that CRC32(file) == ( combine(CRC32{fragments-in-zpaq}) )
			char crc32fromfragments[9];
			sprintf(crc32fromfragments,"%08X",i_crc32);
			if (memcmp(crc32fromfragments,i_sha1+41,8)!=0)
			{
				printf("\nGURU: on file               %s\n",i_filename.c_str());
				printf("GURU: CRC-32 from fragments %s\n",crc32fromfragments);
				printf("GURU: CRC-32 from file      %s\n",i_sha1+41);
				error("Guru checking crc32");
			}

			puti(i_sb, 8+FRANZOFFSET, 4); 	// 8+FRANZOFFSET block
			puti(i_sb, i_data, i_quanti);
			puti(i_sb, 0, (8 - i_quanti));  // pad with zeros (for 7.15 little bug)
			i_sb.write(i_sha1,FRANZOFFSET);
			if (flagverbose)
				printf("SHA1 <<%s>> CRC32 <<%s>> %s\n",i_sha1,i_sha1+41,i_filename.c_str());
		}
		else
			write715attr(i_sb,i_data,i_quanti);
		
	}
	else
	if ((flagcrc32) || (flag715))
	{
// with add -crc32 turn off, going back to 7.15 behaviour
		write715attr(i_sb,i_data,i_quanti);
	}
	else
	{
			
//	OK, we only write the CRC-32 from ZPAQ fragments (file integrity, not file check)
		memset(i_sha1,0,FRANZOFFSET);
		sprintf(i_sha1+41,"%08X",i_crc32);
		puti(i_sb, 8+FRANZOFFSET, 4); 	// 8+FRANZOFFSET block
		puti(i_sb, i_data, i_quanti);
		puti(i_sb, 0, (8 - i_quanti));  // pad with zeros (for 7.15 little bug)
		i_sb.write(i_sha1,FRANZOFFSET);
		if (flagverbose)
			printf("CRC32 by frag <<%s>> %s\n",i_sha1+41,i_filename.c_str());
		
	}
	
}

// Add or delete files from archive. Return 1 if error else 0.
int Jidac::add() 
{
	if (flagchecksum)
		if (flagnopath)
			error("incompatible -checksum and -nopath");
	
	if (flagvss)
	{
		if (flagtest)
			error("incompatible -vss and -test");
		if (flagnopath)
			error("incompatible -vss and -nopath");
	
	}
	
	g_scritti=0;
	
	if (flagvss)
	{

#if defined(_WIN32) || defined(_WIN64)

		char mydrive=0; // only ONE letter, only ONE VSS snapshot
		// abort for something like zpaqfranz a z:\1.zpaq c:\pippo d:\pluto -vss
		// abort for zpaqranz a z:\1.zpaq \\server\dati -vss
		// etc
		for (unsigned i=0; i<files.size(); ++i)
		{
			string temp=files[i];
		
			if (temp[1]!=':')
				error("VSS need X:something as path (missing :)");
				
			char currentdrive=toupper(temp[0]);
			if (!isalpha(currentdrive))
				error("VSS need X:something as path (X not isalpha)");
			if (mydrive)
			{
				if (mydrive!=currentdrive)
				error("VSS can only runs on ONE drive");
			}
			else	
			mydrive=currentdrive;
		}

			
		string	fileoutput	=g_gettempdirectory()+"outputz.txt";
		string	filebatch	=g_gettempdirectory()+"vsz.bat";
	
		printf("Starting VSS\n");
    
		if (fileexists(fileoutput))
			if (remove(fileoutput.c_str())!=0)
					error("Highlander outputz.txt");

		if (fileexists(filebatch))
			if (remove(filebatch.c_str())!=0)
				error("Highlander vsz.bat");
		
		
		FILE* batch=fopen(filebatch.c_str(), "wb");
		fprintf(batch,"@echo OFF\n");
		fprintf(batch,"@wmic shadowcopy delete /nointeractive\n");
		///fprintf(batch,"@wmic shadowcopy call create Volume=C:\\\n");
		fprintf(batch,"@wmic shadowcopy call create Volume=%c:\\\n",mydrive);
		fprintf(batch,"@vssadmin list shadows >%s\n",fileoutput.c_str());
		fclose(batch);
		
		waitexecute(filebatch,"",SW_HIDE);

		if (!fileexists(fileoutput))
			error("VSS Output file KO");
	
		string globalroot="";
		char line[1024];

		FILE* myoutput=fopen(fileoutput.c_str(), "rb");
		
		/* note that fgets don't strip the terminating \n (or \r\n) but we do not like it  */
		
		while (fgets(line, sizeof(line), myoutput)) 
        	if (strstr(line,"GLOBALROOT"))
			{
				globalroot=line;
				myreplaceall(globalroot,"\n","");
				myreplaceall(globalroot,"\r","");
				break;
			}
		
		fclose(myoutput);
		printf("global root |%s|\n",globalroot.c_str());
///	sometimes VSS is not available
		if (globalroot=="")
			error("VSS cannot continue. Maybe impossible to take snapshot?");


		string volume="";
		string vss_shadow="";
		
		int posstart=globalroot.find("\\\\");
			
		if (posstart>0)
			for (unsigned i=posstart; i<globalroot.length(); ++i)
				vss_shadow=vss_shadow+globalroot.at(i);
		printf("VSS VOLUME <<<%s>>>\n",vss_shadow.c_str());
		myreplaceall(vss_shadow,"\\","/");
		printf("VSS SHADOW <<<%s>>>\n",vss_shadow.c_str());
		
			
		tofiles.clear();
		for (unsigned i=0; i<files.size(); ++i)
			tofiles.push_back(files[i]);
		
		for (unsigned i=0; i<tofiles.size(); ++i)
			printf("TOFILESSS %s\n",tofiles[i].c_str());
		printf("\n");
		
		string replaceme=tofiles[0].substr(0,2);
		printf("-------------- %s -------\n",replaceme.c_str());
		
		for (unsigned i=0; i<files.size(); ++i)
		{
			printf("PRE  FILES %s\n",files[i].c_str());
			myreplace(files[i],replaceme,vss_shadow);
			printf("POST FILES %s\n",files[i].c_str());
			if (strstr(files[i].c_str(), "GLOBALROOT")==0)
				error("VSS fail: strange post files");

		}
#else
		printf("Volume Shadow Copies runs only on Windows\n");
#endif

		printf("End VSS\n");
	}


  // Read archive or index into ht, dt, ver.
  int errors=0;
  const bool archive_exists=exists(subpart(archive, 1).c_str());
  string arcname=archive;  // input archive name
  if (index) arcname=index;
  int64_t header_pos=0;
  if (exists(subpart(arcname, 1).c_str()))
    header_pos=read_archive(arcname.c_str(), &errors);

  // Set arcname, offset, header_pos, and salt to open out archive
  arcname=archive;  // output file name
  int64_t offset=0;  // total size of existing parts
  char salt[32]={0};  // encryption salt
  if (password) libzpaq::random(salt, 32);

  // Remote archive
  if (index) {
    if (dcsize>0) error("index is a regular archive");
    if (version!=DEFAULT_VERSION) error("cannot truncate with an index");
    offset=header_pos+dhsize;
    header_pos=32*(password && offset==0);
    arcname=subpart(archive, ver.size());
    if (exists(arcname)) {
      printUTF8(arcname.c_str(), stderr);
      fprintf(stderr, ": archive exists\n");
	  g_exec_text=arcname;
	  g_exec_text+=" archive exists";
      error("archive exists");
    }
    if (password) {  // derive archive salt from index
      FP fp=fopen(index, RB);
      if (fp!=FPNULL) {
        if (fread(salt, 1, 32, fp)!=32) error("cannot read salt from index");
        salt[0]^='7'^'z';
        fclose(fp);
      }
    }
  }

  // Local single or multi-part archive
  else {
    int parts=0;  // number of existing parts in multipart
    string part0=subpart(archive, 0);
    if (part0!=archive) {  // multi-part?
      for (int i=1;; ++i) {
        string partname=subpart(archive, i);
        if (partname==part0) error("too many archive parts");
        FP fp=fopen(partname.c_str(), RB);
        if (fp==FPNULL) break;
        ++parts;
        fseeko(fp, 0, SEEK_END);
        offset+=ftello(fp);
        fclose(fp);
      }
      header_pos=32*(password && parts==0);
      arcname=subpart(archive, parts+1);
      if (exists(arcname)) error("part exists");
    }

    // Get salt from first part if it exists
    if (password) {
      FP fp=fopen(subpart(archive, 1).c_str(), RB);
      if (fp==FPNULL) {
        if (header_pos>32) error("archive first part not found");
        header_pos=32;
      }
      else {
        if (fread(salt, 1, 32, fp)!=32) error("cannot read salt");
        fclose(fp);
      }
    }
  }
  if (exists(arcname)) printf("Updating ");
  else printf("Creating ");
  printUTF8(arcname.c_str());
  printf(" at offset %1.0f + %1.0f\n", double(header_pos), double(offset));

  // Set method
  if (method=="") method="1";
  if (method.size()==1) {  // set default blocksize
    if (method[0]>='2' && method[0]<='9') method+="6";
    else method+="4";
  }
  if (strchr("0123456789xs", method[0])==0)
    error("-method must begin with 0..5, x, s");
  assert(method.size()>=2);
  if (method[0]=='s' && index) error("cannot index in streaming mode");

  // Set block and fragment sizes
  if (fragment<0) fragment=0;
  const int log_blocksize=20+atoi(method.c_str()+1);
  if (log_blocksize<20 || log_blocksize>31) error("blocksize must be 0..11");
  const unsigned blocksize=(1u<<log_blocksize)-4096;
  const unsigned MAX_FRAGMENT=fragment>19 || (8128u<<fragment)>blocksize-12
      ? blocksize-12 : 8128u<<fragment;
  const unsigned MIN_FRAGMENT=fragment>25 || (64u<<fragment)>MAX_FRAGMENT
      ? MAX_FRAGMENT : 64u<<fragment;

  // Don't mix streaming and journaling
  for (unsigned i=0; i<block.size(); ++i) {
    if (method[0]=='s') {
      if (block[i].usize>=0)
        error("cannot update journaling archive in streaming format");
    }
    else if (block[i].usize<0)
      error("cannot update streaming archive in journaling format");
  }
  
  g_bytescanned=0;
  g_filescanned=0;
  g_worked=0;
  for (unsigned i=0; i<files.size(); ++i)
    scandir(true,edt,files[i].c_str());

/*
///	stub for fake file to be added (with someinfos)

	string tempfile=g_gettempdirectory()+"filelist.txt";
	myreplaceall(tempfile,"\\","/");

	printf("\nTemp dir <<%s>>\n",tempfile.c_str());
	FILE* myoutfile=fopen(tempfile.c_str(), "wb");
	fprintf(myoutfile,"This is filelist for version %d\n",ver.size());
	for (DTMap::iterator p=edt.begin(); p!=edt.end(); ++p) 
	{
		if (!strstr(p->first.c_str(), ":$DATA"))
			{
				
				fprintf(myoutfile,"%s %19s ", dateToString(p->second.date).c_str(), migliaia(p->second.size));
				printUTF8(p->first.c_str(),myoutfile);
				fprintf(myoutfile,"\n");
    		}
	}
	fclose(myoutfile);
	
	files.push_back(tempfile);
	scandir(files[files.size()-1].c_str());
	*/
  // Sort the files to be added by filename extension and decreasing size
  vector<DTMap::iterator> vf;
  int64_t total_size=0;  // size of all input
  int64_t total_done=0;  // input deduped so far
  int64_t total_xls=0;
  int64_t file_xls=0;

	int toolongfilenames=0;
	int adsfilenames=0;
	int utf8names=0;
	int casecollision=0;
  
  for (DTMap::iterator p=edt.begin(); p!=edt.end(); ++p) {
		///printf("testo %s\n",p->first.c_str());
    DTMap::iterator a=dt.find(rename(p->first));
	string filename=rename(p->first);
	
	
	if (strstr(filename.c_str(), ":$DATA"))
		adsfilenames++;
		
	if (filename.length()>255)
		toolongfilenames++;
					
	if (filename!=utf8toansi(filename))
		utf8names++;

	/// by default ALWAYS force XLS to be re-packed
	/// this is because sometimes Excel change the metadata (then SHA1 & CRC32)
	/// WITHOUT touching attr or filesize
	if (!flagdonotforcexls) 
		if (isxls(filename))
		{
			if (flagverbose)
				printf("ENFORCING XLS %s\n",filename.c_str());
			total_xls+=p->second.size;
			file_xls++;
		}
	
    if (a!=dt.end()) a->second.data=1;  // keep
    if (
			p->second.date && p->first!="" && p->first[p->first.size()-1]!='/' && 
			(
				flagforce || 
				a==dt.end() || 
				( (!(isads(filename))) && (!flagdonotforcexls) && (isxls(filename))) || 
				p->second.date!=a->second.date || 
				p->second.size!=a->second.size
			)
		) 
			
	{
      total_size+=p->second.size;

      // Key by first 5 bytes of filename extension, case insensitive
      int sp=0;  // sortkey byte position
      for (string::const_iterator q=p->first.begin(); q!=p->first.end(); ++q){
        uint64_t c=*q&255;
        if (c>='A' && c<='Z') c+='a'-'A';
        if (c=='/') sp=0, p->second.data=0;
        else if (c=='.') sp=8, p->second.data=0;
        else if (sp>3) p->second.data+=c<<(--sp*8);
      }

      // Key by descending size rounded to 16K
      int64_t s=p->second.size>>14;
      if (s>=(1<<24)) s=(1<<24)-1;
      p->second.data+=(1<<24)-s-1;
	  
		vf.push_back(p);
    }
	
  }
  
  
  if (!flagnosort)
	std::sort(vf.begin(), vf.end(), compareFilename);

 
  // Test for reliable access to archive
  if (archive_exists!=exists(subpart(archive, 1).c_str()))
    error("archive access is intermittent");

  // Open output
  OutputArchive out(arcname.c_str(), password, salt, offset);
  out.seek(header_pos, SEEK_SET);


  // Start compress and write jobs
  vector<ThreadID> tid(howmanythreads*2-1);
  ThreadID wid;
  CompressJob job(howmanythreads, tid.size(), &out);

	
  printf("Adding %s (%s) in %s files ",migliaia(total_size), tohuman(total_size),migliaia2(int(vf.size())));
	
	if (flagverbose)
		printf("-method %s -threads %d ",method.c_str(), howmanythreads);
	
	printf(" at %s",dateToString(date).c_str());
	
	
	if (flagcomment)
		if (versioncomment.length()>0)
			printf("<<%s>>",versioncomment.c_str());
	printf("\n");



if (casecollision>0)
		printf("Case collisions       %9s (-fix255)\n",migliaia(casecollision));
	if (toolongfilenames)
	{
#ifdef _WIN32	
	printf("Long filenames (>255) %9s *** WARNING *** (-fix255)\n",migliaia(toolongfilenames));
#else
	printf("Long filenames (>255) %9s\n",migliaia(toolongfilenames));
#endif

	}
	if (utf8names)
		printf("Non-latin (UTF-8)     %9s\n",migliaia(utf8names));
	if (adsfilenames)
		printf("ADS ($:DATA)          %9s\n",migliaia(adsfilenames));


  for (unsigned i=0; i<tid.size(); ++i) run(tid[i], compressThread, &job);
  run(wid, writeThread, &job);

  // Append in streaming mode. Each file is a separate block. Large files
  // are split into blocks of size blocksize.
  int64_t dedupesize=0;  // input size after dedupe
  if (method[0]=='s') {
    StringBuffer sb(blocksize+4096-128);
    for (unsigned fi=0; fi<vf.size(); ++fi) {
      DTMap::iterator p=vf[fi];
      print_progress(total_size, total_done,g_scritti);
	  
/*     
	 if (summary<=0) {
        printf("+ ");
        printUTF8(p->first.c_str());
        printf(" %1.0f\n", p->second.size+0.0);
      }
	  */
      FP in=fopen(p->first.c_str(), RB);
      if (in==FPNULL) {
        printerr("16570",p->first.c_str());
        total_size-=p->second.size;
        ++errors;
        continue;
      }
      uint64_t i=0;
      const int BUFSIZE=4096;
      char buf[BUFSIZE];
      while (true) {
        int r=fread(buf, 1, BUFSIZE, in);
        sb.write(buf, r);
        i+=r;
        if (r==0 || sb.size()+BUFSIZE>blocksize) {
          string filename="";
          string comment="";
          if (i==sb.size()) {  // first block?
            filename=rename(p->first);
            comment=itos(p->second.date);
            if ((p->second.attr&255)>0) {
              comment+=" ";
              comment+=char(p->second.attr&255);
              comment+=itos(p->second.attr>>8);
            }
          }
          total_done+=sb.size();
          job.write(sb, filename.c_str(), method, comment.c_str());
          assert(sb.size()==0);
        }
        if (r==0) break;
      }
      fclose(in);
    }

    // Wait for jobs to finish
    job.write(sb, 0, "");  // signal end of input
    for (unsigned i=0; i<tid.size(); ++i) join(tid[i]);
    join(wid);

    // Done
    const int64_t outsize=out.tell();
    printf("%1.0f + (%1.0f -> %1.0f) = %s\n",
        double(header_pos),
        double(total_size),
        double(outsize-header_pos),
        migliaia(outsize));
    out.close();
    return errors>0;
  }  // end if streaming
///fik
  // Adjust date to maintain sequential order
  if (ver.size() && ver.back().lastdate>=date) {
    const int64_t newdate=decimal_time(unix_time(ver.back().lastdate)+1);
    fflush(stdout);
    fprintf(stderr, "Warning: adjusting date from %s to %s\n",
      dateToString(date).c_str(), dateToString(newdate).c_str());
    assert(newdate>date);
    date=newdate;
  }

  // Build htinv for fast lookups of sha1 in ht
  HTIndex htinv(ht, ht.size()+(total_size>>(10+fragment))+vf.size());
  const unsigned htsize=ht.size();  // fragments at start of update

  // reserve space for the header block
  writeJidacHeader(&out, date, -1, htsize);
  const int64_t header_end=out.tell();

  // Compress until end of last file
  assert(method!="");
  StringBuffer sb(blocksize+4096-128);  // block to compress
  unsigned frags=0;    // number of fragments in sb
  unsigned redundancy=0;  // estimated bytes that can be compressed out of sb
  unsigned text=0;     // number of fragents containing text
  unsigned exe=0;      // number of fragments containing x86 (exe, dll)
  const int ON=4;      // number of order-1 tables to save
  unsigned char o1prev[ON*256]={0};  // last ON order 1 predictions
  libzpaq::Array<char> fragbuf(MAX_FRAGMENT);
  vector<unsigned> blocklist;  // list of starting fragments
 

 // For each file to be added
  for (unsigned fi=0; fi<=vf.size(); ++fi) 
  {

        
	FP in=FPNULL;
    const int BUFSIZE=4096;  // input buffer
    char buf[BUFSIZE];
    int bufptr=0, buflen=0;  // read pointer and limit
    if (fi<vf.size()) {
      assert(vf[fi]->second.ptr.size()==0);
      DTMap::iterator p=vf[fi];
	
		///printf("lavoro %s\n",p->first.c_str());

      // Open input file
      bufptr=buflen=0;
      in=fopen(p->first.c_str(), RB);
	   
      if (in==FPNULL) {  // skip if not found
	  p->second.date=0;
        total_size-=p->second.size;
        printerr("16672",p->first.c_str());
        ++errors;
        continue;
      }
	  
      p->second.data=1;  // add
    }

    // Read fragments
    int64_t fsize=0;  // file size after dedupe
    for (unsigned fj=0; true; ++fj) {
      int64_t sz=0;  // fragment size;
      unsigned hits=0;  // correct prediction count
      int c=EOF;  // current byte
      unsigned htptr=0;  // fragment index
      char sha1result[20]={0};  // fragment hash
      unsigned char o1[256]={0};  // order 1 context -> predicted byte
      if (fi<vf.size()) {
        int c1=0;  // previous byte
        unsigned h=0;  // rolling hash for finding fragment boundaries
        libzpaq::SHA1 sha1;
        assert(in!=FPNULL);
        while (true) {
          if (bufptr>=buflen) bufptr=0, buflen=fread(buf, 1, BUFSIZE, in);
          if (bufptr>=buflen) c=EOF;
          else c=(unsigned char)buf[bufptr++];
          if (c!=EOF) {
            if (c==o1[c1]) h=(h+c+1)*314159265u, ++hits;
            else h=(h+c+1)*271828182u;
            o1[c1]=c;
            c1=c;
            sha1.put(c);
            fragbuf[sz++]=c;
          }
          if (c==EOF
              || sz>=MAX_FRAGMENT
              || (fragment<=22 && h<(1u<<(22-fragment)) && sz>=MIN_FRAGMENT))
            break;
        }
        assert(sz<=MAX_FRAGMENT);
        total_done+=sz;

        // Look for matching fragment
        assert(uint64_t(sz)==sha1.usize());
        memcpy(sha1result, sha1.result(), 20);
        htptr=htinv.find(sha1result);
      }  // end if fi<vf.size()

/// OK, lets RE-compute CRC-32 of the fragment, and store
/// used for debug
        uint32_t crc;
		crc=crc32_16bytes (&fragbuf[0],(uint32_t) sz);
		if (htptr)
		{
	///		printf("\nHTPTR  %ld size %ld\n",htptr,sz);
			ht[htptr].crc32=crc;
			ht[htptr].crc32size=sz;
		}
	


      if (htptr==0) {  // not matched or last block

        // Analyze fragment for redundancy, x86, text.
        // Test for text: letters, digits, '.' and ',' followed by spaces
        //   and no invalid UTF-8.
        // Test for exe: 139 (mov reg, r/m) in lots of contexts.
        // 4 tests for redundancy, measured as hits/sz. Take the highest of:
        //   1. Successful prediction count in o1.
        //   2. Non-uniform distribution in o1 (counted in o2).
        //   3. Fraction of zeros in o1 (bytes never seen).
        //   4. Fraction of matches between o1 and previous o1 (o1prev).
        int text1=0, exe1=0;
        int64_t h1=sz;
        unsigned char o1ct[256]={0};  // counts of bytes in o1
        static const unsigned char dt[256]={  // 32768/((i+1)*204)
          160,80,53,40,32,26,22,20,17,16,14,13,12,11,10,10,
            9, 8, 8, 8, 7, 7, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5,
            4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
        for (int i=0; i<256; ++i) {
          if (o1ct[o1[i]]<255) h1-=(sz*dt[o1ct[o1[i]]++])>>15;
          if (o1[i]==' ' && (isalnum(i) || i=='.' || i==',')) ++text1;
          if (o1[i] && (i<9 || i==11 || i==12 || (i>=14 && i<=31) || i>=240))
            --text1;
          if (i>=192 && i<240 && o1[i] && (o1[i]<128 || o1[i]>=192))
            --text1;
          if (o1[i]==139) ++exe1;
        }
        text1=(text1>=3);
        exe1=(exe1>=5);
        if (sz>0) h1=h1*h1/sz; // Test 2: near 0 if random.
        unsigned h2=h1;
        if (h2>hits) hits=h2;
        h2=o1ct[0]*sz/256;  // Test 3: bytes never seen or that predict 0.
        if (h2>hits) hits=h2;
        h2=0;
        for (int i=0; i<256*ON; ++i)  // Test 4: compare to previous o1.
          h2+=o1prev[i]==o1[i&255];
        h2=h2*sz/(256*ON);
        if (h2>hits) hits=h2;
        if (hits>sz) hits=sz;

        // Start a new block if the current block is almost full, or at
        // the start of a file that won't fit or doesn't share mutual
        // information with the current block, or last file.
        bool newblock=false;
        if (frags>0 && fj==0 && fi<vf.size()) {
          const int64_t esize=vf[fi]->second.size;
          const int64_t newsize=sb.size()+esize+(esize>>14)+4096+frags*4;
          if (newsize>blocksize/4 && redundancy<sb.size()/128) newblock=true;
          if (newblock) {  // test for mutual information
            unsigned ct=0;
            for (unsigned i=0; i<256*ON; ++i)
              if (o1prev[i] && o1prev[i]==o1[i&255]) ++ct;
            if (ct>ON*2) newblock=false;
          }
          if (newsize>=blocksize) newblock=true;  // won't fit?
        }
        if (sb.size()+sz+80+frags*4>=blocksize) newblock=true; // full?
        if (fi==vf.size()) newblock=true;  // last file?
        if (frags<1) newblock=false;  // block is empty?

        // Pad sb with fragment size list, then compress
        if (newblock) {
          assert(frags>0);
          assert(frags<ht.size());
          for (unsigned i=ht.size()-frags; i<ht.size(); ++i)
            puti(sb, ht[i].usize, 4);  // list of frag sizes
          puti(sb, 0, 4); // omit first frag ID to make block movable
          puti(sb, frags, 4);  // number of frags
          string m=method;
          if (isdigit(method[0]))
            m+=","+itos(redundancy/(sb.size()/256+1))
                 +","+itos((exe>frags)*2+(text>frags));
          string fn="jDC"+itos(date, 14)+"d"+itos(ht.size()-frags, 10);
          print_progress(total_size, total_done,g_scritti);

/*         
		 if (summary<=0)
            printf("[%u..%u] %u -method %s\n",
                unsigned(ht.size())-frags, unsigned(ht.size())-1,
                unsigned(sb.size()), m.c_str());
*/  
  if (method[0]!='i')
		  {
				
				
			job.write(sb, fn.c_str(), m.c_str());
		  }
          else {  // index: don't compress data
            job.csize.push_back(sb.size());
            sb.resize(0);
          }
          assert(sb.size()==0);
          blocklist.push_back(ht.size()-frags);  // mark block start
		  frags=redundancy=text=exe=0;
          memset(o1prev, 0, sizeof(o1prev));
        }

        // Append fragbuf to sb and update block statistics
        assert(sz==0 || fi<vf.size());
        sb.write(&fragbuf[0], sz);
		
        
        ++frags;
        redundancy+=hits;
        exe+=exe1*4;
        text+=text1*2;
        if (sz>=MIN_FRAGMENT) {
          memmove(o1prev, o1prev+256, 256*(ON-1));
          memcpy(o1prev+256*(ON-1), o1, 256);
        }
      }  // end if frag not matched or last block

      // Update HT and ptr list
      if (fi<vf.size()) {
        if (htptr==0) {
          htptr=ht.size();
          ht.push_back(HT(sha1result, sz));
          htinv.update();
          fsize+=sz;
        }
        vf[fi]->second.ptr.push_back(htptr);

///OK store the crc. Very dirty (to be fixed in future)
		ht[htptr].crc32=crc;
		ht[htptr].crc32size=sz;
	}
      if (c==EOF) break;
	///printf("Fragment fj %ld\n",fj);
    }  // end for each fragment fj
	
	
    if (fi<vf.size()) {
      dedupesize+=fsize;
      print_progress(total_size, total_done,g_scritti);
/*     
	 if (summary<=0) {
        string newname=rename(p->first.c_str());
        DTMap::iterator a=dt.find(newname);
  	if (a==dt.end() || a->second.date==0) printf("+ ");
        else printf("# ");
        printUTF8(p->first.c_str());
		
 
		if (newname!=p->first) {
          printf(" -> ");
          printUTF8(newname.c_str());
        }
        printf(" %1.0f", p->second.size+0.0);
        if (fsize!=p->second.size) printf(" -> %1.0f", fsize+0.0);
        printf("\n");
		
      }
	  */
      assert(in!=FPNULL);
      fclose(in);
      in=FPNULL;
    }
  }  // end for each file fi
  assert(sb.size()==0);

  // Wait for jobs to finish
  job.write(sb, 0, "");  // signal end of input
  for (unsigned i=0; i<tid.size(); ++i) join(tid[i]);
  join(wid);

  // Open index
  salt[0]^='7'^'z';
  OutputArchive outi(index ? index : "", password, salt, 0);
  WriterPair wp;
  wp.a=&out;
  if (index) wp.b=&outi;
  writeJidacHeader(&outi, date, 0, htsize);

  // Append compressed fragment tables to archive
  int64_t cdatasize=out.tell()-header_end;
  StringBuffer is;
  assert(blocklist.size()==job.csize.size());
  blocklist.push_back(ht.size());
  for (unsigned i=0; i<job.csize.size(); ++i) {
    if (blocklist[i]<blocklist[i+1]) {
      puti(is, job.csize[i], 4);  // compressed size of block
      for (unsigned j=blocklist[i]; j<blocklist[i+1]; ++j) {
        is.write((const char*)ht[j].sha1, 20);
        puti(is, ht[j].usize, 4);
      }
      libzpaq::compressBlock(&is, &wp, "0",
          ("jDC"+itos(date, 14)+"h"+itos(blocklist[i], 10)).c_str(),
          "jDC\x01");
      is.resize(0);
    }
  }

  // Delete from archive
  int dtcount=0;  // index block header name
  int removed=0;  // count
  for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) {
    if (p->second.date && !p->second.data) {
      puti(is, 0, 8);
      is.write(p->first.c_str(), strlen(p->first.c_str()));
      is.put(0);
	  /*
      if (summary<=0) {
        printf("- ");
        printUTF8(p->first.c_str());
        printf("\n");
      }
	  */
      ++removed;
      if (is.size()>16000) {
        libzpaq::compressBlock(&is, &wp, "1",
            ("jDC"+itos(date)+"i"+itos(++dtcount, 10)).c_str(), "jDC\x01");
        is.resize(0);
      }
    }
  }
/*
	printf("\n#########################\n");
	
		for (int i=0;i<ht.size();i++)
		{
			unsigned int crc32=ht[i].crc32;
			if (crc32==0)
				printf("ZERO %d\n",i);
		}
	printf("#########################\n");
	*/

	
  
  // Append compressed index to archive
  int added=0;  // count
  for (DTMap::iterator p=edt.begin();; ++p) 
  {
    if (p!=edt.end()) 
	{
		string filename=rename(p->first);
		
		if (searchfrom!="")
			if (replaceto!="")
				replace(filename,searchfrom,replaceto);
	

///	by using FRANZOFFSET we need to cut down the attr during this compare
	
 		DTMap::iterator a=dt.find(filename);
		if (p->second.date && (a==dt.end() // new file
         || a->second.date!=p->second.date  // date change
         || ((int32_t)a->second.attr && (int32_t)a->second.attr!=(int32_t)p->second.attr)  // attr ch. get less bits
         || a->second.size!=p->second.size  // size change
         || (p->second.data && a->second.ptr!=p->second.ptr))) 
		 { 


/*
			if (p->first==tempfile)
			{
				printf("FIX FILELIST\n");
				filename="filelist.txt:$DATA";
			}
*/	 
			uint32_t currentcrc32=0;
			for (unsigned i=0; i<p->second.ptr.size(); ++i)
					currentcrc32=crc32_combine(currentcrc32, ht[p->second.ptr[i]].crc32,ht[p->second.ptr[i]].crc32size);
		///	printf("!!!!!!!!!!!CRC32 %08X  <<%s>>\n",currentcrc32,p->first.c_str());
			
		 /*
				if (summary<=0 && p->second.data==0) 
				{  // not compressed?
					if (a==dt.end() || a->second.date==0) printf("+ ");
					else 
						printf("# ");
					
					printUTF8(p->first.c_str());
					if (filename!=p->first) 
					{
						printf(" -> ");
						printUTF8(filename.c_str());
					}
					printf("\n");
				}
			*/	
				++added;
				puti(is, p->second.date, 8);
				is.write(filename.c_str(), strlen(filename.c_str()));
				is.put(0);
				
				if ((p->second.attr&255)=='u') // unix attributes
					writefranzattr(is,p->second.attr,3,flagchecksum,filename,p->second.sha1hex,currentcrc32);
				else 
				if ((p->second.attr&255)=='w') // windows attributes
					writefranzattr(is,p->second.attr,5,flagchecksum,filename,p->second.sha1hex,currentcrc32);
				else 
					puti(is, 0, 4);  // no attributes
				
        if (a==dt.end() || p->second.data) a=p;  // use new frag pointers
        puti(is, a->second.ptr.size(), 4);  // list of frag pointers
        for (unsigned i=0; i<a->second.ptr.size(); ++i)
          puti(is, a->second.ptr[i], 4);
      }
    }
	else
	{
		if (versioncomment.length()>0)
		{
/// quickly store a fake file (for backward compatibility) with the version comment			
			///VCOMMENT 00000002 seconda_versione:$DATA
			char versioni8[9];
			sprintf(versioni8,"%08lld",(long long)ver.size());
			versioni8[8]=0x0;
			string fakefile="VCOMMENT "+string(versioni8)+" "+versioncomment+":$DATA"; //hidden windows file
			puti(is, 0, 8); // this is the "date". 0 is good, but do not pass paranoid compliance test. damn
			is.write(fakefile.c_str(), strlen(fakefile.c_str()));
			is.put(0);
		///	puti(is, 0, 4);  // no attributes
		///	puti(is, 0, 4);  // list of frag pointers
      
		}
	}
    if (is.size()>16000 || (is.size()>0 && p==edt.end())) {
      libzpaq::compressBlock(&is, &wp, "1",
          ("jDC"+itos(date)+"i"+itos(++dtcount, 10)).c_str(), "jDC\x01");
      is.resize(0);
    }
    if (p==edt.end()) break;
  }






  printf("\n%s +added, %s -removed.\n", migliaia(added), migliaia2(removed));
  assert(is.size()==0);

  // Back up and write the header
  outi.close();
  int64_t archive_end=out.tell();
  out.seek(header_pos, SEEK_SET);
  writeJidacHeader(&out, date, cdatasize, htsize);
  out.seek(0, SEEK_END);
  int64_t archive_size=out.tell();
  out.close();

  // Truncate empty update from archive (if not indexed)
  if (!index) {
    if (added+removed==0 && archive_end-header_pos==104) // no update
      archive_end=header_pos;
    if (archive_end<archive_size) {
      if (archive_end>0) {
        printf("truncating archive from %1.0f to %1.0f\n",
            double(archive_size), double(archive_end));
        if (truncate(arcname.c_str(), archive_end)) printerr("17092",archive.c_str());
      }
      else if (archive_end==0) {
        if (delete_file(arcname.c_str())) {
          printf("deleted ");
          printUTF8(arcname.c_str());
          printf("\n");
        }
      }
    }
  }
  fflush(stdout);
  fprintf(stderr, "\n%s + (%s -> %s -> %s) = %s\n",
      migliaia(header_pos), migliaia2(total_size), migliaia3(dedupesize),
      migliaia4(archive_end-header_pos), migliaia5(archive_end));
	  
	if (total_xls)
  		printf("Forced XLS has included %s bytes in %s files\n",migliaia(total_xls),migliaia2(file_xls));
#if defined(_WIN32) || defined(_WIN64)
	if (flagvss)
		vss_deleteshadows();
#endif
	
	/// late -test    
	if (flagtest)
	{
		printf("\nzpaqfranz: doing a full (with file verify) test\n");
		ver.clear();
		block.clear();
		dt.clear();
		ht.clear();
		edt.clear();
		ht.resize(1);  // element 0 not used
		ver.resize(1); // version 0
		dhsize=dcsize=0;
		///return test();
		return verify();
	}	
		//unz(archive.c_str(), password,all);
  return errors>0;
}

/////////////////////////////// extract ///////////////////////////////

// Return true if the internal file p
// and external file contents are equal or neither exists.
// If filename is 0 then return true if it is possible to compare.
// In the meantime calc the crc32 of the entire file

bool Jidac::equal(DTMap::const_iterator p, const char* filename,uint32_t &o_crc32) 
{
	o_crc32=0;

  // test if all fragment sizes and hashes exist
  if (filename==0) 
  {
    static const char zero[20]={0};
    for (unsigned i=0; i<p->second.ptr.size(); ++i) {
      unsigned j=p->second.ptr[i];
      if (j<1 || j>=ht.size()
          || ht[j].usize<0 || !memcmp(ht[j].sha1, zero, 20))
        return false;
    }
    return true;
  }

  // internal or neither file exists
  if (p->second.date==0) return !exists(filename);

  // directories always match
  if (p->first!="" && isdirectory(p->first))
    return exists(filename);

  // compare sizes
  FP in=fopen(filename, RB);
  if (in==FPNULL) return false;
  fseeko(in, 0, SEEK_END);
  if (ftello(in)!=p->second.size) 
	return fclose(in), false;



  // compare hashes chunk by chunk.
  fseeko(in, 0, SEEK_SET);
  libzpaq::SHA1 sha1;
  const int BUFSIZE=4096;
  char buf[BUFSIZE];

	if (!flagnoeta)
		if (flagverbose || (p->second.size>100000000)) //only 100MB+ files
			printf("Checking equality %s\r",migliaia(p->second.size));
	
	for (unsigned i=0; i<p->second.ptr.size(); ++i) 
	{
		unsigned f=p->second.ptr[i];
		if (f<1 || f>=ht.size() || ht[f].usize<0) 
			return fclose(in), false;
    
		for (int j=0; j<ht[f].usize;) 
		{
			int n=ht[f].usize-j;
			
			if (n>BUFSIZE) 
				n=BUFSIZE;
			
			int r=fread(buf, 1, n, in);
			o_crc32=crc32_16bytes (buf,r,o_crc32);
	
			g_worked+=r;
			
			if (r!=n) 
				return fclose(in), false;
			sha1.write(buf, n);
			j+=n;
		}
		if (memcmp(sha1.result(), ht[f].sha1, 20)!=0) 
			return fclose(in), false;
	}
	if (fread(buf, 1, BUFSIZE, in)!=0) 
		return fclose(in), false;
	fclose(in);
	return true;
}

// An extract job is a set of blocks with at least one file pointing to them.
// Blocks are extracted in separate threads, set READY -> WORKING.
// A block is extracted to memory up to the last fragment that has a file
// pointing to it. Then the checksums are verified. Then for each file
// pointing to the block, each of the fragments that it points to within
// the block are written in order.

struct ExtractJob {         // list of jobs
  Mutex mutex;              // protects state
  Mutex write_mutex;        // protects writing to disk
  int job;                  // number of jobs started
  Jidac& jd;                // what to extract
  FP outf;                  // currently open output file
  DTMap::iterator lastdt;   // currently open output file name
  double maxMemory;         // largest memory used by any block (test mode)
  int64_t total_size;       // bytes to extract
  int64_t total_done;       // bytes extracted so far
  ExtractJob(Jidac& j): job(0), jd(j), outf(FPNULL), lastdt(j.dt.end()),
      maxMemory(0), total_size(0), total_done(0) {
    init_mutex(mutex);
    init_mutex(write_mutex);
  }
  ~ExtractJob() {
    destroy_mutex(mutex);
    destroy_mutex(write_mutex);
  }
};

/*
bool direxists(const std::string& dirName_in)
{
#ifdef _WIN32
	std::wstring w=utow(dirName_in.c_str());  // Windows console: convert to UTF-16
	DWORD ftyp = GetFileAttributesW(w.c_str());
  ///DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
  if (ftyp == INVALID_FILE_ATTRIBUTES)
  {
	///printf("\n\n\nATTRIBUTO\n\n\n");
	return false;  //something is wrong with your path!
  }
  if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
    return true;   // this is a directory!
#endif
  return false;    // this is not a directory!
}
*/

// Decompress blocks in a job until none are READY
ThreadReturn decompressThread(void* arg) {
  ExtractJob& job=*(ExtractJob*)arg;
  int jobNumber=0;

  // Get job number
  lock(job.mutex);
  jobNumber=++job.job;
  release(job.mutex);

  ///printf("K1 %s\n",job.jd.archive.c_str());
  // Open archive for reading
  InputArchive in(job.jd.archive.c_str(), job.jd.password);
  if (!in.isopen()) return 0;
  StringBuffer out;

  // Look for next READY job.
  int next=0;  // current job
  while (true) {
    lock(job.mutex);
    for (unsigned i=0; i<=job.jd.block.size(); ++i) {
      unsigned k=i+next;
      if (k>=job.jd.block.size()) k-=job.jd.block.size();
      if (i==job.jd.block.size()) {  // no more jobs?
        release(job.mutex);
        return 0;
      }
      Block& b=job.jd.block[k];
      if (b.state==Block::READY && b.size>0 && b.usize>=0) {
        b.state=Block::WORKING;
        release(job.mutex);
        next=k;
        break;
      }
    }
    Block& b=job.jd.block[next];

    // Get uncompressed size of block
    unsigned output_size=0;  // minimum size to decompress
    assert(b.start>0);
    for (unsigned j=0; j<b.size; ++j) {
      assert(b.start+j<job.jd.ht.size());
      assert(job.jd.ht[b.start+j].usize>=0);
      output_size+=job.jd.ht[b.start+j].usize;
    }

    // Decompress
    double mem=0;  // how much memory used to decompress
    try {
      assert(b.start>0);
      assert(b.start<job.jd.ht.size());
      assert(b.size>0);
      assert(b.start+b.size<=job.jd.ht.size());
	  ///printf("Andiamo su offset  %lld  %lld\n",b.offset,output_size);
      in.seek(b.offset, SEEK_SET);
      libzpaq::Decompresser d;
      d.setInput(&in);
      out.resize(0);
      assert(b.usize>=0);
      assert(b.usize<=0xffffffffu);
      out.setLimit(b.usize);
      d.setOutput(&out);
      if (!d.findBlock(&mem)) error("archive block not found");
      if (mem>job.maxMemory) job.maxMemory=mem;
      while (d.findFilename()) {
        d.readComment();
        while (out.size()<output_size && d.decompress(1<<14))
		{
		//		printf("|");
		};
        lock(job.mutex);
///		printf("\n");
	///		printf("out size %lld\n",out.size());
	//	printf(".........................................................\n");
    ///	printf("---------------- %s  %s\n",migliaia(job.total_size),migliaia2(job.total_done));
		print_progress(job.total_size, job.total_done,-1);
        /*
		if (job.jd.summary<=0)
          printf("[%d..%d] -> %1.0f\n", b.start, b.start+b.size-1,
              out.size()+0.0);
      */
	  
	  release(job.mutex);
        if (out.size()>=output_size) break;
        d.readSegmentEnd();
      }
      if (out.size()<output_size) {
        lock(job.mutex);
        fflush(stdout);
        fprintf(stderr, "output [%u..%u] %d of %u bytes\n",
             b.start, b.start+b.size-1, int(out.size()), output_size);
        release(job.mutex);
        error("unexpected end of compressed data");
      }

      // Verify fragment checksums if present
      uint64_t q=0;  // fragment start
      libzpaq::SHA1 sha1;
      assert(b.extracted==0);
      for (unsigned j=b.start; j<b.start+b.size; ++j) {
        assert(j>0 && j<job.jd.ht.size());
        assert(job.jd.ht[j].usize>=0);
        assert(job.jd.ht[j].usize<=0x7fffffff);
        if (q+job.jd.ht[j].usize>out.size())
          error("Incomplete decompression");
        char sha1result[20];
        sha1.write(out.c_str()+q, job.jd.ht[j].usize);
        memcpy(sha1result, sha1.result(), 20);
        q+=job.jd.ht[j].usize;
        if (memcmp(sha1result, job.jd.ht[j].sha1, 20)) {
          lock(job.mutex);
          fflush(stdout);
          fprintf(stderr, "Job %d: fragment %u size %d checksum failed\n",
                 jobNumber, j, job.jd.ht[j].usize);
		g_exec_text="fragment checksum failed";
          release(job.mutex);
          error("bad checksum");
        }
        ++b.extracted;
      }
    }

    // If out of memory, let another thread try
    catch (std::bad_alloc& e) {
      lock(job.mutex);
      fflush(stdout);
      fprintf(stderr, "Job %d killed: %s\n", jobNumber, e.what());
	  g_exec_text="Job killed";
      b.state=Block::READY;
      b.extracted=0;
      out.resize(0);
      release(job.mutex);
      return 0;
    }

    // Other errors: assume bad input
    catch (std::exception& e) {
      lock(job.mutex);
      fflush(stdout);
      fprintf(stderr, "Job %d: skipping [%u..%u] at %1.0f: %s\n",
              jobNumber, b.start+b.extracted, b.start+b.size-1,
              b.offset+0.0, e.what());
      release(job.mutex);
      continue;
    }

    // Write the files in dt that point to this block
    lock(job.write_mutex);
    for (unsigned ip=0; ip<b.files.size(); ++ip) {
      DTMap::iterator p=b.files[ip];
      if (p->second.date==0 || p->second.data<0
          || p->second.data>=int64_t(p->second.ptr.size()))
        continue;  // don't write
		
      // Look for pointers to this block
      const vector<unsigned>& ptr=p->second.ptr;
      int64_t offset=0;  // write offset
      for (unsigned j=0; j<ptr.size(); ++j) {
        if (ptr[j]<b.start || ptr[j]>=b.start+b.extracted) {
          offset+=job.jd.ht[ptr[j]].usize;
          continue;
        }

        // Close last opened file if different
        if (p!=job.lastdt) {
          if (job.outf!=FPNULL) {
            assert(job.lastdt!=job.jd.dt.end());
            assert(job.lastdt->second.date);
            assert(job.lastdt->second.data
                   <int64_t(job.lastdt->second.ptr.size()));
            fclose(job.outf);
            job.outf=FPNULL;
          }
          job.lastdt=job.jd.dt.end();
        }

        // Open file for output
        if (job.lastdt==job.jd.dt.end()) 
		{
			string filename=job.jd.rename(p->first);
		  /*
			if (flagutf)
			{
				///filename=purgeansi(forcelatinansi(utf8toansi(filename)));
				if (flagdebug)
					printf("17535: purged filename to <<%s>>\n",filename.c_str());
			}
			*/
			assert(job.outf==FPNULL);
			if (p->second.data==0) 
			{
				if (!job.jd.flagtest) 
				{
					if (flagdebug)
					{
						printf("17544: MAKEPATH:");
						printUTF8(filename.c_str());
						printf("\n");
					}
					makepath(filename);
				}
				lock(job.mutex);
				print_progress(job.total_size, job.total_done,-1);
				release(job.mutex);
            
				if (!job.jd.flagtest) 
				{
					
/// sometimes (in Windows) the path is not made (ex. path too long)
/// let's try to make (is just a fix). Please note utw

#ifdef _WIN32
					string percorso=extractfilepath(filename);
					if (percorso!="")
					{
						myreplaceall(percorso,"/","\\");
						if (!direxists(percorso))
						{
							if (flagdebug)
								printf("17577: ERROR DIRNOTEX %s\n",percorso.c_str());
						
							string temppercorso=percorso;
							size_t barra;
							string percorsino;
				
							while (1==1)
							{
								barra=temppercorso.find('\\');
								if (barra==string::npos)
									break;
								percorsino+=temppercorso.substr(0, barra)+'\\';
								if (direxists(percorsino))
								{
									if (flagdebug)
										printf("17583: Small path exists %s\n",percorsino.c_str());
								}
								else
								{
									string dafare=percorsino;
									///dafare.pop_back();
									if (flagdebug)
										printf("\n\n17950: Make path of %03d\n---\n%s\n---\n",(int)dafare.length(),dafare.c_str());
									bool creazione=CreateDirectory(utow(dafare.c_str()).c_str(), 0);
									if (!creazione)
										printerr("17519",dafare.c_str());
								}
								temppercorso=temppercorso.substr(barra+1,temppercorso.length());
							}
						}
						if (!direxists(percorso))
						{
							printf("17620: percorso does not exists <<%s>>\n",percorso.c_str());
				///exit(0);
						}
					}
#endif				

		///		filename=nomefileseesistegia(filename);
				/*
				if (fileexists(filename))
				{
						printf("KKKKKKKKKKKKKKOOOOOOOOOOOOOOOOOOOOOOOOOOOOO %s\n",filename.c_str());
						printf("Dim %08d\n\n",prendidimensionefile(filename.c_str()));
						
				}
				*/
				job.outf=fopen(filename.c_str(), WB);
				if (job.outf==FPNULL) 
				{
					lock(job.mutex);
					printerr("17451",filename.c_str());
					release(job.mutex);
				}
#ifndef unix
              else if ((p->second.attr&0x200ff)==0x20000+'w') {  // sparse?
                DWORD br=0;
                if (!DeviceIoControl(job.outf, FSCTL_SET_SPARSE,
                    NULL, 0, NULL, 0, &br, NULL))  // set sparse attribute
                  printerr("17459",filename.c_str());
              }
#endif
            }
          }
          else if (!job.jd.flagtest)
		  {
		///	printf("Scriverei su %s\n",filename.c_str());
			job.outf=fopen(filename.c_str(), RBPLUS);  // update existing file
          }
		  if (!job.jd.flagtest && job.outf==FPNULL) break;  // skip errors
          job.lastdt=p;
          assert(job.jd.flagtest || job.outf!=FPNULL);
        }
        assert(job.lastdt==p);

        // Find block offset of fragment
        uint64_t q=0;  // fragment offset from start of block
        for (unsigned k=b.start; k<ptr[j]; ++k) {
          assert(k>0);
          assert(k<job.jd.ht.size());
          if (job.jd.ht[k].usize<0) error("streaming fragment in file");
          assert(job.jd.ht[k].usize>=0);
          q+=job.jd.ht[k].usize;
        }
        assert(q+job.jd.ht[ptr[j]].usize<=out.size());

        // Combine consecutive fragments into a single write
        assert(offset>=0);
        ++p->second.data;
        uint64_t usize=job.jd.ht[ptr[j]].usize;
        assert(usize<=0x7fffffff);
        assert(b.start+b.size<=job.jd.ht.size());
        while (j+1<ptr.size() && ptr[j+1]==ptr[j]+1
               && ptr[j+1]<b.start+b.size
               && job.jd.ht[ptr[j+1]].usize>=0
               && usize+job.jd.ht[ptr[j+1]].usize<=0x7fffffff) {
          ++p->second.data;
          assert(p->second.data<=int64_t(ptr.size()));
          assert(job.jd.ht[ptr[j+1]].usize>=0);
          usize+=job.jd.ht[ptr[++j]].usize;
        }
        assert(usize<=0x7fffffff);
        assert(q+usize<=out.size());

        // Write the merged fragment unless they are all zeros and it
        // does not include the last fragment.
        uint64_t nz=q;  // first nonzero byte in fragments to be written
        while (nz<q+usize && out.c_str()[nz]==0) ++nz;
		
		
        if ((nz<q+usize || j+1==ptr.size())) 

			//if (stristr(job.lastdt->first.c_str(),"Globals.pas"))
			//if (stristr(job.lastdt->first.c_str(),"globals.pas"))
			{

/// let's calc the CRC32 of the block, and store (keyed by filename)
/// in comments debug code
				if (offset>g_scritti)
					g_scritti=offset;
					
				uint32_t crc;
				crc=crc32_16bytes (out.c_str()+q, usize);
	
				s_crc32block myblock;
				myblock.crc32=crc;
				myblock.crc32start=offset;
				myblock.crc32size=usize;
				myblock.filename=job.lastdt->first;
				g_crc32.push_back(myblock);
				/*
				char mynomefile[100];
				sprintf(mynomefile,"z:\\globals_%014lld_%014lld_%08X",offset,offset+usize,crc);
				///printf("Outfile %s %lld Seek to %08lld   size %08lld crc %08lld %s\n",mynomefile,job.outf,offset,usize,crc,job.lastdt->first.c_str());

				FILE* myfile=fopen(mynomefile, "wb");
				fwrite(out.c_str()+q, 1, usize, myfile);
				
				fclose(myfile);
				*/
			}

/// with -kill do not write
		if (!flagkill)
			if (!job.jd.flagtest && (nz<q+usize || j+1==ptr.size())) 
			{
				if (flagdebug)
					printf("17716:OFFSETTONEWRITE   %s  size  %s \n",migliaia(offset),migliaia2(usize));
				fseeko(job.outf, offset, SEEK_SET);
				fwrite(out.c_str()+q, 1, usize, job.outf);
			}

        offset+=usize;
        lock(job.mutex);
        job.total_done+=usize;
        release(job.mutex);
		///printf("Scrittoni %f\n",(double)job.total_done);
		
        // Close file. If this is the last fragment then set date and attr.
        // Do not set read-only attribute in Windows yet.
        if (p->second.data==int64_t(ptr.size())) {
          assert(p->second.date);
          assert(job.lastdt!=job.jd.dt.end());
          assert(job.jd.flagtest || job.outf!=FPNULL);
          if (!job.jd.flagtest) {
            assert(job.outf!=FPNULL);
            string fn=job.jd.rename(p->first);
            int64_t attr=p->second.attr;
            int64_t date=p->second.date;
            if ((p->second.attr&0x1ff)=='w'+256) attr=0;  // read-only?
            if (p->second.data!=int64_t(p->second.ptr.size()))
              date=attr=0;  // not last frag
            close(fn.c_str(), date, attr, job.outf);
            job.outf=FPNULL;
          }
          job.lastdt=job.jd.dt.end();
        }
      } // end for j
    } // end for ip

    // Last file
    release(job.write_mutex);
  } // end while true

  
	
  // Last block
  return 0;
}


// Streaming output destination
struct OutputFile: public libzpaq::Writer {
  FP f;
  void put(int c) {
    char ch=c;
    if (f!=FPNULL) fwrite(&ch, 1, 1, f);
  }
  void write(const char* buf, int n) {if (f!=FPNULL) fwrite(buf, 1, n, f);}
  OutputFile(FP out=FPNULL): f(out) {}
};

// Copy at most n bytes from in to out (default all). Return how many copied.
int64_t copy(libzpaq::Reader& in, libzpaq::Writer& out, uint64_t n=~0ull) {
  const unsigned BUFSIZE=4096;
  int64_t result=0;
  char buf[BUFSIZE];
  while (n>0) {
    int nc=n>BUFSIZE ? BUFSIZE : n;
    int nr=in.read(buf, nc);
    if (nr<1) break;
    out.write(buf, nr);
    result+=nr;
    n-=nr;
  }
  return result;
}






string sanitizzanomefile(string i_filename,int i_filelength,int& io_collisioni,MAPPAFILEHASH& io_mappacollisioni)
{
	if  (i_filename=="")
		return("");
		
	string percorso			=extractfilepath(i_filename);
	string nome				=prendinomefileebasta(i_filename);
				
	string estensione		=prendiestensione(i_filename);
	string senzaestensione	=percorso+nome;
	string newname;
		

	int lunghezza=FRANZMAXPATH;
	if (i_filelength>0)
		if (i_filelength<FRANZMAXPATH)
			lunghezza=i_filelength;
	lunghezza-=9; // antidupe
	if (lunghezza<10)
		lunghezza=10;
	char numero[60];
						
	if (flagflat) /// desperate extract without path
	{
		sprintf(numero,"%08d_%05d_",++io_collisioni,(unsigned int)i_filename.length());
		newname=numero;
		string temp=purgeansi(senzaestensione.substr(0, lunghezza));
		for (int i=0;i<10;i++)
			myreplaceall(temp,"  "," ");
		
		newname+=temp;
		
		if (estensione!="")
			newname+="."+estensione;
		if (flagdebug)
			printf("25396: flatted filename <<%s>>\n",newname.c_str());
		return newname;
	}
	
	if (flagutf)
	{
		string prenome=nome;

// this is name, throw everything (FALSE)
		nome=purgeansi(forcelatinansi(utf8toansi(nome)),false);		
		if (flagdebug)
			if (nome!=prenome)
			{
				printf("25410: flagutf pre  %s\n",prenome.c_str());
				printf("25410: utf8toansi   %s\n",utf8toansi(nome).c_str());
				printf("25410: force2ansi   %s\n",forcelatinansi(utf8toansi(nome)).c_str());
				printf("25411: purgeansi    %s\n",nome.c_str());
			}

		if (flagfixeml)
		{
			if (estensione=="eml")
			{
				prenome=compressemlfilename(nome);
				if (nome!=prenome)
				{
					if (flagdebug)
					{
						printf("18109: eml pre  %s\n",nome.c_str());
						printf("18110: eml post %s\n",prenome.c_str());
					}
									
					nome=prenome;
				}
			}
			else
			{
				for (int i=0;i<10;i++)
					myreplaceall(newname,"  "," ");
					
			}
		}

/// this is a path, so keep \ and / (TRUE)
		string prepercorso=percorso;
		percorso=purgeansi(forcelatinansi(utf8toansi(percorso)),true);		
		if (flagdebug)
			if (percorso!=prepercorso)
			{
				printf("25452: flagutf pre  perc %s\n",prepercorso.c_str());
				printf("25453: flagutf post perc %s\n",percorso.c_str());
			}
	}
					
	if (flagdebug)
	{
		printf("18041: First    %03d %s\n",(int)i_filename.length(),i_filename.c_str());
		printf("18042: Percorso %03d %s\n",(int)percorso.length(),percorso.c_str());
		printf("18043: nome     %03d %s\n",(int)nome.length(),nome.c_str());
		printf("18044: ext      %03d %s\n",(int)estensione.length(),estensione.c_str());
		printf("18045: Senza ex %03d %s\n",(int)senzaestensione.length(),senzaestensione.c_str());
	}

	if (flagfix255)
	{
		int	lunghezzalibera=lunghezza-percorso.length();//%08d_
		if (lunghezzalibera<10)
		{
			if (flagdebug)
			{
				printf("\n\n\n18046: Path too long: need shrink %08d %s\n",(int)percorso.length(),percorso.c_str());
				printf("lunghezzalibera %d\n",lunghezzalibera);
			}
			vector<string> esploso;
						
			string temppercorso=percorso;
			size_t barra;
						
			while (1==1)
			{
				if (flagdebug)
					printf("18031: temppercorso %s\n",temppercorso.c_str());
				barra=temppercorso.find('/');
				if (flagdebug)
					printf("18034: Barra %ld\n",(long int)barra);
				if (barra==string::npos)
					break;
				if (flagdebug)		
					printf("18038: Eureka!!\n");
				esploso.push_back(temppercorso.substr(0, barra));
				temppercorso=temppercorso.substr(barra+1,temppercorso.length());
			}
		
			int lunghezzamassima=0;
			int indicelunghezzamassima=-1;
			for (unsigned int i=0;i<esploso.size();i++)
			{
				if ((int)esploso[i].length()>lunghezzamassima)
				{
					indicelunghezzamassima=i;
					lunghezzamassima=esploso[i].length();
				}
				if (flagdebug)
					printf("18087: Esploso %d %03d %s\n",(int)i,(int)esploso[i].length(),esploso[i].c_str());
			}
						/*
						printf("Lunghezza massima %d\n",lunghezzamassima);
						printf("Indice lunghezza massima %d\n",indicelunghezzamassima);
						
						printf("Lunghezza          %d\n",lunghezza);
						printf("Lunghezza percorso %d %s\n",percorso.length(),percorso.c_str());
						printf("Lunghezza nome     %d %s\n",nome.length(),nome.c_str());
						printf("Lunghezza este     %d %s\n",estensione.length(),estensione.c_str());
						
						printf("Resto percoros     %d\n",percorso.length()-lunghezzamassima);
						*/
						
			int lunghezzacheserve=lunghezza-10-(percorso.length()-lunghezzamassima);
					///	printf("Lunghezza che ser  %d\n",lunghezzacheserve);
						
			if (lunghezzacheserve<lunghezzamassima)
			{
				esploso[indicelunghezzamassima]=esploso[indicelunghezzamassima].substr(0,lunghezzacheserve);
				string imploso;
				for (unsigned 	int i=0;i<esploso.size();i++)
					imploso+=esploso[i]+'/';
				if (flagdebug)
					printf("18114: Imploso           %d %s\n",(int)imploso.length(),imploso.c_str());
				percorso=imploso;
				lunghezzalibera=lunghezza-percorso.length();
				makepath(percorso);
			}
			else
			{
				printf("18088: HOUSTON\n");
			}						
		}
	}
	newname=nome;
	int lunghezzalibera=lunghezza-percorso.length();
					
	if (flagdebug)
		printf("18098:lunghezze per %03d nome %03d tot %03d\n",(int)percorso.length(),lunghezzalibera,(int)(percorso.length()+lunghezzalibera));
					
	if (newname.length()>(unsigned int)lunghezzalibera)
	{
		newname=newname.substr(0,lunghezzalibera-9);
		if (flagdebug)
			printf("18143:Trimmone newname %d %s\n",(int)newname.length(),newname.c_str());
	}
	
	newname=percorso+newname;
	
	if (flagdebug)
		printf("%d: newname %s\n",__LINE__,newname.c_str());
					
	std::map<string,string>::iterator collisione;
	string candidato=newname;
	if (estensione!="")
		candidato=candidato+'.'+estensione;
	if (flagfix255)	/// we are on windows, take care of case
	{
		std::for_each(candidato.begin(), candidato.end(), [](char & c)
		{
			c = ::tolower(c);	
		});
	}
	if (flagdebug)
	printf("25570: candidato %s\n",candidato.c_str());
	
	collisione=io_mappacollisioni.find(candidato); 
	if (collisione!=io_mappacollisioni.end()) 
	{
		if (flagdebug)
			printf("18255 found  1 %s\n",candidato.c_str());
		if (collisione->second!=candidato)
		{
			if (flagdebug)
			{
				printf("25582: Collisione %s\n",collisione->second.c_str());
				printf("25583: newname    %s\n",collisione->second.c_str());
			}
			sprintf(numero,"_%d",io_collisioni++);
			newname+=numero;
			if (flagdebug)
				printf("18267: postname   %s\n\n\n\n\n",newname.c_str());
		}
	}
	io_mappacollisioni.insert(std::pair<string, string>(candidato,i_filename));
	
	if (estensione!="")
		newname+="."+estensione;
	
	if (flagdebug)
		printf("18195: Finalized %d %s\n",(int)newname.length(),newname.c_str());
					
	if (newname.length()>255)
	{
		printf("18123: WARN pre  %08d   %s\n",(int)i_filename.length(),i_filename.c_str());
		printf("18124: WARN post %08d   %s\n",(int)newname.length(),newname.c_str());
		printf("\n");
	}
					

	/*	
	auto ret=mymap.insert( std::pair<string,DT>(newname,p->second) );
	if (ret.second==false) 
	{
		printf("18298: KOLLISION! %s\n",newname.c_str());
	}
	*/
	return newname;
}


void Jidac::printsanitizeflags()
{
		printf("\n");
		printf("******\n");
		if (flagflat)
			printf("****** WARNING: all files FLATted, without non-latin chars, max %d length\n",(int)FRANZMAXPATH);
		else
		{
			if (flagutf)
				printf("****** -utf    No Non-latin chars\n");
			if (flagfix255)
				printf("****** -fix255 Shrink filenames to %d, case insensitive\n",(int)FRANZMAXPATH);
			if (flagfixeml)
				printf("****** -fixeml Heuristic compress .eml filenames (Fwd Fwd Fwd=>Fwd etc)\n");
		}
		printf("******\n\n");
}


// Extract files from archive. If force is true then overwrite
// existing files and set the dates and attributes of exising directories.
// Otherwise create only new files and directories. Return 1 if error else 0.
int Jidac::extract() 
{

/*
	-kill 
	-force
	-comment / versioncomment
	-flat
	-utf -filelenght -dirlength
*/
	if (flagkill)
	{
		if (flagforce)
		{
			printf("-kill incompatible with -force\n");
			return 2;
		}
		printf("\n");
		printf("******\n");
		printf("****** WARNING: -kill switch. Create 0 bytes files (NO data written)\n");
		printf("****** Full-scale extraction test (UTF-8 strange filenames, path too long...)\n");
		printf("****** Highly suggested output on RAMDISK\n");
		printf("******\n\n");
	}
	


	g_scritti=0;
  // Encrypt or decrypt whole archive
  if (repack && all) {
    if (files.size()>0 || tofiles.size()>0 || onlyfiles.size()>0
        || flagnoattributes || version!=DEFAULT_VERSION || method!="")
      error("-repack -all does not allow partial copy");
    InputArchive in(archive.c_str(), password);
    if (flagforce) delete_file(repack);
    if (exists(repack)) error("output file exists");

    // Get key and salt
    char salt[32]={0};
    if (new_password) libzpaq::random(salt, 32);

    // Copy
    OutputArchive out(repack, new_password, salt, 0);
    copy(in, out);
    printUTF8(archive.c_str());
    printf(" %1.0f ", in.tell()+.0);
    printUTF8(repack);
    printf(" -> %1.0f\n", out.tell()+.0);
    out.close();
    return 0;
  }
  
///printf("ZA\n");

	int64_t sz=read_archive(archive.c_str());
	if (sz<1) error("archive not found");
///printf("ZB\n");

	if (flagcomment)
		if (versioncomment.length()>0)
		{
			// try to find the comment, but be careful: 1, and only 1, allowed
			vector<DTMap::iterator> myfilelist;
			int versione=searchcomments(versioncomment,myfilelist);
			if (versione>0)
			{
				printf("Found version -until %d scanning again...\n",versione);
				version=versione;
	
				ver.clear();
				block.clear();
				dt.clear();
				ht.clear();
				ht.resize(1);  // element 0 not used
				ver.resize(1); // version 0
				dhsize=dcsize=0;
				sz=read_archive(archive.c_str());
			}
			else
			if (versione==0)
				error("Cannot find version comment");
			else
				error("Multiple match for version comment. Please use -until");
		}
///printf("ZC\n");

  // test blocks
  for (unsigned i=0; i<block.size(); ++i) {
    if (block[i].bsize<0) error("negative block size");
    if (block[i].start<1) error("block starts at fragment 0");
    if (block[i].start>=ht.size()) error("block start too high");
    if (i>0 && block[i].start<block[i-1].start) error("unordered frags");
    if (i>0 && block[i].start==block[i-1].start) error("empty block");
    if (i>0 && block[i].offset<block[i-1].offset+block[i-1].bsize)
      error("unordered blocks");
    if (i>0 && block[i-1].offset+block[i-1].bsize>block[i].offset)
      error("overlapping blocks");
  }
///printf("ZD\n");

  // Create index instead of extract files
  if (index) {
    if (ver.size()<2) error("no journaling data");
    if (flagforce) delete_file(index);
    if (exists(index)) error("index file exists");

    // Get salt
    char salt[32];
    if (ver[1].offset==32) {  // encrypted?
      FP fp=fopen(subpart(archive, 1).c_str(), RB);
      if (fp==FPNULL) error("cannot read part 1");
      if (fread(salt, 1, 32, fp)!=32) error("cannot read salt");
      salt[0]^='7'^'z';  // for index
      fclose(fp);
    }
    InputArchive in(archive.c_str(), password);
    OutputArchive out(index, password, salt, 0);
    for (unsigned i=1; i<ver.size(); ++i) {
      if (in.tell()!=ver[i].offset) error("I'm lost");

      // Read C block. Assume uncompressed and hash is present
      static char hdr[256]={0};  // Read C block
      int hsize=ver[i].data_offset-ver[i].offset;
      if (hsize<70 || hsize>255) error("bad C block size");
      if (in.read(hdr, hsize)!=hsize) error("EOF in header");
      if (hdr[hsize-36]!=9  // size of uncompressed block low byte
          || (hdr[hsize-22]&255)!=253  // start of SHA1 marker
          || (hdr[hsize-1]&255)!=255) {  // end of block marker
        for (int j=0; j<hsize; ++j)
          printf("%d%c", hdr[j]&255, j%10==9 ? '\n' : ' ');
        printf("at %1.0f\n", ver[i].offset+.0);
        error("C block in weird format");
      }
      memcpy(hdr+hsize-34, 
          "\x00\x00\x00\x00\x00\x00\x00\x00"  // csize = 0
          "\x00\x00\x00\x00"  // compressed data terminator
          "\xfd"  // start of hash marker
          "\x05\xfe\x40\x57\x53\x16\x6f\x12\x55\x59\xe7\xc9\xac\x55\x86"
          "\x54\xf1\x07\xc7\xe9"  // SHA-1('0'*8)
          "\xff", 34);  // EOB
      out.write(hdr, hsize);
      in.seek(ver[i].csize, SEEK_CUR);  // skip D blocks
      int64_t end=sz;
      if (i+1<ver.size()) end=ver[i+1].offset;
      int64_t n=end-in.tell();
      if (copy(in, out, n)!=n) error("EOF");  // copy H and I blocks
    }
    printUTF8(index);
    printf(" -> %1.0f\n", out.tell()+.0);
    out.close();
    return 0;
  }

///printf("ZE\n");

  // Label files to extract with data=0.
  // Skip existing output files. If force then skip only if equal
  // and set date and attributes.
  ExtractJob job(*this);
  int total_files=0, skipped=0;
  int tobeerased=0,erased=0;
  
  uint32_t crc32fromfile;
	int kollision=0;

	if ((flagutf) || (flagflat) || flagfix255)
	{
/// we make a copy of the map (dt) into mymap, changing the filenames
/// wy do not in rename()? Because we need very dirty string manipulations

		printsanitizeflags();
		

		int64_t filesbefore=0;
		int64_t dirsbefore=0;
		
		if (flagdebug)
			for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) 
			{
				if ((p->second.date && p->first!="") && (!isdirectory(p->first)))
					dirsbefore++;
				else
					filesbefore++;
			}
				
		DTMap mymap;
		int kollisioni=0;
		MAPPAFILEHASH mappacollisioni;
		for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) 
		{
			string newname=sanitizzanomefile(p->first,filelength,kollisioni,mappacollisioni);
			auto ret=mymap.insert( std::pair<string,DT>(newname,p->second) );
			if (ret.second==false) 
				printf("18298: KOLLISION! %s\n",newname.c_str());
		}
		
		dt=mymap;
		
		if (flagdebug)
		{
			int64_t filesafter=0;
			int64_t dirsafter=0;
	
			for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) 
			{
				if ((p->second.date && p->first!="") && (!isdirectory(p->first)))
					dirsafter++;
				else
					filesafter++;
			}
			
			printf("18181:size  before %12s\n",migliaia(dt.size()));
			printf("17995:Files before %12s\n",migliaia(filesbefore));
			printf("17996:dirs  before %12s\n",migliaia(dirsbefore));
			printf("\n\n");
			printf("18181:size  after  %12s\n",migliaia(dt.size()));
			printf("17995:Files before %12s\n",migliaia(filesafter));
			printf("17996:dirs  before %12s\n",migliaia(dirsafter));
		}
	///if (flagdebug)
	}
	
#ifdef _WIN32
	for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) 
		if ((p->second.date && p->first!=""))
			if (p->first.length()>254)
			{
				printf("WARN: path too long %03d ",(int)p->first.length());
				printUTF8(p->first.c_str());
				printf("\n");
			}
#endif
	
	for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) 
	{
		p->second.data=-1;  // skip
	    
		if (p->second.date && p->first!="") 
		{
			string fn=rename(p->first);
			const bool isdir=isdirectory(p->first);
	  	  
			if (!repack && !flagtest && flagforce && !isdir && equal(p, fn.c_str(),crc32fromfile)) 
			{
				// identical
				if (flagverbose)
				{
					printf("= ");
					printUTF8(fn.c_str());
					printf("\n");
				}
				close(fn.c_str(), p->second.date, p->second.attr);
				++skipped;
			}
			else 
			if (!repack && !flagtest && !flagforce && exists(fn)) 
			{  
				// exists, skip
				if (flagverbose)
				{
					printf("? ");
					printUTF8(fn.c_str());
					printf("\n");
				}
				++skipped;
			}
			else 
			if (isdir)  // update directories later
				p->second.data=0;
			else 
			if (block.size()>0) 
			{  // files to decompress
				p->second.data=0;
				unsigned lo=0, hi=block.size()-1;  // block indexes for binary search
				for (unsigned i=0; p->second.data>=0 && i<p->second.ptr.size(); ++i) 
				{
					unsigned j=p->second.ptr[i];  // fragment index
					if (j==0 || j>=ht.size() || ht[j].usize<-1) 
					{
						fflush(stdout);
						printUTF8(p->first.c_str(), stderr);
						fprintf(stderr, ": bad frag IDs, skipping...\n");
						p->second.data=-1;  // skip
						continue;
					}
					assert(j>0 && j<ht.size());
					if (lo!=hi || lo>=block.size() || j<block[lo].start
						|| (lo+1<block.size() && j>=block[lo+1].start)) 
					{
						lo=0;  // find block with fragment j by binary search
						hi=block.size()-1;
						while (lo<hi) 
						{
							unsigned mid=(lo+hi+1)/2;
							assert(mid>lo);
							assert(mid<=hi);
							if (j<block[mid].start) 
								hi=mid-1;
							else 
							(lo=mid);
						}
					}
					assert(lo==hi);
					assert(lo>=0 && lo<block.size());
					assert(j>=block[lo].start);
					assert(lo+1==block.size() || j<block[lo+1].start);
					unsigned c=j-block[lo].start+1;
					if (block[lo].size<c) block[lo].size=c;
					if (block[lo].files.size()==0 || block[lo].files.back()!=p)
						block[lo].files.push_back(p);
				}
				++total_files;
				job.total_size+=p->second.size;
				///printf("FACCIO QUALCOSA SU %s per size %d\n",fn.c_str(),p->second.size);
				if (fileexists(fn))
				{
					if (flagverbose)
					{
						printf("* ");
						printUTF8(fn.c_str());
						printf("\n");
					}
					tobeerased++;
					if (delete_file(fn.c_str()))
						erased++;
					else
					printf("************ HIGHLANDER FILE! %s\n",fn.c_str());
				}
				
			}
		}  // end if selected
	}  // end for

	if (!flagforce && skipped>0)
		printf("%08d ?existing files skipped (-force overwrites).\n", skipped);
	if (flagforce && skipped>0)
		printf("%08d =identical files skipped.\n", skipped);
	if (flagforce && tobeerased>0)
		printf("%08d !=different files to be owerwritten => erased %08d\n",tobeerased,erased);
	if (tobeerased!=erased)
		printf("**** GURU **** WE HAVE SOME HIGHLANDER!");
		
  // Repack to new archive
	if (repack) 
	{

    // Get total D block size
    if (ver.size()<2) error("cannot repack streaming archive");
    int64_t csize=0;  // total compressed size of D blocks
    for (unsigned i=0; i<block.size(); ++i) {
      if (block[i].bsize<1) error("empty block");
      if (block[i].size>0) csize+=block[i].bsize;
    }
	
    InputArchive in(archive.c_str(), password);

    // Open output
    if (!flagforce && exists(repack)) 
		error("repack output exists");
	
    delete_file(repack);
    char salt[32]={0};
    if (new_password) libzpaq::random(salt, 32);
    OutputArchive out(repack, new_password, salt, 0);
    int64_t cstart=out.tell();

    // Write C block using first version date
    writeJidacHeader(&out, ver[1].date, -1, 1);
    int64_t dstart=out.tell();
///printf("ZJ\n");

    // Copy only referenced D blocks. If method then recompress.
    for (unsigned i=0; i<block.size(); ++i) {
      if (block[i].size>0) {
        in.seek(block[i].offset, SEEK_SET);
        copy(in, out, block[i].bsize);
      }
    }
    printf("Data %1.0f -> ", csize+.0);
    csize=out.tell()-dstart;
    printf("%1.0f\n", csize+.0);

    // Re-create referenced H blocks using latest date
    for (unsigned i=0; i<block.size(); ++i) {
      if (block[i].size>0) {
        StringBuffer is;
        puti(is, block[i].bsize, 4);
        for (unsigned j=0; j<block[i].frags; ++j) {
          const unsigned k=block[i].start+j;
          if (k<1 || k>=ht.size()) error("frag out of range");
          is.write((const char*)ht[k].sha1, 20);
          puti(is, ht[k].usize, 4);
        }
        libzpaq::compressBlock(&is, &out, "0",
            ("jDC"+itos(ver.back().date, 14)+"h"
            +itos(block[i].start, 10)).c_str(),
            "jDC\x01");
      }
    }
///printf("ZK\n");

    // Append I blocks of selected files
    unsigned dtcount=0;
    StringBuffer is;
    for (DTMap::iterator p=dt.begin();; ++p) {
		///printf("ZK-1\n");

      if (p!=dt.end() && p->second.date>0 && p->second.data>=0) {
        string filename=rename(p->first);
        puti(is, p->second.date, 8);
        is.write(filename.c_str(), strlen(filename.c_str()));
        is.put(0);
		
		
		/// let's write, if necessary, the checksum, or leave the default ZPAQ if no -checksum
        if ((p->second.attr&255)=='u') // unix attributes
			writefranzattr(is,p->second.attr,3,flagchecksum,filename,p->second.sha1hex,2);
		else 
		if ((p->second.attr&255)=='w') // windows attributes
			writefranzattr(is,p->second.attr,5,flagchecksum,filename,p->second.sha1hex,2);
		else 
			puti(is, 0, 4);  // no attributes

        puti(is, p->second.ptr.size(), 4);  // list of frag pointers
        for (unsigned i=0; i<p->second.ptr.size(); ++i)
          puti(is, p->second.ptr[i], 4);
      }
      if (is.size()>16000 || (is.size()>0 && p==dt.end())) {
        libzpaq::compressBlock(&is, &out, "1",
            ("jDC"+itos(ver.back().date)+"i"+itos(++dtcount, 10)).c_str(),
            "jDC\x01");
        is.resize(0);
      }
      if (p==dt.end()) break;
    }
///printf("ZL\n");

    // Summarize result
    printUTF8(archive.c_str());
    printf(" %1.0f -> ", sz+.0);
    printUTF8(repack);
    printf(" %1.0f\n", out.tell()+.0);

    // Rewrite C block
    out.seek(cstart, SEEK_SET);
    writeJidacHeader(&out, ver[1].date, csize, 1);
    out.close();
    return 0;
  }

	g_crc32.clear();
	
  // Decompress archive in parallel
	printf("Extracting %s bytes (%s) in %s files -threads %d\n",migliaia(job.total_size), tohuman(job.total_size),migliaia2(total_files), howmanythreads);
		
	vector<ThreadID> tid(howmanythreads);
	for (unsigned i=0; i<tid.size(); ++i) 
		run(tid[i], decompressThread, &job);

  // Extract streaming files
	unsigned segments=0;  // count
	InputArchive in(archive.c_str(), password);
	if (in.isopen()) 
	{
		FP outf=FPNULL;
		DTMap::iterator dtptr=dt.end();
		for (unsigned i=0; i<block.size(); ++i) 
		{
			if (block[i].usize<0 && block[i].size>0) 
			{
				Block& b=block[i];
				try 
				{
					in.seek(b.offset, SEEK_SET);
					libzpaq::Decompresser d;
					d.setInput(&in);
					if (!d.findBlock()) 
						error("block not found");
					StringWriter filename;
          for (unsigned j=0; j<b.size; ++j) {
            if (!d.findFilename(&filename)) error("segment not found");
            d.readComment();

            // Start of new output file
            if (filename.s!="" || segments==0) {
              unsigned k;
              for (k=0; k<b.files.size(); ++k) {  // find in dt
                if (b.files[k]->second.ptr.size()>0
                    && b.files[k]->second.ptr[0]==b.start+j
                    && b.files[k]->second.date>0
                    && b.files[k]->second.data==0)
                  break;
              }
              if (k<b.files.size()) {  // found new file
                if (outf!=FPNULL) fclose(outf);
                outf=FPNULL;
                string outname=rename(b.files[k]->first);
                dtptr=b.files[k];
                lock(job.mutex);

/*
                  printf("> ");
                  printUTF8(outname.c_str());
                  printf("\n");
*/
				if (!flagtest) {
	                makepath(outname);
				  
                  outf=fopen(outname.c_str(), WB);
                  if (outf==FPNULL) 
				  {
					printerr("18330",outname.c_str());
				  }
                }
                release(job.mutex);
              }
              else {  // end of file
                if (outf!=FPNULL) fclose(outf);
                outf=FPNULL;
                dtptr=dt.end();
              }
            }
			///printf("Z4\n");
            // Decompress segment
            libzpaq::SHA1 sha1;
            d.setSHA1(&sha1);
            OutputFile o(outf);
            d.setOutput(&o);
            d.decompress();

            ///printf("Z5\n");
            // Verify checksum
            char sha1result[21];
            d.readSegmentEnd(sha1result);
            if (sha1result[0]==1) {
              if (memcmp(sha1result+1, sha1.result(), 20)!=0)
                error("checksum failed");
            }
            else if (sha1result[0]!=0)
              error("unknown checksum type");
            ++b.extracted;
            if (dtptr!=dt.end()) ++dtptr->second.data;
            filename.s="";
            ++segments;
          }
        }
        catch(std::exception& e) {
          lock(job.mutex);
          printf("Skipping block: %s\n", e.what());
          release(job.mutex);
        }
      }
    }
    if (outf!=FPNULL) fclose(outf);
  }
  if (segments>0) printf("%u streaming segments extracted\n", segments);

	///printf("Z6\n");

  // Wait for threads to finish
  for (unsigned i=0; i<tid.size(); ++i) join(tid[i]);
///printf("Z7\n");

  // Create empty directories and set file dates and attributes
  if (!flagtest) {
    for (DTMap::reverse_iterator p=dt.rbegin(); p!=dt.rend(); ++p) {
      if (p->second.data>=0 && p->second.date && p->first!="") {
        string s=rename(p->first);
        if (p->first[p->first.size()-1]=='/')
          makepath(s, p->second.date, p->second.attr);
        else if ((p->second.attr&0x1ff)=='w'+256)  // read-only?
          close(s.c_str(), 0, p->second.attr);
      }
    }
  }
///printf("Z8\n");

  // Report failed extractions
  unsigned extracted=0, errors=0;
  for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) {
    string fn=rename(p->first);
    if (p->second.data>=0 && p->second.date
        && fn!="" && fn[fn.size()-1]!='/') {
      ++extracted;
      if (p->second.ptr.size()!=unsigned(p->second.data)) {
        fflush(stdout);
        if (++errors==1)
          fprintf(stderr,
          "\nFailed (extracted/total fragments, file):\n");
        fprintf(stderr, "%u/%u ",
                int(p->second.data), int(p->second.ptr.size()));
        printUTF8(fn.c_str(), stderr);
        fprintf(stderr, "\n");
      }
    }
  }
  ///printf("Z9\n");
	if (kollision>0)
		printf("\nFilenames collisions %08d\n",(int)kollision);
	
  if (errors>0) 
  {
    fflush(stdout);
    fprintf(stderr,
        "\nExtracted %s of %s files OK (%s errors)"
        " using %s bytes x %d threads\n",
        migliaia(extracted-errors), migliaia2(extracted), migliaia3(errors), migliaia4(job.maxMemory),
        int(tid.size()));
  }
  return errors>0;
}


/////////////////////////////// list //////////////////////////////////

// Return p<q for sorting files by decreasing size, then fragment ID list
bool compareFragmentList(DTMap::const_iterator p, DTMap::const_iterator q) {
  if (p->second.size!=q->second.size) return p->second.size>q->second.size;
  if (p->second.ptr<q->second.ptr) return true;
  if (q->second.ptr<p->second.ptr) return false;
  if (p->second.data!=q->second.data) return p->second.data<q->second.data;
  return p->first<q->first;
}

/*
// Return p<q for sort by name and comparison result
bool compareName(DTMap::const_iterator p, DTMap::const_iterator q) {
  if (p->first!=q->first) return p->first<q->first;
  return p->second.data<q->second.data;
}
*/



uint32_t crchex2int(char *hex) 
{
	assert(hex);
	uint32_t val = 0;
	for (int i=0;i<8;i++)
	{
        uint8_t byte = *hex++; 
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;    
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}
void print_datetime(void)
{
	int hours, minutes, seconds, day, month, year;

	time_t now;
	time(&now);
	struct tm *local = localtime(&now);

	hours = local->tm_hour;			// get hours since midnight	(0-23)
	minutes = local->tm_min;		// get minutes passed after the hour (0-59)
	seconds = local->tm_sec;		// get seconds passed after the minute (0-59)

	day = local->tm_mday;			// get day of month (1 to 31)
	month = local->tm_mon + 1;		// get month of year (0 to 11)
	year = local->tm_year + 1900;	// get year since 1900

	printf("%02d/%02d/%d %02d:%02d:%02d ", day, month, year, hours,minutes,seconds);
}

/// quanti==1 => version (>0)
/// quanti==0 => 0
/// else  => -1 multiple match
int Jidac::searchcomments(string i_testo,vector<DTMap::iterator> &filelist)
{
	unsigned int quanti=0;
	int versione=-1;
	filelist.clear();
	
	for (DTMap::iterator a=dt.begin(); a!=dt.end(); ++a) 
	{
		a->second.data='-';
		filelist.push_back(a);
	}
		   
	///VCOMMENT 00000002 seconda_versione:$DATA
	for (unsigned i=0;i<filelist.size();i++) 
	{
		DTMap::iterator p=filelist[i];
		if (isads(p->first))
		{
			string fakefile=p->first;
			myreplace(fakefile,":$DATA","");
			size_t found = fakefile.find("VCOMMENT "); 
			if (found != string::npos)
			{
				///printf("KKKKKKKKKKKKKKKKK <<%s>>\n",fakefile.c_str());
					
				string numeroversione=fakefile.substr(found+9,8);
//esx
	///			int numver=0; //stoi(numeroversione.c_str());
				int numver=std::stoi(numeroversione.c_str());
				string commento=fakefile.substr(found+9+8+1,65000);
				if (i_testo.length()>0)
				{
					size_t start_comment = commento.find(i_testo);
					if (start_comment == std::string::npos)
					continue;
				}
    			mappacommenti.insert(std::pair<int, string>(numver, commento));
				versione=numver;
				quanti++;
			}
		}
	}
	if (quanti==1)
		return versione;
	else
	if (quanti==0)
		return 0;
	else
		return -1;
	
}
int Jidac::enumeratecomments()
{
	
	  // Read archive into dt, which may be "" for empty.
	int64_t csize=0;
	int errors=0;

	if (archive!="") 
		csize=read_archive(archive.c_str(),&errors,1); /// AND NOW THE MAGIC ONE!
	printf("\nVersion(s) enumerator\n");
	
	vector<DTMap::iterator> filelist;
	searchcomments(versioncomment,filelist);

	for (MAPPACOMMENTI::const_iterator p=mappacommenti.begin(); p!=mappacommenti.end(); ++p) 
			printf("%08d <<%s>>\n",p->first,p->second.c_str());

  
	for (DTMap::iterator p=edt.begin(); p!=edt.end(); ++p) 
	{
		DTMap::iterator a=dt.find(rename(p->first));
		if (a!=dt.end() && (true || a->second.date)) 
		{
			a->second.data='-';
			filelist.push_back(a);
		}
		p->second.data='+';
		filelist.push_back(p);
	}
  
	for (DTMap::iterator a=dt.begin(); a!=dt.end(); ++a) 
		if (a->second.data!='-' && (true || a->second.date)) 
		{
			a->second.data='-';
			filelist.push_back(a);
		}
  
	if (all)
	{
		printf("-------------------------------------------------------------------------\n");
		printf("< Ver  > <  date  > < time >  < added > <removed>    <    bytes added   >\n");
		printf("-------------------------------------------------------------------------\n");
		for (unsigned fi=0;fi<filelist.size() /*&& (true || int(fi)<summary)*/; ++fi) 
		{
			DTMap::iterator p=filelist[fi];
			unsigned v;  
			if (p->first.size()==all+1u && (v=atoi(p->first.c_str()))>0 && v<ver.size()) 
			{
				printf("%08u %s ",v,dateToString(p->second.date).c_str());
				printf(" +%08d -%08d -> %20s", ver[v].updates, ver[v].deletes,
					migliaia((v+1<ver.size() ? ver[v+1].offset : csize)-ver[v].offset+0.0));
		
				std::map<int,string>::iterator commento;
				commento=mappacommenti.find(v); 
				if(commento!= mappacommenti.end()) 
					printf(" <<%s>>", commento->second.c_str());
				printf("\n");
			}
		}
	}

	return 0;		   
}


int Jidac::kill() 
{
	
	printf("KILL of:");

	if (files.size()<=0)
		return -1;
	if (archive=="")
		return -1;
		

	read_archive(archive.c_str());

  // Read external files into edt
	///uint64_t howmanyfiles=0;
	
	g_bytescanned=0;
	g_filescanned=0;
	g_worked=0;
	files_count.clear();
	edt.clear();
	

	
	///string cartellaoutput=append_path(tofiles[0], files[0]);
	string cartellaoutput=tofiles[0];
	/*
	printf("FILES %s\n",files[0].c_str());
	printf("TOFILES %s\n",tofiles[0].c_str());
	printf("cartellaoutput  %s\n",cartellaoutput.c_str());
*/
	scandir(false,edt,cartellaoutput);
	if (edt.size()) 
		printf("Total files found: %s\n", migliaia(edt.size()));
	else	
	{
		printf("Found nothing in filesystem\n");
		return 1;
	}
	printf("\n");
	
	vector<string> inzpaqrinominato;
	vector<string> tobekilled;
	vector<string> dirtobekilled;
	
	for (DTMap::iterator a=dt.begin(); a!=dt.end(); ++a) 
	{
		string dentrofile=a->first;
		myreplace(dentrofile,files[0],tofiles[0]);
		if (a->second.date==0)
		{
			if (flagverbose)
				printf("DELETED ");
		}
		else
			inzpaqrinominato.push_back(dentrofile);
		if (flagverbose)
			printf("Into zpaq %s  > renamed %s\n",a->first.c_str(),dentrofile.c_str());
	}

	std::sort(inzpaqrinominato.begin(), inzpaqrinominato.end());

	uint64_t		sizetobekilled=0;
	for (DTMap::iterator a=edt.begin(); a!=edt.end(); ++a) 
		if (binary_search(inzpaqrinominato.begin(),inzpaqrinominato.end(),a->first)==false)
		{
			if (isdirectory(a->first))
				dirtobekilled.push_back(a->first);
			else
			{
				tobekilled.push_back(a->first);
				sizetobekilled+=a->second.size;
			}
		}
	for (unsigned int i=0;i<dirtobekilled.size();i++)
	{
		printf("Dir  to be killed ");
		printUTF8(dirtobekilled[i].c_str());
		printf("\n");
	
	}
	for (unsigned int i=0;i<tobekilled.size();i++)
	{
		printf("File to be killed ");
		printUTF8(tobekilled[i].c_str());
		printf("\n");
	}
	
	printf("\n");
	
	unsigned int 	totaltobekilled=dirtobekilled.size()+tobekilled.size();
	if (totaltobekilled==0)
	{
		printf("Nothing to do\n");
		return 0;
	}
	printf("Directories to be removed %s\n",migliaia(dirtobekilled.size()));
	printf("Files       to be removed %s (%s bytes)\n",migliaia2(tobekilled.size()),migliaia(sizetobekilled));
	
	string captcha="kilL"+std::to_string(dirtobekilled.size()+tobekilled.size()+sizetobekilled);
	printf("Captcha to continue %s\n",captcha.c_str());
	char myline[81];
    scanf("%80s", myline);
	if (myline!=captcha)
	{
		printf("Wrong captcha\n");
		return 1;
	}
	printf("Captcha OK\n");
	
	unsigned int killed=0;
	unsigned int killeddir=0;

	for (unsigned int i=0;i<tobekilled.size();i++)
		if (delete_file(tobekilled[i].c_str()))
				killed++;
	
/// OK iterating instead of recursing
	while (killeddir!=dirtobekilled.size())
		for (unsigned int i=0;i<dirtobekilled.size();i++)
			if (delete_dir(dirtobekilled[i].c_str()))
				killeddir++;

	printf("Dir  to be removed %s -> killed %s\n",migliaia(dirtobekilled.size()),migliaia2(killeddir));
	printf("File to be removed %s -> killed %s\n",migliaia(tobekilled.size()),migliaia2(killed));
	if ((killeddir+killed)!=(dirtobekilled.size()+tobekilled.size())) 
	{	
		printf("FAILED !! some highlander !!\n");
		return 1;
	}
	
	return 0;
	
}


int Jidac::verify() 
{
	
	
	if (files.size()<=0)
		return -1;
	if (archive=="")
		return -1;
		
	printf("Compare archive content of:");


	read_archive(archive.c_str());

  // Read external files into edt
	uint64_t howmanyfiles=0;
	
	g_bytescanned=0;
	g_filescanned=0;
	g_worked=0;
	files_count.clear();
	
	edt.clear();
	for (unsigned i=0; i<files.size(); ++i)
	{
		scandir(true,edt,files[i].c_str());
		files_count.push_back(edt.size()-howmanyfiles);
		howmanyfiles=edt.size();
	}
	printf("\n");
 	for (unsigned i=0; i<files.size(); ++i)
		printf("%9s in <<%s>>\n",migliaia(files_count[i]),files[i].c_str());
	
	
	if (files.size()) 
		printf("Total files found: %s\n", migliaia(edt.size()));
	printf("\n");

  // Compute directory sizes as the sum of their contents
	DTMap* dp[2]={&dt, &edt};
	for (int i=0; i<2; ++i) 
	{
		for (DTMap::iterator p=dp[i]->begin(); p!=dp[i]->end(); ++p) 
		{
			int len=p->first.size();
			if (len>0 && p->first[len]!='/') 
			{
				for (int j=0; j<len; ++j) 
				{
					if (p->first[j]=='/') 
					{
						DTMap::iterator q=dp[i]->find(p->first.substr(0, j+1));
						if (q!=dp[i]->end())
							q->second.size+=p->second.size;
					}
				}
			}
		}
	}

	vector<DTMap::iterator> filelist;
	for (DTMap::iterator p=edt.begin(); p!=edt.end(); ++p) 
	{
		DTMap::iterator a=dt.find(rename(p->first));
		if (a!=dt.end() && (all || a->second.date)) 
		{
			a->second.data='-';
			filelist.push_back(a);
		}
		p->second.data='+';
		filelist.push_back(p);
	}
	
	for (DTMap::iterator a=dt.begin(); a!=dt.end(); ++a) 
		if (a->second.data!='-' && (all || a->second.date)) 
		{
			a->second.data='-';
			filelist.push_back(a);
		}
	
	int64_t usize=0;
	unsigned matches=0, mismatches=0, internal=0, external=0;
	uint32_t crc32fromfile;
	
	vector<string> risultati;
	vector<bool> risultati_utf8;
	char linebuffer[1000];
	unsigned int ultimapercentuale=200;
	unsigned int percentuale;
	
	for (unsigned fi=0;fi<filelist.size(); ++fi) 
	{
		DTMap::iterator p=filelist[fi];
		
		if (menoenne)
			if ((mismatches+external+internal)>menoenne)
				break;
		
		if (!flagnoeta)
		{
			percentuale=100*fi/filelist.size();
			if (ultimapercentuale!=percentuale)
			{
				printf("Done %02d%% %10s of %10s, diff %s bytes\r",percentuale,tohuman(g_worked),tohuman2(g_bytescanned),migliaia2(usize));
				ultimapercentuale=percentuale;
			}
		}
		
    // Compare external files
		if (p->second.data=='-' && fi+1<filelist.size() && filelist[fi+1]->second.data=='+') 
		{
			DTMap::iterator p1=filelist[fi+1];
				
			if (equal(p, p1->first.c_str(),crc32fromfile))
			{
				
				if (p->second.sha1hex[41]!=0)
				{
					if (!isdirectory(p1->first))
					{
						uint32_t crc32stored=crchex2int(p->second.sha1hex+41);
						if (crc32stored!=crc32fromfile)
						{
							p->second.data='#';
							sprintf(linebuffer,"\nGURU SHA1 COLLISION! %08X vs %08X ",crc32stored,crc32fromfile);
							risultati.push_back(linebuffer);
							risultati_utf8.push_back(false);
							
							sprintf(linebuffer,"%s\n",p1->first.c_str());
							risultati.push_back(linebuffer);
							risultati_utf8.push_back(true);
							
						}
						else
						{	
							p->second.data='=';
							++fi;
						}
					}
					else
					{
						p->second.data='=';
						++fi;
					}
					
				}
				else
				{
					p->second.data='=';
					++fi;
				}
			}
			else
			{
				p->second.data='#';
				p1->second.data='!';
			}
		}


		if (p->second.data=='=') ++matches;
		if (p->second.data=='#') ++mismatches;
		if (p->second.data=='-') ++internal;
		if (p->second.data=='+') ++external;

		if (p->second.data!='=') 
		{
			if (!isdirectory(p->first))
				usize+=p->second.size;
				
			sprintf(linebuffer,"%c %s %19s ", char(p->second.data),dateToString(p->second.date).c_str(), migliaia(p->second.size));
			risultati.push_back(linebuffer);
			risultati_utf8.push_back(false);
			
			
			sprintf(linebuffer,"%s\n",p->first.c_str());
			risultati.push_back(linebuffer);
			risultati_utf8.push_back(true);
	  
			if (p->second.data=='!')
			{
				sprintf(linebuffer,"\n");
				risultati.push_back(linebuffer);
				risultati_utf8.push_back(false);
			}
		}
	}  
	printf("\n\n");

	bool myerror=false;
	
	if (menoenne)
		if ((mismatches+external+internal)>menoenne)
			printf("**** STOPPED BY TOO MANY ERRORS -n %d\n",menoenne);
			
	if  (mismatches || external || internal)
		for (unsigned int i=0;i<risultati.size();i++)
		{
			if (risultati_utf8[i])
				printUTF8(risultati[i].c_str());
			else
			printf("%s",risultati[i].c_str());
		}

	if (matches)
		printf("%08d =same\n",matches);
	if (mismatches)
	{
		printf("%08d #different\n",mismatches);
		myerror=true;
	}
	if (external)
	{
		printf("%08d +external (file missing in ZPAQ)\n",external);
		myerror=true;
	}
	if (internal)
	{
		printf("%08d -internal (file in ZPAQ but not on disk)\n",internal);
		myerror=true;
	}
	printf("Total different file size: %s bytes\n",migliaia(usize));
 	if (myerror)
		return 2;
	else
		return 0;
}

int Jidac::info() 
{
	flagcomment=true;
	versioncomment="";
	all=4;
	return enumeratecomments();
}

// List contents
int Jidac::list() 
{
	if (flagcomment)
	{
		enumeratecomments();
		return 0;
	}
	if (files.size()>=1)
		return verify();

  // Read archive into dt, which may be "" for empty.
  int64_t csize=0;
  int errors=0;
  bool flagshow;
  
	if (archive!="") csize=read_archive(archive.c_str(),&errors,1); /// AND NOW THE MAGIC ONE!
	printf("\n");


	g_bytescanned=0;
	g_filescanned=0;
	g_worked=0;
	vector<DTMap::iterator> filelist;

	for (DTMap::iterator a=dt.begin(); a!=dt.end(); ++a) 
		if (a->second.data!='-' && (all || a->second.date)) 
		{
			a->second.data='-';
			filelist.push_back(a);
		}
	

  // Sort by size desc
 	if (menoenne) // sort by size desc
	{
		printf("Sort by size desc, limit to %d\n",menoenne);
		sort(filelist.begin(), filelist.end(), compareFragmentList);
	}

	int64_t usize=0;
	map<int, string> mappacommenti;
	
	//searchcomments(versioncomment,filelist);
		
	///VCOMMENT 00000002 seconda_versione:$DATA
	for (unsigned i=0;i<filelist.size();i++) 
	{
		DTMap::iterator p=filelist[i];
		if (isads(p->first))
		{
			string fakefile=p->first;
			myreplace(fakefile,":$DATA","");
			size_t found = fakefile.find("VCOMMENT "); 
			if (found != string::npos)
			{
    			string numeroversione=fakefile.substr(found+9,8);
//esx		
	//	int numver=0;//std::stoi(numeroversione.c_str());
		int numver=std::stoi(numeroversione.c_str());
				string commento=fakefile.substr(found+9+8+1,65000);
				mappacommenti.insert(std::pair<int, string>(numver, commento));
			}
		}
	}
	   
		   
	unsigned fi;
	for (fi=0;fi<filelist.size(); ++fi) 
	{
    
		if (menoenne) /// list -n 10 => sort by size and stop at 10
			if (fi>=menoenne)
				break;
			
		DTMap::iterator p=filelist[fi];
		flagshow=true;
		
		if (isads(p->first))
			if (strstr(p->first.c_str(),"VCOMMENT "))
				flagshow=false;
		
/// a little of change if -search is used
		if (searchfrom!="")
			flagshow=stristr(p->first.c_str(),searchfrom.c_str());
			
/// redundant, but not a really big deal

		if (flagchecksum)
			if (isdirectory(p->first))
				flagshow=false;

		if ((minsize>0) || (maxsize>0))
			if (isdirectory(p->first))
				flagshow=false;
						
   if (maxsize>0)
 {
 if (maxsize<(uint64_t)p->second.size) 
   flagshow=false;
			}
  if (minsize>0)
 {
	 if (minsize>(uint64_t)p->second.size)
	 flagshow=false;
}
		if (flagshow)
		{
			if (!strchr(nottype.c_str(), p->second.data)) 
			{
				if (p->first!="" && (!isdirectory(p->first)))
					usize+=p->second.size;
	
				if (!flagchecksum) // special output
				{
					printf("%c %s %19s ", char(p->second.data),dateToString(p->second.date).c_str(), migliaia(p->second.size));
					if (!flagnoattributes)
					printf("%s ", attrToString(p->second.attr).c_str());
				}
				
				string myfilename=p->first;
				
	/// houston, we have checksums?
				string mysha1="";
				string mycrc32="";
				
				if (p->second.sha1hex[0]!=0)
						if (p->second.sha1hex[0+40]==0)
							mysha1=(string)"SHA1: "+p->second.sha1hex+" ";
							
				if (p->second.sha1hex[41]!=0)
						if (p->second.sha1hex[41+8]==0)
						{
							if (isdirectory(myfilename))
								mycrc32=(string)"CRC32:          ";
							else
								mycrc32=(string)"CRC32: "+(p->second.sha1hex+41)+" ";
						}
						
							
				if (all)
				{
					string rimpiazza="|";
				
					if (!isdirectory(myfilename))
	//					rimpiazza+="FOLDER ";
		//			else
					{
						rimpiazza+=mysha1;
						rimpiazza+=mycrc32;
					}
					if (!myreplace(myfilename,"|$1",rimpiazza))
						myreplace(myfilename,"/","|");
				}
				else
				{
						myfilename=mysha1+mycrc32+myfilename;
					
				}
				
				
/// search and replace, if requested	
				if ((searchfrom!="") && (replaceto!=""))
					replace(myfilename,searchfrom,replaceto);
								
				printUTF8(myfilename.c_str());
			  
				unsigned v;  // list version updates, deletes, compressed size
			if (all>0 && p->first.size()==all+1u && (v=atoi(p->first.c_str()))>0
          && v<ver.size()) 
				{  // version info
		
					std::map<int,string>::iterator commento;
		
					commento=mappacommenti.find(v); 
					if(commento== mappacommenti.end()) 
						printf(" +%d -%d -> %s", ver[v].updates, ver[v].deletes,
						(v+1<ver.size() ? migliaia(ver[v+1].offset-ver[v].offset) : migliaia(csize-ver[v].offset)));
					else
						printf(" +%d -%d -> %s <<%s>>", ver[v].updates, ver[v].deletes,
						(v+1<ver.size() ? migliaia(ver[v+1].offset-ver[v].offset) : migliaia(csize-ver[v].offset)),commento->second.c_str());
        		
				/*
			if (summary<0)  // print fragment range
			printf(" %u-%u", ver[v].firstFragment,
              v+1<ver.size()?ver[v+1].firstFragment-1:unsigned(ht.size())-1);
*/
				}
				
				printf("\n");
			}
		}
	}  // end for i = each file version

  // Compute dedupe size

	int64_t ddsize=0, allsize=0;
	unsigned nfiles=0, nfrags=0, unknown_frags=0, refs=0;
	vector<bool> ref(ht.size());
	for (DTMap::const_iterator p=dt.begin(); p!=dt.end(); ++p) 
	{
		if (p->second.date) 
		{
			++nfiles;
			for (unsigned j=0; j<p->second.ptr.size(); ++j) 
			{
				unsigned k=p->second.ptr[j];
				if (k>0 && k<ht.size()) 
				{
					++refs;
					if (ht[k].usize>=0) 
						allsize+=ht[k].usize;
					if (!ref[k]) 
					{
						ref[k]=true;
						++nfrags;
						if (ht[k].usize>=0) 
							ddsize+=ht[k].usize;
						else 
						++unknown_frags;
					}
				}
			}
		}
	}
	
  // Print archive statistics
	printf("\n%s (%s) of %s (%s) in %s files shown\n",migliaia(usize),tohuman(usize),migliaia2(allsize),tohuman2(allsize),migliaia3(fi));
  if (unknown_frags)
    printf("%u fragments have unknown size\n", unknown_frags);

  if (dhsize!=dcsize)  // index?
    printf("Note: %1.0f of %1.0f compressed bytes are in archive\n",
        dcsize+0.0, dhsize+0.0);
		
		
  return 0;
}




#define XXH_IMPLEMENTATION   /* access definitions */


#define XXH_STATIC_LINKING_ONLY   /* *_state_t */
/*
 * xxHash - Extremely Fast Hash algorithm
 * Header File
 * Copyright (C) 2012-2020 Yann Collet
 *
 * BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)
 */


#if defined (__cplusplus)
extern "C" {
#endif

#if (defined(XXH_INLINE_ALL) || defined(XXH_PRIVATE_API)) \
    && !defined(XXH_INLINE_ALL_31684351384)
   /* this section should be traversed only once */
#  define XXH_INLINE_ALL_31684351384
   /* give access to the advanced API, required to compile implementations */
#  undef XXH_STATIC_LINKING_ONLY   /* avoid macro redef */
#  define XXH_STATIC_LINKING_ONLY
   /* make all functions private */
#  undef XXH_PUBLIC_API
#  if defined(__GNUC__)
#    define XXH_PUBLIC_API static __inline __attribute__((unused))
#  elif defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
#    define XXH_PUBLIC_API static inline
#  elif defined(_MSC_VER)
#    define XXH_PUBLIC_API static __inline
#  else
     /* note: this version may generate warnings for unused static functions */
#    define XXH_PUBLIC_API static
#  endif

#  ifdef XXH_NAMESPACE
#    error "XXH_INLINE_ALL with XXH_NAMESPACE is not supported"
     /*
      * Note: Alternative: #undef all symbols (it's a pretty large list).
      * Without #error: it compiles, but functions are actually not inlined.
      */
#  endif
#  define XXH_NAMESPACE XXH_INLINE_
#  define XXH_IPREF(Id)   XXH_INLINE_ ## Id
#  define XXH_OK XXH_IPREF(XXH_OK)
#  define XXH_ERROR XXH_IPREF(XXH_ERROR)
#  define XXH_errorcode XXH_IPREF(XXH_errorcode)
#  define XXH32_canonical_t  XXH_IPREF(XXH32_canonical_t)
#  define XXH64_canonical_t  XXH_IPREF(XXH64_canonical_t)
#  define XXH128_canonical_t XXH_IPREF(XXH128_canonical_t)
#  define XXH32_state_s XXH_IPREF(XXH32_state_s)
#  define XXH32_state_t XXH_IPREF(XXH32_state_t)
#  define XXH64_state_s XXH_IPREF(XXH64_state_s)
#  define XXH64_state_t XXH_IPREF(XXH64_state_t)
#  define XXH3_state_s  XXH_IPREF(XXH3_state_s)
#  define XXH3_state_t  XXH_IPREF(XXH3_state_t)
#  define XXH128_hash_t XXH_IPREF(XXH128_hash_t)
   /* Ensure the header is parsed again, even if it was previously included */
#  undef XXHASH_H_5627135585666179
#  undef XXHASH_H_STATIC_13879238742
#endif /* XXH_INLINE_ALL || XXH_PRIVATE_API */



/* ****************************************************************
 *  Stable API
 *****************************************************************/
#ifndef XXHASH_H_5627135585666179
#define XXHASH_H_5627135585666179 1

/* specific declaration modes for Windows */
#if !defined(XXH_INLINE_ALL) && !defined(XXH_PRIVATE_API)
#  if defined(WIN32) && defined(_MSC_VER) && (defined(XXH_IMPORT) || defined(XXH_EXPORT))
#    ifdef XXH_EXPORT
#      define XXH_PUBLIC_API __declspec(dllexport)
#    elif XXH_IMPORT
#      define XXH_PUBLIC_API __declspec(dllimport)
#    endif
#  else
#    define XXH_PUBLIC_API   /* do nothing */
#  endif
#endif

#ifdef XXH_NAMESPACE
#  define XXH_CAT(A,B) A##B
#  define XXH_NAME2(A,B) XXH_CAT(A,B)
#  define XXH_versionNumber XXH_NAME2(XXH_NAMESPACE, XXH_versionNumber)
/* XXH32 */
#  define XXH32 XXH_NAME2(XXH_NAMESPACE, XXH32)
#  define XXH32_createState XXH_NAME2(XXH_NAMESPACE, XXH32_createState)
#  define XXH32_freeState XXH_NAME2(XXH_NAMESPACE, XXH32_freeState)
#  define XXH32_reset XXH_NAME2(XXH_NAMESPACE, XXH32_reset)
#  define XXH32_update XXH_NAME2(XXH_NAMESPACE, XXH32_update)
#  define XXH32_digest XXH_NAME2(XXH_NAMESPACE, XXH32_digest)
#  define XXH32_copyState XXH_NAME2(XXH_NAMESPACE, XXH32_copyState)
#  define XXH32_canonicalFromHash XXH_NAME2(XXH_NAMESPACE, XXH32_canonicalFromHash)
#  define XXH32_hashFromCanonical XXH_NAME2(XXH_NAMESPACE, XXH32_hashFromCanonical)
/* XXH64 */
#  define XXH64 XXH_NAME2(XXH_NAMESPACE, XXH64)
#  define XXH64_createState XXH_NAME2(XXH_NAMESPACE, XXH64_createState)
#  define XXH64_freeState XXH_NAME2(XXH_NAMESPACE, XXH64_freeState)
#  define XXH64_reset XXH_NAME2(XXH_NAMESPACE, XXH64_reset)
#  define XXH64_update XXH_NAME2(XXH_NAMESPACE, XXH64_update)
#  define XXH64_digest XXH_NAME2(XXH_NAMESPACE, XXH64_digest)
#  define XXH64_copyState XXH_NAME2(XXH_NAMESPACE, XXH64_copyState)
#  define XXH64_canonicalFromHash XXH_NAME2(XXH_NAMESPACE, XXH64_canonicalFromHash)
#  define XXH64_hashFromCanonical XXH_NAME2(XXH_NAMESPACE, XXH64_hashFromCanonical)
/* XXH3_64bits */
#  define XXH3_64bits XXH_NAME2(XXH_NAMESPACE, XXH3_64bits)
#  define XXH3_64bits_withSecret XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_withSecret)
#  define XXH3_64bits_withSeed XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_withSeed)
#  define XXH3_createState XXH_NAME2(XXH_NAMESPACE, XXH3_createState)
#  define XXH3_freeState XXH_NAME2(XXH_NAMESPACE, XXH3_freeState)
#  define XXH3_copyState XXH_NAME2(XXH_NAMESPACE, XXH3_copyState)
#  define XXH3_64bits_reset XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_reset)
#  define XXH3_64bits_reset_withSeed XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_reset_withSeed)
#  define XXH3_64bits_reset_withSecret XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_reset_withSecret)
#  define XXH3_64bits_update XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_update)
#  define XXH3_64bits_digest XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_digest)
#  define XXH3_generateSecret XXH_NAME2(XXH_NAMESPACE, XXH3_generateSecret)
/* XXH3_128bits */
#  define XXH128 XXH_NAME2(XXH_NAMESPACE, XXH128)
#  define XXH3_128bits XXH_NAME2(XXH_NAMESPACE, XXH3_128bits)
#  define XXH3_128bits_withSeed XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_withSeed)
#  define XXH3_128bits_withSecret XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_withSecret)
#  define XXH3_128bits_reset XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_reset)
#  define XXH3_128bits_reset_withSeed XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_reset_withSeed)
#  define XXH3_128bits_reset_withSecret XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_reset_withSecret)
#  define XXH3_128bits_update XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_update)
#  define XXH3_128bits_digest XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_digest)
#  define XXH128_isEqual XXH_NAME2(XXH_NAMESPACE, XXH128_isEqual)
#  define XXH128_cmp     XXH_NAME2(XXH_NAMESPACE, XXH128_cmp)
#  define XXH128_canonicalFromHash XXH_NAME2(XXH_NAMESPACE, XXH128_canonicalFromHash)
#  define XXH128_hashFromCanonical XXH_NAME2(XXH_NAMESPACE, XXH128_hashFromCanonical)
#endif


/* *************************************
*  Version
***************************************/
#define XXH_VERSION_MAJOR    0
#define XXH_VERSION_MINOR    8
#define XXH_VERSION_RELEASE  0
#define XXH_VERSION_NUMBER  (XXH_VERSION_MAJOR *100*100 + XXH_VERSION_MINOR *100 + XXH_VERSION_RELEASE)
XXH_PUBLIC_API unsigned XXH_versionNumber (void);


/* ****************************
*  Definitions
******************************/
typedef enum { XXH_OK=0, XXH_ERROR } XXH_errorcode;


/*-**********************************************************************
*  32-bit hash
************************************************************************/
#if !defined (__VMS) \
  && (defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
#   include <stdint.h>
    typedef uint32_t XXH32_hash_t;
#else
#   include <limits.h>
#   if UINT_MAX == 0xFFFFFFFFUL
      typedef unsigned int XXH32_hash_t;
#   else
#     if ULONG_MAX == 0xFFFFFFFFUL
        typedef unsigned long XXH32_hash_t;
#     else
#       error "unsupported platform: need a 32-bit type"
#     endif
#   endif
#endif

XXH_PUBLIC_API XXH32_hash_t XXH32 (const void* input, size_t length, XXH32_hash_t seed);


typedef struct XXH32_state_s XXH32_state_t;   /* incomplete type */
XXH_PUBLIC_API XXH32_state_t* XXH32_createState(void);
XXH_PUBLIC_API XXH_errorcode  XXH32_freeState(XXH32_state_t* statePtr);
XXH_PUBLIC_API void XXH32_copyState(XXH32_state_t* dst_state, const XXH32_state_t* src_state);

XXH_PUBLIC_API XXH_errorcode XXH32_reset  (XXH32_state_t* statePtr, XXH32_hash_t seed);
XXH_PUBLIC_API XXH_errorcode XXH32_update (XXH32_state_t* statePtr, const void* input, size_t length);
XXH_PUBLIC_API XXH32_hash_t  XXH32_digest (const XXH32_state_t* statePtr);


typedef struct { unsigned char digest[4]; } XXH32_canonical_t;
XXH_PUBLIC_API void XXH32_canonicalFromHash(XXH32_canonical_t* dst, XXH32_hash_t hash);
XXH_PUBLIC_API XXH32_hash_t XXH32_hashFromCanonical(const XXH32_canonical_t* src);


#ifndef XXH_NO_LONG_LONG
/*-**********************************************************************
*  64-bit hash
************************************************************************/
#if !defined (__VMS) \
  && (defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
#   include <stdint.h>
    typedef uint64_t XXH64_hash_t;
#else
    /* the following type must have a width of 64-bit */
    typedef unsigned long long XXH64_hash_t;
#endif

XXH_PUBLIC_API XXH64_hash_t XXH64 (const void* input, size_t length, XXH64_hash_t seed);

/*******   Streaming   *******/
typedef struct XXH64_state_s XXH64_state_t;   /* incomplete type */
XXH_PUBLIC_API XXH64_state_t* XXH64_createState(void);
XXH_PUBLIC_API XXH_errorcode  XXH64_freeState(XXH64_state_t* statePtr);
XXH_PUBLIC_API void XXH64_copyState(XXH64_state_t* dst_state, const XXH64_state_t* src_state);

XXH_PUBLIC_API XXH_errorcode XXH64_reset  (XXH64_state_t* statePtr, XXH64_hash_t seed);
XXH_PUBLIC_API XXH_errorcode XXH64_update (XXH64_state_t* statePtr, const void* input, size_t length);
XXH_PUBLIC_API XXH64_hash_t  XXH64_digest (const XXH64_state_t* statePtr);

/*******   Canonical representation   *******/
typedef struct { unsigned char digest[sizeof(XXH64_hash_t)]; } XXH64_canonical_t;
XXH_PUBLIC_API void XXH64_canonicalFromHash(XXH64_canonical_t* dst, XXH64_hash_t hash);
XXH_PUBLIC_API XXH64_hash_t XXH64_hashFromCanonical(const XXH64_canonical_t* src);



/* XXH3_64bits():
 * default 64-bit variant, using default secret and default seed of 0.
 * It's the fastest variant. */
XXH_PUBLIC_API XXH64_hash_t XXH3_64bits(const void* data, size_t len);

XXH_PUBLIC_API XXH64_hash_t XXH3_64bits_withSeed(const void* data, size_t len, XXH64_hash_t seed);

#define XXH3_SECRET_SIZE_MIN 136
XXH_PUBLIC_API XXH64_hash_t XXH3_64bits_withSecret(const void* data, size_t len, const void* secret, size_t secretSize);


typedef struct XXH3_state_s XXH3_state_t;
XXH_PUBLIC_API XXH3_state_t* XXH3_createState(void);
XXH_PUBLIC_API XXH_errorcode XXH3_freeState(XXH3_state_t* statePtr);
XXH_PUBLIC_API void XXH3_copyState(XXH3_state_t* dst_state, const XXH3_state_t* src_state);

/*
 * XXH3_64bits_reset():
 * Initialize with default parameters.
 * digest will be equivalent to `XXH3_64bits()`.
 */
XXH_PUBLIC_API XXH_errorcode XXH3_64bits_reset(XXH3_state_t* statePtr);
/*
 * XXH3_64bits_reset_withSeed():
 * Generate a custom secret from `seed`, and store it into `statePtr`.
 * digest will be equivalent to `XXH3_64bits_withSeed()`.
 */
XXH_PUBLIC_API XXH_errorcode XXH3_64bits_reset_withSeed(XXH3_state_t* statePtr, XXH64_hash_t seed);
/*
 * XXH3_64bits_reset_withSecret():
 * `secret` is referenced, it _must outlive_ the hash streaming session.
 * Similar to one-shot API, `secretSize` must be >= `XXH3_SECRET_SIZE_MIN`,
 * and the quality of produced hash values depends on secret's entropy
 * (secret's content should look like a bunch of random bytes).
 * When in doubt about the randomness of a candidate `secret`,
 * consider employing `XXH3_generateSecret()` instead (see below).
 */
XXH_PUBLIC_API XXH_errorcode XXH3_64bits_reset_withSecret(XXH3_state_t* statePtr, const void* secret, size_t secretSize);

XXH_PUBLIC_API XXH_errorcode XXH3_64bits_update (XXH3_state_t* statePtr, const void* input, size_t length);
XXH_PUBLIC_API XXH64_hash_t  XXH3_64bits_digest (const XXH3_state_t* statePtr);

/* note : canonical representation of XXH3 is the same as XXH64
 * since they both produce XXH64_hash_t values */


/*-**********************************************************************
*  XXH3 128-bit variant
************************************************************************/

typedef struct {
 XXH64_hash_t low64;
 XXH64_hash_t high64;
} XXH128_hash_t;

XXH_PUBLIC_API XXH128_hash_t XXH3_128bits(const void* data, size_t len);
XXH_PUBLIC_API XXH128_hash_t XXH3_128bits_withSeed(const void* data, size_t len, XXH64_hash_t seed);
XXH_PUBLIC_API XXH128_hash_t XXH3_128bits_withSecret(const void* data, size_t len, const void* secret, size_t secretSize);

/*******   Streaming   *******/
/*
 * Streaming requires state maintenance.
 * This operation costs memory and CPU.
 * As a consequence, streaming is slower than one-shot hashing.
 * For better performance, prefer one-shot functions whenever applicable.
 *
 * XXH3_128bits uses the same XXH3_state_t as XXH3_64bits().
 * Use already declared XXH3_createState() and XXH3_freeState().
 *
 * All reset and streaming functions have same meaning as their 64-bit counterpart.
 */

XXH_PUBLIC_API XXH_errorcode XXH3_128bits_reset(XXH3_state_t* statePtr);
XXH_PUBLIC_API XXH_errorcode XXH3_128bits_reset_withSeed(XXH3_state_t* statePtr, XXH64_hash_t seed);
XXH_PUBLIC_API XXH_errorcode XXH3_128bits_reset_withSecret(XXH3_state_t* statePtr, const void* secret, size_t secretSize);

XXH_PUBLIC_API XXH_errorcode XXH3_128bits_update (XXH3_state_t* statePtr, const void* input, size_t length);
XXH_PUBLIC_API XXH128_hash_t XXH3_128bits_digest (const XXH3_state_t* statePtr);
XXH_PUBLIC_API int XXH128_isEqual(XXH128_hash_t h1, XXH128_hash_t h2);

XXH_PUBLIC_API int XXH128_cmp(const void* h128_1, const void* h128_2);


/*******   Canonical representation   *******/
typedef struct { unsigned char digest[sizeof(XXH128_hash_t)]; } XXH128_canonical_t;
XXH_PUBLIC_API void XXH128_canonicalFromHash(XXH128_canonical_t* dst, XXH128_hash_t hash);
XXH_PUBLIC_API XXH128_hash_t XXH128_hashFromCanonical(const XXH128_canonical_t* src);


#endif  /* XXH_NO_LONG_LONG */

#endif /* XXHASH_H_5627135585666179 */



#if defined(XXH_STATIC_LINKING_ONLY) && !defined(XXHASH_H_STATIC_13879238742)
#define XXHASH_H_STATIC_13879238742

struct XXH32_state_s {
   XXH32_hash_t total_len_32;
   XXH32_hash_t large_len;
   XXH32_hash_t v1;
   XXH32_hash_t v2;
   XXH32_hash_t v3;
   XXH32_hash_t v4;
   XXH32_hash_t mem32[4];
   XXH32_hash_t memsize;
   XXH32_hash_t reserved;   /* never read nor write, might be removed in a future version */
};   /* typedef'd to XXH32_state_t */


#ifndef XXH_NO_LONG_LONG  /* defined when there is no 64-bit support */

struct XXH64_state_s {
   XXH64_hash_t total_len;
   XXH64_hash_t v1;
   XXH64_hash_t v2;
   XXH64_hash_t v3;
   XXH64_hash_t v4;
   XXH64_hash_t mem64[4];
   XXH32_hash_t memsize;
   XXH32_hash_t reserved32;  /* required for padding anyway */
   XXH64_hash_t reserved64;  /* never read nor write, might be removed in a future version */
};   /* typedef'd to XXH64_state_t */

#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)   /* C11+ */
#  include <stdalign.h>
#  define XXH_ALIGN(n)      alignas(n)
#elif defined(__GNUC__)
#  define XXH_ALIGN(n)      __attribute__ ((aligned(n)))
#elif defined(_MSC_VER)
#  define XXH_ALIGN(n)      __declspec(align(n))
#else
#  define XXH_ALIGN(n)   /* disabled */
#endif

/* Old GCC versions only accept the attribute after the type in structures. */
#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L))   /* C11+ */ \
    && defined(__GNUC__)
#   define XXH_ALIGN_MEMBER(align, type) type XXH_ALIGN(align)
#else
#   define XXH_ALIGN_MEMBER(align, type) XXH_ALIGN(align) type
#endif

#define XXH3_INTERNALBUFFER_SIZE 256
#define XXH3_SECRET_DEFAULT_SIZE 192
struct XXH3_state_s {
   XXH_ALIGN_MEMBER(64, XXH64_hash_t acc[8]);
   /* used to store a custom secret generated from a seed */
   XXH_ALIGN_MEMBER(64, unsigned char customSecret[XXH3_SECRET_DEFAULT_SIZE]);
   XXH_ALIGN_MEMBER(64, unsigned char buffer[XXH3_INTERNALBUFFER_SIZE]);
   XXH32_hash_t bufferedSize;
   XXH32_hash_t reserved32;
   size_t nbStripesSoFar;
   XXH64_hash_t totalLen;
   size_t nbStripesPerBlock;
   size_t secretLimit;
   XXH64_hash_t seed;
   XXH64_hash_t reserved64;
   const unsigned char* extSecret;  /* reference to external secret;
                                     * if == NULL, use .customSecret instead */
   /* note: there may be some padding at the end due to alignment on 64 bytes */
}; /* typedef'd to XXH3_state_t */

#undef XXH_ALIGN_MEMBER

/* When the XXH3_state_t structure is merely emplaced on stack,
 * it should be initialized with XXH3_INITSTATE() or a memset()
 * in case its first reset uses XXH3_NNbits_reset_withSeed().
 * This init can be omitted if the first reset uses default or _withSecret mode.
 * This operation isn't necessary when the state is created with XXH3_createState().
 * Note that this doesn't prepare the state for a streaming operation,
 * it's still necessary to use XXH3_NNbits_reset*() afterwards.
 */
#define XXH3_INITSTATE(XXH3_state_ptr)   { (XXH3_state_ptr)->seed = 0; }


XXH_PUBLIC_API void XXH3_generateSecret(void* secretBuffer, const void* customSeed, size_t customSeedSize);


/* simple short-cut to pre-selected XXH3_128bits variant */
XXH_PUBLIC_API XXH128_hash_t XXH128(const void* data, size_t len, XXH64_hash_t seed);


#endif  /* XXH_NO_LONG_LONG */


#if defined(XXH_INLINE_ALL) || defined(XXH_PRIVATE_API)
#  define XXH_IMPLEMENTATION
#endif

#endif  /* defined(XXH_STATIC_LINKING_ONLY) && !defined(XXHASH_H_STATIC_13879238742) */



#if ( defined(XXH_INLINE_ALL) || defined(XXH_PRIVATE_API) \
   || defined(XXH_IMPLEMENTATION) ) && !defined(XXH_IMPLEM_13a8737387)
#  define XXH_IMPLEM_13a8737387

#ifndef XXH_FORCE_MEMORY_ACCESS   /* can be defined externally, on command line for example */
#  if !defined(__clang__) && defined(__GNUC__) && defined(__ARM_FEATURE_UNALIGNED) && defined(__ARM_ARCH) && (__ARM_ARCH == 6)
#    define XXH_FORCE_MEMORY_ACCESS 2
#  elif !defined(__clang__) && ((defined(__INTEL_COMPILER) && !defined(_WIN32)) || \
  (defined(__GNUC__) && (defined(__ARM_ARCH) && __ARM_ARCH >= 7)))
#    define XXH_FORCE_MEMORY_ACCESS 1
#  endif
#endif

#ifndef XXH_ACCEPT_NULL_INPUT_POINTER   /* can be defined externally */
#  define XXH_ACCEPT_NULL_INPUT_POINTER 0
#endif

#ifndef XXH_FORCE_ALIGN_CHECK  /* can be defined externally */
#  if defined(__i386)  || defined(__x86_64__) || defined(__aarch64__) \
   || defined(_M_IX86) || defined(_M_X64)     || defined(_M_ARM64) /* visual */
#    define XXH_FORCE_ALIGN_CHECK 0
#  else
#    define XXH_FORCE_ALIGN_CHECK 1
#  endif
#endif

#ifndef XXH_NO_INLINE_HINTS
#  if defined(__OPTIMIZE_SIZE__) /* -Os, -Oz */ \
   || defined(__NO_INLINE__)     /* -O0, -fno-inline */
#    define XXH_NO_INLINE_HINTS 1
#  else
#    define XXH_NO_INLINE_HINTS 0
#  endif
#endif

#ifndef XXH_REROLL
#  if defined(__OPTIMIZE_SIZE__)
#    define XXH_REROLL 1
#  else
#    define XXH_REROLL 0
#  endif
#endif


/* *************************************
*  Includes & Memory related functions
***************************************/
/*!
 * Modify the local functions below should you wish to use
 * different memory routines for malloc() and free()
 */


static void* XXH_malloc(size_t s) { return malloc(s); }
static void XXH_free(void* p) { free(p); }

/*! and for memcpy() */
static void* XXH_memcpy(void* dest, const void* src, size_t size)
{
    return memcpy(dest,src,size);
}



/* *************************************
*  Compiler Specific Options
***************************************/
#ifdef _MSC_VER /* Visual Studio warning fix */
#  pragma warning(disable : 4127) /* disable: C4127: conditional expression is constant */
#endif

#if XXH_NO_INLINE_HINTS  /* disable inlining hints */
#  if defined(__GNUC__)
#    define XXH_FORCE_INLINE static __attribute__((unused))
#  else
#    define XXH_FORCE_INLINE static
#  endif
#  define XXH_NO_INLINE static
/* enable inlining hints */
#elif defined(_MSC_VER)  /* Visual Studio */
#  define XXH_FORCE_INLINE static __forceinline
#  define XXH_NO_INLINE static __declspec(noinline)
#elif defined(__GNUC__)
#  define XXH_FORCE_INLINE static __inline__ __attribute__((always_inline, unused))
#  define XXH_NO_INLINE static __attribute__((noinline))
#elif defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))   /* C99 */
#  define XXH_FORCE_INLINE static inline
#  define XXH_NO_INLINE static
#else
#  define XXH_FORCE_INLINE static
#  define XXH_NO_INLINE static
#endif



#ifndef XXH_DEBUGLEVEL
#  ifdef DEBUGLEVEL /* backwards compat */
#    define XXH_DEBUGLEVEL DEBUGLEVEL
#  else
#    define XXH_DEBUGLEVEL 0
#  endif
#endif

#if (XXH_DEBUGLEVEL>=1)
#  include <assert.h>   /* note: can still be disabled with NDEBUG */
#  define XXH_ASSERT(c)   assert(c)
#else
#  define XXH_ASSERT(c)   ((void)0)
#endif

/* note: use after variable declarations */
#define XXH_STATIC_ASSERT(c)  do { enum { XXH_sa = 1/(int)(!!(c)) }; } while (0)


/* *************************************
*  Basic Types
***************************************/
#if !defined (__VMS) \
 && (defined (__cplusplus) \
 || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
# include <stdint.h>
  typedef uint8_t xxh_u8;
#else
  typedef unsigned char xxh_u8;
#endif
typedef XXH32_hash_t xxh_u32;

#ifdef XXH_OLD_NAMES
#  define BYTE xxh_u8
#  define U8   xxh_u8
#  define U32  xxh_u32
#endif

/* ***   Memory access   *** */

#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))
/*
 * Manual byteshift. Best for old compilers which don't inline memcpy.
 * We actually directly use XXH_readLE32 and XXH_readBE32.
 */
#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==2))

/*
 * Force direct memory access. Only works on CPU which support unaligned memory
 * access in hardware.
 */
static xxh_u32 XXH_read32(const void* memPtr) { return *(const xxh_u32*) memPtr; }

#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==1))

/*
 * __pack instructions are safer but compiler specific, hence potentially
 * problematic for some compilers.
 *
 * Currently only defined for GCC and ICC.
 */
#ifdef XXH_OLD_NAMES
typedef union { xxh_u32 u32; } __attribute__((packed)) unalign;
#endif
static xxh_u32 XXH_read32(const void* ptr)
{
    typedef union { xxh_u32 u32; } __attribute__((packed)) xxh_unalign;
    return ((const xxh_unalign*)ptr)->u32;
}

#else

/*
 * Portable and safe solution. Generally efficient.
 * see: https://stackoverflow.com/a/32095106/646947
 */
static xxh_u32 XXH_read32(const void* memPtr)
{
    xxh_u32 val;
    memcpy(&val, memPtr, sizeof(val));
    return val;
}

#endif   /* XXH_FORCE_DIRECT_MEMORY_ACCESS */


/* ***   Endianess   *** */
typedef enum { XXH_bigEndian=0, XXH_littleEndian=1 } XXH_endianess;

/*!
 * XXH_CPU_LITTLE_ENDIAN:
 * Defined to 1 if the target is little endian, or 0 if it is big endian.
 * It can be defined externally, for example on the compiler command line.
 *
 * If it is not defined, a runtime check (which is usually constant folded)
 * is used instead.
 */
#ifndef XXH_CPU_LITTLE_ENDIAN
/*
 * Try to detect endianness automatically, to avoid the nonstandard behavior
 * in `XXH_isLittleEndian()`
 */
#  if defined(_WIN32) /* Windows is always little endian */ \
     || defined(__LITTLE_ENDIAN__) \
     || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#    define XXH_CPU_LITTLE_ENDIAN 1
#  elif defined(__BIG_ENDIAN__) \
     || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define XXH_CPU_LITTLE_ENDIAN 0
#  else
/*
 * runtime test, presumed to simplify to a constant by compiler
 */
static int XXH_isLittleEndian(void)
{
    /*
     * Portable and well-defined behavior.
     * Don't use static: it is detrimental to performance.
     */
    const union { xxh_u32 u; xxh_u8 c[4]; } one = { 1 };
    return one.c[0];
}
#   define XXH_CPU_LITTLE_ENDIAN   XXH_isLittleEndian()
#  endif
#endif




/* ****************************************
*  Compiler-specific Functions and Macros
******************************************/
#define XXH_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

#ifdef __has_builtin
#  define XXH_HAS_BUILTIN(x) __has_builtin(x)
#else
#  define XXH_HAS_BUILTIN(x) 0
#endif

#if !defined(NO_CLANG_BUILTIN) && XXH_HAS_BUILTIN(__builtin_rotateleft32) \
                               && XXH_HAS_BUILTIN(__builtin_rotateleft64)
#  define XXH_rotl32 __builtin_rotateleft32
#  define XXH_rotl64 __builtin_rotateleft64
/* Note: although _rotl exists for minGW (GCC under windows), performance seems poor */
#elif defined(_MSC_VER)
#  define XXH_rotl32(x,r) _rotl(x,r)
#  define XXH_rotl64(x,r) _rotl64(x,r)
#else
#  define XXH_rotl32(x,r) (((x) << (r)) | ((x) >> (32 - (r))))
#  define XXH_rotl64(x,r) (((x) << (r)) | ((x) >> (64 - (r))))
#endif

#if defined(_MSC_VER)     /* Visual Studio */
#  define XXH_swap32 _byteswap_ulong
#elif XXH_GCC_VERSION >= 403
#  define XXH_swap32 __builtin_bswap32
#else
static xxh_u32 XXH_swap32 (xxh_u32 x)
{
    return  ((x << 24) & 0xff000000 ) |
            ((x <<  8) & 0x00ff0000 ) |
            ((x >>  8) & 0x0000ff00 ) |
            ((x >> 24) & 0x000000ff );
}
#endif


/* ***************************
*  Memory reads
*****************************/
typedef enum { XXH_aligned, XXH_unaligned } XXH_alignment;

/*
 * XXH_FORCE_MEMORY_ACCESS==3 is an endian-independent byteshift load.
 *
 * This is ideal for older compilers which don't inline memcpy.
 */
#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))

XXH_FORCE_INLINE xxh_u32 XXH_readLE32(const void* memPtr)
{
    const xxh_u8* bytePtr = (const xxh_u8 *)memPtr;
    return bytePtr[0]
         | ((xxh_u32)bytePtr[1] << 8)
         | ((xxh_u32)bytePtr[2] << 16)
         | ((xxh_u32)bytePtr[3] << 24);
}

XXH_FORCE_INLINE xxh_u32 XXH_readBE32(const void* memPtr)
{
    const xxh_u8* bytePtr = (const xxh_u8 *)memPtr;
    return bytePtr[3]
         | ((xxh_u32)bytePtr[2] << 8)
         | ((xxh_u32)bytePtr[1] << 16)
         | ((xxh_u32)bytePtr[0] << 24);
}

#else
XXH_FORCE_INLINE xxh_u32 XXH_readLE32(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_read32(ptr) : XXH_swap32(XXH_read32(ptr));
}

static xxh_u32 XXH_readBE32(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_swap32(XXH_read32(ptr)) : XXH_read32(ptr);
}
#endif

XXH_FORCE_INLINE xxh_u32
XXH_readLE32_align(const void* ptr, XXH_alignment align)
{
    if (align==XXH_unaligned) {
        return XXH_readLE32(ptr);
    } else {
        return XXH_CPU_LITTLE_ENDIAN ? *(const xxh_u32*)ptr : XXH_swap32(*(const xxh_u32*)ptr);
    }
}


/* *************************************
*  Misc
***************************************/
XXH_PUBLIC_API unsigned XXH_versionNumber (void) { return XXH_VERSION_NUMBER; }


/* *******************************************************************
*  32-bit hash functions
*********************************************************************/
static const xxh_u32 XXH_PRIME32_1 = 0x9E3779B1U;   /* 0b10011110001101110111100110110001 */
static const xxh_u32 XXH_PRIME32_2 = 0x85EBCA77U;   /* 0b10000101111010111100101001110111 */
static const xxh_u32 XXH_PRIME32_3 = 0xC2B2AE3DU;   /* 0b11000010101100101010111000111101 */
static const xxh_u32 XXH_PRIME32_4 = 0x27D4EB2FU;   /* 0b00100111110101001110101100101111 */
static const xxh_u32 XXH_PRIME32_5 = 0x165667B1U;   /* 0b00010110010101100110011110110001 */

#ifdef XXH_OLD_NAMES
#  define PRIME32_1 XXH_PRIME32_1
#  define PRIME32_2 XXH_PRIME32_2
#  define PRIME32_3 XXH_PRIME32_3
#  define PRIME32_4 XXH_PRIME32_4
#  define PRIME32_5 XXH_PRIME32_5
#endif

static xxh_u32 XXH32_round(xxh_u32 acc, xxh_u32 input)
{
    acc += input * XXH_PRIME32_2;
    acc  = XXH_rotl32(acc, 13);
    acc *= XXH_PRIME32_1;
#if defined(__GNUC__) && defined(__SSE4_1__) && !defined(XXH_ENABLE_AUTOVECTORIZE)
    __asm__("" : "+r" (acc));
#endif
    return acc;
}

/* mix all bits */
static xxh_u32 XXH32_avalanche(xxh_u32 h32)
{
    h32 ^= h32 >> 15;
    h32 *= XXH_PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= XXH_PRIME32_3;
    h32 ^= h32 >> 16;
    return(h32);
}

#define XXH_get32bits(p) XXH_readLE32_align(p, align)

static xxh_u32
XXH32_finalize(xxh_u32 h32, const xxh_u8* ptr, size_t len, XXH_alignment align)
{
#define XXH_PROCESS1 do {                           \
    h32 += (*ptr++) * XXH_PRIME32_5;                \
    h32 = XXH_rotl32(h32, 11) * XXH_PRIME32_1;      \
} while (0)

#define XXH_PROCESS4 do {                           \
    h32 += XXH_get32bits(ptr) * XXH_PRIME32_3;      \
    ptr += 4;                                   \
    h32  = XXH_rotl32(h32, 17) * XXH_PRIME32_4;     \
} while (0)

    /* Compact rerolled version */
    if (XXH_REROLL) {
        len &= 15;
        while (len >= 4) {
            XXH_PROCESS4;
            len -= 4;
        }
        while (len > 0) {
            XXH_PROCESS1;
            --len;
        }
        return XXH32_avalanche(h32);
    } else {
         switch(len&15) /* or switch(bEnd - p) */ {
           case 12:      XXH_PROCESS4;
                         /* fallthrough */
           case 8:       XXH_PROCESS4;
                         /* fallthrough */
           case 4:       XXH_PROCESS4;
                         return XXH32_avalanche(h32);

           case 13:      XXH_PROCESS4;
                         /* fallthrough */
           case 9:       XXH_PROCESS4;
                         /* fallthrough */
           case 5:       XXH_PROCESS4;
                         XXH_PROCESS1;
                         return XXH32_avalanche(h32);

           case 14:      XXH_PROCESS4;
                         /* fallthrough */
           case 10:      XXH_PROCESS4;
                         /* fallthrough */
           case 6:       XXH_PROCESS4;
                         XXH_PROCESS1;
                         XXH_PROCESS1;
                         return XXH32_avalanche(h32);

           case 15:      XXH_PROCESS4;
                         /* fallthrough */
           case 11:      XXH_PROCESS4;
                         /* fallthrough */
           case 7:       XXH_PROCESS4;
                         /* fallthrough */
           case 3:       XXH_PROCESS1;
                         /* fallthrough */
           case 2:       XXH_PROCESS1;
                         /* fallthrough */
           case 1:       XXH_PROCESS1;
                         /* fallthrough */
           case 0:       return XXH32_avalanche(h32);
        }
        XXH_ASSERT(0);
        return h32;   /* reaching this point is deemed impossible */
    }
}

#ifdef XXH_OLD_NAMES
#  define PROCESS1 XXH_PROCESS1
#  define PROCESS4 XXH_PROCESS4
#else
#  undef XXH_PROCESS1
#  undef XXH_PROCESS4
#endif

XXH_FORCE_INLINE xxh_u32
XXH32_endian_align(const xxh_u8* input, size_t len, xxh_u32 seed, XXH_alignment align)
{
    const xxh_u8* bEnd = input + len;
    xxh_u32 h32;

#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
    if (input==NULL) {
        len=0;
        bEnd=input=(const xxh_u8*)(size_t)16;
    }
#endif

    if (len>=16) {
        const xxh_u8* const limit = bEnd - 15;
        xxh_u32 v1 = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
        xxh_u32 v2 = seed + XXH_PRIME32_2;
        xxh_u32 v3 = seed + 0;
        xxh_u32 v4 = seed - XXH_PRIME32_1;

        do {
            v1 = XXH32_round(v1, XXH_get32bits(input)); input += 4;
            v2 = XXH32_round(v2, XXH_get32bits(input)); input += 4;
            v3 = XXH32_round(v3, XXH_get32bits(input)); input += 4;
            v4 = XXH32_round(v4, XXH_get32bits(input)); input += 4;
        } while (input < limit);

        h32 = XXH_rotl32(v1, 1)  + XXH_rotl32(v2, 7)
            + XXH_rotl32(v3, 12) + XXH_rotl32(v4, 18);
    } else {
        h32  = seed + XXH_PRIME32_5;
    }

    h32 += (xxh_u32)len;

    return XXH32_finalize(h32, input, len&15, align);
}


XXH_PUBLIC_API XXH32_hash_t XXH32 (const void* input, size_t len, XXH32_hash_t seed)
{
#if 0
    /* Simple version, good for code maintenance, but unfortunately slow for small inputs */
    XXH32_state_t state;
    XXH32_reset(&state, seed);
    XXH32_update(&state, (const xxh_u8*)input, len);
    return XXH32_digest(&state);

#else

    if (XXH_FORCE_ALIGN_CHECK) {
        if ((((size_t)input) & 3) == 0) {   /* Input is 4-bytes aligned, leverage the speed benefit */
            return XXH32_endian_align((const xxh_u8*)input, len, seed, XXH_aligned);
    }   }

    return XXH32_endian_align((const xxh_u8*)input, len, seed, XXH_unaligned);
#endif
}



/*******   Hash streaming   *******/

XXH_PUBLIC_API XXH32_state_t* XXH32_createState(void)
{
    return (XXH32_state_t*)XXH_malloc(sizeof(XXH32_state_t));
}
XXH_PUBLIC_API XXH_errorcode XXH32_freeState(XXH32_state_t* statePtr)
{
    XXH_free(statePtr);
    return XXH_OK;
}

XXH_PUBLIC_API void XXH32_copyState(XXH32_state_t* dstState, const XXH32_state_t* srcState)
{
    memcpy(dstState, srcState, sizeof(*dstState));
}

XXH_PUBLIC_API XXH_errorcode XXH32_reset(XXH32_state_t* statePtr, XXH32_hash_t seed)
{
    XXH32_state_t state;   /* using a local state to memcpy() in order to avoid strict-aliasing warnings */
    memset(&state, 0, sizeof(state));
    state.v1 = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
    state.v2 = seed + XXH_PRIME32_2;
    state.v3 = seed + 0;
    state.v4 = seed - XXH_PRIME32_1;
    /* do not write into reserved, planned to be removed in a future version */
    memcpy(statePtr, &state, sizeof(state) - sizeof(state.reserved));
    return XXH_OK;
}


XXH_PUBLIC_API XXH_errorcode
XXH32_update(XXH32_state_t* state, const void* input, size_t len)
{
    if (input==NULL)
#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
        return XXH_OK;
#else
        return XXH_ERROR;
#endif

    {   const xxh_u8* p = (const xxh_u8*)input;
        const xxh_u8* const bEnd = p + len;

        state->total_len_32 += (XXH32_hash_t)len;
        state->large_len |= (XXH32_hash_t)((len>=16) | (state->total_len_32>=16));

        if (state->memsize + len < 16)  {   /* fill in tmp buffer */
            XXH_memcpy((xxh_u8*)(state->mem32) + state->memsize, input, len);
            state->memsize += (XXH32_hash_t)len;
            return XXH_OK;
        }

        if (state->memsize) {   /* some data left from previous update */
            XXH_memcpy((xxh_u8*)(state->mem32) + state->memsize, input, 16-state->memsize);
            {   const xxh_u32* p32 = state->mem32;
                state->v1 = XXH32_round(state->v1, XXH_readLE32(p32)); p32++;
                state->v2 = XXH32_round(state->v2, XXH_readLE32(p32)); p32++;
                state->v3 = XXH32_round(state->v3, XXH_readLE32(p32)); p32++;
                state->v4 = XXH32_round(state->v4, XXH_readLE32(p32));
            }
            p += 16-state->memsize;
            state->memsize = 0;
        }

        if (p <= bEnd-16) {
            const xxh_u8* const limit = bEnd - 16;
            xxh_u32 v1 = state->v1;
            xxh_u32 v2 = state->v2;
            xxh_u32 v3 = state->v3;
            xxh_u32 v4 = state->v4;

            do {
                v1 = XXH32_round(v1, XXH_readLE32(p)); p+=4;
                v2 = XXH32_round(v2, XXH_readLE32(p)); p+=4;
                v3 = XXH32_round(v3, XXH_readLE32(p)); p+=4;
                v4 = XXH32_round(v4, XXH_readLE32(p)); p+=4;
            } while (p<=limit);

            state->v1 = v1;
            state->v2 = v2;
            state->v3 = v3;
            state->v4 = v4;
        }

        if (p < bEnd) {
            XXH_memcpy(state->mem32, p, (size_t)(bEnd-p));
            state->memsize = (unsigned)(bEnd-p);
        }
    }

    return XXH_OK;
}


XXH_PUBLIC_API XXH32_hash_t XXH32_digest (const XXH32_state_t* state)
{
    xxh_u32 h32;

    if (state->large_len) {
        h32 = XXH_rotl32(state->v1, 1)
            + XXH_rotl32(state->v2, 7)
            + XXH_rotl32(state->v3, 12)
            + XXH_rotl32(state->v4, 18);
    } else {
        h32 = state->v3 /* == seed */ + XXH_PRIME32_5;
    }

    h32 += state->total_len_32;

    return XXH32_finalize(h32, (const xxh_u8*)state->mem32, state->memsize, XXH_aligned);
}


XXH_PUBLIC_API void XXH32_canonicalFromHash(XXH32_canonical_t* dst, XXH32_hash_t hash)
{
    XXH_STATIC_ASSERT(sizeof(XXH32_canonical_t) == sizeof(XXH32_hash_t));
    if (XXH_CPU_LITTLE_ENDIAN) hash = XXH_swap32(hash);
    memcpy(dst, &hash, sizeof(*dst));
}

XXH_PUBLIC_API XXH32_hash_t XXH32_hashFromCanonical(const XXH32_canonical_t* src)
{
    return XXH_readBE32(src);
}


#ifndef XXH_NO_LONG_LONG

/* *******************************************************************
*  64-bit hash functions
*********************************************************************/

/*******   Memory access   *******/

typedef XXH64_hash_t xxh_u64;

#ifdef XXH_OLD_NAMES
#  define U64 xxh_u64
#endif

#ifndef XXH_REROLL_XXH64
#  if (defined(__ILP32__) || defined(_ILP32)) /* ILP32 is often defined on 32-bit GCC family */ \
   || !(defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) /* x86-64 */ \
     || defined(_M_ARM64) || defined(__aarch64__) || defined(__arm64__) /* aarch64 */ \
     || defined(__PPC64__) || defined(__PPC64LE__) || defined(__ppc64__) || defined(__powerpc64__) /* ppc64 */ \
     || defined(__mips64__) || defined(__mips64)) /* mips64 */ \
   || (!defined(SIZE_MAX) || SIZE_MAX < ULLONG_MAX) /* check limits */
#    define XXH_REROLL_XXH64 1
#  else
#    define XXH_REROLL_XXH64 0
#  endif
#endif /* !defined(XXH_REROLL_XXH64) */

#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))
/*
 * Manual byteshift. Best for old compilers which don't inline memcpy.
 * We actually directly use XXH_readLE64 and XXH_readBE64.
 */
#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==2))

/* Force direct memory access. Only works on CPU which support unaligned memory access in hardware */
static xxh_u64 XXH_read64(const void* memPtr) { return *(const xxh_u64*) memPtr; }

#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==1))

/*
 * __pack instructions are safer, but compiler specific, hence potentially
 * problematic for some compilers.
 *
 * Currently only defined for GCC and ICC.
 */
#ifdef XXH_OLD_NAMES
typedef union { xxh_u32 u32; xxh_u64 u64; } __attribute__((packed)) unalign64;
#endif
static xxh_u64 XXH_read64(const void* ptr)
{
    typedef union { xxh_u32 u32; xxh_u64 u64; } __attribute__((packed)) xxh_unalign64;
    return ((const xxh_unalign64*)ptr)->u64;
}

#else

/*
 * Portable and safe solution. Generally efficient.
 * see: https://stackoverflow.com/a/32095106/646947
 */
static xxh_u64 XXH_read64(const void* memPtr)
{
    xxh_u64 val;
    memcpy(&val, memPtr, sizeof(val));
    return val;
}

#endif   /* XXH_FORCE_DIRECT_MEMORY_ACCESS */

#if defined(_MSC_VER)     /* Visual Studio */
#  define XXH_swap64 _byteswap_uint64
#elif XXH_GCC_VERSION >= 403
#  define XXH_swap64 __builtin_bswap64
#else
static xxh_u64 XXH_swap64 (xxh_u64 x)
{
    return  ((x << 56) & 0xff00000000000000ULL) |
            ((x << 40) & 0x00ff000000000000ULL) |
            ((x << 24) & 0x0000ff0000000000ULL) |
            ((x << 8)  & 0x000000ff00000000ULL) |
            ((x >> 8)  & 0x00000000ff000000ULL) |
            ((x >> 24) & 0x0000000000ff0000ULL) |
            ((x >> 40) & 0x000000000000ff00ULL) |
            ((x >> 56) & 0x00000000000000ffULL);
}
#endif


/* XXH_FORCE_MEMORY_ACCESS==3 is an endian-independent byteshift load. */
#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))

XXH_FORCE_INLINE xxh_u64 XXH_readLE64(const void* memPtr)
{
    const xxh_u8* bytePtr = (const xxh_u8 *)memPtr;
    return bytePtr[0]
         | ((xxh_u64)bytePtr[1] << 8)
         | ((xxh_u64)bytePtr[2] << 16)
         | ((xxh_u64)bytePtr[3] << 24)
         | ((xxh_u64)bytePtr[4] << 32)
         | ((xxh_u64)bytePtr[5] << 40)
         | ((xxh_u64)bytePtr[6] << 48)
         | ((xxh_u64)bytePtr[7] << 56);
}

XXH_FORCE_INLINE xxh_u64 XXH_readBE64(const void* memPtr)
{
    const xxh_u8* bytePtr = (const xxh_u8 *)memPtr;
    return bytePtr[7]
         | ((xxh_u64)bytePtr[6] << 8)
         | ((xxh_u64)bytePtr[5] << 16)
         | ((xxh_u64)bytePtr[4] << 24)
         | ((xxh_u64)bytePtr[3] << 32)
         | ((xxh_u64)bytePtr[2] << 40)
         | ((xxh_u64)bytePtr[1] << 48)
         | ((xxh_u64)bytePtr[0] << 56);
}

#else
XXH_FORCE_INLINE xxh_u64 XXH_readLE64(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_read64(ptr) : XXH_swap64(XXH_read64(ptr));
}

static xxh_u64 XXH_readBE64(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_swap64(XXH_read64(ptr)) : XXH_read64(ptr);
}
#endif

XXH_FORCE_INLINE xxh_u64
XXH_readLE64_align(const void* ptr, XXH_alignment align)
{
    if (align==XXH_unaligned)
        return XXH_readLE64(ptr);
    else
        return XXH_CPU_LITTLE_ENDIAN ? *(const xxh_u64*)ptr : XXH_swap64(*(const xxh_u64*)ptr);
}


/*******   xxh64   *******/

static const xxh_u64 XXH_PRIME64_1 = 0x9E3779B185EBCA87ULL;   /* 0b1001111000110111011110011011000110000101111010111100101010000111 */
static const xxh_u64 XXH_PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;   /* 0b1100001010110010101011100011110100100111110101001110101101001111 */
static const xxh_u64 XXH_PRIME64_3 = 0x165667B19E3779F9ULL;   /* 0b0001011001010110011001111011000110011110001101110111100111111001 */
static const xxh_u64 XXH_PRIME64_4 = 0x85EBCA77C2B2AE63ULL;   /* 0b1000010111101011110010100111011111000010101100101010111001100011 */
static const xxh_u64 XXH_PRIME64_5 = 0x27D4EB2F165667C5ULL;   /* 0b0010011111010100111010110010111100010110010101100110011111000101 */

#ifdef XXH_OLD_NAMES
#  define PRIME64_1 XXH_PRIME64_1
#  define PRIME64_2 XXH_PRIME64_2
#  define PRIME64_3 XXH_PRIME64_3
#  define PRIME64_4 XXH_PRIME64_4
#  define PRIME64_5 XXH_PRIME64_5
#endif

static xxh_u64 XXH64_round(xxh_u64 acc, xxh_u64 input)
{
    acc += input * XXH_PRIME64_2;
    acc  = XXH_rotl64(acc, 31);
    acc *= XXH_PRIME64_1;
    return acc;
}

static xxh_u64 XXH64_mergeRound(xxh_u64 acc, xxh_u64 val)
{
    val  = XXH64_round(0, val);
    acc ^= val;
    acc  = acc * XXH_PRIME64_1 + XXH_PRIME64_4;
    return acc;
}

static xxh_u64 XXH64_avalanche(xxh_u64 h64)
{
    h64 ^= h64 >> 33;
    h64 *= XXH_PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= XXH_PRIME64_3;
    h64 ^= h64 >> 32;
    return h64;
}


#define XXH_get64bits(p) XXH_readLE64_align(p, align)

static xxh_u64
XXH64_finalize(xxh_u64 h64, const xxh_u8* ptr, size_t len, XXH_alignment align)
{
#define XXH_PROCESS1_64 do {                                   \
    h64 ^= (*ptr++) * XXH_PRIME64_5;                           \
    h64 = XXH_rotl64(h64, 11) * XXH_PRIME64_1;                 \
} while (0)

#define XXH_PROCESS4_64 do {                                   \
    h64 ^= (xxh_u64)(XXH_get32bits(ptr)) * XXH_PRIME64_1;      \
    ptr += 4;                                              \
    h64 = XXH_rotl64(h64, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;     \
} while (0)

#define XXH_PROCESS8_64 do {                                   \
    xxh_u64 const k1 = XXH64_round(0, XXH_get64bits(ptr)); \
    ptr += 8;                                              \
    h64 ^= k1;                                             \
    h64  = XXH_rotl64(h64,27) * XXH_PRIME64_1 + XXH_PRIME64_4;     \
} while (0)

    /* Rerolled version for 32-bit targets is faster and much smaller. */
    if (XXH_REROLL || XXH_REROLL_XXH64) {
        len &= 31;
        while (len >= 8) {
            XXH_PROCESS8_64;
            len -= 8;
        }
        if (len >= 4) {
            XXH_PROCESS4_64;
            len -= 4;
        }
        while (len > 0) {
            XXH_PROCESS1_64;
            --len;
        }
         return  XXH64_avalanche(h64);
    } else {
        switch(len & 31) {
           case 24: XXH_PROCESS8_64;
                         /* fallthrough */
           case 16: XXH_PROCESS8_64;
                         /* fallthrough */
           case  8: XXH_PROCESS8_64;
                    return XXH64_avalanche(h64);

           case 28: XXH_PROCESS8_64;
                         /* fallthrough */
           case 20: XXH_PROCESS8_64;
                         /* fallthrough */
           case 12: XXH_PROCESS8_64;
                         /* fallthrough */
           case  4: XXH_PROCESS4_64;
                    return XXH64_avalanche(h64);

           case 25: XXH_PROCESS8_64;
                         /* fallthrough */
           case 17: XXH_PROCESS8_64;
                         /* fallthrough */
           case  9: XXH_PROCESS8_64;
                    XXH_PROCESS1_64;
                    return XXH64_avalanche(h64);

           case 29: XXH_PROCESS8_64;
                         /* fallthrough */
           case 21: XXH_PROCESS8_64;
                         /* fallthrough */
           case 13: XXH_PROCESS8_64;
                         /* fallthrough */
           case  5: XXH_PROCESS4_64;
                    XXH_PROCESS1_64;
                    return XXH64_avalanche(h64);

           case 26: XXH_PROCESS8_64;
                         /* fallthrough */
           case 18: XXH_PROCESS8_64;
                         /* fallthrough */
           case 10: XXH_PROCESS8_64;
                    XXH_PROCESS1_64;
                    XXH_PROCESS1_64;
                    return XXH64_avalanche(h64);

           case 30: XXH_PROCESS8_64;
                         /* fallthrough */
           case 22: XXH_PROCESS8_64;
                         /* fallthrough */
           case 14: XXH_PROCESS8_64;
                         /* fallthrough */
           case  6: XXH_PROCESS4_64;
                    XXH_PROCESS1_64;
                    XXH_PROCESS1_64;
                    return XXH64_avalanche(h64);

           case 27: XXH_PROCESS8_64;
                         /* fallthrough */
           case 19: XXH_PROCESS8_64;
                         /* fallthrough */
           case 11: XXH_PROCESS8_64;
                    XXH_PROCESS1_64;
                    XXH_PROCESS1_64;
                    XXH_PROCESS1_64;
                    return XXH64_avalanche(h64);

           case 31: XXH_PROCESS8_64;
                         /* fallthrough */
           case 23: XXH_PROCESS8_64;
                         /* fallthrough */
           case 15: XXH_PROCESS8_64;
                         /* fallthrough */
           case  7: XXH_PROCESS4_64;
                         /* fallthrough */
           case  3: XXH_PROCESS1_64;
                         /* fallthrough */
           case  2: XXH_PROCESS1_64;
                         /* fallthrough */
           case  1: XXH_PROCESS1_64;
                         /* fallthrough */
           case  0: return XXH64_avalanche(h64);
        }
    }
    /* impossible to reach */
    XXH_ASSERT(0);
    return 0;  /* unreachable, but some compilers complain without it */
}

#ifdef XXH_OLD_NAMES
#  define PROCESS1_64 XXH_PROCESS1_64
#  define PROCESS4_64 XXH_PROCESS4_64
#  define PROCESS8_64 XXH_PROCESS8_64
#else
#  undef XXH_PROCESS1_64
#  undef XXH_PROCESS4_64
#  undef XXH_PROCESS8_64
#endif

XXH_FORCE_INLINE xxh_u64
XXH64_endian_align(const xxh_u8* input, size_t len, xxh_u64 seed, XXH_alignment align)
{
    const xxh_u8* bEnd = input + len;
    xxh_u64 h64;

#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
    if (input==NULL) {
        len=0;
        bEnd=input=(const xxh_u8*)(size_t)32;
    }
#endif

    if (len>=32) {
        const xxh_u8* const limit = bEnd - 32;
        xxh_u64 v1 = seed + XXH_PRIME64_1 + XXH_PRIME64_2;
        xxh_u64 v2 = seed + XXH_PRIME64_2;
        xxh_u64 v3 = seed + 0;
        xxh_u64 v4 = seed - XXH_PRIME64_1;

        do {
            v1 = XXH64_round(v1, XXH_get64bits(input)); input+=8;
            v2 = XXH64_round(v2, XXH_get64bits(input)); input+=8;
            v3 = XXH64_round(v3, XXH_get64bits(input)); input+=8;
            v4 = XXH64_round(v4, XXH_get64bits(input)); input+=8;
        } while (input<=limit);

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);
        h64 = XXH64_mergeRound(h64, v1);
        h64 = XXH64_mergeRound(h64, v2);
        h64 = XXH64_mergeRound(h64, v3);
        h64 = XXH64_mergeRound(h64, v4);

    } else {
        h64  = seed + XXH_PRIME64_5;
    }

    h64 += (xxh_u64) len;

    return XXH64_finalize(h64, input, len, align);
}


XXH_PUBLIC_API XXH64_hash_t XXH64 (const void* input, size_t len, XXH64_hash_t seed)
{
#if 0
    /* Simple version, good for code maintenance, but unfortunately slow for small inputs */
    XXH64_state_t state;
    XXH64_reset(&state, seed);
    XXH64_update(&state, (const xxh_u8*)input, len);
    return XXH64_digest(&state);

#else

    if (XXH_FORCE_ALIGN_CHECK) {
        if ((((size_t)input) & 7)==0) {  /* Input is aligned, let's leverage the speed advantage */
            return XXH64_endian_align((const xxh_u8*)input, len, seed, XXH_aligned);
    }   }

    return XXH64_endian_align((const xxh_u8*)input, len, seed, XXH_unaligned);

#endif
}

/*******   Hash Streaming   *******/

XXH_PUBLIC_API XXH64_state_t* XXH64_createState(void)
{
    return (XXH64_state_t*)XXH_malloc(sizeof(XXH64_state_t));
}
XXH_PUBLIC_API XXH_errorcode XXH64_freeState(XXH64_state_t* statePtr)
{
    XXH_free(statePtr);
    return XXH_OK;
}

XXH_PUBLIC_API void XXH64_copyState(XXH64_state_t* dstState, const XXH64_state_t* srcState)
{
    memcpy(dstState, srcState, sizeof(*dstState));
}

XXH_PUBLIC_API XXH_errorcode XXH64_reset(XXH64_state_t* statePtr, XXH64_hash_t seed)
{
    XXH64_state_t state;   /* use a local state to memcpy() in order to avoid strict-aliasing warnings */
    memset(&state, 0, sizeof(state));
    state.v1 = seed + XXH_PRIME64_1 + XXH_PRIME64_2;
    state.v2 = seed + XXH_PRIME64_2;
    state.v3 = seed + 0;
    state.v4 = seed - XXH_PRIME64_1;
     /* do not write into reserved64, might be removed in a future version */
    memcpy(statePtr, &state, sizeof(state) - sizeof(state.reserved64));
    return XXH_OK;
}

XXH_PUBLIC_API XXH_errorcode
XXH64_update (XXH64_state_t* state, const void* input, size_t len)
{
    if (input==NULL)
#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
        return XXH_OK;
#else
        return XXH_ERROR;
#endif

    {   const xxh_u8* p = (const xxh_u8*)input;
        const xxh_u8* const bEnd = p + len;

        state->total_len += len;

        if (state->memsize + len < 32) {  /* fill in tmp buffer */
            XXH_memcpy(((xxh_u8*)state->mem64) + state->memsize, input, len);
            state->memsize += (xxh_u32)len;
            return XXH_OK;
        }

        if (state->memsize) {   /* tmp buffer is full */
            XXH_memcpy(((xxh_u8*)state->mem64) + state->memsize, input, 32-state->memsize);
            state->v1 = XXH64_round(state->v1, XXH_readLE64(state->mem64+0));
            state->v2 = XXH64_round(state->v2, XXH_readLE64(state->mem64+1));
            state->v3 = XXH64_round(state->v3, XXH_readLE64(state->mem64+2));
            state->v4 = XXH64_round(state->v4, XXH_readLE64(state->mem64+3));
            p += 32-state->memsize;
            state->memsize = 0;
        }

        if (p+32 <= bEnd) {
            const xxh_u8* const limit = bEnd - 32;
            xxh_u64 v1 = state->v1;
            xxh_u64 v2 = state->v2;
            xxh_u64 v3 = state->v3;
            xxh_u64 v4 = state->v4;

            do {
                v1 = XXH64_round(v1, XXH_readLE64(p)); p+=8;
                v2 = XXH64_round(v2, XXH_readLE64(p)); p+=8;
                v3 = XXH64_round(v3, XXH_readLE64(p)); p+=8;
                v4 = XXH64_round(v4, XXH_readLE64(p)); p+=8;
            } while (p<=limit);

            state->v1 = v1;
            state->v2 = v2;
            state->v3 = v3;
            state->v4 = v4;
        }

        if (p < bEnd) {
            XXH_memcpy(state->mem64, p, (size_t)(bEnd-p));
            state->memsize = (unsigned)(bEnd-p);
        }
    }

    return XXH_OK;
}


XXH_PUBLIC_API XXH64_hash_t XXH64_digest (const XXH64_state_t* state)
{
    xxh_u64 h64;

    if (state->total_len >= 32) {
        xxh_u64 const v1 = state->v1;
        xxh_u64 const v2 = state->v2;
        xxh_u64 const v3 = state->v3;
        xxh_u64 const v4 = state->v4;

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);
        h64 = XXH64_mergeRound(h64, v1);
        h64 = XXH64_mergeRound(h64, v2);
        h64 = XXH64_mergeRound(h64, v3);
        h64 = XXH64_mergeRound(h64, v4);
    } else {
        h64  = state->v3 /*seed*/ + XXH_PRIME64_5;
    }

    h64 += (xxh_u64) state->total_len;

    return XXH64_finalize(h64, (const xxh_u8*)state->mem64, (size_t)state->total_len, XXH_aligned);
}


/******* Canonical representation   *******/

XXH_PUBLIC_API void XXH64_canonicalFromHash(XXH64_canonical_t* dst, XXH64_hash_t hash)
{
    XXH_STATIC_ASSERT(sizeof(XXH64_canonical_t) == sizeof(XXH64_hash_t));
    if (XXH_CPU_LITTLE_ENDIAN) hash = XXH_swap64(hash);
    memcpy(dst, &hash, sizeof(*dst));
}

XXH_PUBLIC_API XXH64_hash_t XXH64_hashFromCanonical(const XXH64_canonical_t* src)
{
    return XXH_readBE64(src);
}



/* *********************************************************************
*  XXH3
*  New generation hash designed for speed on small keys and vectorization
************************************************************************ */

/* ===   Compiler specifics   === */

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* >= C99 */
#  define XXH_RESTRICT   restrict
#else
/* Note: it might be useful to define __restrict or __restrict__ for some C++ compilers */
#  define XXH_RESTRICT   /* disable */
#endif

#if (defined(__GNUC__) && (__GNUC__ >= 3))  \
  || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800)) \
  || defined(__clang__)
#    define XXH_likely(x) __builtin_expect(x, 1)
#    define XXH_unlikely(x) __builtin_expect(x, 0)
#else
#    define XXH_likely(x) (x)
#    define XXH_unlikely(x) (x)
#endif

#if defined(__GNUC__)
#  if defined(__AVX2__)
#    include <immintrin.h>
#  elif defined(__SSE2__)
#    include <emmintrin.h>
#  elif defined(__ARM_NEON__) || defined(__ARM_NEON)
#    define inline __inline__  /* circumvent a clang bug */
#    include <arm_neon.h>
#    undef inline
#  endif
#elif defined(_MSC_VER)
#  include <intrin.h>
#endif

#if defined(__thumb__) && !defined(__thumb2__) && defined(__ARM_ARCH_ISA_ARM)
#   warning "XXH3 is highly inefficient without ARM or Thumb-2."
#endif

/* ==========================================
 * Vectorization detection
 * ========================================== */
#define XXH_SCALAR 0  /* Portable scalar version */
#define XXH_SSE2   1  /* SSE2 for Pentium 4 and all x86_64 */
#define XXH_AVX2   2  /* AVX2 for Haswell and Bulldozer */
#define XXH_AVX512 3  /* AVX512 for Skylake and Icelake */
#define XXH_NEON   4  /* NEON for most ARMv7-A and all AArch64 */
#define XXH_VSX    5  /* VSX and ZVector for POWER8/z13 */

#ifndef XXH_VECTOR    /* can be defined on command line */
#  if defined(__AVX512F__)
#    define XXH_VECTOR XXH_AVX512
#  elif defined(__AVX2__)
#    define XXH_VECTOR XXH_AVX2
#  elif defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP == 2))
#    define XXH_VECTOR XXH_SSE2
#  elif defined(__GNUC__) /* msvc support maybe later */ \
  && (defined(__ARM_NEON__) || defined(__ARM_NEON)) \
  && (defined(__LITTLE_ENDIAN__) /* We only support little endian NEON */ \
    || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#    define XXH_VECTOR XXH_NEON
#  elif (defined(__PPC64__) && defined(__POWER8_VECTOR__)) \
     || (defined(__s390x__) && defined(__VEC__)) \
     && defined(__GNUC__) /* TODO: IBM XL */
#    define XXH_VECTOR XXH_VSX
#  else
#    define XXH_VECTOR XXH_SCALAR
#  endif
#endif

/*
 * Controls the alignment of the accumulator,
 * for compatibility with aligned vector loads, which are usually faster.
 */
#ifndef XXH_ACC_ALIGN
#  if defined(XXH_X86DISPATCH)
#     define XXH_ACC_ALIGN 64  /* for compatibility with avx512 */
#  elif XXH_VECTOR == XXH_SCALAR  /* scalar */
#     define XXH_ACC_ALIGN 8
#  elif XXH_VECTOR == XXH_SSE2  /* sse2 */
#     define XXH_ACC_ALIGN 16
#  elif XXH_VECTOR == XXH_AVX2  /* avx2 */
#     define XXH_ACC_ALIGN 32
#  elif XXH_VECTOR == XXH_NEON  /* neon */
#     define XXH_ACC_ALIGN 16
#  elif XXH_VECTOR == XXH_VSX   /* vsx */
#     define XXH_ACC_ALIGN 16
#  elif XXH_VECTOR == XXH_AVX512  /* avx512 */
#     define XXH_ACC_ALIGN 64
#  endif
#endif

#if defined(XXH_X86DISPATCH) || XXH_VECTOR == XXH_SSE2 \
    || XXH_VECTOR == XXH_AVX2 || XXH_VECTOR == XXH_AVX512
#  define XXH_SEC_ALIGN XXH_ACC_ALIGN
#else
#  define XXH_SEC_ALIGN 8
#endif

#if XXH_VECTOR == XXH_AVX2 /* AVX2 */ \
  && defined(__GNUC__) && !defined(__clang__) /* GCC, not Clang */ \
  && defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__) /* respect -O0 and -Os */
#  pragma GCC push_options
#  pragma GCC optimize("-O2")
#endif


#if XXH_VECTOR == XXH_NEON
# if !defined(XXH_NO_VZIP_HACK) /* define to disable */ \
   && defined(__GNUC__) \
   && !defined(__aarch64__) && !defined(__arm64__)
#  define XXH_SPLIT_IN_PLACE(in, outLo, outHi)                                              \
    do {                                                                                    \
      /* Undocumented GCC/Clang operand modifier: %e0 = lower D half, %f0 = upper D half */ \
      /* https://github.com/gcc-mirror/gcc/blob/38cf91e5/gcc/config/arm/arm.c#L22486 */     \
      /* https://github.com/llvm-mirror/llvm/blob/2c4ca683/lib/Target/ARM/ARMAsmPrinter.cpp#L399 */ \
      __asm__("vzip.32  %e0, %f0" : "+w" (in));                                             \
      (outLo) = vget_low_u32 (vreinterpretq_u32_u64(in));                                   \
      (outHi) = vget_high_u32(vreinterpretq_u32_u64(in));                                   \
   } while (0)
# else
#  define XXH_SPLIT_IN_PLACE(in, outLo, outHi)                                            \
    do {                                                                                  \
      (outLo) = vmovn_u64    (in);                                                        \
      (outHi) = vshrn_n_u64  ((in), 32);                                                  \
    } while (0)
# endif
#endif  /* XXH_VECTOR == XXH_NEON */

/*
 * VSX and Z Vector helpers.
 *
 * This is very messy, and any pull requests to clean this up are welcome.
 *
 * There are a lot of problems with supporting VSX and s390x, due to
 * inconsistent intrinsics, spotty coverage, and multiple endiannesses.
 */
#if XXH_VECTOR == XXH_VSX
#  if defined(__s390x__)
#    include <s390intrin.h>
#  else
/* gcc's altivec.h can have the unwanted consequence to unconditionally
 * #define bool, vector, and pixel keywords,
 * with bad consequences for programs already using these keywords for other purposes.
 * The paragraph defining these macros is skipped when __APPLE_ALTIVEC__ is defined.
 * __APPLE_ALTIVEC__ is _generally_ defined automatically by the compiler,
 * but it seems that, in some cases, it isn't.
 * Force the build macro to be defined, so that keywords are not altered.
 */
#    if defined(__GNUC__) && !defined(__APPLE_ALTIVEC__)
#      define __APPLE_ALTIVEC__
#    endif
#    include <altivec.h>
#  endif

typedef __vector unsigned long long xxh_u64x2;
typedef __vector unsigned char xxh_u8x16;
typedef __vector unsigned xxh_u32x4;

# ifndef XXH_VSX_BE
#  if defined(__BIG_ENDIAN__) \
  || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define XXH_VSX_BE 1
#  elif defined(__VEC_ELEMENT_REG_ORDER__) && __VEC_ELEMENT_REG_ORDER__ == __ORDER_BIG_ENDIAN__
#    warning "-maltivec=be is not recommended. Please use native endianness."
#    define XXH_VSX_BE 1
#  else
#    define XXH_VSX_BE 0
#  endif
# endif /* !defined(XXH_VSX_BE) */

# if XXH_VSX_BE
/* A wrapper for POWER9's vec_revb. */
#  if defined(__POWER9_VECTOR__) || (defined(__clang__) && defined(__s390x__))
#    define XXH_vec_revb vec_revb
#  else
XXH_FORCE_INLINE xxh_u64x2 XXH_vec_revb(xxh_u64x2 val)
{
    xxh_u8x16 const vByteSwap = { 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
                                  0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08 };
    return vec_perm(val, val, vByteSwap);
}
#  endif
# endif /* XXH_VSX_BE */

/*
 * Performs an unaligned load and byte swaps it on big endian.
 */
XXH_FORCE_INLINE xxh_u64x2 XXH_vec_loadu(const void *ptr)
{
    xxh_u64x2 ret;
    memcpy(&ret, ptr, sizeof(xxh_u64x2));
# if XXH_VSX_BE
    ret = XXH_vec_revb(ret);
# endif
    return ret;
}

/*
 * vec_mulo and vec_mule are very problematic intrinsics on PowerPC
 *
 * These intrinsics weren't added until GCC 8, despite existing for a while,
 * and they are endian dependent. Also, their meaning swap depending on version.
 * */
# if defined(__s390x__)
 /* s390x is always big endian, no issue on this platform */
#  define XXH_vec_mulo vec_mulo
#  define XXH_vec_mule vec_mule
# elif defined(__clang__) && XXH_HAS_BUILTIN(__builtin_altivec_vmuleuw)
/* Clang has a better way to control this, we can just use the builtin which doesn't swap. */
#  define XXH_vec_mulo __builtin_altivec_vmulouw
#  define XXH_vec_mule __builtin_altivec_vmuleuw
# else
/* gcc needs inline assembly */
/* Adapted from https://github.com/google/highwayhash/blob/master/highwayhash/hh_vsx.h. */
XXH_FORCE_INLINE xxh_u64x2 XXH_vec_mulo(xxh_u32x4 a, xxh_u32x4 b)
{
    xxh_u64x2 result;
    __asm__("vmulouw %0, %1, %2" : "=v" (result) : "v" (a), "v" (b));
    return result;
}
XXH_FORCE_INLINE xxh_u64x2 XXH_vec_mule(xxh_u32x4 a, xxh_u32x4 b)
{
    xxh_u64x2 result;
    __asm__("vmuleuw %0, %1, %2" : "=v" (result) : "v" (a), "v" (b));
    return result;
}
# endif /* XXH_vec_mulo, XXH_vec_mule */
#endif /* XXH_VECTOR == XXH_VSX */


/* prefetch
 * can be disabled, by declaring XXH_NO_PREFETCH build macro */
#if defined(XXH_NO_PREFETCH)
#  define XXH_PREFETCH(ptr)  (void)(ptr)  /* disabled */
#else
#  if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_I86))  /* _mm_prefetch() is not defined outside of x86/x64 */
#    include <mmintrin.h>   /* https://msdn.microsoft.com/fr-fr/library/84szxsww(v=vs.90).aspx */
#    define XXH_PREFETCH(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_T0)
#  elif defined(__GNUC__) && ( (__GNUC__ >= 4) || ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) ) )
#    define XXH_PREFETCH(ptr)  __builtin_prefetch((ptr), 0 /* rw==read */, 3 /* locality */)
#  else
#    define XXH_PREFETCH(ptr) (void)(ptr)  /* disabled */
#  endif
#endif  /* XXH_NO_PREFETCH */


/* ==========================================
 * XXH3 default settings
 * ========================================== */

#define XXH_SECRET_DEFAULT_SIZE 192   /* minimum XXH3_SECRET_SIZE_MIN */

#if (XXH_SECRET_DEFAULT_SIZE < XXH3_SECRET_SIZE_MIN)
#  error "default keyset is not large enough"
#endif

/* Pseudorandom secret taken directly from FARSH */
XXH_ALIGN(64) static const xxh_u8 XXH3_kSecret[XXH_SECRET_DEFAULT_SIZE] = {
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
    0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
    0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
    0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb, 0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
    0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
    0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31, 0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
    0xea, 0xc5, 0xac, 0x83, 0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e,
    0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce,
    0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
};


#ifdef XXH_OLD_NAMES
#  define kSecret XXH3_kSecret
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
#    include <intrin.h>
#    define XXH_mult32to64(x, y) __emulu((unsigned)(x), (unsigned)(y))
#else
/*
 * Downcast + upcast is usually better than masking on older compilers like
 * GCC 4.2 (especially 32-bit ones), all without affecting newer compilers.
 *
 * The other method, (x & 0xFFFFFFFF) * (y & 0xFFFFFFFF), will AND both operands
 * and perform a full 64x64 multiply -- entirely redundant on 32-bit.
 */
#    define XXH_mult32to64(x, y) ((xxh_u64)(xxh_u32)(x) * (xxh_u64)(xxh_u32)(y))
#endif

/*
 * Calculates a 64->128-bit long multiply.
 *
 * Uses __uint128_t and _umul128 if available, otherwise uses a scalar version.
 */
static XXH128_hash_t
XXH_mult64to128(xxh_u64 lhs, xxh_u64 rhs)
{
#if defined(__GNUC__) && !defined(__wasm__) \
    && defined(__SIZEOF_INT128__) \
    || (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 128)

    __uint128_t const product = (__uint128_t)lhs * (__uint128_t)rhs;
    XXH128_hash_t r128;
    r128.low64  = (xxh_u64)(product);
    r128.high64 = (xxh_u64)(product >> 64);
    return r128;

    /*
     * MSVC for x64's _umul128 method.
     *
     * xxh_u64 _umul128(xxh_u64 Multiplier, xxh_u64 Multiplicand, xxh_u64 *HighProduct);
     *
     * This compiles to single operand MUL on x64.
     */
#elif defined(_M_X64) || defined(_M_IA64)

#ifndef _MSC_VER
#   pragma intrinsic(_umul128)
#endif
    xxh_u64 product_high;
    xxh_u64 const product_low = _umul128(lhs, rhs, &product_high);
    XXH128_hash_t r128;
    r128.low64  = product_low;
    r128.high64 = product_high;
    return r128;

#else

    /* First calculate all of the cross products. */
    xxh_u64 const lo_lo = XXH_mult32to64(lhs & 0xFFFFFFFF, rhs & 0xFFFFFFFF);
    xxh_u64 const hi_lo = XXH_mult32to64(lhs >> 32,        rhs & 0xFFFFFFFF);
    xxh_u64 const lo_hi = XXH_mult32to64(lhs & 0xFFFFFFFF, rhs >> 32);
    xxh_u64 const hi_hi = XXH_mult32to64(lhs >> 32,        rhs >> 32);

    /* Now add the products together. These will never overflow. */
    xxh_u64 const cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    xxh_u64 const upper = (hi_lo >> 32) + (cross >> 32)        + hi_hi;
    xxh_u64 const lower = (cross << 32) | (lo_lo & 0xFFFFFFFF);

    XXH128_hash_t r128;
    r128.low64  = lower;
    r128.high64 = upper;
    return r128;
#endif
}

static xxh_u64
XXH3_mul128_fold64(xxh_u64 lhs, xxh_u64 rhs)
{
    XXH128_hash_t product = XXH_mult64to128(lhs, rhs);
    return product.low64 ^ product.high64;
}

/* Seems to produce slightly better code on GCC for some reason. */
XXH_FORCE_INLINE xxh_u64 XXH_xorshift64(xxh_u64 v64, int shift)
{
    XXH_ASSERT(0 <= shift && shift < 64);
    return v64 ^ (v64 >> shift);
}

/*
 * This is a fast avalanche stage,
 * suitable when input bits are already partially mixed
 */
static XXH64_hash_t XXH3_avalanche(xxh_u64 h64)
{
    h64 = XXH_xorshift64(h64, 37);
    h64 *= 0x165667919E3779F9ULL;
    h64 = XXH_xorshift64(h64, 32);
    return h64;
}

/*
 * This is a stronger avalanche,
 * inspired by Pelle Evensen's rrmxmx
 * preferable when input has not been previously mixed
 */
static XXH64_hash_t XXH3_rrmxmx(xxh_u64 h64, xxh_u64 len)
{
    /* this mix is inspired by Pelle Evensen's rrmxmx */
    h64 ^= XXH_rotl64(h64, 49) ^ XXH_rotl64(h64, 24);
    h64 *= 0x9FB21C651E98DF25ULL;
    h64 ^= (h64 >> 35) + len ;
    h64 *= 0x9FB21C651E98DF25ULL;
    return XXH_xorshift64(h64, 28);
}


XXH_FORCE_INLINE XXH64_hash_t
XXH3_len_1to3_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(1 <= len && len <= 3);
    XXH_ASSERT(secret != NULL);
    /*
     * len = 1: combined = { input[0], 0x01, input[0], input[0] }
     * len = 2: combined = { input[1], 0x02, input[0], input[1] }
     * len = 3: combined = { input[2], 0x03, input[0], input[1] }
     */
    {   xxh_u8  const c1 = input[0];
        xxh_u8  const c2 = input[len >> 1];
        xxh_u8  const c3 = input[len - 1];
        xxh_u32 const combined = ((xxh_u32)c1 << 16) | ((xxh_u32)c2  << 24)
                               | ((xxh_u32)c3 <<  0) | ((xxh_u32)len << 8);
        xxh_u64 const bitflip = (XXH_readLE32(secret) ^ XXH_readLE32(secret+4)) + seed;
        xxh_u64 const keyed = (xxh_u64)combined ^ bitflip;
        return XXH64_avalanche(keyed);
    }
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_len_4to8_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(secret != NULL);
    XXH_ASSERT(4 <= len && len < 8);
    seed ^= (xxh_u64)XXH_swap32((xxh_u32)seed) << 32;
    {   xxh_u32 const input1 = XXH_readLE32(input);
        xxh_u32 const input2 = XXH_readLE32(input + len - 4);
        xxh_u64 const bitflip = (XXH_readLE64(secret+8) ^ XXH_readLE64(secret+16)) - seed;
        xxh_u64 const input64 = input2 + (((xxh_u64)input1) << 32);
        xxh_u64 const keyed = input64 ^ bitflip;
        return XXH3_rrmxmx(keyed, len);
    }
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_len_9to16_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(secret != NULL);
    XXH_ASSERT(8 <= len && len <= 16);
    {   xxh_u64 const bitflip1 = (XXH_readLE64(secret+24) ^ XXH_readLE64(secret+32)) + seed;
        xxh_u64 const bitflip2 = (XXH_readLE64(secret+40) ^ XXH_readLE64(secret+48)) - seed;
        xxh_u64 const input_lo = XXH_readLE64(input)           ^ bitflip1;
        xxh_u64 const input_hi = XXH_readLE64(input + len - 8) ^ bitflip2;
        xxh_u64 const acc = len
                          + XXH_swap64(input_lo) + input_hi
                          + XXH3_mul128_fold64(input_lo, input_hi);
        return XXH3_avalanche(acc);
    }
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_len_0to16_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(len <= 16);
    {   if (XXH_likely(len >  8)) return XXH3_len_9to16_64b(input, len, secret, seed);
        if (XXH_likely(len >= 4)) return XXH3_len_4to8_64b(input, len, secret, seed);
        if (len) return XXH3_len_1to3_64b(input, len, secret, seed);
        return XXH64_avalanche(seed ^ (XXH_readLE64(secret+56) ^ XXH_readLE64(secret+64)));
    }
}

XXH_FORCE_INLINE xxh_u64 XXH3_mix16B(const xxh_u8* XXH_RESTRICT input,
                                     const xxh_u8* XXH_RESTRICT secret, xxh_u64 seed64)
{
#if defined(__GNUC__) && !defined(__clang__) /* GCC, not Clang */ \
  && defined(__i386__) && defined(__SSE2__)  /* x86 + SSE2 */ \
  && !defined(XXH_ENABLE_AUTOVECTORIZE)      /* Define to disable like XXH32 hack */
    __asm__ ("" : "+r" (seed64));
#endif
    {   xxh_u64 const input_lo = XXH_readLE64(input);
        xxh_u64 const input_hi = XXH_readLE64(input+8);
        return XXH3_mul128_fold64(
            input_lo ^ (XXH_readLE64(secret)   + seed64),
            input_hi ^ (XXH_readLE64(secret+8) - seed64)
        );
    }
}

/* For mid range keys, XXH3 uses a Mum-hash variant. */
XXH_FORCE_INLINE XXH64_hash_t
XXH3_len_17to128_64b(const xxh_u8* XXH_RESTRICT input, size_t len,
                     const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                     XXH64_hash_t seed)
{
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN); (void)secretSize;
    XXH_ASSERT(16 < len && len <= 128);

    {   xxh_u64 acc = len * XXH_PRIME64_1;
        if (len > 32) {
            if (len > 64) {
                if (len > 96) {
                    acc += XXH3_mix16B(input+48, secret+96, seed);
                    acc += XXH3_mix16B(input+len-64, secret+112, seed);
                }
                acc += XXH3_mix16B(input+32, secret+64, seed);
                acc += XXH3_mix16B(input+len-48, secret+80, seed);
            }
            acc += XXH3_mix16B(input+16, secret+32, seed);
            acc += XXH3_mix16B(input+len-32, secret+48, seed);
        }
        acc += XXH3_mix16B(input+0, secret+0, seed);
        acc += XXH3_mix16B(input+len-16, secret+16, seed);

        return XXH3_avalanche(acc);
    }
}

#define XXH3_MIDSIZE_MAX 240

XXH_NO_INLINE XXH64_hash_t
XXH3_len_129to240_64b(const xxh_u8* XXH_RESTRICT input, size_t len,
                      const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                      XXH64_hash_t seed)
{
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN); (void)secretSize;
    XXH_ASSERT(128 < len && len <= XXH3_MIDSIZE_MAX);

    #define XXH3_MIDSIZE_STARTOFFSET 3
    #define XXH3_MIDSIZE_LASTOFFSET  17

    {   xxh_u64 acc = len * XXH_PRIME64_1;
        int const nbRounds = (int)len / 16;
        int i;
        for (i=0; i<8; i++) {
            acc += XXH3_mix16B(input+(16*i), secret+(16*i), seed);
        }
        acc = XXH3_avalanche(acc);
        XXH_ASSERT(nbRounds >= 8);
#if defined(__clang__)                                /* Clang */ \
    && (defined(__ARM_NEON) || defined(__ARM_NEON__)) /* NEON */ \
    && !defined(XXH_ENABLE_AUTOVECTORIZE)             /* Define to disable */
        #pragma clang loop vectorize(disable)
#endif
        for (i=8 ; i < nbRounds; i++) {
            acc += XXH3_mix16B(input+(16*i), secret+(16*(i-8)) + XXH3_MIDSIZE_STARTOFFSET, seed);
        }
        /* last bytes */
        acc += XXH3_mix16B(input + len - 16, secret + XXH3_SECRET_SIZE_MIN - XXH3_MIDSIZE_LASTOFFSET, seed);
        return XXH3_avalanche(acc);
    }
}


/* =======     Long Keys     ======= */

#define XXH_STRIPE_LEN 64
#define XXH_SECRET_CONSUME_RATE 8   /* nb of secret bytes consumed at each accumulation */
#define XXH_ACC_NB (XXH_STRIPE_LEN / sizeof(xxh_u64))

#ifdef XXH_OLD_NAMES
#  define STRIPE_LEN XXH_STRIPE_LEN
#  define ACC_NB XXH_ACC_NB
#endif

XXH_FORCE_INLINE void XXH_writeLE64(void* dst, xxh_u64 v64)
{
    if (!XXH_CPU_LITTLE_ENDIAN) v64 = XXH_swap64(v64);
    memcpy(dst, &v64, sizeof(v64));
}

/* Several intrinsic functions below are supposed to accept __int64 as argument,
 * as documented in https://software.intel.com/sites/landingpage/IntrinsicsGuide/ .
 * However, several environments do not define __int64 type,
 * requiring a workaround.
 */
#if !defined (__VMS) \
  && (defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
    typedef int64_t xxh_i64;
#else
    /* the following type must have a width of 64-bit */
    typedef long long xxh_i64;
#endif


#if (XXH_VECTOR == XXH_AVX512) || defined(XXH_X86DISPATCH)

#ifndef XXH_TARGET_AVX512
# define XXH_TARGET_AVX512  /* disable attribute target */
#endif

XXH_FORCE_INLINE XXH_TARGET_AVX512 void
XXH3_accumulate_512_avx512(void* XXH_RESTRICT acc,
                     const void* XXH_RESTRICT input,
                     const void* XXH_RESTRICT secret)
{
    XXH_ALIGN(64) __m512i* const xacc = (__m512i *) acc;
    XXH_ASSERT((((size_t)acc) & 63) == 0);
    XXH_STATIC_ASSERT(XXH_STRIPE_LEN == sizeof(__m512i));

    {
        /* data_vec    = input[0]; */
        __m512i const data_vec    = _mm512_loadu_si512   (input);
        /* key_vec     = secret[0]; */
        __m512i const key_vec     = _mm512_loadu_si512   (secret);
        /* data_key    = data_vec ^ key_vec; */
        __m512i const data_key    = _mm512_xor_si512     (data_vec, key_vec);
        /* data_key_lo = data_key >> 32; */
        __m512i const data_key_lo = _mm512_shuffle_epi32 (data_key, (_MM_PERM_ENUM)_MM_SHUFFLE(0, 3, 0, 1));
        /* product     = (data_key & 0xffffffff) * (data_key_lo & 0xffffffff); */
        __m512i const product     = _mm512_mul_epu32     (data_key, data_key_lo);
        /* xacc[0] += swap(data_vec); */
        __m512i const data_swap = _mm512_shuffle_epi32(data_vec, (_MM_PERM_ENUM)_MM_SHUFFLE(1, 0, 3, 2));
        __m512i const sum       = _mm512_add_epi64(*xacc, data_swap);
        /* xacc[0] += product; */
        *xacc = _mm512_add_epi64(product, sum);
    }
}


XXH_FORCE_INLINE XXH_TARGET_AVX512 void
XXH3_scrambleAcc_avx512(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 63) == 0);
    XXH_STATIC_ASSERT(XXH_STRIPE_LEN == sizeof(__m512i));
    {   XXH_ALIGN(64) __m512i* const xacc = (__m512i*) acc;
        const __m512i prime32 = _mm512_set1_epi32((int)XXH_PRIME32_1);

        /* xacc[0] ^= (xacc[0] >> 47) */
        __m512i const acc_vec     = *xacc;
        __m512i const shifted     = _mm512_srli_epi64    (acc_vec, 47);
        __m512i const data_vec    = _mm512_xor_si512     (acc_vec, shifted);
        /* xacc[0] ^= secret; */
        __m512i const key_vec     = _mm512_loadu_si512   (secret);
        __m512i const data_key    = _mm512_xor_si512     (data_vec, key_vec);

        /* xacc[0] *= XXH_PRIME32_1; */
        __m512i const data_key_hi = _mm512_shuffle_epi32 (data_key, (_MM_PERM_ENUM)_MM_SHUFFLE(0, 3, 0, 1));
        __m512i const prod_lo     = _mm512_mul_epu32     (data_key, prime32);
        __m512i const prod_hi     = _mm512_mul_epu32     (data_key_hi, prime32);
        *xacc = _mm512_add_epi64(prod_lo, _mm512_slli_epi64(prod_hi, 32));
    }
}

XXH_FORCE_INLINE XXH_TARGET_AVX512 void
XXH3_initCustomSecret_avx512(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 63) == 0);
    XXH_STATIC_ASSERT(XXH_SEC_ALIGN == 64);
    XXH_ASSERT(((size_t)customSecret & 63) == 0);
    (void)(&XXH_writeLE64);
    {   int const nbRounds = XXH_SECRET_DEFAULT_SIZE / sizeof(__m512i);
        __m512i const seed = _mm512_mask_set1_epi64(_mm512_set1_epi64((xxh_i64)seed64), 0xAA, -(xxh_i64)seed64);

        XXH_ALIGN(64) const __m512i* const src  = (const __m512i*) XXH3_kSecret;
        XXH_ALIGN(64)       __m512i* const dest = (      __m512i*) customSecret;
        int i;
        for (i=0; i < nbRounds; ++i) {
            /* GCC has a bug, _mm512_stream_load_si512 accepts 'void*', not 'void const*',
             * this will warn "discards ?const? qualifier". */
            union {
                XXH_ALIGN(64) const __m512i* cp;
                XXH_ALIGN(64) void* p;
            } remote_const_void;
            remote_const_void.cp = src + i;
            dest[i] = _mm512_add_epi64(_mm512_stream_load_si512(remote_const_void.p), seed);
    }   }
}

#endif

#if (XXH_VECTOR == XXH_AVX2) || defined(XXH_X86DISPATCH)

#ifndef XXH_TARGET_AVX2
# define XXH_TARGET_AVX2  /* disable attribute target */
#endif

XXH_FORCE_INLINE XXH_TARGET_AVX2 void
XXH3_accumulate_512_avx2( void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 31) == 0);
    {   XXH_ALIGN(32) __m256i* const xacc    =       (__m256i *) acc;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm256_loadu_si256 requires  a const __m256i * pointer for some reason. */
        const         __m256i* const xinput  = (const __m256i *) input;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm256_loadu_si256 requires a const __m256i * pointer for some reason. */
        const         __m256i* const xsecret = (const __m256i *) secret;

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m256i); i++) {
            /* data_vec    = xinput[i]; */
            __m256i const data_vec    = _mm256_loadu_si256    (xinput+i);
            /* key_vec     = xsecret[i]; */
            __m256i const key_vec     = _mm256_loadu_si256   (xsecret+i);
            /* data_key    = data_vec ^ key_vec; */
            __m256i const data_key    = _mm256_xor_si256     (data_vec, key_vec);
            /* data_key_lo = data_key >> 32; */
            __m256i const data_key_lo = _mm256_shuffle_epi32 (data_key, _MM_SHUFFLE(0, 3, 0, 1));
            /* product     = (data_key & 0xffffffff) * (data_key_lo & 0xffffffff); */
            __m256i const product     = _mm256_mul_epu32     (data_key, data_key_lo);
            /* xacc[i] += swap(data_vec); */
            __m256i const data_swap = _mm256_shuffle_epi32(data_vec, _MM_SHUFFLE(1, 0, 3, 2));
            __m256i const sum       = _mm256_add_epi64(xacc[i], data_swap);
            /* xacc[i] += product; */
            xacc[i] = _mm256_add_epi64(product, sum);
    }   }
}

XXH_FORCE_INLINE XXH_TARGET_AVX2 void
XXH3_scrambleAcc_avx2(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 31) == 0);
    {   XXH_ALIGN(32) __m256i* const xacc = (__m256i*) acc;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm256_loadu_si256 requires a const __m256i * pointer for some reason. */
        const         __m256i* const xsecret = (const __m256i *) secret;
        const __m256i prime32 = _mm256_set1_epi32((int)XXH_PRIME32_1);

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m256i); i++) {
            /* xacc[i] ^= (xacc[i] >> 47) */
            __m256i const acc_vec     = xacc[i];
            __m256i const shifted     = _mm256_srli_epi64    (acc_vec, 47);
            __m256i const data_vec    = _mm256_xor_si256     (acc_vec, shifted);
            /* xacc[i] ^= xsecret; */
            __m256i const key_vec     = _mm256_loadu_si256   (xsecret+i);
            __m256i const data_key    = _mm256_xor_si256     (data_vec, key_vec);

            /* xacc[i] *= XXH_PRIME32_1; */
            __m256i const data_key_hi = _mm256_shuffle_epi32 (data_key, _MM_SHUFFLE(0, 3, 0, 1));
            __m256i const prod_lo     = _mm256_mul_epu32     (data_key, prime32);
            __m256i const prod_hi     = _mm256_mul_epu32     (data_key_hi, prime32);
            xacc[i] = _mm256_add_epi64(prod_lo, _mm256_slli_epi64(prod_hi, 32));
        }
    }
}

XXH_FORCE_INLINE XXH_TARGET_AVX2 void XXH3_initCustomSecret_avx2(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 31) == 0);
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE / sizeof(__m256i)) == 6);
    XXH_STATIC_ASSERT(XXH_SEC_ALIGN <= 64);
    (void)(&XXH_writeLE64);
    XXH_PREFETCH(customSecret);
    {   __m256i const seed = _mm256_set_epi64x(-(xxh_i64)seed64, (xxh_i64)seed64, -(xxh_i64)seed64, (xxh_i64)seed64);

        XXH_ALIGN(64) const __m256i* const src  = (const __m256i*) XXH3_kSecret;
        XXH_ALIGN(64)       __m256i*       dest = (      __m256i*) customSecret;

#       if defined(__GNUC__) || defined(__clang__)
        /*
         * On GCC & Clang, marking 'dest' as modified will cause the compiler:
         *   - do not extract the secret from sse registers in the internal loop
         *   - use less common registers, and avoid pushing these reg into stack
         * The asm hack causes Clang to assume that XXH3_kSecretPtr aliases with
         * customSecret, and on aarch64, this prevented LDP from merging two
         * loads together for free. Putting the loads together before the stores
         * properly generates LDP.
         */
        __asm__("" : "+r" (dest));
#       endif

        /* GCC -O2 need unroll loop manually */
        dest[0] = _mm256_add_epi64(_mm256_stream_load_si256(src+0), seed);
        dest[1] = _mm256_add_epi64(_mm256_stream_load_si256(src+1), seed);
        dest[2] = _mm256_add_epi64(_mm256_stream_load_si256(src+2), seed);
        dest[3] = _mm256_add_epi64(_mm256_stream_load_si256(src+3), seed);
        dest[4] = _mm256_add_epi64(_mm256_stream_load_si256(src+4), seed);
        dest[5] = _mm256_add_epi64(_mm256_stream_load_si256(src+5), seed);
    }
}

#endif

#if (XXH_VECTOR == XXH_SSE2) || defined(XXH_X86DISPATCH)

#ifndef XXH_TARGET_SSE2
# define XXH_TARGET_SSE2  /* disable attribute target */
#endif

XXH_FORCE_INLINE XXH_TARGET_SSE2 void
XXH3_accumulate_512_sse2( void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{
    /* SSE2 is just a half-scale version of the AVX2 version. */
    XXH_ASSERT((((size_t)acc) & 15) == 0);
    {   XXH_ALIGN(16) __m128i* const xacc    =       (__m128i *) acc;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm_loadu_si128 requires a const __m128i * pointer for some reason. */
        const         __m128i* const xinput  = (const __m128i *) input;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm_loadu_si128 requires a const __m128i * pointer for some reason. */
        const         __m128i* const xsecret = (const __m128i *) secret;

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m128i); i++) {
            /* data_vec    = xinput[i]; */
            __m128i const data_vec    = _mm_loadu_si128   (xinput+i);
            /* key_vec     = xsecret[i]; */
            __m128i const key_vec     = _mm_loadu_si128   (xsecret+i);
            /* data_key    = data_vec ^ key_vec; */
            __m128i const data_key    = _mm_xor_si128     (data_vec, key_vec);
            /* data_key_lo = data_key >> 32; */
            __m128i const data_key_lo = _mm_shuffle_epi32 (data_key, _MM_SHUFFLE(0, 3, 0, 1));
            /* product     = (data_key & 0xffffffff) * (data_key_lo & 0xffffffff); */
            __m128i const product     = _mm_mul_epu32     (data_key, data_key_lo);
            /* xacc[i] += swap(data_vec); */
            __m128i const data_swap = _mm_shuffle_epi32(data_vec, _MM_SHUFFLE(1,0,3,2));
            __m128i const sum       = _mm_add_epi64(xacc[i], data_swap);
            /* xacc[i] += product; */
            xacc[i] = _mm_add_epi64(product, sum);
    }   }
}

XXH_FORCE_INLINE XXH_TARGET_SSE2 void
XXH3_scrambleAcc_sse2(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 15) == 0);
    {   XXH_ALIGN(16) __m128i* const xacc = (__m128i*) acc;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm_loadu_si128 requires a const __m128i * pointer for some reason. */
        const         __m128i* const xsecret = (const __m128i *) secret;
        const __m128i prime32 = _mm_set1_epi32((int)XXH_PRIME32_1);

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m128i); i++) {
            /* xacc[i] ^= (xacc[i] >> 47) */
            __m128i const acc_vec     = xacc[i];
            __m128i const shifted     = _mm_srli_epi64    (acc_vec, 47);
            __m128i const data_vec    = _mm_xor_si128     (acc_vec, shifted);
            /* xacc[i] ^= xsecret[i]; */
            __m128i const key_vec     = _mm_loadu_si128   (xsecret+i);
            __m128i const data_key    = _mm_xor_si128     (data_vec, key_vec);

            /* xacc[i] *= XXH_PRIME32_1; */
            __m128i const data_key_hi = _mm_shuffle_epi32 (data_key, _MM_SHUFFLE(0, 3, 0, 1));
            __m128i const prod_lo     = _mm_mul_epu32     (data_key, prime32);
            __m128i const prod_hi     = _mm_mul_epu32     (data_key_hi, prime32);
            xacc[i] = _mm_add_epi64(prod_lo, _mm_slli_epi64(prod_hi, 32));
        }
    }
}

XXH_FORCE_INLINE XXH_TARGET_SSE2 void XXH3_initCustomSecret_sse2(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 15) == 0);
    (void)(&XXH_writeLE64);
    {   int const nbRounds = XXH_SECRET_DEFAULT_SIZE / sizeof(__m128i);

#       if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER < 1900
        // MSVC 32bit mode does not support _mm_set_epi64x before 2015
        XXH_ALIGN(16) const xxh_i64 seed64x2[2] = { (xxh_i64)seed64, -(xxh_i64)seed64 };
        __m128i const seed = _mm_load_si128((__m128i const*)seed64x2);
#       else
        __m128i const seed = _mm_set_epi64x(-(xxh_i64)seed64, (xxh_i64)seed64);
#       endif
        int i;

        XXH_ALIGN(64)        const float* const src  = (float const*) XXH3_kSecret;
        XXH_ALIGN(XXH_SEC_ALIGN) __m128i*       dest = (__m128i*) customSecret;
#       if defined(__GNUC__) || defined(__clang__)
        /*
         * On GCC & Clang, marking 'dest' as modified will cause the compiler:
         *   - do not extract the secret from sse registers in the internal loop
         *   - use less common registers, and avoid pushing these reg into stack
         */
        __asm__("" : "+r" (dest));
#       endif

        for (i=0; i < nbRounds; ++i) {
            dest[i] = _mm_add_epi64(_mm_castps_si128(_mm_load_ps(src+i*4)), seed);
    }   }
}

#endif

#if (XXH_VECTOR == XXH_NEON)

XXH_FORCE_INLINE void
XXH3_accumulate_512_neon( void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 15) == 0);
    {
        XXH_ALIGN(16) uint64x2_t* const xacc = (uint64x2_t *) acc;
        /* We don't use a uint32x4_t pointer because it causes bus errors on ARMv7. */
        uint8_t const* const xinput = (const uint8_t *) input;
        uint8_t const* const xsecret  = (const uint8_t *) secret;

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN / sizeof(uint64x2_t); i++) {
            /* data_vec = xinput[i]; */
            uint8x16_t data_vec    = vld1q_u8(xinput  + (i * 16));
            /* key_vec  = xsecret[i];  */
            uint8x16_t key_vec     = vld1q_u8(xsecret + (i * 16));
            uint64x2_t data_key;
            uint32x2_t data_key_lo, data_key_hi;
            /* xacc[i] += swap(data_vec); */
            uint64x2_t const data64  = vreinterpretq_u64_u8(data_vec);
            uint64x2_t const swapped = vextq_u64(data64, data64, 1);
            xacc[i] = vaddq_u64 (xacc[i], swapped);
            /* data_key = data_vec ^ key_vec; */
            data_key = vreinterpretq_u64_u8(veorq_u8(data_vec, key_vec));
            /* data_key_lo = (uint32x2_t) (data_key & 0xFFFFFFFF);
             * data_key_hi = (uint32x2_t) (data_key >> 32);
             * data_key = UNDEFINED; */
            XXH_SPLIT_IN_PLACE(data_key, data_key_lo, data_key_hi);
            /* xacc[i] += (uint64x2_t) data_key_lo * (uint64x2_t) data_key_hi; */
            xacc[i] = vmlal_u32 (xacc[i], data_key_lo, data_key_hi);

        }
    }
}

XXH_FORCE_INLINE void
XXH3_scrambleAcc_neon(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 15) == 0);

    {   uint64x2_t* xacc       = (uint64x2_t*) acc;
        uint8_t const* xsecret = (uint8_t const*) secret;
        uint32x2_t prime       = vdup_n_u32 (XXH_PRIME32_1);

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(uint64x2_t); i++) {
            /* xacc[i] ^= (xacc[i] >> 47); */
            uint64x2_t acc_vec  = xacc[i];
            uint64x2_t shifted  = vshrq_n_u64 (acc_vec, 47);
            uint64x2_t data_vec = veorq_u64   (acc_vec, shifted);

            /* xacc[i] ^= xsecret[i]; */
            uint8x16_t key_vec  = vld1q_u8(xsecret + (i * 16));
            uint64x2_t data_key = veorq_u64(data_vec, vreinterpretq_u64_u8(key_vec));

            /* xacc[i] *= XXH_PRIME32_1 */
            uint32x2_t data_key_lo, data_key_hi;
            /* data_key_lo = (uint32x2_t) (xacc[i] & 0xFFFFFFFF);
             * data_key_hi = (uint32x2_t) (xacc[i] >> 32);
             * xacc[i] = UNDEFINED; */
            XXH_SPLIT_IN_PLACE(data_key, data_key_lo, data_key_hi);
            {   /*
                 * prod_hi = (data_key >> 32) * XXH_PRIME32_1;
                 *
                 * Avoid vmul_u32 + vshll_n_u32 since Clang 6 and 7 will
                 * incorrectly "optimize" this:
                 *   tmp     = vmul_u32(vmovn_u64(a), vmovn_u64(b));
                 *   shifted = vshll_n_u32(tmp, 32);
                 * to this:
                 *   tmp     = "vmulq_u64"(a, b); // no such thing!
                 *   shifted = vshlq_n_u64(tmp, 32);
                 *
                 * However, unlike SSE, Clang lacks a 64-bit multiply routine
                 * for NEON, and it scalarizes two 64-bit multiplies instead.
                 *
                 * vmull_u32 has the same timing as vmul_u32, and it avoids
                 * this bug completely.
                 * See https://bugs.llvm.org/show_bug.cgi?id=39967
                 */
                uint64x2_t prod_hi = vmull_u32 (data_key_hi, prime);
                /* xacc[i] = prod_hi << 32; */
                xacc[i] = vshlq_n_u64(prod_hi, 32);
                /* xacc[i] += (prod_hi & 0xFFFFFFFF) * XXH_PRIME32_1; */
                xacc[i] = vmlal_u32(xacc[i], data_key_lo, prime);
            }
    }   }
}

#endif

#if (XXH_VECTOR == XXH_VSX)

XXH_FORCE_INLINE void
XXH3_accumulate_512_vsx(  void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{
          xxh_u64x2* const xacc     =       (xxh_u64x2*) acc;    /* presumed aligned */
    xxh_u64x2 const* const xinput   = (xxh_u64x2 const*) input;   /* no alignment restriction */
    xxh_u64x2 const* const xsecret  = (xxh_u64x2 const*) secret;    /* no alignment restriction */
    xxh_u64x2 const v32 = { 32, 32 };
    size_t i;
    for (i = 0; i < XXH_STRIPE_LEN / sizeof(xxh_u64x2); i++) {
        /* data_vec = xinput[i]; */
        xxh_u64x2 const data_vec = XXH_vec_loadu(xinput + i);
        /* key_vec = xsecret[i]; */
        xxh_u64x2 const key_vec  = XXH_vec_loadu(xsecret + i);
        xxh_u64x2 const data_key = data_vec ^ key_vec;
        /* shuffled = (data_key << 32) | (data_key >> 32); */
        xxh_u32x4 const shuffled = (xxh_u32x4)vec_rl(data_key, v32);
        /* product = ((xxh_u64x2)data_key & 0xFFFFFFFF) * ((xxh_u64x2)shuffled & 0xFFFFFFFF); */
        xxh_u64x2 const product  = XXH_vec_mulo((xxh_u32x4)data_key, shuffled);
        xacc[i] += product;

        /* swap high and low halves */
#ifdef __s390x__
        xacc[i] += vec_permi(data_vec, data_vec, 2);
#else
        xacc[i] += vec_xxpermdi(data_vec, data_vec, 2);
#endif
    }
}

XXH_FORCE_INLINE void
XXH3_scrambleAcc_vsx(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 15) == 0);

    {         xxh_u64x2* const xacc    =       (xxh_u64x2*) acc;
        const xxh_u64x2* const xsecret = (const xxh_u64x2*) secret;
        /* constants */
        xxh_u64x2 const v32  = { 32, 32 };
        xxh_u64x2 const v47 = { 47, 47 };
        xxh_u32x4 const prime = { XXH_PRIME32_1, XXH_PRIME32_1, XXH_PRIME32_1, XXH_PRIME32_1 };
        size_t i;
        for (i = 0; i < XXH_STRIPE_LEN / sizeof(xxh_u64x2); i++) {
            /* xacc[i] ^= (xacc[i] >> 47); */
            xxh_u64x2 const acc_vec  = xacc[i];
            xxh_u64x2 const data_vec = acc_vec ^ (acc_vec >> v47);

            /* xacc[i] ^= xsecret[i]; */
            xxh_u64x2 const key_vec  = XXH_vec_loadu(xsecret + i);
            xxh_u64x2 const data_key = data_vec ^ key_vec;

            /* xacc[i] *= XXH_PRIME32_1 */
            /* prod_lo = ((xxh_u64x2)data_key & 0xFFFFFFFF) * ((xxh_u64x2)prime & 0xFFFFFFFF);  */
            xxh_u64x2 const prod_even  = XXH_vec_mule((xxh_u32x4)data_key, prime);
            /* prod_hi = ((xxh_u64x2)data_key >> 32) * ((xxh_u64x2)prime >> 32);  */
            xxh_u64x2 const prod_odd  = XXH_vec_mulo((xxh_u32x4)data_key, prime);
            xacc[i] = prod_odd + (prod_even << v32);
    }   }
}

#endif

/* scalar variants - universal */

XXH_FORCE_INLINE void
XXH3_accumulate_512_scalar(void* XXH_RESTRICT acc,
                     const void* XXH_RESTRICT input,
                     const void* XXH_RESTRICT secret)
{
    XXH_ALIGN(XXH_ACC_ALIGN) xxh_u64* const xacc = (xxh_u64*) acc; /* presumed aligned */
    const xxh_u8* const xinput  = (const xxh_u8*) input;  /* no alignment restriction */
    const xxh_u8* const xsecret = (const xxh_u8*) secret;   /* no alignment restriction */
    size_t i;
    XXH_ASSERT(((size_t)acc & (XXH_ACC_ALIGN-1)) == 0);
    for (i=0; i < XXH_ACC_NB; i++) {
        xxh_u64 const data_val = XXH_readLE64(xinput + 8*i);
        xxh_u64 const data_key = data_val ^ XXH_readLE64(xsecret + i*8);
        xacc[i ^ 1] += data_val; /* swap adjacent lanes */
        xacc[i] += XXH_mult32to64(data_key & 0xFFFFFFFF, data_key >> 32);
    }
}

XXH_FORCE_INLINE void
XXH3_scrambleAcc_scalar(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ALIGN(XXH_ACC_ALIGN) xxh_u64* const xacc = (xxh_u64*) acc;   /* presumed aligned */
    const xxh_u8* const xsecret = (const xxh_u8*) secret;   /* no alignment restriction */
    size_t i;
    XXH_ASSERT((((size_t)acc) & (XXH_ACC_ALIGN-1)) == 0);
    for (i=0; i < XXH_ACC_NB; i++) {
        xxh_u64 const key64 = XXH_readLE64(xsecret + 8*i);
        xxh_u64 acc64 = xacc[i];
        acc64 = XXH_xorshift64(acc64, 47);
        acc64 ^= key64;
        acc64 *= XXH_PRIME32_1;
        xacc[i] = acc64;
    }
}

XXH_FORCE_INLINE void
XXH3_initCustomSecret_scalar(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    /*
     * We need a separate pointer for the hack below,
     * which requires a non-const pointer.
     * Any decent compiler will optimize this out otherwise.
     */
    const xxh_u8* kSecretPtr = XXH3_kSecret;
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 15) == 0);

#if defined(__clang__) && defined(__aarch64__)
    __asm__("" : "+r" (kSecretPtr));
#endif
    /*
     * Note: in debug mode, this overrides the asm optimization
     * and Clang will emit MOVK chains again.
     */
    XXH_ASSERT(kSecretPtr == XXH3_kSecret);

    {   int const nbRounds = XXH_SECRET_DEFAULT_SIZE / 16;
        int i;
        for (i=0; i < nbRounds; i++) {
            /*
             * The asm hack causes Clang to assume that kSecretPtr aliases with
             * customSecret, and on aarch64, this prevented LDP from merging two
             * loads together for free. Putting the loads together before the stores
             * properly generates LDP.
             */
            xxh_u64 lo = XXH_readLE64(kSecretPtr + 16*i)     + seed64;
            xxh_u64 hi = XXH_readLE64(kSecretPtr + 16*i + 8) - seed64;
            XXH_writeLE64((xxh_u8*)customSecret + 16*i,     lo);
            XXH_writeLE64((xxh_u8*)customSecret + 16*i + 8, hi);
    }   }
}


typedef void (*XXH3_f_accumulate_512)(void* XXH_RESTRICT, const void*, const void*);
typedef void (*XXH3_f_scrambleAcc)(void* XXH_RESTRICT, const void*);
typedef void (*XXH3_f_initCustomSecret)(void* XXH_RESTRICT, xxh_u64);


#if (XXH_VECTOR == XXH_AVX512)

#define XXH3_accumulate_512 XXH3_accumulate_512_avx512
#define XXH3_scrambleAcc    XXH3_scrambleAcc_avx512
#define XXH3_initCustomSecret XXH3_initCustomSecret_avx512

#elif (XXH_VECTOR == XXH_AVX2)

#define XXH3_accumulate_512 XXH3_accumulate_512_avx2
#define XXH3_scrambleAcc    XXH3_scrambleAcc_avx2
#define XXH3_initCustomSecret XXH3_initCustomSecret_avx2

#elif (XXH_VECTOR == XXH_SSE2)

#define XXH3_accumulate_512 XXH3_accumulate_512_sse2
#define XXH3_scrambleAcc    XXH3_scrambleAcc_sse2
#define XXH3_initCustomSecret XXH3_initCustomSecret_sse2

#elif (XXH_VECTOR == XXH_NEON)

#define XXH3_accumulate_512 XXH3_accumulate_512_neon
#define XXH3_scrambleAcc    XXH3_scrambleAcc_neon
#define XXH3_initCustomSecret XXH3_initCustomSecret_scalar

#elif (XXH_VECTOR == XXH_VSX)

#define XXH3_accumulate_512 XXH3_accumulate_512_vsx
#define XXH3_scrambleAcc    XXH3_scrambleAcc_vsx
#define XXH3_initCustomSecret XXH3_initCustomSecret_scalar

#else /* scalar */

#define XXH3_accumulate_512 XXH3_accumulate_512_scalar
#define XXH3_scrambleAcc    XXH3_scrambleAcc_scalar
#define XXH3_initCustomSecret XXH3_initCustomSecret_scalar

#endif



#ifndef XXH_PREFETCH_DIST
#  ifdef __clang__
#    define XXH_PREFETCH_DIST 320
#  else
#    if (XXH_VECTOR == XXH_AVX512)
#      define XXH_PREFETCH_DIST 512
#    else
#      define XXH_PREFETCH_DIST 384
#    endif
#  endif  /* __clang__ */
#endif  /* XXH_PREFETCH_DIST */

/*
 * XXH3_accumulate()
 * Loops over XXH3_accumulate_512().
 * Assumption: nbStripes will not overflow the secret size
 */
XXH_FORCE_INLINE void
XXH3_accumulate(     xxh_u64* XXH_RESTRICT acc,
                const xxh_u8* XXH_RESTRICT input,
                const xxh_u8* XXH_RESTRICT secret,
                      size_t nbStripes,
                      XXH3_f_accumulate_512 f_acc512)
{
    size_t n;
    for (n = 0; n < nbStripes; n++ ) {
        const xxh_u8* const in = input + n*XXH_STRIPE_LEN;
        XXH_PREFETCH(in + XXH_PREFETCH_DIST);
        f_acc512(acc,
                 in,
                 secret + n*XXH_SECRET_CONSUME_RATE);
    }
}

XXH_FORCE_INLINE void
XXH3_hashLong_internal_loop(xxh_u64* XXH_RESTRICT acc,
                      const xxh_u8* XXH_RESTRICT input, size_t len,
                      const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                            XXH3_f_accumulate_512 f_acc512,
                            XXH3_f_scrambleAcc f_scramble)
{
    size_t const nbStripesPerBlock = (secretSize - XXH_STRIPE_LEN) / XXH_SECRET_CONSUME_RATE;
    size_t const block_len = XXH_STRIPE_LEN * nbStripesPerBlock;
    size_t const nb_blocks = (len - 1) / block_len;

    size_t n;

    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN);

    for (n = 0; n < nb_blocks; n++) {
        XXH3_accumulate(acc, input + n*block_len, secret, nbStripesPerBlock, f_acc512);
        f_scramble(acc, secret + secretSize - XXH_STRIPE_LEN);
    }

    /* last partial block */
    XXH_ASSERT(len > XXH_STRIPE_LEN);
    {   size_t const nbStripes = ((len - 1) - (block_len * nb_blocks)) / XXH_STRIPE_LEN;
        XXH_ASSERT(nbStripes <= (secretSize / XXH_SECRET_CONSUME_RATE));
        XXH3_accumulate(acc, input + nb_blocks*block_len, secret, nbStripes, f_acc512);

        /* last stripe */
        {   const xxh_u8* const p = input + len - XXH_STRIPE_LEN;
#define XXH_SECRET_LASTACC_START 7  /* not aligned on 8, last secret is different from acc & scrambler */
            f_acc512(acc, p, secret + secretSize - XXH_STRIPE_LEN - XXH_SECRET_LASTACC_START);
    }   }
}

XXH_FORCE_INLINE xxh_u64
XXH3_mix2Accs(const xxh_u64* XXH_RESTRICT acc, const xxh_u8* XXH_RESTRICT secret)
{
    return XXH3_mul128_fold64(
               acc[0] ^ XXH_readLE64(secret),
               acc[1] ^ XXH_readLE64(secret+8) );
}

static XXH64_hash_t
XXH3_mergeAccs(const xxh_u64* XXH_RESTRICT acc, const xxh_u8* XXH_RESTRICT secret, xxh_u64 start)
{
    xxh_u64 result64 = start;
    size_t i = 0;

    for (i = 0; i < 4; i++) {
        result64 += XXH3_mix2Accs(acc+2*i, secret + 16*i);
#if defined(__clang__)                                /* Clang */ \
    && (defined(__arm__) || defined(__thumb__))       /* ARMv7 */ \
    && (defined(__ARM_NEON) || defined(__ARM_NEON__)) /* NEON */  \
    && !defined(XXH_ENABLE_AUTOVECTORIZE)             /* Define to disable */
        /*
         * UGLY HACK:
         * Prevent autovectorization on Clang ARMv7-a. Exact same problem as
         * the one in XXH3_len_129to240_64b. Speeds up shorter keys > 240b.
         * XXH3_64bits, len == 256, Snapdragon 835:
         *   without hack: 2063.7 MB/s
         *   with hack:    2560.7 MB/s
         */
        __asm__("" : "+r" (result64));
#endif
    }

    return XXH3_avalanche(result64);
}

#define XXH3_INIT_ACC { XXH_PRIME32_3, XXH_PRIME64_1, XXH_PRIME64_2, XXH_PRIME64_3, \
                        XXH_PRIME64_4, XXH_PRIME32_2, XXH_PRIME64_5, XXH_PRIME32_1 }

XXH_FORCE_INLINE XXH64_hash_t
XXH3_hashLong_64b_internal(const void* XXH_RESTRICT input, size_t len,
                           const void* XXH_RESTRICT secret, size_t secretSize,
                           XXH3_f_accumulate_512 f_acc512,
                           XXH3_f_scrambleAcc f_scramble)
{
    XXH_ALIGN(XXH_ACC_ALIGN) xxh_u64 acc[XXH_ACC_NB] = XXH3_INIT_ACC;

    XXH3_hashLong_internal_loop(acc, (const xxh_u8*)input, len, (const xxh_u8*)secret, secretSize, f_acc512, f_scramble);

    /* converge into final hash */
    XXH_STATIC_ASSERT(sizeof(acc) == 64);
    /* do not align on 8, so that the secret is different from the accumulator */
#define XXH_SECRET_MERGEACCS_START 11
    XXH_ASSERT(secretSize >= sizeof(acc) + XXH_SECRET_MERGEACCS_START);
    return XXH3_mergeAccs(acc, (const xxh_u8*)secret + XXH_SECRET_MERGEACCS_START, (xxh_u64)len * XXH_PRIME64_1);
}

/*
 * It's important for performance that XXH3_hashLong is not inlined.
 */
XXH_NO_INLINE XXH64_hash_t
XXH3_hashLong_64b_withSecret(const void* XXH_RESTRICT input, size_t len,
                             XXH64_hash_t seed64, const xxh_u8* XXH_RESTRICT secret, size_t secretLen)
{
    (void)seed64;
    return XXH3_hashLong_64b_internal(input, len, secret, secretLen, XXH3_accumulate_512, XXH3_scrambleAcc);
}

/*
 * It's important for performance that XXH3_hashLong is not inlined.
 * Since the function is not inlined, the compiler may not be able to understand that,
 * in some scenarios, its `secret` argument is actually a compile time constant.
 * This variant enforces that the compiler can detect that,
 * and uses this opportunity to streamline the generated code for better performance.
 */
XXH_NO_INLINE XXH64_hash_t
XXH3_hashLong_64b_default(const void* XXH_RESTRICT input, size_t len,
                          XXH64_hash_t seed64, const xxh_u8* XXH_RESTRICT secret, size_t secretLen)
{
    (void)seed64; (void)secret; (void)secretLen;
    return XXH3_hashLong_64b_internal(input, len, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_accumulate_512, XXH3_scrambleAcc);
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_hashLong_64b_withSeed_internal(const void* input, size_t len,
                                    XXH64_hash_t seed,
                                    XXH3_f_accumulate_512 f_acc512,
                                    XXH3_f_scrambleAcc f_scramble,
                                    XXH3_f_initCustomSecret f_initSec)
{
    if (seed == 0)
        return XXH3_hashLong_64b_internal(input, len,
                                          XXH3_kSecret, sizeof(XXH3_kSecret),
                                          f_acc512, f_scramble);
    {   XXH_ALIGN(XXH_SEC_ALIGN) xxh_u8 secret[XXH_SECRET_DEFAULT_SIZE];
        f_initSec(secret, seed);
        return XXH3_hashLong_64b_internal(input, len, secret, sizeof(secret),
                                          f_acc512, f_scramble);
    }
}

/*
 * It's important for performance that XXH3_hashLong is not inlined.
 */
XXH_NO_INLINE XXH64_hash_t
XXH3_hashLong_64b_withSeed(const void* input, size_t len,
                           XXH64_hash_t seed, const xxh_u8* secret, size_t secretLen)
{
    (void)secret; (void)secretLen;
    return XXH3_hashLong_64b_withSeed_internal(input, len, seed,
                XXH3_accumulate_512, XXH3_scrambleAcc, XXH3_initCustomSecret);
}


typedef XXH64_hash_t (*XXH3_hashLong64_f)(const void* XXH_RESTRICT, size_t,
                                          XXH64_hash_t, const xxh_u8* XXH_RESTRICT, size_t);

XXH_FORCE_INLINE XXH64_hash_t
XXH3_64bits_internal(const void* XXH_RESTRICT input, size_t len,
                     XXH64_hash_t seed64, const void* XXH_RESTRICT secret, size_t secretLen,
                     XXH3_hashLong64_f f_hashLong)
{
    XXH_ASSERT(secretLen >= XXH3_SECRET_SIZE_MIN);
    /*
     * If an action is to be taken if `secretLen` condition is not respected,
     * it should be done here.
     * For now, it's a contract pre-condition.
     * Adding a check and a branch here would cost performance at every hash.
     * Also, note that function signature doesn't offer room to return an error.
     */
    if (len <= 16)
        return XXH3_len_0to16_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, seed64);
    if (len <= 128)
        return XXH3_len_17to128_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    if (len <= XXH3_MIDSIZE_MAX)
        return XXH3_len_129to240_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    return f_hashLong(input, len, seed64, (const xxh_u8*)secret, secretLen);
}


/* ===   Public entry point   === */

XXH_PUBLIC_API XXH64_hash_t XXH3_64bits(const void* input, size_t len)
{
    return XXH3_64bits_internal(input, len, 0, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_hashLong_64b_default);
}

XXH_PUBLIC_API XXH64_hash_t
XXH3_64bits_withSecret(const void* input, size_t len, const void* secret, size_t secretSize)
{
    return XXH3_64bits_internal(input, len, 0, secret, secretSize, XXH3_hashLong_64b_withSecret);
}

XXH_PUBLIC_API XXH64_hash_t
XXH3_64bits_withSeed(const void* input, size_t len, XXH64_hash_t seed)
{
    return XXH3_64bits_internal(input, len, seed, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_hashLong_64b_withSeed);
}

static void* XXH_alignedMalloc(size_t s, size_t align)
{
    XXH_ASSERT(align <= 128 && align >= 8); /* range check */
    XXH_ASSERT((align & (align-1)) == 0);   /* power of 2 */
    XXH_ASSERT(s != 0 && s < (s + align));  /* empty/overflow */
    {   /* Overallocate to make room for manual realignment and an offset byte */
        xxh_u8* base = (xxh_u8*)XXH_malloc(s + align);
        if (base != NULL) {
            /*
             * Get the offset needed to align this pointer.
             *
             * Even if the returned pointer is aligned, there will always be
             * at least one byte to store the offset to the original pointer.
             */
            size_t offset = align - ((size_t)base & (align - 1)); /* base % align */
            /* Add the offset for the now-aligned pointer */
            xxh_u8* ptr = base + offset;

            XXH_ASSERT((size_t)ptr % align == 0);

            /* Store the offset immediately before the returned pointer. */
            ptr[-1] = (xxh_u8)offset;
            return ptr;
        }
        return NULL;
    }
}
/*
 * Frees an aligned pointer allocated by XXH_alignedMalloc(). Don't pass
 * normal malloc'd pointers, XXH_alignedMalloc has a specific data layout.
 */
static void XXH_alignedFree(void* p)
{
    if (p != NULL) {
        xxh_u8* ptr = (xxh_u8*)p;
        /* Get the offset byte we added in XXH_malloc. */
        xxh_u8 offset = ptr[-1];
        /* Free the original malloc'd pointer */
        xxh_u8* base = ptr - offset;
        XXH_free(base);
    }
}
XXH_PUBLIC_API XXH3_state_t* XXH3_createState(void)
{
    XXH3_state_t* const state = (XXH3_state_t*)XXH_alignedMalloc(sizeof(XXH3_state_t), 64);
    if (state==NULL) return NULL;
    XXH3_INITSTATE(state);
    return state;
}

XXH_PUBLIC_API XXH_errorcode XXH3_freeState(XXH3_state_t* statePtr)
{
    XXH_alignedFree(statePtr);
    return XXH_OK;
}

XXH_PUBLIC_API void
XXH3_copyState(XXH3_state_t* dst_state, const XXH3_state_t* src_state)
{
    memcpy(dst_state, src_state, sizeof(*dst_state));
}

static void
XXH3_64bits_reset_internal(XXH3_state_t* statePtr,
                           XXH64_hash_t seed,
                           const void* secret, size_t secretSize)
{
    size_t const initStart = offsetof(XXH3_state_t, bufferedSize);
    size_t const initLength = offsetof(XXH3_state_t, nbStripesPerBlock) - initStart;
    XXH_ASSERT(offsetof(XXH3_state_t, nbStripesPerBlock) > initStart);
    XXH_ASSERT(statePtr != NULL);
    /* set members from bufferedSize to nbStripesPerBlock (excluded) to 0 */
    memset((char*)statePtr + initStart, 0, initLength);
    statePtr->acc[0] = XXH_PRIME32_3;
    statePtr->acc[1] = XXH_PRIME64_1;
    statePtr->acc[2] = XXH_PRIME64_2;
    statePtr->acc[3] = XXH_PRIME64_3;
    statePtr->acc[4] = XXH_PRIME64_4;
    statePtr->acc[5] = XXH_PRIME32_2;
    statePtr->acc[6] = XXH_PRIME64_5;
    statePtr->acc[7] = XXH_PRIME32_1;
    statePtr->seed = seed;
    statePtr->extSecret = (const unsigned char*)secret;
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN);
    statePtr->secretLimit = secretSize - XXH_STRIPE_LEN;
    statePtr->nbStripesPerBlock = statePtr->secretLimit / XXH_SECRET_CONSUME_RATE;
}

XXH_PUBLIC_API XXH_errorcode
XXH3_64bits_reset(XXH3_state_t* statePtr)
{
    if (statePtr == NULL) return XXH_ERROR;
    XXH3_64bits_reset_internal(statePtr, 0, XXH3_kSecret, XXH_SECRET_DEFAULT_SIZE);
    return XXH_OK;
}

XXH_PUBLIC_API XXH_errorcode
XXH3_64bits_reset_withSecret(XXH3_state_t* statePtr, const void* secret, size_t secretSize)
{
    if (statePtr == NULL) return XXH_ERROR;
    XXH3_64bits_reset_internal(statePtr, 0, secret, secretSize);
    if (secret == NULL) return XXH_ERROR;
    if (secretSize < XXH3_SECRET_SIZE_MIN) return XXH_ERROR;
    return XXH_OK;
}

XXH_PUBLIC_API XXH_errorcode
XXH3_64bits_reset_withSeed(XXH3_state_t* statePtr, XXH64_hash_t seed)
{
    if (statePtr == NULL) return XXH_ERROR;
    if (seed==0) return XXH3_64bits_reset(statePtr);
    if (seed != statePtr->seed) XXH3_initCustomSecret(statePtr->customSecret, seed);
    XXH3_64bits_reset_internal(statePtr, seed, NULL, XXH_SECRET_DEFAULT_SIZE);
    return XXH_OK;
}

/* Note : when XXH3_consumeStripes() is invoked,
 * there must be a guarantee that at least one more byte must be consumed from input
 * so that the function can blindly consume all stripes using the "normal" secret segment */
XXH_FORCE_INLINE void
XXH3_consumeStripes(xxh_u64* XXH_RESTRICT acc,
                    size_t* XXH_RESTRICT nbStripesSoFarPtr, size_t nbStripesPerBlock,
                    const xxh_u8* XXH_RESTRICT input, size_t nbStripes,
                    const xxh_u8* XXH_RESTRICT secret, size_t secretLimit,
                    XXH3_f_accumulate_512 f_acc512,
                    XXH3_f_scrambleAcc f_scramble)
{
    XXH_ASSERT(nbStripes <= nbStripesPerBlock);  /* can handle max 1 scramble per invocation */
    XXH_ASSERT(*nbStripesSoFarPtr < nbStripesPerBlock);
    if (nbStripesPerBlock - *nbStripesSoFarPtr <= nbStripes) {
        /* need a scrambling operation */
        size_t const nbStripesToEndofBlock = nbStripesPerBlock - *nbStripesSoFarPtr;
        size_t const nbStripesAfterBlock = nbStripes - nbStripesToEndofBlock;
        XXH3_accumulate(acc, input, secret + nbStripesSoFarPtr[0] * XXH_SECRET_CONSUME_RATE, nbStripesToEndofBlock, f_acc512);
        f_scramble(acc, secret + secretLimit);
        XXH3_accumulate(acc, input + nbStripesToEndofBlock * XXH_STRIPE_LEN, secret, nbStripesAfterBlock, f_acc512);
        *nbStripesSoFarPtr = nbStripesAfterBlock;
    } else {
        XXH3_accumulate(acc, input, secret + nbStripesSoFarPtr[0] * XXH_SECRET_CONSUME_RATE, nbStripes, f_acc512);
        *nbStripesSoFarPtr += nbStripes;
    }
}

/*
 * Both XXH3_64bits_update and XXH3_128bits_update use this routine.
 */
XXH_FORCE_INLINE XXH_errorcode
XXH3_update(XXH3_state_t* state,
            const xxh_u8* input, size_t len,
            XXH3_f_accumulate_512 f_acc512,
            XXH3_f_scrambleAcc f_scramble)
{
    if (input==NULL)
#if defined(XXH_ACCEPT_NULL_INPUT_POINTER) && (XXH_ACCEPT_NULL_INPUT_POINTER>=1)
        return XXH_OK;
#else
        return XXH_ERROR;
#endif

    {   const xxh_u8* const bEnd = input + len;
        const unsigned char* const secret = (state->extSecret == NULL) ? state->customSecret : state->extSecret;

        state->totalLen += len;

        if (state->bufferedSize + len <= XXH3_INTERNALBUFFER_SIZE) {  /* fill in tmp buffer */
            XXH_memcpy(state->buffer + state->bufferedSize, input, len);
            state->bufferedSize += (XXH32_hash_t)len;
            return XXH_OK;
        }
        /* total input is now > XXH3_INTERNALBUFFER_SIZE */

        #define XXH3_INTERNALBUFFER_STRIPES (XXH3_INTERNALBUFFER_SIZE / XXH_STRIPE_LEN)
        XXH_STATIC_ASSERT(XXH3_INTERNALBUFFER_SIZE % XXH_STRIPE_LEN == 0);   /* clean multiple */

        /*
         * Internal buffer is partially filled (always, except at beginning)
         * Complete it, then consume it.
         */
        if (state->bufferedSize) {
            size_t const loadSize = XXH3_INTERNALBUFFER_SIZE - state->bufferedSize;
            XXH_memcpy(state->buffer + state->bufferedSize, input, loadSize);
            input += loadSize;
            XXH3_consumeStripes(state->acc,
                               &state->nbStripesSoFar, state->nbStripesPerBlock,
                                state->buffer, XXH3_INTERNALBUFFER_STRIPES,
                                secret, state->secretLimit,
                                f_acc512, f_scramble);
            state->bufferedSize = 0;
        }
        XXH_ASSERT(input < bEnd);

        /* Consume input by a multiple of internal buffer size */
        if (input+XXH3_INTERNALBUFFER_SIZE < bEnd) {
            const xxh_u8* const limit = bEnd - XXH3_INTERNALBUFFER_SIZE;
            do {
                XXH3_consumeStripes(state->acc,
                                   &state->nbStripesSoFar, state->nbStripesPerBlock,
                                    input, XXH3_INTERNALBUFFER_STRIPES,
                                    secret, state->secretLimit,
                                    f_acc512, f_scramble);
                input += XXH3_INTERNALBUFFER_SIZE;
            } while (input<limit);
            /* for last partial stripe */
            memcpy(state->buffer + sizeof(state->buffer) - XXH_STRIPE_LEN, input - XXH_STRIPE_LEN, XXH_STRIPE_LEN);
        }
        XXH_ASSERT(input < bEnd);

        /* Some remaining input (always) : buffer it */
        XXH_memcpy(state->buffer, input, (size_t)(bEnd-input));
        state->bufferedSize = (XXH32_hash_t)(bEnd-input);
    }

    return XXH_OK;
}

XXH_PUBLIC_API XXH_errorcode
XXH3_64bits_update(XXH3_state_t* state, const void* input, size_t len)
{
    return XXH3_update(state, (const xxh_u8*)input, len,
                       XXH3_accumulate_512, XXH3_scrambleAcc);
}


XXH_FORCE_INLINE void
XXH3_digest_long (XXH64_hash_t* acc,
                  const XXH3_state_t* state,
                  const unsigned char* secret)
{
    /*
     * Digest on a local copy. This way, the state remains unaltered, and it can
     * continue ingesting more input afterwards.
     */
    memcpy(acc, state->acc, sizeof(state->acc));
    if (state->bufferedSize >= XXH_STRIPE_LEN) {
        size_t const nbStripes = (state->bufferedSize - 1) / XXH_STRIPE_LEN;
        size_t nbStripesSoFar = state->nbStripesSoFar;
        XXH3_consumeStripes(acc,
                           &nbStripesSoFar, state->nbStripesPerBlock,
                            state->buffer, nbStripes,
                            secret, state->secretLimit,
                            XXH3_accumulate_512, XXH3_scrambleAcc);
        /* last stripe */
        XXH3_accumulate_512(acc,
                            state->buffer + state->bufferedSize - XXH_STRIPE_LEN,
                            secret + state->secretLimit - XXH_SECRET_LASTACC_START);
    } else {  /* bufferedSize < XXH_STRIPE_LEN */
        xxh_u8 lastStripe[XXH_STRIPE_LEN];
        size_t const catchupSize = XXH_STRIPE_LEN - state->bufferedSize;
        XXH_ASSERT(state->bufferedSize > 0);  /* there is always some input buffered */
        memcpy(lastStripe, state->buffer + sizeof(state->buffer) - catchupSize, catchupSize);
        memcpy(lastStripe + catchupSize, state->buffer, state->bufferedSize);
        XXH3_accumulate_512(acc,
                            lastStripe,
                            secret + state->secretLimit - XXH_SECRET_LASTACC_START);
    }
}

XXH_PUBLIC_API XXH64_hash_t XXH3_64bits_digest (const XXH3_state_t* state)
{
    const unsigned char* const secret = (state->extSecret == NULL) ? state->customSecret : state->extSecret;
    if (state->totalLen > XXH3_MIDSIZE_MAX) {
        XXH_ALIGN(XXH_ACC_ALIGN) XXH64_hash_t acc[XXH_ACC_NB];
        XXH3_digest_long(acc, state, secret);
        return XXH3_mergeAccs(acc,
                              secret + XXH_SECRET_MERGEACCS_START,
                              (xxh_u64)state->totalLen * XXH_PRIME64_1);
    }
    /* totalLen <= XXH3_MIDSIZE_MAX: digesting a short input */
    if (state->seed)
        return XXH3_64bits_withSeed(state->buffer, (size_t)state->totalLen, state->seed);
    return XXH3_64bits_withSecret(state->buffer, (size_t)(state->totalLen),
                                  secret, state->secretLimit + XXH_STRIPE_LEN);
}


#define XXH_MIN(x, y) (((x) > (y)) ? (y) : (x))

XXH_PUBLIC_API void
XXH3_generateSecret(void* secretBuffer, const void* customSeed, size_t customSeedSize)
{
    XXH_ASSERT(secretBuffer != NULL);
    if (customSeedSize == 0) {
        memcpy(secretBuffer, XXH3_kSecret, XXH_SECRET_DEFAULT_SIZE);
        return;
    }
    XXH_ASSERT(customSeed != NULL);

    {   size_t const segmentSize = sizeof(XXH128_hash_t);
        size_t const nbSegments = XXH_SECRET_DEFAULT_SIZE / segmentSize;
        XXH128_canonical_t scrambler;
        XXH64_hash_t seeds[12];
        size_t segnb;
        XXH_ASSERT(nbSegments == 12);
        XXH_ASSERT(segmentSize * nbSegments == XXH_SECRET_DEFAULT_SIZE); /* exact multiple */
        XXH128_canonicalFromHash(&scrambler, XXH128(customSeed, customSeedSize, 0));

        /*
        * Copy customSeed to seeds[], truncating or repeating as necessary.
        */
        {   size_t toFill = XXH_MIN(customSeedSize, sizeof(seeds));
            size_t filled = toFill;
            memcpy(seeds, customSeed, toFill);
            while (filled < sizeof(seeds)) {
                toFill = XXH_MIN(filled, sizeof(seeds) - filled);
                memcpy((char*)seeds + filled, seeds, toFill);
                filled += toFill;
        }   }

        /* generate secret */
        memcpy(secretBuffer, &scrambler, sizeof(scrambler));
        for (segnb=1; segnb < nbSegments; segnb++) {
            size_t const segmentStart = segnb * segmentSize;
            XXH128_canonical_t segment;
            XXH128_canonicalFromHash(&segment,
                XXH128(&scrambler, sizeof(scrambler), XXH_readLE64(seeds + segnb) + segnb) );
            memcpy((char*)secretBuffer + segmentStart, &segment, sizeof(segment));
    }   }
}



XXH_FORCE_INLINE XXH128_hash_t
XXH3_len_1to3_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    /* A doubled version of 1to3_64b with different constants. */
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(1 <= len && len <= 3);
    XXH_ASSERT(secret != NULL);
    /*
     * len = 1: combinedl = { input[0], 0x01, input[0], input[0] }
     * len = 2: combinedl = { input[1], 0x02, input[0], input[1] }
     * len = 3: combinedl = { input[2], 0x03, input[0], input[1] }
     */
    {   xxh_u8 const c1 = input[0];
        xxh_u8 const c2 = input[len >> 1];
        xxh_u8 const c3 = input[len - 1];
        xxh_u32 const combinedl = ((xxh_u32)c1 <<16) | ((xxh_u32)c2 << 24)
                                | ((xxh_u32)c3 << 0) | ((xxh_u32)len << 8);
        xxh_u32 const combinedh = XXH_rotl32(XXH_swap32(combinedl), 13);
        xxh_u64 const bitflipl = (XXH_readLE32(secret) ^ XXH_readLE32(secret+4)) + seed;
        xxh_u64 const bitfliph = (XXH_readLE32(secret+8) ^ XXH_readLE32(secret+12)) - seed;
        xxh_u64 const keyed_lo = (xxh_u64)combinedl ^ bitflipl;
        xxh_u64 const keyed_hi = (xxh_u64)combinedh ^ bitfliph;
        XXH128_hash_t h128;
        h128.low64  = XXH64_avalanche(keyed_lo);
        h128.high64 = XXH64_avalanche(keyed_hi);
        return h128;
    }
}

XXH_FORCE_INLINE XXH128_hash_t
XXH3_len_4to8_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(secret != NULL);
    XXH_ASSERT(4 <= len && len <= 8);
    seed ^= (xxh_u64)XXH_swap32((xxh_u32)seed) << 32;
    {   xxh_u32 const input_lo = XXH_readLE32(input);
        xxh_u32 const input_hi = XXH_readLE32(input + len - 4);
        xxh_u64 const input_64 = input_lo + ((xxh_u64)input_hi << 32);
        xxh_u64 const bitflip = (XXH_readLE64(secret+16) ^ XXH_readLE64(secret+24)) + seed;
        xxh_u64 const keyed = input_64 ^ bitflip;

        /* Shift len to the left to ensure it is even, this avoids even multiplies. */
        XXH128_hash_t m128 = XXH_mult64to128(keyed, XXH_PRIME64_1 + (len << 2));

        m128.high64 += (m128.low64 << 1);
        m128.low64  ^= (m128.high64 >> 3);

        m128.low64   = XXH_xorshift64(m128.low64, 35);
        m128.low64  *= 0x9FB21C651E98DF25ULL;
        m128.low64   = XXH_xorshift64(m128.low64, 28);
        m128.high64  = XXH3_avalanche(m128.high64);
        return m128;
    }
}

XXH_FORCE_INLINE XXH128_hash_t
XXH3_len_9to16_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(secret != NULL);
    XXH_ASSERT(9 <= len && len <= 16);
    {   xxh_u64 const bitflipl = (XXH_readLE64(secret+32) ^ XXH_readLE64(secret+40)) - seed;
        xxh_u64 const bitfliph = (XXH_readLE64(secret+48) ^ XXH_readLE64(secret+56)) + seed;
        xxh_u64 const input_lo = XXH_readLE64(input);
        xxh_u64       input_hi = XXH_readLE64(input + len - 8);
        XXH128_hash_t m128 = XXH_mult64to128(input_lo ^ input_hi ^ bitflipl, XXH_PRIME64_1);
        /*
         * Put len in the middle of m128 to ensure that the length gets mixed to
         * both the low and high bits in the 128x64 multiply below.
         */
        m128.low64 += (xxh_u64)(len - 1) << 54;
        input_hi   ^= bitfliph;
        /*
         * Add the high 32 bits of input_hi to the high 32 bits of m128, then
         * add the long product of the low 32 bits of input_hi and XXH_PRIME32_2 to
         * the high 64 bits of m128.
         *
         * The best approach to this operation is different on 32-bit and 64-bit.
         */
        if (sizeof(void *) < sizeof(xxh_u64)) { /* 32-bit */
            /*
             * 32-bit optimized version, which is more readable.
             *
             * On 32-bit, it removes an ADC and delays a dependency between the two
             * halves of m128.high64, but it generates an extra mask on 64-bit.
             */
            m128.high64 += (input_hi & 0xFFFFFFFF00000000ULL) + XXH_mult32to64((xxh_u32)input_hi, XXH_PRIME32_2);
        } else {
            m128.high64 += input_hi + XXH_mult32to64((xxh_u32)input_hi, XXH_PRIME32_2 - 1);
        }
        /* m128 ^= XXH_swap64(m128 >> 64); */
        m128.low64  ^= XXH_swap64(m128.high64);

        {   /* 128x64 multiply: h128 = m128 * XXH_PRIME64_2; */
            XXH128_hash_t h128 = XXH_mult64to128(m128.low64, XXH_PRIME64_2);
            h128.high64 += m128.high64 * XXH_PRIME64_2;

            h128.low64   = XXH3_avalanche(h128.low64);
            h128.high64  = XXH3_avalanche(h128.high64);
            return h128;
    }   }
}

/*
 * Assumption: `secret` size is >= XXH3_SECRET_SIZE_MIN
 */
XXH_FORCE_INLINE XXH128_hash_t
XXH3_len_0to16_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(len <= 16);
    {   if (len > 8) return XXH3_len_9to16_128b(input, len, secret, seed);
        if (len >= 4) return XXH3_len_4to8_128b(input, len, secret, seed);
        if (len) return XXH3_len_1to3_128b(input, len, secret, seed);
        {   XXH128_hash_t h128;
            xxh_u64 const bitflipl = XXH_readLE64(secret+64) ^ XXH_readLE64(secret+72);
            xxh_u64 const bitfliph = XXH_readLE64(secret+80) ^ XXH_readLE64(secret+88);
            h128.low64 = XXH64_avalanche(seed ^ bitflipl);
            h128.high64 = XXH64_avalanche( seed ^ bitfliph);
            return h128;
    }   }
}

/*
 * A bit slower than XXH3_mix16B, but handles multiply by zero better.
 */
XXH_FORCE_INLINE XXH128_hash_t
XXH128_mix32B(XXH128_hash_t acc, const xxh_u8* input_1, const xxh_u8* input_2,
              const xxh_u8* secret, XXH64_hash_t seed)
{
    acc.low64  += XXH3_mix16B (input_1, secret+0, seed);
    acc.low64  ^= XXH_readLE64(input_2) + XXH_readLE64(input_2 + 8);
    acc.high64 += XXH3_mix16B (input_2, secret+16, seed);
    acc.high64 ^= XXH_readLE64(input_1) + XXH_readLE64(input_1 + 8);
    return acc;
}


XXH_FORCE_INLINE XXH128_hash_t
XXH3_len_17to128_128b(const xxh_u8* XXH_RESTRICT input, size_t len,
                      const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                      XXH64_hash_t seed)
{
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN); (void)secretSize;
    XXH_ASSERT(16 < len && len <= 128);

    {   XXH128_hash_t acc;
        acc.low64 = len * XXH_PRIME64_1;
        acc.high64 = 0;
        if (len > 32) {
            if (len > 64) {
                if (len > 96) {
                    acc = XXH128_mix32B(acc, input+48, input+len-64, secret+96, seed);
                }
                acc = XXH128_mix32B(acc, input+32, input+len-48, secret+64, seed);
            }
            acc = XXH128_mix32B(acc, input+16, input+len-32, secret+32, seed);
        }
        acc = XXH128_mix32B(acc, input, input+len-16, secret, seed);
        {   XXH128_hash_t h128;
            h128.low64  = acc.low64 + acc.high64;
            h128.high64 = (acc.low64    * XXH_PRIME64_1)
                        + (acc.high64   * XXH_PRIME64_4)
                        + ((len - seed) * XXH_PRIME64_2);
            h128.low64  = XXH3_avalanche(h128.low64);
            h128.high64 = (XXH64_hash_t)0 - XXH3_avalanche(h128.high64);
            return h128;
        }
    }
}

XXH_NO_INLINE XXH128_hash_t
XXH3_len_129to240_128b(const xxh_u8* XXH_RESTRICT input, size_t len,
                       const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                       XXH64_hash_t seed)
{
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN); (void)secretSize;
    XXH_ASSERT(128 < len && len <= XXH3_MIDSIZE_MAX);

    {   XXH128_hash_t acc;
        int const nbRounds = (int)len / 32;
        int i;
        acc.low64 = len * XXH_PRIME64_1;
        acc.high64 = 0;
        for (i=0; i<4; i++) {
            acc = XXH128_mix32B(acc,
                                input  + (32 * i),
                                input  + (32 * i) + 16,
                                secret + (32 * i),
                                seed);
        }
        acc.low64 = XXH3_avalanche(acc.low64);
        acc.high64 = XXH3_avalanche(acc.high64);
        XXH_ASSERT(nbRounds >= 4);
        for (i=4 ; i < nbRounds; i++) {
            acc = XXH128_mix32B(acc,
                                input + (32 * i),
                                input + (32 * i) + 16,
                                secret + XXH3_MIDSIZE_STARTOFFSET + (32 * (i - 4)),
                                seed);
        }
        /* last bytes */
        acc = XXH128_mix32B(acc,
                            input + len - 16,
                            input + len - 32,
                            secret + XXH3_SECRET_SIZE_MIN - XXH3_MIDSIZE_LASTOFFSET - 16,
                            0ULL - seed);

        {   XXH128_hash_t h128;
            h128.low64  = acc.low64 + acc.high64;
            h128.high64 = (acc.low64    * XXH_PRIME64_1)
                        + (acc.high64   * XXH_PRIME64_4)
                        + ((len - seed) * XXH_PRIME64_2);
            h128.low64  = XXH3_avalanche(h128.low64);
            h128.high64 = (XXH64_hash_t)0 - XXH3_avalanche(h128.high64);
            return h128;
        }
    }
}

XXH_FORCE_INLINE XXH128_hash_t
XXH3_hashLong_128b_internal(const void* XXH_RESTRICT input, size_t len,
                            const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                            XXH3_f_accumulate_512 f_acc512,
                            XXH3_f_scrambleAcc f_scramble)
{
    XXH_ALIGN(XXH_ACC_ALIGN) xxh_u64 acc[XXH_ACC_NB] = XXH3_INIT_ACC;

    XXH3_hashLong_internal_loop(acc, (const xxh_u8*)input, len, secret, secretSize, f_acc512, f_scramble);

    /* converge into final hash */
    XXH_STATIC_ASSERT(sizeof(acc) == 64);
    XXH_ASSERT(secretSize >= sizeof(acc) + XXH_SECRET_MERGEACCS_START);
    {   XXH128_hash_t h128;
        h128.low64  = XXH3_mergeAccs(acc,
                                     secret + XXH_SECRET_MERGEACCS_START,
                                     (xxh_u64)len * XXH_PRIME64_1);
        h128.high64 = XXH3_mergeAccs(acc,
                                     secret + secretSize
                                            - sizeof(acc) - XXH_SECRET_MERGEACCS_START,
                                     ~((xxh_u64)len * XXH_PRIME64_2));
        return h128;
    }
}

/*
 * It's important for performance that XXH3_hashLong is not inlined.
 */
XXH_NO_INLINE XXH128_hash_t
XXH3_hashLong_128b_default(const void* XXH_RESTRICT input, size_t len,
                           XXH64_hash_t seed64,
                           const void* XXH_RESTRICT secret, size_t secretLen)
{
    (void)seed64; (void)secret; (void)secretLen;
    return XXH3_hashLong_128b_internal(input, len, XXH3_kSecret, sizeof(XXH3_kSecret),
                                       XXH3_accumulate_512, XXH3_scrambleAcc);
}

/*
 * It's important for performance that XXH3_hashLong is not inlined.
 */
XXH_NO_INLINE XXH128_hash_t
XXH3_hashLong_128b_withSecret(const void* XXH_RESTRICT input, size_t len,
                              XXH64_hash_t seed64,
                              const void* XXH_RESTRICT secret, size_t secretLen)
{
    (void)seed64;
    return XXH3_hashLong_128b_internal(input, len, (const xxh_u8*)secret, secretLen,
                                       XXH3_accumulate_512, XXH3_scrambleAcc);
}

XXH_FORCE_INLINE XXH128_hash_t
XXH3_hashLong_128b_withSeed_internal(const void* XXH_RESTRICT input, size_t len,
                                XXH64_hash_t seed64,
                                XXH3_f_accumulate_512 f_acc512,
                                XXH3_f_scrambleAcc f_scramble,
                                XXH3_f_initCustomSecret f_initSec)
{
    if (seed64 == 0)
        return XXH3_hashLong_128b_internal(input, len,
                                           XXH3_kSecret, sizeof(XXH3_kSecret),
                                           f_acc512, f_scramble);
    {   XXH_ALIGN(XXH_SEC_ALIGN) xxh_u8 secret[XXH_SECRET_DEFAULT_SIZE];
        f_initSec(secret, seed64);
        return XXH3_hashLong_128b_internal(input, len, (const xxh_u8*)secret, sizeof(secret),
                                           f_acc512, f_scramble);
    }
}

/*
 * It's important for performance that XXH3_hashLong is not inlined.
 */
XXH_NO_INLINE XXH128_hash_t
XXH3_hashLong_128b_withSeed(const void* input, size_t len,
                            XXH64_hash_t seed64, const void* XXH_RESTRICT secret, size_t secretLen)
{
    (void)secret; (void)secretLen;
    return XXH3_hashLong_128b_withSeed_internal(input, len, seed64,
                XXH3_accumulate_512, XXH3_scrambleAcc, XXH3_initCustomSecret);
}

typedef XXH128_hash_t (*XXH3_hashLong128_f)(const void* XXH_RESTRICT, size_t,
                                            XXH64_hash_t, const void* XXH_RESTRICT, size_t);

XXH_FORCE_INLINE XXH128_hash_t
XXH3_128bits_internal(const void* input, size_t len,
                      XXH64_hash_t seed64, const void* XXH_RESTRICT secret, size_t secretLen,
                      XXH3_hashLong128_f f_hl128)
{
    XXH_ASSERT(secretLen >= XXH3_SECRET_SIZE_MIN);
    /*
     * If an action is to be taken if `secret` conditions are not respected,
     * it should be done here.
     * For now, it's a contract pre-condition.
     * Adding a check and a branch here would cost performance at every hash.
     */
    if (len <= 16)
        return XXH3_len_0to16_128b((const xxh_u8*)input, len, (const xxh_u8*)secret, seed64);
    if (len <= 128)
        return XXH3_len_17to128_128b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    if (len <= XXH3_MIDSIZE_MAX)
        return XXH3_len_129to240_128b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    return f_hl128(input, len, seed64, secret, secretLen);
}


/* ===   Public XXH128 API   === */

XXH_PUBLIC_API XXH128_hash_t XXH3_128bits(const void* input, size_t len)
{
    return XXH3_128bits_internal(input, len, 0,
                                 XXH3_kSecret, sizeof(XXH3_kSecret),
                                 XXH3_hashLong_128b_default);
}

XXH_PUBLIC_API XXH128_hash_t
XXH3_128bits_withSecret(const void* input, size_t len, const void* secret, size_t secretSize)
{
    return XXH3_128bits_internal(input, len, 0,
                                 (const xxh_u8*)secret, secretSize,
                                 XXH3_hashLong_128b_withSecret);
}

XXH_PUBLIC_API XXH128_hash_t
XXH3_128bits_withSeed(const void* input, size_t len, XXH64_hash_t seed)
{
    return XXH3_128bits_internal(input, len, seed,
                                 XXH3_kSecret, sizeof(XXH3_kSecret),
                                 XXH3_hashLong_128b_withSeed);
}

XXH_PUBLIC_API XXH128_hash_t
XXH128(const void* input, size_t len, XXH64_hash_t seed)
{
    return XXH3_128bits_withSeed(input, len, seed);
}


/* ===   XXH3 128-bit streaming   === */

/*
 * All the functions are actually the same as for 64-bit streaming variant.
 * The only difference is the finalizatiom routine.
 */

static void
XXH3_128bits_reset_internal(XXH3_state_t* statePtr,
                            XXH64_hash_t seed,
                            const void* secret, size_t secretSize)
{
    XXH3_64bits_reset_internal(statePtr, seed, secret, secretSize);
}

XXH_PUBLIC_API XXH_errorcode
XXH3_128bits_reset(XXH3_state_t* statePtr)
{
    if (statePtr == NULL) return XXH_ERROR;
    XXH3_128bits_reset_internal(statePtr, 0, XXH3_kSecret, XXH_SECRET_DEFAULT_SIZE);
    return XXH_OK;
}

XXH_PUBLIC_API XXH_errorcode
XXH3_128bits_reset_withSecret(XXH3_state_t* statePtr, const void* secret, size_t secretSize)
{
    if (statePtr == NULL) return XXH_ERROR;
    XXH3_128bits_reset_internal(statePtr, 0, secret, secretSize);
    if (secret == NULL) return XXH_ERROR;
    if (secretSize < XXH3_SECRET_SIZE_MIN) return XXH_ERROR;
    return XXH_OK;
}

XXH_PUBLIC_API XXH_errorcode
XXH3_128bits_reset_withSeed(XXH3_state_t* statePtr, XXH64_hash_t seed)
{
    if (statePtr == NULL) return XXH_ERROR;
    if (seed==0) return XXH3_128bits_reset(statePtr);
    if (seed != statePtr->seed) XXH3_initCustomSecret(statePtr->customSecret, seed);
    XXH3_128bits_reset_internal(statePtr, seed, NULL, XXH_SECRET_DEFAULT_SIZE);
    return XXH_OK;
}

XXH_PUBLIC_API XXH_errorcode
XXH3_128bits_update(XXH3_state_t* state, const void* input, size_t len)
{
    return XXH3_update(state, (const xxh_u8*)input, len,
                       XXH3_accumulate_512, XXH3_scrambleAcc);
}

XXH_PUBLIC_API XXH128_hash_t XXH3_128bits_digest (const XXH3_state_t* state)
{
    const unsigned char* const secret = (state->extSecret == NULL) ? state->customSecret : state->extSecret;
    if (state->totalLen > XXH3_MIDSIZE_MAX) {
        XXH_ALIGN(XXH_ACC_ALIGN) XXH64_hash_t acc[XXH_ACC_NB];
        XXH3_digest_long(acc, state, secret);
        XXH_ASSERT(state->secretLimit + XXH_STRIPE_LEN >= sizeof(acc) + XXH_SECRET_MERGEACCS_START);
        {   XXH128_hash_t h128;
            h128.low64  = XXH3_mergeAccs(acc,
                                         secret + XXH_SECRET_MERGEACCS_START,
                                         (xxh_u64)state->totalLen * XXH_PRIME64_1);
            h128.high64 = XXH3_mergeAccs(acc,
                                         secret + state->secretLimit + XXH_STRIPE_LEN
                                                - sizeof(acc) - XXH_SECRET_MERGEACCS_START,
                                         ~((xxh_u64)state->totalLen * XXH_PRIME64_2));
            return h128;
        }
    }
    /* len <= XXH3_MIDSIZE_MAX : short code */
    if (state->seed)
        return XXH3_128bits_withSeed(state->buffer, (size_t)state->totalLen, state->seed);
    return XXH3_128bits_withSecret(state->buffer, (size_t)(state->totalLen),
                                   secret, state->secretLimit + XXH_STRIPE_LEN);
}

/* 128-bit utility functions */


/* return : 1 is equal, 0 if different */
XXH_PUBLIC_API int XXH128_isEqual(XXH128_hash_t h1, XXH128_hash_t h2)
{
    /* note : XXH128_hash_t is compact, it has no padding byte */
    return !(memcmp(&h1, &h2, sizeof(h1)));
}

/* This prototype is compatible with stdlib's qsort().
 * return : >0 if *h128_1  > *h128_2
 *          <0 if *h128_1  < *h128_2
 *          =0 if *h128_1 == *h128_2  */
XXH_PUBLIC_API int XXH128_cmp(const void* h128_1, const void* h128_2)
{
    XXH128_hash_t const h1 = *(const XXH128_hash_t*)h128_1;
    XXH128_hash_t const h2 = *(const XXH128_hash_t*)h128_2;
    int const hcmp = (h1.high64 > h2.high64) - (h2.high64 > h1.high64);
    /* note : bets that, in most cases, hash values are different */
    if (hcmp) return hcmp;
    return (h1.low64 > h2.low64) - (h2.low64 > h1.low64);
}


/*======   Canonical representation   ======*/
XXH_PUBLIC_API void
XXH128_canonicalFromHash(XXH128_canonical_t* dst, XXH128_hash_t hash)
{
    XXH_STATIC_ASSERT(sizeof(XXH128_canonical_t) == sizeof(XXH128_hash_t));
    if (XXH_CPU_LITTLE_ENDIAN) {
        hash.high64 = XXH_swap64(hash.high64);
        hash.low64  = XXH_swap64(hash.low64);
    }
    memcpy(dst, &hash.high64, sizeof(hash.high64));
    memcpy((char*)dst + sizeof(hash.high64), &hash.low64, sizeof(hash.low64));
}

XXH_PUBLIC_API XXH128_hash_t
XXH128_hashFromCanonical(const XXH128_canonical_t* src)
{
    XXH128_hash_t h;
    h.high64 = XXH_readBE64(src);
    h.low64  = XXH_readBE64(src->digest + 8);
    return h;
}

/* Pop our optimization override from above */
#if XXH_VECTOR == XXH_AVX2 /* AVX2 */ \
  && defined(__GNUC__) && !defined(__clang__) /* GCC, not Clang */ \
  && defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__) /* respect -O0 and -Os */
#  pragma GCC pop_options
#endif

#endif  /* XXH_NO_LONG_LONG */


#endif  /* XXH_IMPLEMENTATION */


#if defined (__cplusplus)
}
#endif


/////////// other functions


/// take sha256
string sha256_calc_file(const char * i_filename,const int64_t i_inizio,const int64_t i_totali,int64_t& io_lavorati)
{
	string risultato="ERROR";
	FILE* myfile = freadopen(i_filename);
	if(myfile==NULL )
 		return risultato;
	
	const int BUFSIZE	=65536*8;
	char 				buf[BUFSIZE];
	int 				n=BUFSIZE;
				
	libzpaq::SHA256 sha256;
	while (1)
	{
		int r=fread(buf, 1, n, myfile);
		
		for (int i=0;i<r;i++)
			sha256.put(*(buf+i));
 		if (r!=n) 
			break;
		io_lavorati+=n;
		if (flagnoeta==false)
			avanzamento(io_lavorati,i_totali,i_inizio);
	}
	fclose(myfile);
	
	char sha256result[32];
	memcpy(sha256result, sha256.result(), 32);
	char myhex[4];
	risultato="";
	for (int j=0; j <= 31; j++)
	{
		sprintf(myhex,"%02X", (unsigned char)sha256result[j]);
		risultato.push_back(myhex[0]);
		risultato.push_back(myhex[1]);
	}
	return risultato;
}	
	


//// calc xxh3 (maybe)
string xxhash_calc_file(const char * i_filename,const int64_t i_inizio,const int64_t i_totali,int64_t& io_lavorati,
uint64_t& o_high64,uint64_t& o_low64)
{
	o_high64=0;
	o_low64=0;
		
	size_t const blockSize = 65536;
	unsigned char buffer[blockSize];
    FILE* inFile = freadopen(i_filename);
	if (inFile==NULL) 
		return "ERROR";
	
	XXH3_state_t state128;
    (void)XXH3_128bits_reset(&state128);
	
	size_t readSize;
	while ((readSize = fread(buffer, 1, blockSize, inFile)) > 0) 
	{
		(void)XXH3_128bits_update(&state128, buffer, readSize);
		io_lavorati+=readSize;
		///printf("IIIIIIIIIIIIIIIIIIIIIIIIIII OOOOO  %lld\n",io_lavorati);
		if (flagnoeta==false)
				avanzamento(io_lavorati,i_totali,i_inizio);
	}
	fclose(inFile);
	XXH128_hash_t myhash=XXH3_128bits_digest(&state128);
	o_high64=myhash.high64;
	o_low64=myhash.low64;
	
	char risultato[33];
	sprintf(risultato,"%016llX%016llX",(unsigned long long)o_high64,(unsigned long long)o_low64);
	return risultato;
}




/// finally get crc32-c 
#define SIZE (262144*3)
#define CHUNK SIZE
unsigned int crc32c_calc_file(const char * i_filename,const int64_t i_inizio,const int64_t i_totali,int64_t& io_lavorati)
{
	FILE* myfile = freadopen(i_filename);
	
	if( myfile==NULL )
    	return 0;
    
	char buf[SIZE];
    ssize_t got;
    size_t off, n;
    uint32_t crc=0;
	
    while ((got = fread(buf, sizeof(char),SIZE,myfile)) > 0) 
	{
		off = 0;
        do 
		{
            n = (size_t)got - off;
            if (n > CHUNK)
                n = CHUNK;
            crc = crc32c(crc, (const unsigned char*)buf + off, n);
            off += n;
        } while (off < (size_t)got);
				
		io_lavorati+=got;	
		if (flagnoeta==false)
			if (i_totali)
			avanzamento(io_lavorati,i_totali,i_inizio);

    }
    fclose(myfile);
	return crc;
}


string hash_calc_file(int i_algo,const char * i_filename,const int64_t i_inizio,const int64_t i_totali,int64_t& io_lavorati)
{
	char risultato[33];
	
	switch (i_algo) 
	{
		/*
		case ALGO_WYHASH:
			sprintf(risultato,"%016llX",wyhash_calc_file(i_filename,i_inizio,i_totali,io_lavorati));
			return risultato;		
		break;
		*/
		case ALGO_SHA1:
			return  sha1_calc_file(i_filename,i_inizio,i_totali,io_lavorati);
		break;
		
		case ALGO_CRC32C:
			sprintf(risultato,"%08X",crc32c_calc_file(i_filename,i_inizio,i_totali,io_lavorati));
			return risultato;		
		break;
		
		case ALGO_CRC32:
			sprintf(risultato,"%08X",crc32_calc_file(i_filename,i_inizio,i_totali,io_lavorati));
			return risultato;		
		break;
		
		case ALGO_XXHASH:
			uint64_t dummybasso64;
			uint64_t dummyalto64;
			return xxhash_calc_file(i_filename,i_inizio,i_totali,io_lavorati,dummybasso64,dummyalto64);
			
		break;
		
		case ALGO_SHA256: 
		return  sha256_calc_file(i_filename,i_inizio,i_totali,io_lavorati);
		break;
		
		default: 
			error("GURU 13480 algo ??");
	}
	

	return "ERROR";
}


///////////////////////////////////////////////
//// support functions



string mygetalgo()
{
	if (flagwyhash)
		return "WYHASH";
	else
	if (flagcrc32)
		return "CRC32";
	else
	if (flagcrc32c)
		return "CRC-32C";
	else
	if (flagxxhash)
		return "XXH3";
	else
	if (flagsha256)
		return "SHA256";
	else
		return "SHA1";
}
int flag2algo()
{
	if (flagwyhash)
		return ALGO_WYHASH;
	else
	if (flagcrc32)
		return ALGO_CRC32;
	else
	if (flagcrc32c)
		return ALGO_CRC32C;
	else
	if (flagxxhash)
		return ALGO_XXHASH;
	else
	if (flagsha256)
		return ALGO_SHA256;
	else
		return ALGO_SHA1;
}


struct tparametrihash
{
	vector<string> filestobehashed;
	vector<string> hashcalculated;
	uint64_t	timestart;
	uint64_t	timeend;
	int64_t	inizio;
	int64_t dimensione;
	int	tnumber;
};

void * scansionahash(void *t) 
{
	assert(t);
	tparametrihash* par= ((struct tparametrihash*)(t));
	vector<string>& tmpfilestobehashed= par->filestobehashed;
	vector<string>& tmphashcalculated= par->hashcalculated;
	
	for (unsigned int i=0;i<tmpfilestobehashed.size();i++)
	{
		tmphashcalculated.push_back(hash_calc_file(flag2algo(),tmpfilestobehashed[i].c_str(),par->inizio,par->dimensione,g_dimensione));
		///printf("INIZIO %lld dimensione %lld lavora %lld\n",par->inizio,par->dimensione,g_dimensione);
	}
	pthread_exit(NULL);
	return 0;
}



bool sortbyval(const std::pair<string, string> &a, 
               const std::pair<string, string> &b) 
{ 
    return (a.second < b.second); 
} 

bool sortbysize(const std::pair<uint64_t, string> &a, 
               const std::pair<uint64_t, string> &b) 
{ 
    return (a.first < b.first); 
} 

int Jidac::deduplicate() 
{
	printf("*** DIRECTLY DELETE DUPLICATED FILES ***\n");
	if (files.size()>1)
	{
		printf("Sorry, works only on exactly 1 dir\n");
		return 1;
	}
	if (!isdirectory(files[0]))
	{
		printf("Sorry, you must enter a directory\n");
		return 1;
	}
	if (flagverbose)
	summary=-1;
	else
	summary=1;
	flagchecksum=false;
	flagverify=true;
	
	flagcrc32c=false;
	flagcrc32=false;
	if (flagsha256)
		flagxxhash=false;
	else
		flagxxhash=true;
	flagkill=true;
	
	if (!flagforce)
		printf("-force not present: dry run\n");
	return summa();
}

int Jidac::summa() 
{
	if (!flagpakka)
	{
		printf("Getting %s",mygetalgo().c_str());
		if (!flagforcezfs)
			printf(" ignoring .zfs and :$DATA\n");
	}

	if (flagkill)
		if (flagforce)
			if (!getcaptcha("iamsure","Delete files without confirmation"))
				return 1;

	int quantifiles			=0;
	int64_t total_size		=0;  
	unsigned int duplicated_files=0;
	uint64_t duplicated_size=0;
	uint64_t scannedsize=0;
	vector<string> myfiles;
	
	flagskipzfs					=true;  // strip down zfs
	int64_t startscan=mtime();
	
	g_bytescanned=0;
	g_filescanned=0;
	g_dimensione=0;
	g_worked=0;


	vector<string> mydirs;
	
	if (flagchecksum)
	{
		printf("Getting cumulative 1-level SHA256\n");
		if (files.size()!=1)
		{
			printf("Error: with -checksum exactly one directory can be searched\n");
			return 1;
		}
		uint64_t startscan=0;			
///		if (!isdirectory(files[0]))
	///		files[0]+="/";
		scandir(false,edt,files[0].c_str(),false);
		
		for (DTMap::iterator p=edt.begin(); p!=edt.end(); ++p) 
			if (p->first != files[0])
			{
			if (isdirectory(p->first))
				mydirs.push_back(p->first);
			else
				myfiles.push_back(p->first);
			}
		uint64_t scantime=mtime()-startscan;
		
		if (myfiles.size()+mydirs.size()==0)
		{
			printf("Nothing to do\n");
			return 0;
		}
		libzpaq::SHA256 sha256;
		libzpaq::SHA256 globalesha256;
	
		vector<std::pair<string, string>> vec;
		
		uint64_t hashtime=0;
		uint64_t printtime=0;
		uint64_t total_size=0;
		
		for (unsigned int i=0;i<mydirs.size();i++)
		{
			if (!flagnoeta)
				printf("Scanning %s\r",mydirs[i].c_str());
			edt.clear();
			startscan=mtime();
			scandir(true,edt,mydirs[i].c_str(),true);
			scantime+=mtime()-startscan;
			
			vec.clear();
			for (DTMap::iterator p=edt.begin(); p!=edt.end(); ++p) 
				if (!isdirectory(p->first))
				{
					startscan=mtime();
					vec.push_back(make_pair("dummy",hash_calc_file(flag2algo(),p->first.c_str(),-1,-1,g_dimensione)));
					total_size+=p->second.size;
					hashtime+=mtime()-startscan;
				}
				
			std::sort(vec.begin(), vec.end(),sortbyval);

			for (unsigned int j=0;j<vec.size();j++)
				for (const char* p=vec[j].second.c_str(); *p; ++p) 
				{
					sha256.put(*p);
					globalesha256.put(*p);
				}

			scantime=mtime();
			char sha256result[32];
			memcpy(sha256result, sha256.result(), 32);
			printf("MAGIC_%s: ",mygetalgo().c_str());
			for (int j=0; j <= 31; j++)
				printf("%02X", (unsigned char)sha256result[j]);

			printf(" ");
			string directorypurgata=mydirs[i];
			myreplace(directorypurgata,files[0],"");
			printUTF8(directorypurgata.c_str());
			printf("\n");
			printtime+=mtime()-scantime;
			
		}
		
		vec.clear();
		
		startscan=mtime();
		for (unsigned int i=0;i<myfiles.size();i++)
			vec.push_back(make_pair("dummy",hash_calc_file(flag2algo(),myfiles[i].c_str(),-1,-1,g_dimensione)));
		hashtime+=mtime()-startscan;
		
		std::sort(vec.begin(), vec.end(),sortbyval);

		for (unsigned int j=0;j<vec.size();j++)
			for (const char* p=vec[j].second.c_str(); *p; ++p) 
					globalesha256.put(*p);
		
		scantime=mtime();
		char sha256result[32];
		memcpy(sha256result, globalesha256.result(), 32);
		printf("\nGLOBAL_MAGIC_%s: ",mygetalgo().c_str());
		for (int j=0; j <= 31; j++)
			printf("%02X:", (unsigned char)sha256result[j]);
		printf("\n");
		printtime+=mtime()-scantime;
		
		if (!flagnoeta)
			{
				printf("Algo %s\n",mygetalgo().c_str());
				printf("Scanning filesystem time  %f s\n",scantime/1000.0);
				printf("Data transfer+CPU   time  %f s\n",hashtime/1000.0);
				printf("Data output         time  %f s\n",printtime/1000.0);
				printf("Total size  %19s (%10s)\n",migliaia(total_size),tohuman(total_size));
				int64_t myspeed=(int64_t)(total_size*1000.0/(hashtime));
				printf("Worked on %s bytes avg speed (hashtime) %s B/s\n",migliaia(total_size),migliaia2(myspeed));
			}
		return 0;
	}
	
	for (unsigned i=0; i<files.size(); ++i)
		scandir(true,edt,files[i].c_str());


	if (flagverify)
	{
//	deduplication: I am very lazy, so do a lot of copy
		printf("\n*** Searching for duplicates with %s minsize %s ***\n",mygetalgo().c_str(),migliaia(minsize));
// get size and name
		std::vector<std::pair<uint64_t,string>> filestobepurged;
		for (DTMap::iterator p=edt.begin(); p!=edt.end(); ++p) 
			if (p->second.date && p->first!="" && (!isdirectory(p->first)) && (!isads(p->first)) ) 
			{
				if (minsize>0)
				{
					if ((uint64_t)p->second.size>minsize)
					{
						filestobepurged.push_back(make_pair(p->second.size,p->first));
						scannedsize+=p->second.size;
					}
				}
				else
				{
					filestobepurged.push_back(make_pair(p->second.size,p->first));
					scannedsize+=p->second.size;
				}
			}
// ok, let's purge
		std::sort(filestobepurged.begin(), filestobepurged.end(),sortbysize);
		myfiles.clear();
		unsigned int i=0;
// get only files with equal size
// from filestobepurged to myfiles
		while (i<filestobepurged.size())
		{
			int j=i;
			if (filestobepurged[j].first==filestobepurged[j+1].first)
			{
				myfiles.push_back(filestobepurged[j].second);
				total_size+=filestobepurged[j].first;
				quantifiles++;
			}
			while (filestobepurged[j].first==filestobepurged[j+1].first)
			{
				myfiles.push_back(filestobepurged[j+1].second);
				total_size+=filestobepurged[j+1].first;
				quantifiles++;
				j++;
			}
			j++;
			i=j;
		}
	}
	else
	{
//	default: get everything
		for (DTMap::iterator p=edt.begin(); p!=edt.end(); ++p) 
			if (p->second.date && p->first!="" && (!isdirectory(p->first)) && (!isads(p->first)) ) 
			{
				if (minsize>0)
				{
					if ((uint64_t)p->second.size>minsize)
					{
						myfiles.push_back(p->first);
						total_size+=p->second.size;
						quantifiles++;
					}
				}
				else
				{
					myfiles.push_back(p->first);
					total_size+=p->second.size;
					quantifiles++;
				}

			}
		scannedsize=total_size;
	}	
	
	if (myfiles.size()==0)
	{
		printf("Nothing to do\n");
		return 0;
	}
	
	
	vector<string> myhash;
	vector<tparametrihash> 	vettoreparametrihash;

	int mythreads=howmanythreads;
	
	if (!all)
		mythreads=1;
		
	tparametrihash 	myblock;
	for (int i=0;i<mythreads;i++)
	{
		myblock.tnumber=(i%mythreads);
		myblock.inizio=mtime();
		myblock.dimensione=total_size;
		myblock.timestart=0;
		myblock.timeend=0;
		vettoreparametrihash.push_back(myblock);
	}
	
	
	if (!flagnoeta)
	printf("Computing filesize for %s files/directory...\n",migliaia(files.size()));
	
    
	
	int64_t inizio		=mtime();
	double scantime=(inizio-startscan)/1000.0;
	if (!flagnoeta)
	printf("Found (%s) => %s bytes (%s) / %s files in %f\n",tohuman2(scannedsize),migliaia(total_size),tohuman(total_size),migliaia2(quantifiles),scantime);
	
	for (unsigned int i=0;i<myfiles.size();i++)
		vettoreparametrihash[i%mythreads].filestobehashed.push_back(myfiles[i]);

	int totfile=0;
	for (int i=0;i<mythreads;i++)
	{
		if (flagverbose)
			printf("Thread [%02d] files %s\n",i,migliaia(vettoreparametrihash[i].filestobehashed.size()));
		totfile+=+vettoreparametrihash[i].filestobehashed.size();
	}
	if (flagverbose)
	printf("Total files %s -> in threads %s\n",migliaia(myfiles.size()),migliaia2(totfile));
	
	

	int rc;
	///pthread_t threads[mythreads];
	pthread_t* threads = new pthread_t[mythreads];

	pthread_attr_t attr;
	void *status;

		// ini and set thread joinable
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if (!flagnoeta)
		printf("\nCreating %d hashing thread(s)\n",mythreads);
	
	uint64_t iniziohash=mtime();
	for(int i = 0; i < mythreads; i++ ) 
	{
		vettoreparametrihash[i].timestart=mtime();
		rc = pthread_create(&threads[i], &attr, scansionahash, (void*)&vettoreparametrihash[i]);
		if (rc) 
		{
			printf("Error creating thread\n");
			exit(-1);
		}
	}

	pthread_attr_destroy(&attr);
	for(int i = 0; i <mythreads; i++ ) 
	{
		rc = pthread_join(threads[i], &status);
		if (rc) 
		{
			error("Unable to join\n");
			exit(-1);
		}
	///		printf("Thread completed %d status %d\n",i,status);
	}
	uint64_t hashtime=mtime()-iniziohash+1;
	
	vector<std::pair<string, string>> vec;
	
	for(int i = 0; i <mythreads; i++ )
		for (unsigned int j=0;j<vettoreparametrihash[i].filestobehashed.size();j++)
			vec.push_back(make_pair(vettoreparametrihash[i].filestobehashed[j],vettoreparametrihash[i].hashcalculated[j]));

	if ((!flagnosort) || (flagverify))
		std::sort(vec.begin(), vec.end(),sortbyval);
	
	libzpaq::SHA256 sha256;
	
	int64_t startprint=mtime();

	uint64_t testedbytes=0;
	int64_t deletedfiles=0;
	int64_t deletedsize=0;
	for (unsigned int i=0;i<vec.size();i++)
	{
			
		string filename=vec[i].first;
		DTMap::iterator p=edt.find(filename);
	
		if (p != edt.end())
			testedbytes+=p->second.size;
		
		for (const char* p=vec[i].second.c_str(); *p; ++p) 
			sha256.put(*p);
    
		if (searchfrom!="")
			if (replaceto!="")
				replace(filename,searchfrom,replaceto);	
		
					
		if (flagkill)
		{
					
// get ready to make a script (no kill whatsoever!) by hand
			if (i>0)
				if (vec[i-1].second==vec[i].second)
				{
					if (summary<0)
					{
						#if defined(_WIN32) || defined(_WIN64)
						myreplaceall(filename,"/","\\");
						#endif
						printf("=== \"");
						printUTF8(filename.c_str());
						printf("\"\n");
					}
					duplicated_files++;
					if (p != edt.end())
						duplicated_size+=p->second.size;
					if (flagforce)
						if (delete_file(filename.c_str()))
						{
							deletedfiles++;
							if (p != edt.end())
								deletedsize+=p->second.size;
						}
				}
		}
		else
		{
			if (summary<0)
			{
				printf("%s: %s ",mygetalgo().c_str(),vec[i].second.c_str());
				if (p != edt.end())
					printf("[%19s] ",migliaia(p->second.size));
			}
			if (!flagnosort)
			{			
				if (i==0)
				{	
					if (summary<0)
						printf("    ");
				}	
				else
				{
					if (vec[i-1].second==vec[i].second)
					{
						if (summary<0)
							printf("=== ");
						duplicated_files++;
						if (p != edt.end())
							duplicated_size+=p->second.size;
					}
					else
					{
						if (summary<0)
							printf("    ");
					}
				}
			}
			if (summary<0)
			{
				printUTF8(filename.c_str());
				printf("\n");
			}
		}
	}
	
	int64_t printtime=mtime()-startprint;
	if (!flagnoeta)
	{
		printf("Algo %s by %d threads\n",mygetalgo().c_str(),mythreads);
		printf("Scanning filesystem time  %f s\n",scantime);
		printf("Data transfer+CPU   time  %f s\n",hashtime/1000.0);
		printf("Data output         time  %f s\n",printtime/1000.0);
		printf("Total size                %19s (%10s)\n",migliaia(scannedsize),tohuman(scannedsize));
		printf("Tested size               %19s (%10s)\n",migliaia(testedbytes),tohuman(testedbytes));
		printf("Duplicated size           %19s (%10s)\n",migliaia(duplicated_size),tohuman(duplicated_size));
		printf("Duplicated files          %19s\n",migliaia(duplicated_files));
		int64_t myspeed=(int64_t)(testedbytes*1000.0/(hashtime));
		printf("Worked on %s bytes avg speed (hashtime) %s B/s\n",migliaia(total_size),migliaia2(myspeed));
	}
	char sha256result[32];
	memcpy(sha256result, sha256.result(), 32);
	printf("GLOBAL SHA256: ");
	for (int j=0; j <= 31; j++)
		printf("%02X", (unsigned char)sha256result[j]);
	printf("\n");

	if (flagkill)
		if (flagforce)
			if (deletedfiles)
				printf("Duplicated deleted files %s for %s bytes\n",migliaia2(deletedfiles),migliaia(deletedsize));
	
	delete [] threads;
	return 0;
}


/////////////////////////////// main //////////////////////////////////


#ifdef _WIN32
	#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
		#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
	#endif

	static HANDLE stdoutHandle;
	static DWORD outModeInit;
	 
	void setupConsole(void) 
	{
		DWORD outMode 	= 0;
		stdoutHandle 	= GetStdHandle(STD_OUTPUT_HANDLE);
	 
		if(stdoutHandle == INVALID_HANDLE_VALUE)
			exit(GetLastError());
		
		if(!GetConsoleMode(stdoutHandle, &outMode))
			exit(GetLastError());
		outModeInit = outMode;
		
		 // Enable ANSI escape codes
		outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	 
		if(!SetConsoleMode(stdoutHandle, outMode)) 
			exit(GetLastError());
	}
	 
	void restoreConsole(void) 
	{
		// Reset colors
		printf("\x1b[0m");	
		
		// Reset console mode
		if(!SetConsoleMode(stdoutHandle, outModeInit)) 
			exit(GetLastError());
	}
#else // Houston, we have Unix
	void setupConsole(void) 
	{
	}

	void restoreConsole(void) 
	{
		//colors
		printf("\x1b[0m");
	}
#endif

/// control-c handler
void my_handler(int s)
{
	// 2==control-C (maybe)
	if (s==2)
	{	// we want the cursor back!
		setupConsole();
		printf("\e[?25h");
		fflush(stdout);
		restoreConsole();
	}
	exit(1); 
}

// Convert argv to UTF-8 and replace \ with /
#ifdef unix
	int main(int argc, const char** argv) 
	{
#else
	#ifdef _MSC_VER
		int wmain(int argc, LPWSTR* argw) 
		{
	#else
		int main() 
		{
			int argc=0;
			LPWSTR* argw=CommandLineToArgvW(GetCommandLine(), &argc);
	#endif

///	This is Windows: take care of parameters
	vector<string> args(argc);

	libzpaq::Array<const char*> argp(argc);
	for (int i=0; i<argc; ++i) 
	{
		args[i]=wtou(argw[i]);
		argp[i]=args[i].c_str();
	}
	const char** argv=&argp[0];
#endif

	global_start=mtime();  		// get start time
	signal (SIGINT,my_handler); // the control-C handler
   
#ifdef _WIN32
//// set UTF-8 for console
	SetConsoleCP(65001);
	SetConsoleOutputCP(65001);
	OSVERSIONINFO vi = {0};
	vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&vi);
	windows7_or_above = (vi.dwMajorVersion >= 6) && (vi.dwMinorVersion >= 1);
#endif

	int errorcode=0;
	try 
	{
		Jidac jidac;
		errorcode=jidac.doCommand(argc, argv);
	}
	catch (std::exception& e) 
	{
		fflush(stdout);
		fprintf(stderr, "23013: zpaq error: %s\n", e.what());
		errorcode=2;
	}
  
	fflush(stdout);
	fprintf(stderr, "\n%1.3f seconds %s\n", (mtime()-global_start)/1000.0,
      errorcode>1 ? "(with errors)" :
      errorcode>0 ? "(with warnings)" : "(all OK)");
	  
	if (errorcode)
		xcommand(g_exec_error,g_exec_text);
	else
		xcommand(g_exec_ok,g_exec_text);
	
	return errorcode;
}






struct xorshift128plus_key_s {
    uint64_t part1;
    uint64_t part2;
};

typedef struct xorshift128plus_key_s xorshift128plus_key_t;

static inline void xorshift128plus_init(uint64_t key1, uint64_t key2, xorshift128plus_key_t *key) {
  key->part1 = key1;
  key->part2 = key2;
}

uint64_t xorshift128plus(xorshift128plus_key_t * key) {
    uint64_t s1 = key->part1;
    const uint64_t s0 = key->part2;
    key->part1 = s0;
    s1 ^= s1 << 23; // a
    key->part2 = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5); // b, c
    return key->part2 + s0;
}


void xorshift128plus_jump(xorshift128plus_key_t * key) {
    static const uint64_t JUMP[] = { 0x8a5cd789635d2dff, 0x121fd2155c472f96 };
    uint64_t s0 = 0;
    uint64_t s1 = 0;
    for(unsigned int i = 0; i < sizeof (JUMP) / sizeof (*JUMP); i++)
        for(int b = 0; b < 64; b++) {
            if (JUMP[i] & 1ULL << b) {
                s0 ^= key->part1;
                s1 ^= key->part2;
            }
            xorshift128plus(key);
        }

    key->part1 = s0;
    key->part2 = s1;
}

void populateRandom_xorshift128plus(uint32_t *answer, uint32_t size,uint64_t i_key1, uint64_t i_key2) 
{
  xorshift128plus_key_t mykey = {.part1 = i_key1, .part2 = i_key2};
  xorshift128plus_init(i_key1, i_key2, &mykey);
  uint32_t i = size;
  while (i > 2) {
    *(uint64_t *)(answer + size - i) = xorshift128plus(&mykey);
    i -= 2;
  }
  if (i != 0)
    answer[size - i] = (uint32_t)xorshift128plus(&mykey);
}





typedef map<string, string> MAPPASTRINGASTRINGA;

int Jidac::utf() 
{

#ifdef ESX
	printf("GURU: sorry: ESXi does not like this things\n");
	exit(0);
#endif
		
	vector<string> sourcefile;     
	
	g_bytescanned=0;
	g_filescanned=0;
	g_worked=0;
	edt.clear();
	
	for (unsigned i=0; i<files.size(); ++i)
		scandir(true,edt,files[i].c_str());
	
	for (DTMap::iterator p=edt.begin(); p!=edt.end(); ++p) 
		if (!isdirectory(p->first))
			sourcefile.push_back(p->first);
	
	vector<string> strange;     
	
	printf("\nCheck directories %s\n",migliaia(sourcefile.size()));
	for (int unsigned i=0;i<sourcefile.size();i++)
	{
		string filename=extractfilepath(sourcefile[i]);
		string ansifilename=utf8toansi(filename);
			
		string ragione="";
		if (filename!=ansifilename)
			ragione+="CHAR |";

		if (dirlength>0)
			if (filename.length()>dirlength)
				ragione+="LEN "+std::to_string(dirlength)+"-|";

		if (ragione!="")
			strange.push_back(padleft(ragione,15)+filename);
	}
	
	
	sort( strange.begin(), strange.end() );
	strange.erase( unique( strange.begin(), strange.end() ), strange.end() );
	if (!flagutf)
		for (int unsigned i=0;i<strange.size();i++)
		{
			printf("DIR:%08lu <<",(long unsigned int)strange[i].length());
			printUTF8(strange[i].c_str());
			printf(">>\n");
		}
	printf("DIR:Total %s\n",migliaia(strange.size()));
	
	strange.clear();
	printf("\nCheck files %s\n",migliaia(sourcefile.size()));
	
	for (unsigned int i=0;i<sourcefile.size();i++)
	{
		string filename=extractfilename(sourcefile[i]);
		string ansifilename=utf8toansi(filename);
	
		string ragione="";
		
		if (filename!=ansifilename)
			ragione+="CHAR |";

		if (filelength>0)
			if (filename.length()>filelength)
				ragione+="LEN "+std::to_string(filelength)+"-|";
		if (ragione!="")
			strange.push_back(padleft(ragione,15)+filename);

	}
	sort( strange.begin(), strange.end() );
	strange.erase( unique( strange.begin(), strange.end() ), strange.end() );
	
	if (!flagutf)
		for (unsigned int i=0;i<strange.size();i++)
		{
			printf("FILE:%08ld <<",(long int)strange[i].length());
			printUTF8(strange[i].c_str());
			printf(">>\n");
		}
	printf("FILE:Total %s\n",migliaia(strange.size()));

	if (!flagutf)
		return 0;
		
	printf("******* RENAME STRANGE FILES\n");

	printsanitizeflags();

	if (!flagkill)
	printf("******* -kill not present => this is a dry run\n");
	
	int rinominati=0;
	int nonrinominati=0;
	int kollision=0;
	MAPPAFILEHASH mappacollisioni;
	MAPPASTRINGASTRINGA mappatofrom;
	
	for (unsigned int i=0;i<sourcefile.size();i++)
	{
		if (isdirectory(sourcefile[i]))
		{
			printf("This is strange, dir skipped %s\n",sourcefile[i].c_str());
		}
		else
		{
		string newname=sanitizzanomefile(sourcefile[i],filelength,kollision,mappacollisioni);
		
		if ((newname!=sourcefile[i]))
		{
			if (flagdebug)
			{
				printf("25733: pre       <<%s>>\n",sourcefile[i].c_str());
				printf("25734: Inserisco <<%s>>\n\n\n",newname.c_str());
			}
			mappatofrom.insert(std::pair<string, string>(newname, "DUMMY"));
						
			string sfrom=sourcefile[i];
#ifdef _WIN32
			myreplaceall(sfrom,"/","\\");
#endif
			string sto=newname; 
#ifdef _WIN32
			sto=nomefileseesistegia(sto);
			myreplaceall(sto,"/","\\");
#endif
			if (flagverbose)
			{
				printf("FROM: ");
				printUTF8(sfrom.c_str());
				printf("\nTO:   %s\n\n",sto.c_str());
			}
			
			if (flagkill)
			{
#ifdef _WIN32				
				std::wstring wfrom=utow(sfrom.c_str());  
				std::wstring wto=utow(sto.c_str());  
				if (!MoveFileW(wfrom.c_str(),wto.c_str()))
				{
					nonrinominati++;
					printf("ERROR WIN renaming\n");
					printf("from <<");
					printUTF8(sfrom.c_str());
					printf(">>\n");
					printf("to   <<%s>>\n",sto.c_str());
					
					printf("ERROR renaming <<");
					printUTF8(sfrom.c_str());
					printf(">> to <<%s>>\n",sto.c_str());
					if (flagverbose)
						for (unsigned int j=0;j<sfrom.length();j++)
							printf("%03d %03d %c\n",j,sto[j],sto[j]);
				}
#else
				if (::rename(sfrom.c_str(), sto.c_str()) != 0)
				{
					nonrinominati++;
					printf("ERROR NIX renaming\n");
					printf("from <<%s>>\n",sfrom.c_str());
					printf("to   <<%s>>\n",sto.c_str());
					if (flagverbose)
						for (unsigned int j=0;j<sfrom.length();j++)
							printf("%03d %03d %c\n",j,sto[j],sto[j]);

				}
#endif	
				else
					rinominati++;		
		///		printf("Renaming %08d failed %08d\r",rinominati,nonrinominati);
			}
			else
				nonrinominati++;

		}
		}
	}
	printf("\n");
	printf("Candidates  %08d\n",rinominati+nonrinominati);
	if (flagkill)
	{
		printf("Renamed     %08d\n",rinominati);
		printf("Failed      %08d\n",nonrinominati);
	}
	else
		printf("*** dry run\n");
	if (nonrinominati>0)
		return 2;
	else
		return 0;
	
}




///////// This is a merge of unzpaq206.cpp, patched by me to become unz.cpp
///////// Now support FRANZOFFSET (embedding SHA1 into ZPAQ's c block)


int Jidac::paranoid() 
{
#ifdef _WIN32
#ifndef _WIN64
	printf("WARNING: paranoid test use lots of RAM, not good for Win32, better Win64\n");
#endif
#endif
#ifdef ESX
	printf("GURU: sorry: ESXi does not like to give so much RAM\n");
	exit(0);
#endif

	unz(archive.c_str(), password,all);
	return 0;
}


// Callback for user defined ZPAQ error handling.
// It will be called on input error with an English language error message.
// This function should not return.
extern void unzerror(const char* msg);

// Virtual base classes for input and output
// get() and put() must be overridden to read or write 1 byte.
class unzReader {
public:
  virtual int get() = 0;  // should return 0..255, or -1 at EOF
  virtual ~unzReader() {}
};

class unzWriter {
public:
  virtual void put(int c) = 0;  // should output low 8 bits of c
  virtual ~unzWriter() {}
};

// An Array of T is cleared and aligned on a 64 byte address
//   with no constructors called. No copy or assignment.
// Array<T> a(n, ex=0);  - creates n<<ex elements of type T
// a[i] - index
// a(i) - index mod n, n must be a power of 2
// a.size() - gets n
template <typename T>
class Array {
  T *data;     // user location of [0] on a 64 byte boundary
  size_t n;    // user size
  int offset;  // distance back in bytes to start of actual allocation
  void operator=(const Array&);  // no assignment
  Array(const Array&);  // no copy
public:
  Array(size_t sz=0, int ex=0): data(0), n(0), offset(0) {
    resize(sz, ex);} // [0..sz-1] = 0
  void resize(size_t sz, int ex=0); // change size, erase content to zeros
  ~Array() {resize(0);}  // free memory
  size_t size() const {return n;}  // get size
  int isize() const {return int(n);}  // get size as an int
  T& operator[](size_t i) {assert(n>0 && i<n); return data[i];}
  T& operator()(size_t i) {assert(n>0 && (n&(n-1))==0); return data[i&(n-1)];}
};

// Change size to sz<<ex elements of 0
template<typename T>
void Array<T>::resize(size_t sz, int ex) {
  assert(size_t(-1)>0);  // unsigned type?
  while (ex>0) {
    if (sz>sz*2) unzerror("Array too big");
    sz*=2, --ex;
  }
  if (n>0) {
    assert(offset>=0 && offset<=64);
    assert((char*)data-offset);
    free((char*)data-offset);
  }
  n=0;
  if (sz==0) return;
  n=sz;
  const size_t nb=128+n*sizeof(T);  // test for overflow
  if (nb<=128 || (nb-128)/sizeof(T)!=n) unzerror("Array too big");
  data=(T*)calloc(nb, 1);
  if (!data) unzerror("out of memory");

  // Align array on a 64 byte address.
  // This optimization is NOT required by the ZPAQ standard.
  offset=64-(((char*)data-(char*)0)&63);
  assert(offset>0 && offset<=64);
  data=(T*)((char*)data+offset);
}

//////////////////////////// unzSHA1 ////////////////////////////

// For computing SHA-1 checksums
class unzSHA1 {
public:
  void put(int c) {  // hash 1 byte
    uint32_t& r=w[len0>>5&15];
    r=(r<<8)|(c&255);
    if (!(len0+=8)) ++len1;
    if ((len0&511)==0) process();
  }
  double size() const {return len0/8+len1*536870912.0;} // size in bytes
  const char* result();  // get hash and reset
  unzSHA1() {init();}
private:
  void init();      // reset, but don't clear hbuf
  uint32_t len0, len1;   // length in bits (low, high)
  uint32_t h[5];         // hash state
  uint32_t w[80];        // input buffer
  char hbuf[20];    // result
  void process();   // hash 1 block
};

// Start a new hash
void unzSHA1::init() {
  len0=len1=0;
  h[0]=0x67452301;
  h[1]=0xEFCDAB89;
  h[2]=0x98BADCFE;
  h[3]=0x10325476;
  h[4]=0xC3D2E1F0;
}

// Return old result and start a new hash
const char* unzSHA1::result() {

  // pad and append length
  const uint32_t s1=len1, s0=len0;
  put(0x80);
  while ((len0&511)!=448)
    put(0);
  put(s1>>24);
  put(s1>>16);
  put(s1>>8);
  put(s1);
  put(s0>>24);
  put(s0>>16);
  put(s0>>8);
  put(s0);

  // copy h to hbuf
  for (int i=0; i<5; ++i) {
    hbuf[4*i]=h[i]>>24;
    hbuf[4*i+1]=h[i]>>16;
    hbuf[4*i+2]=h[i]>>8;
    hbuf[4*i+3]=h[i];
  }

  // return hash prior to clearing state
  init();
  return hbuf;
}

// Hash 1 block of 64 bytes
void unzSHA1::process() {
  for (int i=16; i<80; ++i) {
    w[i]=w[i-3]^w[i-8]^w[i-14]^w[i-16];
    w[i]=w[i]<<1|w[i]>>31;
  }
  uint32_t a=h[0];
  uint32_t b=h[1];
  uint32_t c=h[2];
  uint32_t d=h[3];
  uint32_t e=h[4];
  const uint32_t k1=0x5A827999, k2=0x6ED9EBA1, k3=0x8F1BBCDC, k4=0xCA62C1D6;
#define f1(a,b,c,d,e,i) e+=(a<<5|a>>27)+((b&c)|(~b&d))+k1+w[i]; b=b<<30|b>>2;
#define f5(i) f1(a,b,c,d,e,i) f1(e,a,b,c,d,i+1) f1(d,e,a,b,c,i+2) \
              f1(c,d,e,a,b,i+3) f1(b,c,d,e,a,i+4)
  f5(0) f5(5) f5(10) f5(15)
#undef f1
#define f1(a,b,c,d,e,i) e+=(a<<5|a>>27)+(b^c^d)+k2+w[i]; b=b<<30|b>>2;
  f5(20) f5(25) f5(30) f5(35)
#undef f1
#define f1(a,b,c,d,e,i) e+=(a<<5|a>>27)+((b&c)|(b&d)|(c&d))+k3+w[i]; \
        b=b<<30|b>>2;
  f5(40) f5(45) f5(50) f5(55)
#undef f1
#define f1(a,b,c,d,e,i) e+=(a<<5|a>>27)+(b^c^d)+k4+w[i]; b=b<<30|b>>2;
  f5(60) f5(65) f5(70) f5(75)
#undef f1
#undef f5
  h[0]+=a;
  h[1]+=b;
  h[2]+=c;
  h[3]+=d;
  h[4]+=e;
}

//////////////////////////// unzSHA256 //////////////////////////

// For computing SHA-256 checksums
// http://en.wikipedia.org/wiki/SHA-2
class unzSHA256 {
public:
  void put(int c) {  // hash 1 byte
    unsigned& r=w[len0>>5&15];
    r=(r<<8)|(c&255);
    if (!(len0+=8)) ++len1;
    if ((len0&511)==0) process();
  }
  double size() const {return len0/8+len1*536870912.0;} // size in bytes
  uint64_t usize() const {return len0/8+(uint64_t(len1)<<29);} //size in bytes
  const char* result();  // get hash and reset
  unzSHA256() {init();}
private:
  void init();           // reset, but don't clear hbuf
  unsigned len0, len1;   // length in bits (low, high)
  unsigned s[8];         // hash state
  unsigned w[16];        // input buffer
  char hbuf[32];         // result
  void process();        // hash 1 block
};

void unzSHA256::init() {
  len0=len1=0;
  s[0]=0x6a09e667;
  s[1]=0xbb67ae85;
  s[2]=0x3c6ef372;
  s[3]=0xa54ff53a;
  s[4]=0x510e527f;
  s[5]=0x9b05688c;
  s[6]=0x1f83d9ab;
  s[7]=0x5be0cd19;
  memset(w, 0, sizeof(w));
}

void unzSHA256::process() {

  #define ror(a,b) ((a)>>(b)|(a<<(32-(b))))

  #define m(i) \
     w[(i)&15]+=w[(i-7)&15] \
       +(ror(w[(i-15)&15],7)^ror(w[(i-15)&15],18)^(w[(i-15)&15]>>3)) \
       +(ror(w[(i-2)&15],17)^ror(w[(i-2)&15],19)^(w[(i-2)&15]>>10))

  #define r(a,b,c,d,e,f,g,h,i) { \
    unsigned t1=ror(e,14)^e; \
    t1=ror(t1,5)^e; \
    h+=ror(t1,6)+((e&f)^(~e&g))+k[i]+w[(i)&15]; } \
    d+=h; \
    {unsigned t1=ror(a,9)^a; \
    t1=ror(t1,11)^a; \
    h+=ror(t1,2)+((a&b)^(c&(a^b))); }

  #define mr(a,b,c,d,e,f,g,h,i) m(i); r(a,b,c,d,e,f,g,h,i);

  #define r8(i) \
    r(a,b,c,d,e,f,g,h,i);   \
    r(h,a,b,c,d,e,f,g,i+1); \
    r(g,h,a,b,c,d,e,f,i+2); \
    r(f,g,h,a,b,c,d,e,i+3); \
    r(e,f,g,h,a,b,c,d,i+4); \
    r(d,e,f,g,h,a,b,c,i+5); \
    r(c,d,e,f,g,h,a,b,i+6); \
    r(b,c,d,e,f,g,h,a,i+7);

  #define mr8(i) \
    mr(a,b,c,d,e,f,g,h,i);   \
    mr(h,a,b,c,d,e,f,g,i+1); \
    mr(g,h,a,b,c,d,e,f,i+2); \
    mr(f,g,h,a,b,c,d,e,i+3); \
    mr(e,f,g,h,a,b,c,d,i+4); \
    mr(d,e,f,g,h,a,b,c,i+5); \
    mr(c,d,e,f,g,h,a,b,i+6); \
    mr(b,c,d,e,f,g,h,a,i+7);

  static const unsigned k[64]={
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

  unsigned a=s[0];
  unsigned b=s[1];
  unsigned c=s[2];
  unsigned d=s[3];
  unsigned e=s[4];
  unsigned f=s[5];
  unsigned g=s[6];
  unsigned h=s[7];

  r8(0);
  r8(8);
  mr8(16);
  mr8(24);
  mr8(32);
  mr8(40);
  mr8(48);
  mr8(56);

  s[0]+=a;
  s[1]+=b;
  s[2]+=c;
  s[3]+=d;
  s[4]+=e;
  s[5]+=f;
  s[6]+=g;
  s[7]+=h;

  #undef mr8
  #undef r8
  #undef mr
  #undef r
  #undef m
  #undef ror
}

// Return old result and start a new hash
const char* unzSHA256::result() {

  // pad and append length
  const unsigned s1=len1, s0=len0;
  put(0x80);
  while ((len0&511)!=448) put(0);
  put(s1>>24);
  put(s1>>16);
  put(s1>>8);
  put(s1);
  put(s0>>24);
  put(s0>>16);
  put(s0>>8);
  put(s0);

  // copy s to hbuf
  for (int i=0; i<8; ++i) {
    hbuf[4*i]=s[i]>>24;
    hbuf[4*i+1]=s[i]>>16;
    hbuf[4*i+2]=s[i]>>8;
    hbuf[4*i+3]=s[i];
  }

  // return hash prior to clearing state
  init();
  return hbuf;
}

//////////////////////////// AES /////////////////////////////

// For encrypting with AES in CTR mode.
// The i'th 16 byte block is encrypted by XOR with AES(i)
// (i is big endian or MSB first, starting with 0).
class unzAES_CTR {
  uint32_t Te0[256], Te1[256], Te2[256], Te3[256], Te4[256]; // encryption tables
  uint32_t ek[60];  // round key
  int Nr;  // number of rounds (10, 12, 14 for AES 128, 192, 256)
  uint32_t iv0, iv1;  // first 8 bytes in CTR mode
public:
  unzAES_CTR(const char* key, int keylen, const char* iv=0);
    // Schedule: keylen is 16, 24, or 32, iv is 8 bytes or NULL
  void encrypt(uint32_t s0, uint32_t s1, uint32_t s2, uint32_t s3, unsigned char* ct);
  void encrypt(char* unzBuf, int n, uint64_t offset);  // encrypt n bytes of unzBuf
};

// Some AES code is derived from libtomcrypt 1.17 (public domain).

#define Te4_0 0x000000FF & Te4
#define Te4_1 0x0000FF00 & Te4
#define Te4_2 0x00FF0000 & Te4
#define Te4_3 0xFF000000 & Te4

// Extract byte n of x
static inline unsigned unzbyte(unsigned x, unsigned n) {return (x>>(8*n))&255;}

// x = y[0..3] MSB first
static inline void LOAD32H(uint32_t& x, const char* y) {
  const unsigned char* u=(const unsigned char*)y;
  x=u[0]<<24|u[1]<<16|u[2]<<8|u[3];
}

// y[0..3] = x MSB first
static inline void STORE32H(uint32_t& x, unsigned char* y) {
  y[0]=x>>24;
  y[1]=x>>16;
  y[2]=x>>8;
  y[3]=x;
}

#define setup_mix(temp) \
  ((Te4_3[unzbyte(temp, 2)]) ^ (Te4_2[unzbyte(temp, 1)]) ^ \
   (Te4_1[unzbyte(temp, 0)]) ^ (Te4_0[unzbyte(temp, 3)]))

// Initialize encryption tables and round key. keylen is 16, 24, or 32.
unzAES_CTR::unzAES_CTR(const char* key, int keylen, const char* iv) {
  assert(key  != NULL);
  assert(keylen==16 || keylen==24 || keylen==32);

  // Initialize IV (default 0)
  iv0=iv1=0;
  if (iv) {
    LOAD32H(iv0, iv);
    LOAD32H(iv1, iv+4);
  }

  // Initialize encryption tables
  for (int i=0; i<256; ++i) {
    unsigned s1=
    "\x63\x7c\x77\x7b\xf2\x6b\x6f\xc5\x30\x01\x67\x2b\xfe\xd7\xab\x76"
    "\xca\x82\xc9\x7d\xfa\x59\x47\xf0\xad\xd4\xa2\xaf\x9c\xa4\x72\xc0"
    "\xb7\xfd\x93\x26\x36\x3f\xf7\xcc\x34\xa5\xe5\xf1\x71\xd8\x31\x15"
    "\x04\xc7\x23\xc3\x18\x96\x05\x9a\x07\x12\x80\xe2\xeb\x27\xb2\x75"
    "\x09\x83\x2c\x1a\x1b\x6e\x5a\xa0\x52\x3b\xd6\xb3\x29\xe3\x2f\x84"
    "\x53\xd1\x00\xed\x20\xfc\xb1\x5b\x6a\xcb\xbe\x39\x4a\x4c\x58\xcf"
    "\xd0\xef\xaa\xfb\x43\x4d\x33\x85\x45\xf9\x02\x7f\x50\x3c\x9f\xa8"
    "\x51\xa3\x40\x8f\x92\x9d\x38\xf5\xbc\xb6\xda\x21\x10\xff\xf3\xd2"
    "\xcd\x0c\x13\xec\x5f\x97\x44\x17\xc4\xa7\x7e\x3d\x64\x5d\x19\x73"
    "\x60\x81\x4f\xdc\x22\x2a\x90\x88\x46\xee\xb8\x14\xde\x5e\x0b\xdb"
    "\xe0\x32\x3a\x0a\x49\x06\x24\x5c\xc2\xd3\xac\x62\x91\x95\xe4\x79"
    "\xe7\xc8\x37\x6d\x8d\xd5\x4e\xa9\x6c\x56\xf4\xea\x65\x7a\xae\x08"
    "\xba\x78\x25\x2e\x1c\xa6\xb4\xc6\xe8\xdd\x74\x1f\x4b\xbd\x8b\x8a"
    "\x70\x3e\xb5\x66\x48\x03\xf6\x0e\x61\x35\x57\xb9\x86\xc1\x1d\x9e"
    "\xe1\xf8\x98\x11\x69\xd9\x8e\x94\x9b\x1e\x87\xe9\xce\x55\x28\xdf"
    "\x8c\xa1\x89\x0d\xbf\xe6\x42\x68\x41\x99\x2d\x0f\xb0\x54\xbb\x16"
    [i]&255;
    unsigned s2=s1<<1;
    if (s2>=0x100) s2^=0x11b;
    unsigned s3=s1^s2;
    Te0[i]=s2<<24|s1<<16|s1<<8|s3;
    Te1[i]=s3<<24|s2<<16|s1<<8|s1;
    Te2[i]=s1<<24|s3<<16|s2<<8|s1;
    Te3[i]=s1<<24|s1<<16|s3<<8|s2;
    Te4[i]=s1<<24|s1<<16|s1<<8|s1;
  }

  // setup the forward key
  Nr = 10 + ((keylen/8)-2)*2;  // 10, 12, or 14 rounds
  int i = 0;
  uint32_t* rk = &ek[0];
  uint32_t temp;
  static const uint32_t rcon[10] = {
    0x01000000UL, 0x02000000UL, 0x04000000UL, 0x08000000UL,
    0x10000000UL, 0x20000000UL, 0x40000000UL, 0x80000000UL,
    0x1B000000UL, 0x36000000UL};  // round constants

  LOAD32H(rk[0], key   );
  LOAD32H(rk[1], key +  4);
  LOAD32H(rk[2], key +  8);
  LOAD32H(rk[3], key + 12);
  if (keylen == 16) {
    for (;;) {
      temp  = rk[3];
      rk[4] = rk[0] ^ setup_mix(temp) ^ rcon[i];
      rk[5] = rk[1] ^ rk[4];
      rk[6] = rk[2] ^ rk[5];
      rk[7] = rk[3] ^ rk[6];
      if (++i == 10) {
         break;
      }
      rk += 4;
    }
  }
  else if (keylen == 24) {
    LOAD32H(rk[4], key + 16);
    LOAD32H(rk[5], key + 20);
    for (;;) {
      temp = rk[5];
      rk[ 6] = rk[ 0] ^ setup_mix(temp) ^ rcon[i];
      rk[ 7] = rk[ 1] ^ rk[ 6];
      rk[ 8] = rk[ 2] ^ rk[ 7];
      rk[ 9] = rk[ 3] ^ rk[ 8];
      if (++i == 8) {
        break;
      }
      rk[10] = rk[ 4] ^ rk[ 9];
      rk[11] = rk[ 5] ^ rk[10];
      rk += 6;
    }
  }
  else if (keylen == 32) {
    LOAD32H(rk[4], key + 16);
    LOAD32H(rk[5], key + 20);
    LOAD32H(rk[6], key + 24);
    LOAD32H(rk[7], key + 28);
    for (;;) {
      temp = rk[7];
      rk[ 8] = rk[ 0] ^ setup_mix(temp) ^ rcon[i];
      rk[ 9] = rk[ 1] ^ rk[ 8];
      rk[10] = rk[ 2] ^ rk[ 9];
      rk[11] = rk[ 3] ^ rk[10];
      if (++i == 7) {
        break;
      }
      temp = rk[11];
      rk[12] = rk[ 4] ^ setup_mix(temp<<24|temp>>8);
      rk[13] = rk[ 5] ^ rk[12];
      rk[14] = rk[ 6] ^ rk[13];
      rk[15] = rk[ 7] ^ rk[14];
      rk += 8;
    }
  }
}

// Encrypt to ct[16]
void unzAES_CTR::encrypt(uint32_t s0, uint32_t s1, uint32_t s2, uint32_t s3, unsigned char* ct) {
  int r = Nr >> 1;
  uint32_t *rk = &ek[0];
  uint32_t t0=0, t1=0, t2=0, t3=0;
  s0 ^= rk[0];
  s1 ^= rk[1];
  s2 ^= rk[2];
  s3 ^= rk[3];
  for (;;) {
    t0 =
      Te0[unzbyte(s0, 3)] ^
      Te1[unzbyte(s1, 2)] ^
      Te2[unzbyte(s2, 1)] ^
      Te3[unzbyte(s3, 0)] ^
      rk[4];
    t1 =
      Te0[unzbyte(s1, 3)] ^
      Te1[unzbyte(s2, 2)] ^
      Te2[unzbyte(s3, 1)] ^
      Te3[unzbyte(s0, 0)] ^
      rk[5];
    t2 =
      Te0[unzbyte(s2, 3)] ^
      Te1[unzbyte(s3, 2)] ^
      Te2[unzbyte(s0, 1)] ^
      Te3[unzbyte(s1, 0)] ^
      rk[6];
    t3 =
      Te0[unzbyte(s3, 3)] ^
      Te1[unzbyte(s0, 2)] ^
      Te2[unzbyte(s1, 1)] ^
      Te3[unzbyte(s2, 0)] ^
      rk[7];

    rk += 8;
    if (--r == 0) {
      break;
    }

    s0 =
      Te0[unzbyte(t0, 3)] ^
      Te1[unzbyte(t1, 2)] ^
      Te2[unzbyte(t2, 1)] ^
      Te3[unzbyte(t3, 0)] ^
      rk[0];
    s1 =
      Te0[unzbyte(t1, 3)] ^
      Te1[unzbyte(t2, 2)] ^
      Te2[unzbyte(t3, 1)] ^
      Te3[unzbyte(t0, 0)] ^
      rk[1];
    s2 =
      Te0[unzbyte(t2, 3)] ^
      Te1[unzbyte(t3, 2)] ^
      Te2[unzbyte(t0, 1)] ^
      Te3[unzbyte(t1, 0)] ^
      rk[2];
    s3 =
      Te0[unzbyte(t3, 3)] ^
      Te1[unzbyte(t0, 2)] ^
      Te2[unzbyte(t1, 1)] ^
      Te3[unzbyte(t2, 0)] ^
      rk[3];
  }

  // apply last round and map cipher state to byte array block:
  s0 =
    (Te4_3[unzbyte(t0, 3)]) ^
    (Te4_2[unzbyte(t1, 2)]) ^
    (Te4_1[unzbyte(t2, 1)]) ^
    (Te4_0[unzbyte(t3, 0)]) ^
    rk[0];
  STORE32H(s0, ct);
  s1 =
    (Te4_3[unzbyte(t1, 3)]) ^
    (Te4_2[unzbyte(t2, 2)]) ^
    (Te4_1[unzbyte(t3, 1)]) ^
    (Te4_0[unzbyte(t0, 0)]) ^
    rk[1];
  STORE32H(s1, ct+4);
  s2 =
    (Te4_3[unzbyte(t2, 3)]) ^
    (Te4_2[unzbyte(t3, 2)]) ^
    (Te4_1[unzbyte(t0, 1)]) ^
    (Te4_0[unzbyte(t1, 0)]) ^
    rk[2];
  STORE32H(s2, ct+8);
  s3 =
    (Te4_3[unzbyte(t3, 3)]) ^
    (Te4_2[unzbyte(t0, 2)]) ^
    (Te4_1[unzbyte(t1, 1)]) ^
    (Te4_0[unzbyte(t2, 0)]) ^ 
    rk[3];
  STORE32H(s3, ct+12);
}

// Encrypt or decrypt slice unzBuf[0..n-1] at offset by XOR with AES(i) where
// i is the 128 bit big-endian distance from the start in 16 byte blocks.
void unzAES_CTR::encrypt(char* unzBuf, int n, uint64_t offset) {
  for (uint64_t i=offset/16; i<=(offset+n)/16; ++i) {
    unsigned char ct[16];
    encrypt(iv0, iv1, i>>32, i, ct);
    for (int j=0; j<16; ++j) {
      const int k=i*16-offset+j;
      if (k>=0 && k<n)
        unzBuf[k]^=ct[j];
    }
  }
}

#undef setup_mix
#undef Te4_3
#undef Te4_2
#undef Te4_1
#undef Te4_0

//////////////////////////// stretchKey //////////////////////

// PBKDF2(pw[0..pwlen], salt[0..saltlen], c) to unzBuf[0..dkLen-1]
// using HMAC-unzSHA256, for the special case of c = 1 iterations
// output size dkLen a multiple of 32, and pwLen <= 64.
static void unzpbkdf2(const char* pw, int pwLen, const char* salt, int saltLen,
                   int c, char* unzBuf, int dkLen) {
  assert(c==1);
  assert(dkLen%32==0);
  assert(pwLen<=64);

  unzSHA256 sha256;
  char b[32];
  for (int i=1; i*32<=dkLen; ++i) {
    for (int j=0; j<pwLen; ++j) sha256.put(pw[j]^0x36);
    for (int j=pwLen; j<64; ++j) sha256.put(0x36);
    for (int j=0; j<saltLen; ++j) sha256.put(salt[j]);
    for (int j=24; j>=0; j-=8) sha256.put(i>>j);
    memcpy(b, sha256.result(), 32);
    for (int j=0; j<pwLen; ++j) sha256.put(pw[j]^0x5c);
    for (int j=pwLen; j<64; ++j) sha256.put(0x5c);
    for (int j=0; j<32; ++j) sha256.put(b[j]);
    memcpy(unzBuf+i*32-32, sha256.result(), 32);
  }
}

// Hash b[0..15] using 8 rounds of salsa20
// Modified from http://cr.yp.to/salsa20.html (public domain) to 8 rounds
static void salsa8(uint32_t* b) {
  unsigned x[16]={0};
  memcpy(x, b, 64);
  for (int i=0; i<4; ++i) {
    #define R(a,b) (((a)<<(b))+((a)>>(32-b)))
    x[ 4] ^= R(x[ 0]+x[12], 7);  x[ 8] ^= R(x[ 4]+x[ 0], 9);
    x[12] ^= R(x[ 8]+x[ 4],13);  x[ 0] ^= R(x[12]+x[ 8],18);
    x[ 9] ^= R(x[ 5]+x[ 1], 7);  x[13] ^= R(x[ 9]+x[ 5], 9);
    x[ 1] ^= R(x[13]+x[ 9],13);  x[ 5] ^= R(x[ 1]+x[13],18);
    x[14] ^= R(x[10]+x[ 6], 7);  x[ 2] ^= R(x[14]+x[10], 9);
    x[ 6] ^= R(x[ 2]+x[14],13);  x[10] ^= R(x[ 6]+x[ 2],18);
    x[ 3] ^= R(x[15]+x[11], 7);  x[ 7] ^= R(x[ 3]+x[15], 9);
    x[11] ^= R(x[ 7]+x[ 3],13);  x[15] ^= R(x[11]+x[ 7],18);
    x[ 1] ^= R(x[ 0]+x[ 3], 7);  x[ 2] ^= R(x[ 1]+x[ 0], 9);
    x[ 3] ^= R(x[ 2]+x[ 1],13);  x[ 0] ^= R(x[ 3]+x[ 2],18);
    x[ 6] ^= R(x[ 5]+x[ 4], 7);  x[ 7] ^= R(x[ 6]+x[ 5], 9);
    x[ 4] ^= R(x[ 7]+x[ 6],13);  x[ 5] ^= R(x[ 4]+x[ 7],18);
    x[11] ^= R(x[10]+x[ 9], 7);  x[ 8] ^= R(x[11]+x[10], 9);
    x[ 9] ^= R(x[ 8]+x[11],13);  x[10] ^= R(x[ 9]+x[ 8],18);
    x[12] ^= R(x[15]+x[14], 7);  x[13] ^= R(x[12]+x[15], 9);
    x[14] ^= R(x[13]+x[12],13);  x[15] ^= R(x[14]+x[13],18);
    #undef R
  }
  for (int i=0; i<16; ++i) b[i]+=x[i];
}

// BlockMix_{Salsa20/8, r} on b[0..128*r-1]
static void blockmix(uint32_t* b, int r) {
  assert(r<=8);
  uint32_t x[16];
  uint32_t y[256];
  memcpy(x, b+32*r-16, 64);
  for (int i=0; i<2*r; ++i) {
    for (int j=0; j<16; ++j) x[j]^=b[i*16+j];
    salsa8(x);
    memcpy(&y[i*16], x, 64);
  }
  for (int i=0; i<r; ++i) memcpy(b+i*16, &y[i*32], 64);
  for (int i=0; i<r; ++i) memcpy(b+(i+r)*16, &y[i*32+16], 64);
}

// Mix b[0..128*r-1]. Uses 128*r*n bytes of memory and O(r*n) time
static void smix(char* b, int r, int n) {
  Array<uint32_t> x(32*r), v(32*r*n);
  for (int i=0; i<r*128; ++i) x[i/4]+=(b[i]&255)<<i%4*8;
  for (int i=0; i<n; ++i) {
    memcpy(&v[i*r*32], &x[0], r*128);
    blockmix(&x[0], r);
  }
  for (int i=0; i<n; ++i) {
    uint32_t j=x[(2*r-1)*16]&(n-1);
    for (int k=0; k<r*32; ++k) x[k]^=v[j*r*32+k];
    blockmix(&x[0], r);
  }
  for (int i=0; i<r*128; ++i) b[i]=x[i/4]>>(i%4*8);
}

// Strengthen password pw[0..pwlen-1] and salt[0..saltlen-1]
// to produce key unzBuf[0..buflen-1]. Uses O(n*r*p) time and 128*r*n bytes
// of memory. n must be a power of 2 and r <= 8.
void unzscrypt(const char* pw, int pwlen,
            const char* salt, int saltlen,
            int n, int r, int p, char* unzBuf, int buflen) {
  assert(r<=8);
  assert(n>0 && (n&(n-1))==0);  // power of 2?
  Array<char> b(p*r*128);
  unzpbkdf2(pw, pwlen, salt, saltlen, 1, &b[0], p*r*128);
  for (int i=0; i<p; ++i) smix(&b[i*r*128], r, n);
  unzpbkdf2(pw, pwlen, &b[0], p*r*128, 1, unzBuf, buflen);
}

// Stretch key in[0..31], assumed to be unzSHA256(password), with
// NUL terminate salt to produce new key out[0..31]
void stretchKey(char* out, const char* in, const char* salt) {
  unzscrypt(in, 32, salt, 32, 1<<14, 8, 1, out, 32);
}

//////////////////////////// unzZPAQL ///////////////////////////

// Symbolic instructions and their sizes
typedef enum {NONE,CONS,CM,ICM,MATCH,AVG,MIX2,MIX,ISSE,SSE} CompType;
const int compsize[256]={0,2,3,2,3,4,6,6,3,5};

// A unzZPAQL virtual machine COMP+HCOMP or PCOMP.
class unzZPAQL {
public:
  unzZPAQL();
  void clear();           // Free memory, erase program, reset machine state
  void inith();           // Initialize as HCOMP to run
  void initp();           // Initialize as PCOMP to run
  void run(uint32_t input);    // Execute with input
  int read(unzReader* in2);  // Read header
  void outc(int c);       // output a byte

  unzWriter* output;         // Destination for OUT instruction, or 0 to suppress
  unzSHA1* sha1;             // Points to checksum computer
  uint32_t H(int i) {return h(i);}  // get element of h

  // unzZPAQL block header
  Array<uint8_t> header;   // hsize[2] hh hm ph pm n COMP (guard) HCOMP (guard)
  int cend;           // COMP in header[7...cend-1]
  int hbegin, hend;   // HCOMP/PCOMP in header[hbegin...hend-1]

private:
  // Machine state for executing HCOMP
  Array<uint8_t> m;        // memory array M for HCOMP
  Array<uint32_t> h;       // hash array H for HCOMP
  Array<uint32_t> r;       // 256 element register array
  uint32_t a, b, c, d;     // machine registers
  int f;              // condition flag
  int pc;             // program counter

  // Support code
  void init(int hbits, int mbits);  // initialize H and M sizes
  int execute();  // execute 1 instruction, return 0 after HALT, else 1
  void div(uint32_t x) {if (x) a/=x; else a=0;}
  void mod(uint32_t x) {if (x) a%=x; else a=0;}
  void swap(uint32_t& x) {a^=x; x^=a; a^=x;}
  void swap(uint8_t& x)  {a^=x; x^=a; a^=x;}
  void err();  // exit with run time error
};

// Read header from in2
int unzZPAQL::read(unzReader* in2) {

  // Get header size and allocate
  int hsize=in2->get();
  if (hsize<0) unzerror("end of header");
  hsize+=in2->get()*256;
  if (hsize<0) unzerror("end of header");
  header.resize(hsize+300);
  cend=hbegin=hend=0;
  header[cend++]=hsize&255;
  header[cend++]=hsize>>8;
  while (cend<7) header[cend++]=in2->get(); // hh hm ph pm n

  // Read COMP
  int n=header[cend-1];
  for (int i=0; i<n; ++i) {
    int type=in2->get();  // component type
    if (type==-1) unzerror("unexpected end of file");
    header[cend++]=type;  // component type
    int size=compsize[type];
    if (size<1) unzerror("Invalid component type");
    if (cend+size>hsize) unzerror("COMP overflows header");
    for (int j=1; j<size; ++j)
      header[cend++]=in2->get();
  }
  if ((header[cend++]=in2->get())!=0) unzerror("missing COMP END");

  // Insert a guard gap and read HCOMP
  hbegin=hend=cend+128;
  if (hend>hsize+129) unzerror("missing HCOMP");
  while (hend<hsize+129) {
    assert(hend<header.isize()-8);
    int op=in2->get();
    if (op<0) unzerror("unexpected end of file");
    header[hend++]=op;
  }
  if ((header[hend++]=in2->get())!=0) unzerror("missing HCOMP END");
  assert(cend>=7 && cend<header.isize());
  assert(hbegin==cend+128 && hbegin<header.isize());
  assert(hend>hbegin && hend<header.isize());
  assert(hsize==header[0]+256*header[1]);
  assert(hsize==cend-2+hend-hbegin);
  return cend+hend-hbegin;
}

// Free memory, but preserve output, sha1 pointers
void unzZPAQL::clear() {
  cend=hbegin=hend=0;  // COMP and HCOMP locations
  a=b=c=d=f=pc=0;      // machine state
  header.resize(0);
  h.resize(0);
  m.resize(0);
  r.resize(0);
}

// Constructor
unzZPAQL::unzZPAQL() {
  output=0;
  sha1=0;
  clear();
}

// Initialize machine state as HCOMP
void unzZPAQL::inith() {
  assert(header.isize()>6);
  assert(output==0);
  assert(sha1==0);
  init(header[2], header[3]); // hh, hm
}

// Initialize machine state as PCOMP
void unzZPAQL::initp() {
  assert(header.isize()>6);
  init(header[4], header[5]); // ph, pm
}

// Initialize machine state to run a program.
void unzZPAQL::init(int hbits, int mbits) {
  assert(header.isize()>0);
  assert(cend>=7);
  assert(hbegin>=cend+128);
  assert(hend>=hbegin);
  assert(hend<header.isize()-130);
  assert(header[0]+256*header[1]==cend-2+hend-hbegin);
  h.resize(1, hbits);
  m.resize(1, mbits);
  r.resize(256);
  a=b=c=d=pc=f=0;
}

// Run program on input by interpreting header
void unzZPAQL::run(uint32_t input) {
  assert(cend>6);
  assert(hbegin>=cend+128);
  assert(hend>=hbegin);
  assert(hend<header.isize()-130);
  assert(m.size()>0);
  assert(h.size()>0);
  assert(header[0]+256*header[1]==cend+hend-hbegin-2);
  pc=hbegin;
  a=input;
  while (execute()) ;
}

void unzZPAQL::outc(int c) {
  if (output) output->put(c);
  if (sha1) sha1->put(c);
}

// Execute one instruction, return 0 after HALT else 1
int unzZPAQL::execute() {
  switch(header[pc++]) {
    case 0: err(); break; // ERROR
    case 1: ++a; break; // A++
    case 2: --a; break; // A--
    case 3: a = ~a; break; // A!
    case 4: a = 0; break; // A=0
    case 7: a = r[header[pc++]]; break; // A=R N
    case 8: swap(b); break; // B<>A
    case 9: ++b; break; // B++
    case 10: --b; break; // B--
    case 11: b = ~b; break; // B!
    case 12: b = 0; break; // B=0
    case 15: b = r[header[pc++]]; break; // B=R N
    case 16: swap(c); break; // C<>A
    case 17: ++c; break; // C++
    case 18: --c; break; // C--
    case 19: c = ~c; break; // C!
    case 20: c = 0; break; // C=0
    case 23: c = r[header[pc++]]; break; // C=R N
    case 24: swap(d); break; // D<>A
    case 25: ++d; break; // D++
    case 26: --d; break; // D--
    case 27: d = ~d; break; // D!
    case 28: d = 0; break; // D=0
    case 31: d = r[header[pc++]]; break; // D=R N
    case 32: swap(m(b)); break; // *B<>A
    case 33: ++m(b); break; // *B++
    case 34: --m(b); break; // *B--
    case 35: m(b) = ~m(b); break; // *B!
    case 36: m(b) = 0; break; // *B=0
    case 39: if (f) pc+=((header[pc]+128)&255)-127; else ++pc; break; // JT N
    case 40: swap(m(c)); break; // *C<>A
    case 41: ++m(c); break; // *C++
    case 42: --m(c); break; // *C--
    case 43: m(c) = ~m(c); break; // *C!
    case 44: m(c) = 0; break; // *C=0
    case 47: if (!f) pc+=((header[pc]+128)&255)-127; else ++pc; break; // JF N
    case 48: swap(h(d)); break; // *D<>A
    case 49: ++h(d); break; // *D++
    case 50: --h(d); break; // *D--
    case 51: h(d) = ~h(d); break; // *D!
    case 52: h(d) = 0; break; // *D=0
    case 55: r[header[pc++]] = a; break; // R=A N
    case 56: return 0  ; // HALT
    case 57: outc(a&255); break; // OUT
    case 59: a = (a+m(b)+512)*773; break; // HASH
    case 60: h(d) = (h(d)+a+512)*773; break; // HASHD
    case 63: pc+=((header[pc]+128)&255)-127; break; // JMP N
    case 64: break; // A=A
    case 65: a = b; break; // A=B
    case 66: a = c; break; // A=C
    case 67: a = d; break; // A=D
    case 68: a = m(b); break; // A=*B
    case 69: a = m(c); break; // A=*C
    case 70: a = h(d); break; // A=*D
    case 71: a = header[pc++]; break; // A= N
    case 72: b = a; break; // B=A
    case 73: break; // B=B
    case 74: b = c; break; // B=C
    case 75: b = d; break; // B=D
    case 76: b = m(b); break; // B=*B
    case 77: b = m(c); break; // B=*C
    case 78: b = h(d); break; // B=*D
    case 79: b = header[pc++]; break; // B= N
    case 80: c = a; break; // C=A
    case 81: c = b; break; // C=B
    case 82: break; // C=C
    case 83: c = d; break; // C=D
    case 84: c = m(b); break; // C=*B
    case 85: c = m(c); break; // C=*C
    case 86: c = h(d); break; // C=*D
    case 87: c = header[pc++]; break; // C= N
    case 88: d = a; break; // D=A
    case 89: d = b; break; // D=B
    case 90: d = c; break; // D=C
    case 91: break; // D=D
    case 92: d = m(b); break; // D=*B
    case 93: d = m(c); break; // D=*C
    case 94: d = h(d); break; // D=*D
    case 95: d = header[pc++]; break; // D= N
    case 96: m(b) = a; break; // *B=A
    case 97: m(b) = b; break; // *B=B
    case 98: m(b) = c; break; // *B=C
    case 99: m(b) = d; break; // *B=D
    case 100: break; // *B=*B
    case 101: m(b) = m(c); break; // *B=*C
    case 102: m(b) = h(d); break; // *B=*D
    case 103: m(b) = header[pc++]; break; // *B= N
    case 104: m(c) = a; break; // *C=A
    case 105: m(c) = b; break; // *C=B
    case 106: m(c) = c; break; // *C=C
    case 107: m(c) = d; break; // *C=D
    case 108: m(c) = m(b); break; // *C=*B
    case 109: break; // *C=*C
    case 110: m(c) = h(d); break; // *C=*D
    case 111: m(c) = header[pc++]; break; // *C= N
    case 112: h(d) = a; break; // *D=A
    case 113: h(d) = b; break; // *D=B
    case 114: h(d) = c; break; // *D=C
    case 115: h(d) = d; break; // *D=D
    case 116: h(d) = m(b); break; // *D=*B
    case 117: h(d) = m(c); break; // *D=*C
    case 118: break; // *D=*D
    case 119: h(d) = header[pc++]; break; // *D= N
    case 128: a += a; break; // A+=A
    case 129: a += b; break; // A+=B
    case 130: a += c; break; // A+=C
    case 131: a += d; break; // A+=D
    case 132: a += m(b); break; // A+=*B
    case 133: a += m(c); break; // A+=*C
    case 134: a += h(d); break; // A+=*D
    case 135: a += header[pc++]; break; // A+= N
    case 136: a -= a; break; // A-=A
    case 137: a -= b; break; // A-=B
    case 138: a -= c; break; // A-=C
    case 139: a -= d; break; // A-=D
    case 140: a -= m(b); break; // A-=*B
    case 141: a -= m(c); break; // A-=*C
    case 142: a -= h(d); break; // A-=*D
    case 143: a -= header[pc++]; break; // A-= N
    case 144: a *= a; break; // A*=A
    case 145: a *= b; break; // A*=B
    case 146: a *= c; break; // A*=C
    case 147: a *= d; break; // A*=D
    case 148: a *= m(b); break; // A*=*B
    case 149: a *= m(c); break; // A*=*C
    case 150: a *= h(d); break; // A*=*D
    case 151: a *= header[pc++]; break; // A*= N
    case 152: div(a); break; // A/=A
    case 153: div(b); break; // A/=B
    case 154: div(c); break; // A/=C
    case 155: div(d); break; // A/=D
    case 156: div(m(b)); break; // A/=*B
    case 157: div(m(c)); break; // A/=*C
    case 158: div(h(d)); break; // A/=*D
    case 159: div(header[pc++]); break; // A/= N
    case 160: mod(a); break; // A%=A
    case 161: mod(b); break; // A%=B
    case 162: mod(c); break; // A%=C
    case 163: mod(d); break; // A%=D
    case 164: mod(m(b)); break; // A%=*B
    case 165: mod(m(c)); break; // A%=*C
    case 166: mod(h(d)); break; // A%=*D
    case 167: mod(header[pc++]); break; // A%= N
    case 168: a &= a; break; // A&=A
    case 169: a &= b; break; // A&=B
    case 170: a &= c; break; // A&=C
    case 171: a &= d; break; // A&=D
    case 172: a &= m(b); break; // A&=*B
    case 173: a &= m(c); break; // A&=*C
    case 174: a &= h(d); break; // A&=*D
    case 175: a &= header[pc++]; break; // A&= N
    case 176: a &= ~ a; break; // A&~A
    case 177: a &= ~ b; break; // A&~B
    case 178: a &= ~ c; break; // A&~C
    case 179: a &= ~ d; break; // A&~D
    case 180: a &= ~ m(b); break; // A&~*B
    case 181: a &= ~ m(c); break; // A&~*C
    case 182: a &= ~ h(d); break; // A&~*D
    case 183: a &= ~ header[pc++]; break; // A&~ N
    case 184: a |= a; break; // A|=A
    case 185: a |= b; break; // A|=B
    case 186: a |= c; break; // A|=C
    case 187: a |= d; break; // A|=D
    case 188: a |= m(b); break; // A|=*B
    case 189: a |= m(c); break; // A|=*C
    case 190: a |= h(d); break; // A|=*D
    case 191: a |= header[pc++]; break; // A|= N
    case 192: a ^= a; break; // A^=A
    case 193: a ^= b; break; // A^=B
    case 194: a ^= c; break; // A^=C
    case 195: a ^= d; break; // A^=D
    case 196: a ^= m(b); break; // A^=*B
    case 197: a ^= m(c); break; // A^=*C
    case 198: a ^= h(d); break; // A^=*D
    case 199: a ^= header[pc++]; break; // A^= N
    case 200: a <<= (a&31); break; // A<<=A
    case 201: a <<= (b&31); break; // A<<=B
    case 202: a <<= (c&31); break; // A<<=C
    case 203: a <<= (d&31); break; // A<<=D
    case 204: a <<= (m(b)&31); break; // A<<=*B
    case 205: a <<= (m(c)&31); break; // A<<=*C
    case 206: a <<= (h(d)&31); break; // A<<=*D
    case 207: a <<= (header[pc++]&31); break; // A<<= N
    case 208: a >>= (a&31); break; // A>>=A
    case 209: a >>= (b&31); break; // A>>=B
    case 210: a >>= (c&31); break; // A>>=C
    case 211: a >>= (d&31); break; // A>>=D
    case 212: a >>= (m(b)&31); break; // A>>=*B
    case 213: a >>= (m(c)&31); break; // A>>=*C
    case 214: a >>= (h(d)&31); break; // A>>=*D
    case 215: a >>= (header[pc++]&31); break; // A>>= N
    case 216: f = (true); break; // A==A f = (a == a)
    case 217: f = (a == b); break; // A==B
    case 218: f = (a == c); break; // A==C
    case 219: f = (a == d); break; // A==D
    case 220: f = (a == uint32_t(m(b))); break; // A==*B
    case 221: f = (a == uint32_t(m(c))); break; // A==*C
    case 222: f = (a == h(d)); break; // A==*D
    case 223: f = (a == uint32_t(header[pc++])); break; // A== N
    case 224: f = (false); break; // A<A f = (a < a)
    case 225: f = (a < b); break; // A<B
    case 226: f = (a < c); break; // A<C
    case 227: f = (a < d); break; // A<D
    case 228: f = (a < uint32_t(m(b))); break; // A<*B
    case 229: f = (a < uint32_t(m(c))); break; // A<*C
    case 230: f = (a < h(d)); break; // A<*D
    case 231: f = (a < uint32_t(header[pc++])); break; // A< N
    case 232: f = (false); break; // A>A f= (a > a)
    case 233: f = (a > b); break; // A>B
    case 234: f = (a > c); break; // A>C
    case 235: f = (a > d); break; // A>D
    case 236: f = (a > uint32_t(m(b))); break; // A>*B
    case 237: f = (a > uint32_t(m(c))); break; // A>*C
    case 238: f = (a > h(d)); break; // A>*D
    case 239: f = (a > uint32_t(header[pc++])); break; // A> N
    case 255: if((pc=hbegin+header[pc]+256*header[pc+1])>=hend)err();break;//LJ
    default: err();
  }
  return 1;
}

// Print illegal instruction error message and exit
void unzZPAQL::err() {
  unzerror("unzZPAQL execution error");
}

///////////////////////// Component //////////////////////////

// A Component is a context model, indirect context model, match model,
// fixed weight mixer, adaptive 2 input mixer without or with current
// partial byte as context, adaptive m input mixer (without or with),
// or SSE (without or with).

struct unzComponent {
  uint32_t limit;      // max count for cm
  uint32_t cxt;        // saved context
  uint32_t a, b, c;    // multi-purpose variables
  Array<uint32_t> cm;  // cm[cxt] -> p in bits 31..10, n in 9..0; MATCH index
  Array<uint8_t> ht;   // ICM/ISSE hash table[0..size1][0..15] and MATCH unzBuf
  Array<uint16_t> a16; // MIX weights
  void init();    // initialize to all 0
  unzComponent() {init();}
};

void unzComponent::init() {
  limit=cxt=a=b=c=0;
  cm.resize(0);
  ht.resize(0);
  a16.resize(0);
}

////////////////////////// StateTable ////////////////////////

// Next state table generator
class StateTable {
  enum {N=64}; // sizes of b, t
  int num_states(int n0, int n1);  // compute t[n0][n1][1]
  void discount(int& n0);  // set new value of n0 after 1 or n1 after 0
  void next_state(int& n0, int& n1, int y);  // new (n0,n1) after bit y
public:
  uint8_t ns[1024]; // state*4 -> next state if 0, if 1, n0, n1
  int next(int state, int y) {  // next state for bit y
    assert(state>=0 && state<256);
    assert(y>=0 && y<4);
    return ns[state*4+y];
  }
  int cminit(int state) {  // initial probability of 1 * 2^23
    assert(state>=0 && state<256);
    return ((ns[state*4+3]*2+1)<<22)/(ns[state*4+2]+ns[state*4+3]+1);
  }
  StateTable();
};

// How many states with count of n0 zeros, n1 ones (0...2)
int StateTable::num_states(int n0, int n1) {
  const int B=6;
  const int bound[B]={20,48,15,8,6,5}; // n0 -> max n1, n1 -> max n0
  if (n0<n1) return num_states(n1, n0);
  if (n0<0 || n1<0 || n1>=B || n0>bound[n1]) return 0;
  return 1+(n1>0 && n0+n1<=17);
}

// New value of count n0 if 1 is observed (and vice versa)
void StateTable::discount(int& n0) {
  n0=(n0>=1)+(n0>=2)+(n0>=3)+(n0>=4)+(n0>=5)+(n0>=7)+(n0>=8);
}

// compute next n0,n1 (0 to N) given input y (0 or 1)
void StateTable::next_state(int& n0, int& n1, int y) {
  if (n0<n1)
    next_state(n1, n0, 1-y);
  else {
    if (y) {
      ++n1;
      discount(n0);
    }
    else {
      ++n0;
      discount(n1);
    }
    // 20,0,0 -> 20,0
    // 48,1,0 -> 48,1
    // 15,2,0 -> 8,1
    //  8,3,0 -> 6,2
    //  8,3,1 -> 5,3
    //  6,4,0 -> 5,3
    //  5,5,0 -> 5,4
    //  5,5,1 -> 4,5
    while (!num_states(n0, n1)) {
      if (n1<2) --n0;
      else {
        n0=(n0*(n1-1)+(n1/2))/n1;
        --n1;
      }
    }
  }
}

// Initialize next state table ns[state*4] -> next if 0, next if 1, n0, n1
StateTable::StateTable() {

  // Assign states by increasing priority
  const int N=50;
  uint8_t t[N][N][2]={{{0}}}; // (n0,n1,y) -> state number
  int state=0;
  for (int i=0; i<N; ++i) {
    for (int n1=0; n1<=i; ++n1) {
      int n0=i-n1;
      int n=num_states(n0, n1);
      assert(n>=0 && n<=2);
      if (n) {
        t[n0][n1][0]=state;
        t[n0][n1][1]=state+n-1;
        state+=n;
      }
    }
  }
       
  // Generate next state table
  memset(ns, 0, sizeof(ns));
  for (int n0=0; n0<N; ++n0) {
    for (int n1=0; n1<N; ++n1) {
      for (int y=0; y<num_states(n0, n1); ++y) {
        int s=t[n0][n1][y];
        assert(s>=0 && s<256);
        int s0=n0, s1=n1;
        next_state(s0, s1, 0);
        assert(s0>=0 && s0<N && s1>=0 && s1<N);
        ns[s*4+0]=t[s0][s1][0];
        s0=n0, s1=n1;
        next_state(s0, s1, 1);
        assert(s0>=0 && s0<N && s1>=0 && s1<N);
        ns[s*4+1]=t[s0][s1][1];
        ns[s*4+2]=n0;
        ns[s*4+3]=n1;
      }
    }
  }
}

///////////////////////// Predictor //////////////////////////

// A predictor guesses the next bit
class unzPredictor {
public:
  unzPredictor(unzZPAQL&);
  void init();          // build model
  int predict();        // probability that next bit is a 1 (0..32767)
  void update(int y);   // train on bit y (0..1)
  bool isModeled() {    // n>0 components?
    assert(z.header.isize()>6);
    return z.header[6]!=0;
  }

private:

  // unzPredictor state
  int c8;               // last 0...7 bits.
  int hmap4;            // c8 split into nibbles
  int p[256];           // predictions
  uint32_t h[256];           // unrolled copy of z.h
  unzZPAQL& z;             // VM to compute context hashes, includes H, n
  unzComponent comp[256];  // the model, includes P

  // Modeling support functions
  int dt2k[256];        // division table for match: dt2k[i] = 2^12/i
  int dt[1024];         // division table for cm: dt[i] = 2^16/(i+1.5)
  uint16_t squasht[4096];    // squash() lookup table
  short stretcht[32768];// stretch() lookup table
  StateTable st;        // next, cminit functions

  // reduce prediction error in cr.cm
  void train(unzComponent& cr, int y) {
    assert(y==0 || y==1);
    uint32_t& pn=cr.cm(cr.cxt);
    uint32_t count=pn&0x3ff;
    int error=y*32767-(cr.cm(cr.cxt)>>17);
    pn+=(error*dt[count]&-1024)+(count<cr.limit);
  }

  // x -> floor(32768/(1+exp(-x/64)))
  int squash(int x) {
    assert(x>=-2048 && x<=2047);
    return squasht[x+2048];
  }

  // x -> round(64*log((x+0.5)/(32767.5-x))), approx inverse of squash
  int stretch(int x) {
    assert(x>=0 && x<=32767);
    return stretcht[x];
  }

  // bound x to a 12 bit signed int
  int clamp2k(int x) {
    if (x<-2048) return -2048;
    else if (x>2047) return 2047;
    else return x;
  }

  // bound x to a 20 bit signed int
  int clamp512k(int x) {
    if (x<-(1<<19)) return -(1<<19);
    else if (x>=(1<<19)) return (1<<19)-1;
    else return x;
  }

  // Get cxt in ht, creating a new row if needed
  size_t find(Array<uint8_t>& ht, int sizebits, uint32_t cxt);
};

// Initailize model-independent tables
unzPredictor::unzPredictor(unzZPAQL& zr):
    c8(1), hmap4(1), z(zr) {
  assert(sizeof(uint8_t)==1);
  assert(sizeof(uint16_t)==2);
  assert(sizeof(uint32_t)==4);
  assert(sizeof(uint64_t)==8);
  assert(sizeof(short)==2);
  assert(sizeof(int)==4);

  // Initialize tables
  dt2k[0]=0;
  for (int i=1; i<256; ++i)
    dt2k[i]=2048/i;
  for (int i=0; i<1024; ++i)
    dt[i]=(1<<17)/(i*2+3)*2;
  for (int i=0; i<32768; ++i)
    stretcht[i]=int(log((i+0.5)/(32767.5-i))*64+0.5+100000)-100000;
  for (int i=0; i<4096; ++i)
    squasht[i]=int(32768.0/(1+exp((i-2048)*(-1.0/64))));

  // Verify floating point math for squash() and stretch()
  uint32_t sqsum=0, stsum=0;
  for (int i=32767; i>=0; --i)
    stsum=stsum*3+stretch(i);
  for (int i=4095; i>=0; --i)
    sqsum=sqsum*3+squash(i-2048);
  assert(stsum==3887533746u);
  assert(sqsum==2278286169u);
}

// Initialize the predictor with a new model in z
void unzPredictor::init() {

  // Initialize context hash function
  z.inith();

  // Initialize predictions
  for (int i=0; i<256; ++i) h[i]=p[i]=0;

  // Initialize components
  for (int i=0; i<256; ++i)  // clear old model
    comp[i].init();
  int n=z.header[6]; // hsize[0..1] hh hm ph pm n (comp)[n] 0 0[128] (hcomp) 0
  const uint8_t* cp=&z.header[7];  // start of component list
  for (int i=0; i<n; ++i) {
    assert(cp<&z.header[z.cend]);
    assert(cp>&z.header[0] && cp<&z.header[z.header.isize()-8]);
    unzComponent& cr=comp[i];
    switch(cp[0]) {
      case CONS:  // c
        p[i]=(cp[1]-128)*4;
        break;
      case CM: // sizebits limit
        if (cp[1]>32) unzerror("max size for CM is 32");
        cr.cm.resize(1, cp[1]);  // packed CM (22 bits) + CMCOUNT (10 bits)
        cr.limit=cp[2]*4;
        for (size_t j=0; j<cr.cm.size(); ++j)
          cr.cm[j]=0x80000000;
        break;
      case ICM: // sizebits
        if (cp[1]>26) unzerror("max size for ICM is 26");
        cr.limit=1023;
        cr.cm.resize(256);
        cr.ht.resize(64, cp[1]);
        for (size_t j=0; j<cr.cm.size(); ++j)
          cr.cm[j]=st.cminit(j);
        break;
      case MATCH:  // sizebits
        if (cp[1]>32 || cp[2]>32) unzerror("max size for MATCH is 32 32");
        cr.cm.resize(1, cp[1]);  // index
        cr.ht.resize(1, cp[2]);  // unzBuf
        cr.ht(0)=1;
        break;
      case AVG: // j k wt
        if (cp[1]>=i) unzerror("AVG j >= i");
        if (cp[2]>=i) unzerror("AVG k >= i");
        break;
      case MIX2:  // sizebits j k rate mask
        if (cp[1]>32) unzerror("max size for MIX2 is 32");
        if (cp[3]>=i) unzerror("MIX2 k >= i");
        if (cp[2]>=i) unzerror("MIX2 j >= i");
        cr.c=(size_t(1)<<cp[1]); // size (number of contexts)
        cr.a16.resize(1, cp[1]);  // wt[size][m]
        for (size_t j=0; j<cr.a16.size(); ++j)
          cr.a16[j]=32768;
        break;
      case MIX: {  // sizebits j m rate mask
        if (cp[1]>32) unzerror("max size for MIX is 32");
        if (cp[2]>=i) unzerror("MIX j >= i");
        if (cp[3]<1 || cp[3]>i-cp[2]) unzerror("MIX m not in 1..i-j");
        int m=cp[3];  // number of inputs
        assert(m>=1);
        cr.c=(size_t(1)<<cp[1]); // size (number of contexts)
        cr.cm.resize(m, cp[1]);  // wt[size][m]
        for (size_t j=0; j<cr.cm.size(); ++j)
          cr.cm[j]=65536/m;
        break;
      }
      case ISSE:  // sizebits j
        if (cp[1]>32) unzerror("max size for ISSE is 32");
        if (cp[2]>=i) unzerror("ISSE j >= i");
        cr.ht.resize(64, cp[1]);
        cr.cm.resize(512);
        for (int j=0; j<256; ++j) {
          cr.cm[j*2]=1<<15;
          cr.cm[j*2+1]=clamp512k(stretch(st.cminit(j)>>8)*1024);
        }
        break;
      case SSE: // sizebits j start limit
        if (cp[1]>32) unzerror("max size for SSE is 32");
        if (cp[2]>=i) unzerror("SSE j >= i");
        if (cp[3]>cp[4]*4) unzerror("SSE start > limit*4");
        cr.cm.resize(32, cp[1]);
        cr.limit=cp[4]*4;
        for (size_t j=0; j<cr.cm.size(); ++j)
          cr.cm[j]=squash((j&31)*64-992)<<17|cp[3];
        break;
      default: unzerror("unknown component type");
    }
    assert(compsize[*cp]>0);
    cp+=compsize[*cp];
    assert(cp>=&z.header[7] && cp<&z.header[z.cend]);
  }
}

// Return next bit prediction using interpreted COMP code
int unzPredictor::predict() {
  assert(c8>=1 && c8<=255);

  // Predict next bit
  int n=z.header[6];
  assert(n>0 && n<=255);
  const uint8_t* cp=&z.header[7];
  assert(cp[-1]==n);
  for (int i=0; i<n; ++i) {
    assert(cp>&z.header[0] && cp<&z.header[z.header.isize()-8]);
    unzComponent& cr=comp[i];
    switch(cp[0]) {
      case CONS:  // c
        break;
      case CM:  // sizebits limit
        cr.cxt=h[i]^hmap4;
        p[i]=stretch(cr.cm(cr.cxt)>>17);
        break;
      case ICM: // sizebits
        assert((hmap4&15)>0);
        if (c8==1 || (c8&0xf0)==16) cr.c=find(cr.ht, cp[1]+2, h[i]+16*c8);
        cr.cxt=cr.ht[cr.c+(hmap4&15)];
        p[i]=stretch(cr.cm(cr.cxt)>>8);
        break;
      case MATCH: // sizebits bufbits: a=len, b=offset, c=bit, cxt=bitpos,
                  //                   ht=unzBuf, limit=pos
        assert(cr.cm.size()==(size_t(1)<<cp[1]));
        assert(cr.ht.size()==(size_t(1)<<cp[2]));
        assert(cr.a<=255);
        assert(cr.c==0 || cr.c==1);
        assert(cr.cxt<8);
        assert(cr.limit<cr.ht.size());
        if (cr.a==0) p[i]=0;
        else {
          cr.c=(cr.ht(cr.limit-cr.b)>>(7-cr.cxt))&1; // predicted bit
          p[i]=stretch(dt2k[cr.a]*(cr.c*-2+1)&32767);
        }
        break;
      case AVG: // j k wt
        p[i]=(p[cp[1]]*cp[3]+p[cp[2]]*(256-cp[3]))>>8;
        break;
      case MIX2: { // sizebits j k rate mask
                   // c=size cm=wt[size] cxt=input
        cr.cxt=((h[i]+(c8&cp[5]))&(cr.c-1));
        assert(cr.cxt<cr.a16.size());
        int w=cr.a16[cr.cxt];
        assert(w>=0 && w<65536);
        p[i]=(w*p[cp[2]]+(65536-w)*p[cp[3]])>>16;
        assert(p[i]>=-2048 && p[i]<2048);
      }
        break;
      case MIX: {  // sizebits j m rate mask
                   // c=size cm=wt[size][m] cxt=index of wt in cm
        int m=cp[3];
        assert(m>=1 && m<=i);
        cr.cxt=h[i]+(c8&cp[5]);
        cr.cxt=(cr.cxt&(cr.c-1))*m; // pointer to row of weights
        assert(cr.cxt<=cr.cm.size()-m);
        int* wt=(int*)&cr.cm[cr.cxt];
        p[i]=0;
        for (int j=0; j<m; ++j)
          p[i]+=(wt[j]>>8)*p[cp[2]+j];
        p[i]=clamp2k(p[i]>>8);
      }
        break;
      case ISSE: { // sizebits j -- c=hi, cxt=bh
        assert((hmap4&15)>0);
        if (c8==1 || (c8&0xf0)==16)
          cr.c=find(cr.ht, cp[1]+2, h[i]+16*c8);
        cr.cxt=cr.ht[cr.c+(hmap4&15)];  // bit history
        int *wt=(int*)&cr.cm[cr.cxt*2];
        p[i]=clamp2k((wt[0]*p[cp[2]]+wt[1]*64)>>16);
      }
        break;
      case SSE: { // sizebits j start limit
        cr.cxt=(h[i]+c8)*32;
        int pq=p[cp[2]]+992;
        if (pq<0) pq=0;
        if (pq>1983) pq=1983;
        int wt=pq&63;
        pq>>=6;
        assert(pq>=0 && pq<=30);
        cr.cxt+=pq;
        p[i]=stretch(((cr.cm(cr.cxt)>>10)*(64-wt)+(cr.cm(cr.cxt+1)>>10)*wt)
             >>13);
        cr.cxt+=wt>>5;
      }
        break;
      default:
        unzerror("component predict not implemented");
    }
    cp+=compsize[cp[0]];
    assert(cp<&z.header[z.cend]);
    assert(p[i]>=-2048 && p[i]<2048);
  }
  assert(cp[0]==NONE);
  return squash(p[n-1]);
}

// Update model with decoded bit y (0...1)
void unzPredictor::update(int y) {
  assert(y==0 || y==1);
  assert(c8>=1 && c8<=255);
  assert(hmap4>=1 && hmap4<=511);

  // Update components
  const uint8_t* cp=&z.header[7];
  int n=z.header[6];
  assert(n>=1 && n<=255);
  assert(cp[-1]==n);
  for (int i=0; i<n; ++i) {
    unzComponent& cr=comp[i];
    switch(cp[0]) {
      case CONS:  // c
        break;
      case CM:  // sizebits limit
        train(cr, y);
        break;
      case ICM: { // sizebits: cxt=ht[b]=bh, ht[c][0..15]=bh row, cxt=bh
        cr.ht[cr.c+(hmap4&15)]=st.next(cr.ht[cr.c+(hmap4&15)], y);
        uint32_t& pn=cr.cm(cr.cxt);
        pn+=int(y*32767-(pn>>8))>>2;
      }
        break;
      case MATCH: // sizebits bufbits:
                  //   a=len, b=offset, c=bit, cm=index, cxt=bitpos
                  //   ht=unzBuf, limit=pos
      {
        assert(cr.a<=255);
        assert(cr.c==0 || cr.c==1);
        assert(cr.cxt<8);
        assert(cr.cm.size()==(size_t(1)<<cp[1]));
        assert(cr.ht.size()==(size_t(1)<<cp[2]));
        assert(cr.limit<cr.ht.size());
        if (int(cr.c)!=y) cr.a=0;  // mismatch?
        cr.ht(cr.limit)+=cr.ht(cr.limit)+y;
        if (++cr.cxt==8) {
          cr.cxt=0;
          ++cr.limit;
          cr.limit&=(1<<cp[2])-1;
          if (cr.a==0) {  // look for a match
            cr.b=cr.limit-cr.cm(h[i]);
            if (cr.b&(cr.ht.size()-1))
              while (cr.a<255
                     && cr.ht(cr.limit-cr.a-1)==cr.ht(cr.limit-cr.a-cr.b-1))
                ++cr.a;
          }
          else cr.a+=cr.a<255;
          cr.cm(h[i])=cr.limit;
        }
      }
        break;
      case AVG:  // j k wt
        break;
      case MIX2: { // sizebits j k rate mask
                   // cm=wt[size], cxt=input
        assert(cr.a16.size()==cr.c);
        assert(cr.cxt<cr.a16.size());
        int err=(y*32767-squash(p[i]))*cp[4]>>5;
        int w=cr.a16[cr.cxt];
        w+=(err*(p[cp[2]]-p[cp[3]])+(1<<12))>>13;
        if (w<0) w=0;
        if (w>65535) w=65535;
        cr.a16[cr.cxt]=w;
      }
        break;
      case MIX: {   // sizebits j m rate mask
                    // cm=wt[size][m], cxt=input
        int m=cp[3];
        assert(m>0 && m<=i);
        assert(cr.cm.size()==m*cr.c);
        assert(cr.cxt+m<=cr.cm.size());
        int err=(y*32767-squash(p[i]))*cp[4]>>4;
        int* wt=(int*)&cr.cm[cr.cxt];
        for (int j=0; j<m; ++j)
          wt[j]=clamp512k(wt[j]+((err*p[cp[2]+j]+(1<<12))>>13));
      }
        break;
      case ISSE: { // sizebits j  -- c=hi, cxt=bh
        assert(cr.cxt==uint32_t(cr.ht[cr.c+(hmap4&15)]));
        int err=y*32767-squash(p[i]);
        int *wt=(int*)&cr.cm[cr.cxt*2];
        wt[0]=clamp512k(wt[0]+((err*p[cp[2]]+(1<<12))>>13));
        wt[1]=clamp512k(wt[1]+((err+16)>>5));
        cr.ht[cr.c+(hmap4&15)]=st.next(cr.cxt, y);
      }
        break;
      case SSE:  // sizebits j start limit
        train(cr, y);
        break;
      default:
        assert(0);
    }
    cp+=compsize[cp[0]];
    assert(cp>=&z.header[7] && cp<&z.header[z.cend] 
           && cp<&z.header[z.header.isize()-8]);
  }
  assert(cp[0]==NONE);

  // Save bit y in c8, hmap4
  c8+=c8+y;
  if (c8>=256) {
    z.run(c8-256);
    hmap4=1;
    c8=1;
    for (int i=0; i<n; ++i) h[i]=z.H(i);
  }
  else if (c8>=16 && c8<32)
    hmap4=(hmap4&0xf)<<5|y<<4|1;
  else
    hmap4=(hmap4&0x1f0)|(((hmap4&0xf)*2+y)&0xf);
}

// Find cxt row in hash table ht. ht has rows of 16 indexed by the
// low sizebits of cxt with element 0 having the next higher 8 bits for
// collision detection. If not found after 3 adjacent tries, replace the
// row with lowest element 1 as priority. Return index of row.
size_t unzPredictor::find(Array<uint8_t>& ht, int sizebits, uint32_t cxt) {
  assert(ht.size()==size_t(16)<<sizebits);
  int chk=cxt>>sizebits&255;
  size_t h0=(cxt*16)&(ht.size()-16);
  if (ht[h0]==chk) return h0;
  size_t h1=h0^16;
  if (ht[h1]==chk) return h1;
  size_t h2=h0^32;
  if (ht[h2]==chk) return h2;
  if (ht[h0+1]<=ht[h1+1] && ht[h0+1]<=ht[h2+1])
    return memset(&ht[h0], 0, 16), ht[h0]=chk, h0;
  else if (ht[h1+1]<ht[h2+1])
    return memset(&ht[h1], 0, 16), ht[h1]=chk, h1;
  else
    return memset(&ht[h2], 0, 16), ht[h2]=chk, h2;
}

//////////////////////////// unzDecoder /////////////////////////

// unzDecoder decompresses using an arithmetic code
class unzDecoder {
public:
  unzReader* in;        // destination
  unzDecoder(unzZPAQL& z);
  int decompress();  // return a byte or EOF
  void init();       // initialize at start of block
private:
  uint32_t low, high;     // range
  uint32_t curr;          // last 4 bytes of archive
  unzPredictor pr;      // to get p
  int decode(int p); // return decoded bit (0..1) with prob. p (0..65535)
};

unzDecoder::unzDecoder(unzZPAQL& z):
    in(0), low(1), high(0xFFFFFFFF), curr(0), pr(z) {
}

void unzDecoder::init() {
  pr.init();
  if (pr.isModeled()) low=1, high=0xFFFFFFFF, curr=0;
  else low=high=curr=0;
}

// Return next bit of decoded input, which has 16 bit probability p of being 1
int unzDecoder::decode(int p) {
  assert(p>=0 && p<65536);
  assert(high>low && low>0);
  if (curr<low || curr>high) unzerror("archive corrupted");
  assert(curr>=low && curr<=high);
  uint32_t mid=low+uint32_t(((high-low)*uint64_t(uint32_t(p)))>>16);  // split range
  assert(high>mid && mid>=low);
  int y=curr<=mid;
  if (y) high=mid; else low=mid+1; // pick half
  while ((high^low)<0x1000000) { // shift out identical leading bytes
    high=high<<8|255;
    low=low<<8;
    low+=(low==0);
    int c=in->get();
    if (c<0) unzerror("unexpected end of file");
    curr=curr<<8|c;
  }
  return y;
}

// Decompress 1 byte or -1 at end of input
int unzDecoder::decompress() {
  if (pr.isModeled()) {  // n>0 components?
    if (curr==0) {  // segment initialization
      for (int i=0; i<4; ++i)
        curr=curr<<8|in->get();
    }
    if (decode(0)) {
      if (curr!=0) unzerror("decoding end of input");
      return -1;
    }
    else {
      int c=1;
      while (c<256) {  // get 8 bits
        int p=pr.predict()*2+1;
        c+=c+decode(p);
        pr.update(c&1);
      }
      return c-256;
    }
  }
  else {
    if (curr==0) {  // segment initialization
      for (int i=0; i<4; ++i)
        curr=curr<<8|in->get();
      if (curr==0) return -1;
    }
    assert(curr>0);
    --curr;
    return in->get();
  }
}

/////////////////////////// unzPostProcessor ////////////////////

class unzPostProcessor {
  int state;   // input parse state: 0=INIT, 1=PASS, 2..4=loading, 5=POST
  int hsize;   // header size
  int ph, pm;  // sizes of H and M in z
public:
  unzZPAQL z;     // holds PCOMP
  unzPostProcessor(): state(0), hsize(0), ph(0), pm(0) {}
  void init(int h, int m);  // ph, pm sizes of H and M
  int write(int c);  // Input a byte, return state
  void setOutput(unzWriter* out) {z.output=out;}
  void setSHA1(unzSHA1* sha1ptr) {z.sha1=sha1ptr;}
  int getState() const {return state;}
};

// Copy ph, pm from block header
void unzPostProcessor::init(int h, int m) {
  state=hsize=0;
  ph=h;
  pm=m;
  z.clear();
}

// (PASS=0 | PROG=1 psize[0..1] pcomp[0..psize-1]) data... EOB=-1
// Return state: 1=PASS, 2..4=loading PROG, 5=PROG loaded
int unzPostProcessor::write(int c) {
  assert(c>=-1 && c<=255);
  switch (state) {
    case 0:  // initial state
      if (c<0) unzerror("Unexpected EOS");
      state=c+1;  // 1=PASS, 2=PROG
      if (state>2) unzerror("unknown post processing type");
      if (state==1) z.clear();
      break;
    case 1:  // PASS
      if (c>=0) z.outc(c);
      break;
    case 2: // PROG
      if (c<0) unzerror("Unexpected EOS");
      hsize=c;  // low byte of size
      state=3;
      break;
    case 3:  // PROG psize[0]
      if (c<0) unzerror("Unexpected EOS");
      hsize+=c*256;  // high byte of psize
      if (hsize<1) unzerror("Empty PCOMP");
      z.header.resize(hsize+300);
      z.cend=8;
      z.hbegin=z.hend=z.cend+128;
      z.header[4]=ph;
      z.header[5]=pm;
      state=4;
      break;
    case 4:  // PROG psize[0..1] pcomp[0...]
      if (c<0) unzerror("Unexpected EOS");
      assert(z.hend<z.header.isize());
      z.header[z.hend++]=c;  // one byte of pcomp
      if (z.hend-z.hbegin==hsize) {  // last byte of pcomp?
        hsize=z.cend-2+z.hend-z.hbegin;
        z.header[0]=hsize&255;  // header size with empty COMP
        z.header[1]=hsize>>8;
        z.initp();
        state=5;
      }
      break;
    case 5:  // PROG ... data
      z.run(c);
      break;
  }
  return state;
}

//////////////////////// unzDecompresser ////////////////////////

// For decompression and listing archive contents
class unzDecompresser {
public:
  unzDecompresser(): z(), dec(z), pp(), state(BLOCK), decode_state(FIRSTSEG) {}
  void setInput(unzReader* in) {dec.in=in;}
  bool findBlock();
  bool findFilename(unzWriter* = 0);
  void readComment(unzWriter* = 0);
  void setOutput(unzWriter* out) {pp.setOutput(out);}
  void setSHA1(unzSHA1* sha1ptr) {pp.setSHA1(sha1ptr);}
  void decompress();  // decompress segment
  void readSegmentEnd(char* sha1string = 0);
private:
  unzZPAQL z;
  unzDecoder dec;
  unzPostProcessor pp;
  enum {BLOCK, FILENAME, COMMENT, DATA, SEGEND} state;  // expected next
  enum {FIRSTSEG, SEG} decode_state;  // which segment in block?
};

// Find the start of a block and return true if found. Set memptr
// to memory used.
bool unzDecompresser::findBlock() {
  assert(state==BLOCK);

  // Find start of block
  uint32_t h1=0x3D49B113, h2=0x29EB7F93, h3=0x2614BE13, h4=0x3828EB13;
  // Rolling hashes initialized to hash of first 13 bytes
  int c;
  while ((c=dec.in->get())!=-1) {
    h1=h1*12+c;
    h2=h2*20+c;
    h3=h3*28+c;
    h4=h4*44+c;
    if (h1==0xB16B88F1 && h2==0xFF5376F1 && h3==0x72AC5BF1 && h4==0x2F909AF1)
      break;  // hash of 16 byte string
  }
  if (c==-1) return false;

  // Read header
  if ((c=dec.in->get())!=1 && c!=2) unzerror("unsupported ZPAQ level");
  if (dec.in->get()!=1) unzerror("unsupported unzZPAQL type");
  z.read(dec.in);
  if (c==1 && z.header.isize()>6 && z.header[6]==0)
    unzerror("ZPAQ level 1 requires at least 1 component");
  state=FILENAME;
  decode_state=FIRSTSEG;
  return true;
}

// Read the start of a segment (1) or end of block code (255).
// If a segment is found, write the filename and return true, else false.
bool unzDecompresser::findFilename(unzWriter* filename) {
  assert(state==FILENAME);
  int c=dec.in->get();
  if (c==1) {  // segment found
    while (true) {
      c=dec.in->get();
      if (c==-1) unzerror("unexpected EOF");
      if (c==0) {
        state=COMMENT;
        return true;
      }
      if (filename) filename->put(c);
    }
  }
  else if (c==255) {  // end of block found
    state=BLOCK;
    return false;
  }
  else
    unzerror("missing segment or end of block");
  return false;
}

// Read the comment from the segment header
void unzDecompresser::readComment(unzWriter* comment) {
  assert(state==COMMENT);
  state=DATA;
  while (true) {
    int c=dec.in->get();
    if (c==-1) unzerror("unexpected EOF");
    if (c==0) break;
    if (comment) comment->put(c);
  }
  if (dec.in->get()!=0) unzerror("missing reserved byte");
}

// Decompress n bytes, or all if n < 0. Return false if done
void unzDecompresser::decompress() {
  assert(state==DATA);

  // Initialize models to start decompressing block
  if (decode_state==FIRSTSEG) {
    dec.init();
    assert(z.header.size()>5);
    pp.init(z.header[4], z.header[5]);
    decode_state=SEG;
  }

  // Decompress and load PCOMP into postprocessor
  while ((pp.getState()&3)!=1)
    pp.write(dec.decompress());

  // Decompress n bytes, or all if n < 0
  while (true) {
    int c=dec.decompress();
    pp.write(c);
    if (c==-1) {
      state=SEGEND;
      return;
    }
  }
}

// Read end of block. If a unzSHA1 checksum is present, write 1 and the
// 20 byte checksum into sha1string, else write 0 in first byte.
// If sha1string is 0 then discard it.
void unzDecompresser::readSegmentEnd(char* sha1string) {
  assert(state==SEGEND);

  // Read checksum
  int c=dec.in->get();
  if (c==254) {
    if (sha1string) sha1string[0]=0;  // no checksum
  }
  else if (c==253) {
    if (sha1string) sha1string[0]=1;
    for (int i=1; i<=20; ++i) {
      c=dec.in->get();
      if (sha1string) sha1string[i]=c;
    }
  }
  else
    unzerror("missing end of segment marker");
  state=FILENAME;
}

///////////////////////// Driver program ////////////////////

uint64_t offset=0;  // number of bytes input prior to current block

// Handle errors
void unzerror(const char* msg) {
  printf("\nError at offset %1.0f: %s\n", double(offset), msg);
  exit(1);
}

// Input archive
class unzInputFile: public unzReader {
  FILE* f;  // input file
  enum {BUFSIZE=4096};
  uint64_t offset;  // number of bytes read
  unsigned p, end;  // start and end of unread bytes in unzBuf
  unzAES_CTR* aes;  // to decrypt
  char unzBuf[BUFSIZE];  // input buffer
  int64_t filesize;
public:
  unzInputFile(): f(0), offset(0), p(0), end(0), aes(0),filesize(-1) {}

  void open(const char* filename, const char* key);

  // Return one input byte or -1 for EOF
  int get() {
    if (f && p>=end) {
      p=0;
      end=fread(unzBuf, 1, BUFSIZE, f);
      if (aes) aes->encrypt(unzBuf, end, offset);
    }
    if (p>=end) return -1;
    ++offset;
    return unzBuf[p++]&255;
  }

  // Return number of bytes read
  uint64_t tell() {return offset;}
  int64_t getfilesize() {return filesize;}
};

	
// Open input. Decrypt with key.
void unzInputFile::open(const char* filename, const char* key) {
  f=fopen(filename, "rb");
  if (!f) {
    perror(filename);
    return ;
  }
	fseeko(f, 0, SEEK_END);
	filesize=ftello(f);
	fseeko(f, 0, SEEK_SET);
	
  if (key) {
    char salt[32], stretched_key[32];
    unzSHA256 sha256;
    for (int i=0; i<32; ++i) salt[i]=get();
    if (offset!=32) unzerror("no salt");
    while (*key) sha256.put(*key++);
    stretchKey(stretched_key, sha256.result(), salt);
    aes=new unzAES_CTR(stretched_key, 32, salt);
    if (!aes) unzerror("out of memory");
    aes->encrypt(unzBuf, end, 0);
  }
}

// File to extract
class unzOutputFile: public unzWriter {
  FILE* f;  // output file or NULL
  unsigned p;  // number of pending bytes to write
  enum {BUFSIZE=4096};
  char unzBuf[BUFSIZE];  // output buffer
public:
  unzOutputFile(): f(0), p(0) {}
  void open(const char* filename);
  void close();

  // write 1 byte
  void put(int c) {
    if (f) {
      unzBuf[p++]=c;
      if (p==BUFSIZE) fwrite(unzBuf, 1, p, f), p=0;
    }
  }

  virtual ~unzOutputFile() {close();}
};

// Open file unless it exists. Print error message if unsuccessful.
void unzOutputFile::open(const char* filename) {
  close();
  f=fopen(filename, "rb");
  if (f) {
    fclose(f);
    f=0;
    fprintf(stderr, "file exists: %s\n", filename);
  }
  f=fopen(filename, "wb");
  if (!f) perror(filename);
}

// Flush output and close file
void unzOutputFile::close() {
  if (f && p>0) fwrite(unzBuf, 1, p, f);
  if (f) fclose(f), f=0;
  p=0;
}


// Write to string
struct unzBuf: public unzWriter {
  size_t limit;  // maximum size
  std::string s;  // saved output
  unzBuf(size_t l): limit(l) {}

  // Save c in s
  void put(int c) {
    if (s.size()>=limit) unzerror("output overflow");
    s+=char(c);
  }
};

// Test if 14 digit date is valid YYYYMMDDHHMMSS format
void verify_date(uint64_t date) {
  int year=date/1000000/10000;
  int month=date/100000000%100;
  int day=date/1000000%100;
  int hour=date/10000%100;
  int min=date/100%100;
  int sec=date%100;
  if (year<1900 || year>2999 || month<1 || month>12 || day<1 || day>31
      || hour<0 || hour>59 || min<0 || min>59 || sec<0 || sec>59)
    unzerror("invalid date");
}

// Test if string is valid UTF8
void unzverify_utf8(const char* s) {
  while (true) {
    int c=uint8_t(*s);
    if (c==0) return;
    if ((c>=128 && c<194) || c>=245) unzerror("invalid UTF-8 first byte");
    int len=1+(c>=192)+(c>=224)+(c>=240);
    for (int i=1; i<len; ++i)
      if ((s[i]&192)!=128) unzerror("invalid UTF-8 extra byte");
    if (c==224 && uint8_t(s[1])<160) unzerror("UTF-8 3 byte long code");
    if (c==240 && uint8_t(s[1])<144) unzerror("UTF-8 4 byte long code");
    s+=len;
  }
}

// read 8 byte LSB number
uint64_t unzget8(const char* p) {
  uint64_t r=0;
  for (int i=0; i<8; ++i)
    r+=(p[i]&255ull)<<(i*8);
  return r;
}

// read 4 byte LSB number
uint32_t unzget4(const char* p) {
  uint32_t r=0;
  for (int i=0; i<4; ++i)
    r+=(p[i]&255u)<<(i*8);
  return r;
}

// file metadata
struct unzDT {
  uint64_t date;  // YYYYMMDDHHMMSS or 0 if deleted
  uint64_t attr;  // first 8 bytes, LSB first
  std::vector<uint32_t> ptr;  // fragment IDs
  char sha1hex[66];		 // 1+32+32 (unzSHA256)+ zero
  char sha1decompressedhex[66];		 // 1+32+32 (unzSHA256)+ zero
  std::string sha1fromfile;		 // 1+32+32 (unzSHA256)+ zero

  unzDT(): date(0), attr(0) {sha1hex[0]=0x0;sha1decompressedhex[0]=0x0;sha1fromfile="";}
};

typedef std::map<std::string, unzDT> unzDTMap;

bool unzcomparesha1hex(unzDTMap::iterator i_primo, unzDTMap::iterator i_secondo) 
{
	return (strcmp(i_primo->second.sha1hex,i_secondo->second.sha1hex)<0);
}

bool unzcompareprimo(unzDTMap::iterator i_primo, unzDTMap::iterator i_secondo) 
{
	return (i_primo->first<i_secondo->first);
}
/*
template <typename T>
std::string unznumbertostring( T Number )
{
     std::ostringstream ss;
     ss << Number;
     return ss.str();
}
*/

void myavanzamento(int64_t i_lavorati,int64_t i_totali,int64_t i_inizio)
{
	static int ultimapercentuale=0;
	int percentuale=int(i_lavorati*100.0/(i_totali+0.5));
	if (((percentuale%5)==0) && (percentuale>0))
	if (percentuale!=ultimapercentuale)
	{
		ultimapercentuale=percentuale;
		double eta=0.001*(mtime()-i_inizio)*(i_totali-i_lavorati)/(i_lavorati+1.0);
		int secondi=(mtime()-i_inizio)/1000;
		if (secondi==0)
			secondi=1;
		if (eta<356000)
		printf("%03d%% %02d:%02d:%02d (%9s) of (%9s) %20s/sec\n", percentuale,
		int(eta/3600), int(eta/60)%60, int(eta)%60, tohuman(i_lavorati), tohuman2(i_totali),migliaia3(i_lavorati/secondi));
		fflush(stdout);
	}
}


	
	void list_print_datetime(void)
{
	int hours, minutes, seconds, day, month, year;

	time_t now;
	time(&now);
	struct tm *local = localtime(&now);

	hours = local->tm_hour;			// get hours since midnight	(0-23)
	minutes = local->tm_min;		// get minutes passed after the hour (0-59)
	seconds = local->tm_sec;		// get seconds passed after the minute (0-59)

	day = local->tm_mday;			// get day of month (1 to 31)
	month = local->tm_mon + 1;		// get month of year (0 to 11)
	year = local->tm_year + 1900;	// get year since 1900

	printf("%02d/%02d/%d %02d:%02d:%02d ", day, month, year, hours,minutes,seconds);

}

	/// a patched... main()!
int unz(const char * archive,const char * key,bool all)
{

	
	/// really cannot run on ESXi: take WAY too much RAM
	if (!archive)
		return 0;
	
  uint64_t until=0;
  bool index=false;
 	printf("PARANOID TEST: working on %s\n",archive);
	
  // Journaling archive state
	std::map<uint32_t, unsigned> bsize;  // frag ID -> d block compressed size
	std::map<std::string, unzDT> dt;   // filename -> date, attr, frags
	std::vector<std::string> frag;  // ID -> hash[20] size[4] data
	std::string last_filename;      // streaming destination
	uint64_t ramsize=0;
	uint64_t csize=0;                    // expected offset of next non d block
	bool streaming=false, journaling=false;  // archive type
	int64_t inizio=mtime();
	uint64_t lavorati=0;
  // Decompress blocks
	unzInputFile in;  // Archive
	in.open(archive, key);
	int64_t total_size=in.getfilesize();
	
	
	
	
	unzDecompresser d;
	d.setInput(&in);
	////unzOutputFile out;   // streaming output
	bool done=false;  // stop reading?
	bool firstSegment=true;
  
	while (!done && d.findBlock()) 
  {
	unzBuf filename(65535);
    while (!done && d.findFilename(&filename)) 
	{
		unzBuf comment(65535);
		d.readComment(&comment);
		
		unzverify_utf8(filename.s.c_str());
		
		// Test for journaling or streaming block. They cannot be mixed.
		uint64_t jsize=0;  // journaling block size in comment
		if (comment.s.size()>=4 && comment.s.substr(comment.s.size()-4)=="jDC\x01") 
		{

        // read jsize = uncompressed size from comment as a decimal string
			unsigned i;
			for (i=0; i<comment.s.size()-5 && isdigit(comment.s[i]); ++i) 
			{
				jsize=jsize*10+comment.s[i]-'0';
				if (jsize>>32) unzerror("size in comment > 4294967295");
			}
			if (i<1) 
				unzerror("missing size in comment");
			if (streaming) 
				unzerror("journaling block after streaming block");
			journaling=true;
		}
		else 
		{
			if (journaling) 
			unzerror("streaming block after journaling block");
			if (index) 
				unzerror("streaming block in index");
			streaming=true;
			///d.setOutput(&out);
			///d.setOutput();
		}
		/*
		if (streaming)
		printf("Streaming\n");
		if (journaling)
		printf("journaling\n");
		*/
      // Test journaling filename. The format must be
      // jDC[YYYYMMDDHHMMSS][t][NNNNNNNNNN]
      // where YYYYMMDDHHMMSS is the date, t is the type {c,d,h,i}, and
      // NNNNNNNNNN is the 10 digit first fragment ID for types c,d,h.
      // They must be in ascending lexicographical order.

		uint64_t date=0, id=0;  // date and frag ID from filename
		char type=0;  // c,d,h,i
		if (journaling)  // di solito
		{
			if (filename.s.size()!=28) 
				unzerror("filename size not 28");
			if (filename.s.substr(0, 3)!="jDC") 
				unzerror("filename not jDC");
			type=filename.s[17];
			if (!strchr("cdhi", type)) 
				unzerror("type not c,d,h,i");
			
			if (filename.s<=last_filename) 
			unzerror("filenames out of order");
        
			last_filename=filename.s;

        // Read date
			for (int i=3; i<17; ++i) 
			{
				if (!isdigit(filename.s[i])) 
					unzerror("non-digit in date");
				date=date*10+filename.s[i]-'0';
			}
			verify_date(date);

        // Read frag ID
			for (int i=18; i<28; ++i) 
			{
				if (!isdigit(filename.s[i])) 
					unzerror("non-digit in fragment ID");
				id=id*10+filename.s[i]-'0';
			}
			if (id<1 || id>4294967295llu) 
				unzerror("fragment ID out of range");
		}

      // Select streaming output file
		if (streaming && (firstSegment || filename.s!="")) 
		{
			std::string fn=filename.s;
			/// out.open(fn.c_str());
		}
		firstSegment=false;

		// Decompress
		fflush(stdout);
		unzBuf seg(jsize);
		if (journaling) 
			d.setOutput(&seg);
		unzSHA1 sha1;
		d.setSHA1(&sha1);
		d.decompress();
		if (journaling && seg.s.size()!=jsize) 
			unzerror("incomplete output");

      // Verify checksum
		char checksum[21];
		d.readSegmentEnd(checksum);
//		if (!noeta)
///		printf("s");
		if (checksum[0]==1) 
		{
			if (memcmp(checksum+1, sha1.result(), 20)) 
				unzerror("unzSHA1 mismatch");
      //  else printf("OK");
   ///   printf("\nZ1: SHA1 checksum OK\n");
		}
		else 
		if (checksum[0]==0) 
			printf("not checked");
		else unzerror("invalid checksum type");
      ///printf("\n");
      
		filename.s="";

      // check csize at first non-d block
		if (csize && strchr("chi", type)) 
		{
			if (csize!=offset) 
			{
				printf("Z2:    csize=%1.0f, offset=%1.0f\n",double(csize), double(offset));		
				unzerror("csize does not point here");
			}
			csize=0;
		}
///printf("=======================================\n");

      // Get csize from c block
		const size_t len=seg.s.size();
		if (type=='c') 
		{
			if (len<8) 
				unzerror("c block too small");
        
			csize=unzget8(seg.s.data());
			lavorati+=csize;
		
			if (flagnoeta==false)
			{
				///printf("\n");
				list_print_datetime();
				printf("%20s (%15s)\n", migliaia(lavorati),migliaia(csize));
			}
        // test for end of archive marker
			if (csize>>63) 
			{
				printf("Incomplete transaction at end of archive (OK)\n");
				done=true;
			}
			else 
			if (index && csize!=0) 
				unzerror("nonzero csize in index");

        // test for rollback
			if (until && date>until) 
			{
				printf("Rollback: %1.0f is after %1.0f\n",double(date), double(until));
			done=true;
			}

        // Set csize to expected offset of first non d block
        // assuming 1 more byte for unread end of block marker.
			csize+=in.tell()+1;
		}

      // Test and save d block
		if (type=='d') 
		{
			if (index) 
				unzerror("d block in index");
			bsize[id]=in.tell()+1-offset;  // compressed size
   ///     printf("    %u -> %1.0f ", bsize[id], double(seg.s.size()));

        // Test frag size list at end. The format is f[id..id+n-1] fid n
        // where fid may be id or 0. sizes must sum to the rest of block.
			if (len<8) 
				unzerror("d block too small");
			
			const char* end=seg.s.data()+len;  // end of block
			uint32_t fid=unzget4(end-8);  // 0 or ID
			const uint32_t n=unzget4(end-4);    // number of frags
			
			if (fid==0) 
			fid=id;
			///if (!noeta)
				//printf("u");
				///printf(".");
        ///printf("[%u..%u) ", fid, fid+n);
			if (fid!=id) 
				unzerror("missing ID");
			if (n>(len-8)/4) 
				unzerror("frag list too big");
			uint64_t sum=0;  // computed sum of frag sizes
			for (unsigned i=0; i<n; ++i) 
				sum+=unzget4(end-12-4*i);
		//if (!noeta)
			//printf("");	//printf("m");
        ///printf("= %1.0f ", double(sum));
			if (sum+n*4+8!=len) 
				unzerror("bad frag size list");

        // Save frag hashes and sizes. For output, save data too.
			const char* data=seg.s.data();  // uncompressed data
			const char* flist=data+sum;  // frag size list
			assert(flist+n*4+8==end);
			for (uint32_t i=0; i<n; ++i) 
			{
				while (frag.size()<=id+i) 
					frag.push_back("");
				if (frag[id+i]!="") 
					unzerror("duplicate frag ID");
				uint32_t f=unzget4(flist);  // frag size
				unzSHA1 sha1;
				for (uint32_t j=0; j<f; ++j) 
					sha1.put(data[j]);
				const char* h=sha1.result();  // frag hash
				frag[id+i]=std::string(h, h+20)+std::string(flist, flist+4);
				frag[id+i]+=std::string(data, data+f);
				data+=f;
				flist+=4;
				ramsize+=frag[id+i].size();
			}
			assert(data+n*4+8==end);
			assert(flist+8==end);
			///if (!noeta)
				///printf("o"); //printf("O");
			///printf("OK\n");
		}

      // Test and save h block. Format is: bsize (sha1[20] size)...
      // where bsize is the compressed size of the d block with the same id,
      // and each size corresonds to a fragment in that block. The list
      // must match the list in the d block if present.

		if (type=='h') 
		{
			if (len%24!=4) 
			unzerror("bad h block size");
			uint32_t b=unzget4(seg.s.data());
			b++;
///if (!noeta)
				///printf("O");//printf("i");
        ///printf("    [%u..%u) %u ", uint32_t(id), uint32_t(id+len/24), b);

        // Compare hashes and sizes
			const char* p=seg.s.data()+4;  // next hash, size
			uint32_t sum=0;  // uncompressed size of all frags
			for (uint32_t i=0; i<len/24; ++i) 
			{
				if (index) 
				{
					while (frag.size()<=id+i) frag.push_back("");
					if (frag[id+i]!="") unzerror("data in index");
						frag[id+i]=std::string(p, p+24);
				}
				else 
				if (id+i>=frag.size() || frag[id+i].size()<24)
					unzerror("no matching d block");
				else 
				if (frag[id+i].substr(0, 24)!=std::string(p, p+24))
					unzerror("frag size or hash mismatch");
				
				sum+=unzget4(p+20);
				p+=24;
			}
			///if (!noeta)
				///printf("0");//printf("o");
        ///printf("-> %u OK\n", sum);
		}
		
      // Test i blocks and save files to extract. Format is:
      //   date filename 0 na attr[0..na) ni ptr[0..ni)   (to update)
      //   0    filename                                  (to delete)
      // Date is 64 bits in YYYYMMDDHHMMSS format.

		if (type=='i') 
		{
			const char* p=seg.s.data();
			const char* end=p+seg.s.size();
			while (p<end) 
			{

          // read date
				if (end-p<8) 
				unzerror("missing date");
				
				unzDT f;
				f.date=unzget8(p), p+=8;
				if (f.date!=0) 
					verify_date(f.date);

          // read filename
				std::string fn;
				while (p<end && *p) 
				{
				///		if (*p>=0 && *p<32) printf("^%c", *p+64);
       ///     else putchar(*p);
					fn+=*p++;
				}
				if (p==end) 
					unzerror("missing NUL in filename");
				++p;
				if (fn.size()>65535) 
				unzerror("filename size > 65535");
				unzverify_utf8(fn.c_str());

          // read attr
				if (f.date>0) 
				{
					if (end-p<4) 
						unzerror("missing attr size");
					uint32_t na=unzget4(p);
					p+=4;
					if (na>65535) 
					unzerror("attr size > 65535");
			
					if (na>FRANZOFFSET) // houston we have a FRANZBLOCK? SHA1 0 CRC32?
					{
						/// paranoid works with SHA1, not CRC32. Get (if any)
						for (unsigned int i=0;i<40;i++)
							f.sha1hex[i]=*(p+(na-FRANZOFFSET)+i);
						f.sha1hex[40]=0x0;
					///printf("---FRANZ OFFSET---\n");
					}
					else
					{
						f.sha1hex[0]=0x0;
					///printf("---NORMAL OFFSET---\n");
					}
			
					for (unsigned i=0; i<na; ++i) 
					{
						if (end-p<1) 
							unzerror("missing attr");
						uint64_t a=*p++&255;
						if (i<8) 
							f.attr|=a<<(i*8);
					}

					// read frag IDs
					if (end-p<4) 
						unzerror("missing frag ptr size");
					uint32_t ni=unzget4(p);
					p+=4;
					for (uint32_t i=0; i<ni; ++i) 
					{
						if (end-p<4) 
							unzerror("missing frag ID");
						uint32_t a=unzget4(p);
						p+=4;
						f.ptr.push_back(a);

					// frag must refer to valid data except in an index
						if (!index) 
						{
							if (a<1 || a>=frag.size()) 
								unzerror("frag ID out of range");
							if (frag[a]=="") 
								unzerror("missing frag data");
						}
					}
				}
				dt[fn]=f;
			}
		}
		printf("\r");
		list_print_datetime();
		printf("%s frags %s (RAM used ~ %s)\r",migliaia2(100-(offset*100/(total_size+1))),migliaia(frag.size()),migliaia2(ramsize));

    }  // end while findFilename
    offset=in.tell();
  }  // end while findBlock
  printf("\n%s bytes of %s tested\n", migliaia(in.tell()), archive);

  	
	std::vector<unzDTMap::iterator> mappadt;
	std::vector<unzDTMap::iterator> vf;
	std::vector<unzDTMap::iterator> errori;
	std::vector<unzDTMap::iterator> warning;
	std::map<std::string, unzDT>::iterator p;

	int64_t iniziocalcolo=mtime();
		
	for (p=dt.begin(); p!=dt.end(); ++p)
		mappadt.push_back(p);
		
	std::sort(mappadt.begin(), mappadt.end(),unzcompareprimo);

	for (unsigned i=0; i<mappadt.size(); ++i) 
	{
		unzDTMap::iterator p=mappadt[i];
		unzDT f=p->second;
		if (f.date) 
		{
			uint64_t size=0;
			for (uint32_t i=0; i<f.ptr.size(); ++i)
				if (f.ptr[i]>0 && f.ptr[i]<frag.size() && frag[f.ptr[i]].size()>=24)
					size+=unzget4(frag[f.ptr[i]].data()+20);
			if (!index) 
			{
				std::string fn=p->first;
				
				if 	((fn.find(":$DATA")==std::string::npos) 	&&
						(fn.find(".zfs")==std::string::npos)	&&
						(fn.back()!='/')) ///changeme
					
				{
					int64_t startrecalc=mtime();
					unzSHA1 mysha1;
					for (uint32_t i=0; i<f.ptr.size(); ++i) 
						if (f.ptr[i]<frag.size())
							for (uint32_t j=24; j<frag[f.ptr[i]].size(); ++j)
								mysha1.put(frag[f.ptr[i]][j]);
									
					char sha1result[20];
					memcpy(sha1result, mysha1.result(), 20);
					for (int j=0; j <= 19; j++)
						sprintf(p->second.sha1decompressedhex+j*2,"%02X", (unsigned char)sha1result[j]);
					p->second.sha1decompressedhex[40]=0x0;
					vf.push_back(p);
					
					if (flagnoeta==false)
						printf("File %08u of %08lld (%20s) %1.3f %s\n",i+1,(long long)mappadt.size(),migliaia(size),(mtime()-startrecalc)/1000.0,fn.c_str());
						
				}
			}
		}
	}
if (!flagnoeta)
	printf("\n");
		std::sort(vf.begin(), vf.end(), unzcomparesha1hex);
	printf("\n");
	int64_t finecalcolo=mtime();
	
	printf("Calc time %f\n",(finecalcolo-iniziocalcolo)/1000.0);
	
//////////////////////////////
/////// now some tests
/////// if possible check SHA1 into the ZPAQ with SHA1 extracted with SHA1 of the current file

		unsigned int status_e=0;
		unsigned int status_w=0;

		unsigned int status_0=0;
		unsigned int status_1=0;
		unsigned int status_2=0;
		
		std::string sha1delfile;
		
		for (unsigned i=0; i<vf.size(); ++i) 
		{
			
			unzDTMap::iterator p=vf[i];
			
			unsigned int statusok=0;
			bool flagerror=false;
		
			
			if (flagverify)
			{
				printf("Re-hashing %s\r",p->first.c_str());
				int64_t dummy;
				sha1delfile=sha1_calc_file(p->first.c_str(),-1,-1,dummy);
			}
			std::string sha1decompresso=p->second.sha1decompressedhex;
			std::string sha1stored=p->second.sha1hex;

	
			if (flagverbose)
			{
				printf("SHA1 OF FILE:   %s\n",p->first.c_str());
				printf("DECOMPRESSED:   %s\n",sha1decompresso.c_str());
			
				if (!sha1stored.empty())
				printf("STORED IN ZPAQ: %s\n",p->second.sha1hex);
			
				if (!sha1delfile.empty())
				printf("FROM FILE:      %s\n",sha1delfile.c_str());
			}
			
			if (!sha1delfile.empty())
			{
				if (sha1delfile==sha1decompresso)
					statusok++;
				else
					flagerror=true;
			}
			if (!sha1stored.empty())
			{
				if (sha1stored==sha1decompresso)
					statusok++;
				else
					flagerror=true;
			}
			if (flagerror)
			{
				if (flagverbose)
				printf("ERROR IN VERIFY!\n");
				status_e++;
				p->second.sha1fromfile=sha1delfile;
				errori.push_back(p);
			}
			else
			{
				if (flagverbose)
					printf("OK status:      %d (0=N/A, 1=good, 2=sure)\n",statusok);
				if (statusok==1)
					status_1++;
				if (statusok==2)
					status_2++;
			}
			if (statusok==0)
			{
				status_w++;
				p->second.sha1fromfile=sha1delfile;
				warning.push_back(p);
			}
			
			if (flagverbose)
				printf("\n");
		}

		int64_t fine=mtime();
		
		if (warning.size()>0)
		{
			printf("WARNING(S) FOUND!\n");
			
			for (unsigned i=0; i<warning.size(); ++i) 
			{
				unzDTMap::iterator p=warning[i];
				
				std::string sha1delfile=p->second.sha1fromfile;
				std::string sha1decompresso=p->second.sha1decompressedhex;
				std::string sha1stored=p->second.sha1hex;
				
				printf("SHA1 OF FILE:   %s\n",p->first.c_str());
				printf("DECOMPRESSED:   %s\n",sha1decompresso.c_str());
			
				if (!sha1stored.empty())
				printf("STORED IN ZPAQ: %s\n",p->second.sha1hex);
			
				if (!sha1delfile.empty())
				printf("FROM FILE:      %s\n",sha1delfile.c_str());
				printf("\n");
			}
			printf("\n\n");
		}
		if (errori.size()>0)
		{
			printf("ERROR(S) FOUND!\n");
			
			for (unsigned i=0; i<errori.size(); ++i) 
			{
				unzDTMap::iterator p=errori[i];
				
				std::string sha1delfile=p->second.sha1fromfile;
				std::string sha1decompresso=p->second.sha1decompressedhex;
				std::string sha1stored=p->second.sha1hex;
				
				printf("SHA1 OF FILE:   %s\n",p->first.c_str());
				printf("DECOMPRESSED:   %s\n",sha1decompresso.c_str());
			
				if (!sha1stored.empty())
				printf("STORED IN ZPAQ: %s\n",sha1stored.c_str());
			
				if (!sha1delfile.empty())
				printf("FROM FILE:      %s\n",sha1delfile.c_str());
				printf("\n");
			}
		}
	
	unsigned int total_files=vf.size();
	
	printf("SUMMARY  %1.3f s\n",(fine-inizio)/1000.0);
	printf("Total file: %08d\n",total_files);


	if (status_e>0)
	printf("ERRORS  : %08d (ERROR:  something WRONG)\n",status_e);
	
	if (status_0>0)
	printf("WARNING : %08d (UNKNOWN:cannot verify)\n",status_0);
	
	if (status_1>0)
	printf("GOOD    : %08d of %08d (stored=decompressed)\n",status_1,total_files);
	
	if (status_2>0)
	printf("SURE    : %08d of %08d (stored=decompressed=file on disk)\n",status_2,total_files);
		
	if (status_e==0)
	{
		if (status_0)
			printf("Unknown (cannot verify)\n");
		else
			printf("All OK (paranoid test)\n");
	}
	else
	{
		printf("WITH ERRORS\n");
		return 2;
	}
  return 0;
}



/*
ZPAQ does not store blocks of zeros at all.
This means that they are not, materially, in the file, 
and therefore cannot be used to calculate the CRC-32.

This is a function capable of doing it, 
even for large sizes (hundreds of gigabytes or more in the case of thick virtual machine disks),
up to ~9.000TB

The function I wrote split a number into 
its powers of 2, takes the CRC-32 code from a precomputed table, 
and finally combines them. 
It's not the most efficient method (some 10-20 iterations are typically needed), 
but it's still decent

*/
const uint32_t zero_block_crc32[54] =
{
0xD202EF8D,0x41D912FF,0x2144DF1C,0x6522DF69,0xECBB4B55,0x190A55AD,0x758D6336,0xC2A8FA9D,
0x0D968558,0xB2AA7578,0xEFB5AF2E,0xF1E8BA9E,0xC71C0011,0xD8F49994,0xAB54D286,0x011FFCA6,
0xD7978EEB,0x7EE8CDCD,0xE20EEA22,0x75660AAC,0xA738EA1C,0x8D89877E,0x1147406A,0x1AD2BC45,
0xA47CA14A,0x59450445,0xB2EB30ED,0x80654151,0x2A0E7DBB,0x6DB88320,0x5B64C2B0,0x4DBDF21C,
0xD202EF8D,0x41D912FF,0x2144DF1C,0x6522DF69,0xECBB4B55,0x190A55AD,0x758D6336,0xC2A8FA9D,
0x0D968558,0xB2AA7578,0xEFB5AF2E,0xF1E8BA9E,0xC71C0011,0xD8F49994,0xAB54D286,0x011FFCA6,
0xD7978EEB,0x7EE8CDCD,0xE20EEA22,0x75660AAC,0xA738EA1C,0x8D89877E
};
uint32_t crc32ofzeroblock(uint64_t i_size) 
{
	assert(i_size<9.007.199.254.740.992); //8D89877E 2^53 9.007.199.254.740.992
	if (i_size==0)
		return 0;
	uint32_t mycrc=0;
	unsigned int i=0;
	while (i_size > 0)
	{
		if ((i_size%2)==1)
			mycrc=crc32_combine(mycrc,zero_block_crc32[i],1<<i);
		i_size=i_size/2;
		i++;
	}
   	return mycrc;
}


uint32_t crc32zeros(int64_t i_size)
{
	uint64_t inizio=mtime();
	uint32_t crc=crc32ofzeroblock(i_size);
	g_zerotime+=mtime()-inizio;
	return crc;
}

void printbar(char i_carattere,bool i_printbarraenne=true)
{
	int twidth=terminalwidth();
	if (twidth>10)
	{
		for (int i=0;i<twidth-4;i++)
			printf("%c",i_carattere);
		if (i_printbarraenne)
			printf("\n");
	}
	else
		printf("\n");
}



/// concatenate multipart archive into one zpaq
/// check for freespace and (optionally) hash check
int Jidac::consolidate(string i_archive)
{
	assert(i_archive);
	printf("*** Merge (consolidate) ***\n");
	
	vector<string>	chunk_name;
	vector<int64_t>	chunk_size;
	
	const string part0=subpart(i_archive, 0);
	for (unsigned i=1; ;i++) 
	{
		const string parti=subpart(i_archive, i);
		if (i>1 && parti==part0) 
			break;
		if (!fileexists(parti))
			break;
		int64_t dimensione=prendidimensionefile(parti.c_str());
		if (dimensione<0)
			break;
		chunk_size.push_back(dimensione);
		chunk_name.push_back(parti);
	}
	if (chunk_name.size()==0)
	{
		printf("29515: Something strange: cannot find archive chunks\n");
		return 2;
	}
	int64_t total_size=0;
	for (int unsigned i=0;i<chunk_name.size();i++)
	{
		printf("Chunk %08d %20s <<%s>>\n",i,migliaia(chunk_size[i]),chunk_name[i].c_str());
		total_size+=chunk_size[i];
	}
	printbar('-');
	printf("Total %s %20s (%s)\n",migliaia2(chunk_name.size()),migliaia(total_size),tohuman(total_size));
	if (files.size()!=1)
	{
		printf("29545: exactly one file as output for consolidate\n");
		return 1;
	}
	
	string outfile=files[0];
	if (fileexists(outfile))
		if (!flagforce)
		{
			printf("29553: outfile exists, and no -force. Quit %s\n",outfile.c_str());
			return 1;
		}
	int64_t spazio=getfreespace(outfile.c_str());
	printf("Outfile    <<%s>>\n",outfile.c_str());
	printf("Free space %20s\n",migliaia(spazio));
	printf("Needed     %20s\n",migliaia(total_size));
	
	if (spazio<total_size)
		if (!flagforce)
		{
			printf("29564: Free space seems < needed, and no -force. Quit\n");
			return 1;
		}

#ifdef _WIN32
	wstring widename=utow(outfile.c_str());
	FILE* outFile=_wfopen(widename.c_str(), L"wb" );
#else
	FILE* outFile=fopen(outfile.c_str(), "wb");
#endif

	if (outFile==NULL)
	{
		printf("29579 :CANNOT OPEN outfile %s\n",outfile.c_str());
		return 2;
	}
	size_t const blockSize = 65536;
	unsigned char buffer[blockSize];
	int64_t donesize=0;
	int64_t startcopy=mtime();

	XXH3_state_t state128;
    (void)XXH3_128bits_reset(&state128);
	if (flagverify)
		printf("-verify: trust, but check...\n");
		
	for (int unsigned i=0;i<chunk_name.size();i++)
	{
		string	sorgente_nome=chunk_name[i];
		
		FILE* inFile = freadopen(sorgente_nome.c_str());
		if (inFile==NULL) 
		{
#ifdef _WIN32
		int err=GetLastError();
#else
		int err=1;
#endif
		printf("\n29585: ERR <%s> kind %d\n",sorgente_nome.c_str(),err); 

		return 2;
		}
		
		size_t readSize;
		int64_t	chunk_readed=0;
		int64_t	chunk_written=0;
		while ((readSize = fread(buffer, 1, blockSize, inFile)) > 0) 
		{
			int64_t written=fwrite(buffer,1,readSize,outFile);
			chunk_written+=written;
			chunk_readed+=readSize;
			donesize+=written;
			if (flagverify)
				(void)XXH3_128bits_update(&state128, buffer, readSize);
			avanzamento(donesize,total_size,startcopy);
		}
		
		fclose(inFile);
		if (flagverbose)
			printf("Chunk %08d R %20s W %20s E %20s\n",i,migliaia(chunk_readed),migliaia2(chunk_written),migliaia3(chunk_size[i]));
		if ((chunk_readed!=chunk_written) || (chunk_readed!=chunk_size[i]))
		{
			printf("29632: GURU on chunk %d Read, Write, Expec not equal!\n",i);
			return 1;
		}
	}
	fclose(outFile);
	printf("Done\n");
	printf("Written  %20s\n",migliaia(donesize));
	printf("Expected %20s\n\n",migliaia(total_size));
	if (donesize!=total_size)
	{
		printf("29645: GURU bytes written does not match expected\n");
		return 1;
	}
	if (flagverify)
	{
		printf("-flagverify: double check...\n");
		XXH128_hash_t myhash=XXH3_128bits_digest(&state128);
		char risultato[33];
		sprintf(risultato,"%016llX%016llX",(unsigned long long)myhash.high64,(unsigned long long)myhash.low64);
		
		uint64_t dummybasso64;
		uint64_t dummyalto64;
		int64_t startverify=mtime();
		int64_t io_lavorati=0;
		string hashreloaded=xxhash_calc_file(outfile.c_str(),startverify,total_size,io_lavorati,dummybasso64,dummyalto64);
		printf("Expected   XXH3 hash of the output file %s\n",risultato);
		printf("Calculated XXH3 hash of the output file %s\n",risultato);
		if (hashreloaded!=risultato)
		{
			printf("29658: GURU hash of output file does not match!\n");
			return 1;
		}
	}
	return 0;
	
}
	

/// franz-test
/// extract data (multithread) and check CRC-32, if any

int Jidac::test() 
{
	flagtest=true;
	summary=1;
  
	const int64_t sz=read_archive(archive.c_str());
	if (sz<1) error("archive not found");
	
 	
	for (unsigned i=0; i<block.size(); ++i) 
	{
		if (block[i].bsize<0) error("negative block size");
		if (block[i].start<1) error("block starts at fragment 0");
		if (block[i].start>=ht.size()) error("block start too high");
		if (i>0 && block[i].start<block[i-1].start) error("unordered frags");
		if (i>0 && block[i].start==block[i-1].start) error("empty block");
		if (i>0 && block[i].offset<block[i-1].offset+block[i-1].bsize)
			error("unordered blocks");
		if (i>0 && block[i-1].offset+block[i-1].bsize>block[i].offset)
			error("overlapping blocks");
	}

	///printf("franz: fine test blocchi\n");
	int64_t dimtotalefile=0;
  // Label files to extract with data=0.
  ExtractJob job(*this);
  int total_files=0;
  for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) 
  {
		p->second.data=-1;  // skip
		if (p->second.date && p->first!="") 
		{
			const string fn=rename(p->first);
///			printf("--------------------> %s\n",fn.c_str());
			const bool isdir=p->first[p->first.size()-1]=='/';
			
			if (isdir)  // update directories later
				p->second.data=0;
			else 
				if (block.size()>0) 
			{  // files to decompress
				p->second.data=0;
				unsigned lo=0, hi=block.size()-1;  // block indexes for binary search
				for (unsigned i=0; p->second.data>=0 && i<p->second.ptr.size(); ++i) 
				{
					unsigned j=p->second.ptr[i];  // fragment index
					///printf("Fragment index %lld\n",j);
					if (j==0 || j>=ht.size() || ht[j].usize<-1) 
					{
						fflush(stdout);
						printUTF8(p->first.c_str(), stderr);
						fprintf(stderr, ": bad frag IDs, skipping...\n");
						p->second.data=-1;  // skip
						continue;
					}
					assert(j>0 && j<ht.size());
					
					if (lo!=hi || lo>=block.size() || j<block[lo].start
						|| (lo+1<block.size() && j>=block[lo+1].start)) 
						{
							lo=0;  // find block with fragment j by binary search
							hi=block.size()-1;
							while (lo<hi) 
							{
								unsigned mid=(lo+hi+1)/2;
								assert(mid>lo);
								assert(mid<=hi);
								if (j<block[mid].start) 
									hi=mid-1;
								else 
									(lo=mid);
							}
						}
						assert(lo==hi);
						assert(lo>=0 && lo<block.size());
						assert(j>=block[lo].start);
						assert(lo+1==block.size() || j<block[lo+1].start);
						unsigned c=j-block[lo].start+1;
						if (block[lo].size<c) 
							block[lo].size=c;
						if (block[lo].files.size()==0 || block[lo].files.back()!=p)
						{
							block[lo].files.push_back(p);
						///	printf("+++++++++++Pushato %s\n",p->first.c_str());
						}
				}
				++total_files;
				job.total_size+=p->second.size;
			}
		}  // end if selected
  }  // end for

	dimtotalefile=job.total_size;
	printf("Check %s in %s files -threads %d\n",migliaia(job.total_size), migliaia2(total_files), howmanythreads);
	vector<ThreadID> tid(howmanythreads);
	
	for (unsigned i=0; i<tid.size(); ++i) 
		run(tid[i], decompressThread, &job);

  // Wait for threads to finish
	for (unsigned i=0; i<tid.size(); ++i) 
		join(tid[i]);

 
 
  // Report failed extractions
  unsigned extracted=0, errors=0;
  for (DTMap::iterator p=dt.begin(); p!=dt.end(); ++p) 
  {
    string fn=rename(p->first);
    if (p->second.data>=0 && p->second.date
        && fn!="" && fn[fn.size()-1]!='/') {
      ++extracted;
      if (p->second.ptr.size()!=unsigned(p->second.data)) {
        fflush(stdout);
        if (++errors==1)
          fprintf(stderr,
          "\nFailed (extracted/total fragments, file):\n");
        fprintf(stderr, "%u/%u ",
                int(p->second.data), int(p->second.ptr.size()));
        printUTF8(fn.c_str(), stderr);
        fprintf(stderr, "\n");
      }
    }
  }
  if (errors>0) 
  {
    fflush(stdout);
    fprintf(stderr,
        "\nChecked %u of %u files OK (%u errors)"
        " using %1.3f MB x %d threads\n",
        extracted-errors, extracted, errors, job.maxMemory/1000000,
        int(tid.size()));
  }
  else
  {
	printf("No error detected in first stage (standard 7.15), now try CRC-32 (if present)\n");
  }
//// OK now check against CRC32   

	uint64_t startverify=mtime();
	
	sort(g_crc32.begin(),g_crc32.end(),comparecrc32block);
	

	unsigned int status_e=0;
	unsigned int status_0=0;
	unsigned int status_1=0;
	unsigned int status_2=0;
	uint32_t 	checkedfiles=0;
	uint32_t 	uncheckedfiles=0;
	
	unsigned int i=0;
	unsigned int parti=1;
	int64_t lavorati=0;
	uint32_t currentcrc32=0;
	char * crc32storedhex;
	uint32_t crc32stored;
	uint64_t dalavorare=0;
	
	
	for (unsigned int i=0;i<g_crc32.size();i++)
		dalavorare+=g_crc32[i].crc32size;
	
	printf("\n\nChecking  %s blocks with CRC32 (%s)\n",migliaia(g_crc32.size()),migliaia2(dalavorare));
	if (flagverify)
		printf("Re-testing CRC-32 from filesystem\n");
		
	uint32_t parts=0;
	uint64_t	zeroedblocks=0;
	uint32_t	howmanyzero=0;
	
	g_zerotime=0;
	/*
	printf("======================\n");
	for (unsigned int i=0;i<g_crc32.size();i++)
	{
				printf("Start %08d %08lld\n",i,g_crc32[i].crc32start);
				printf("Size  %08d %08lld\n",i,g_crc32[i].crc32size);
	}
	printf("======================\n");
	*/
	while (i<g_crc32.size())
	{
		if ( ++parts % 1000 ==0)
			if (!flagnoeta)
				printf("Block %08u K %s\r",i/1000,migliaia(lavorati));
            
		s_crc32block it=g_crc32[i];
		DTMap::iterator p=dt.find(it.filename);
		
		crc32storedhex=0;
		crc32stored=0;
		if (p != dt.end())
			if (p->second.sha1hex[41]!=0)
			{
				crc32storedhex=p->second.sha1hex+41;
				crc32stored=crchex2int(crc32storedhex);
			}
		
/// Houston, we have something that start with a sequence of zeros, let's compute the missing CRC		
		if (it.crc32start>0)
		{
			uint64_t holesize=it.crc32start;
			/*
			printf("Holesize iniziale %ld\n",holesize);
			*/
			
			uint32_t zerocrc=crc32zeros(holesize);
			currentcrc32=crc32_combine(currentcrc32, zerocrc,holesize);
			lavorati+=holesize;	
			zeroedblocks+=holesize;
			howmanyzero++;
			/*
			char mynomefile[100];
			sprintf(mynomefile,"z:\\globals_%014lld_%014lld_%08X_prezero",holestart,holestart+holesize,zerocrc);
			FILE* myfile=fopen(mynomefile, "wb");
			fwrite(g_allzeros, 1, holesize, myfile);
			fclose(myfile);				
			*/
		}
		
		
		currentcrc32=crc32_combine(currentcrc32, it.crc32,it.crc32size);
		lavorati+=it.crc32size;
		
		while (g_crc32[i].filename==g_crc32[i+1].filename)
		{
			if ((g_crc32[i].crc32start+g_crc32[i].crc32size) != (g_crc32[i+1].crc32start))
			{
/// Houston: we have an "hole" of zeros from i to i+1 block. Need to take the fair CRC32
				/*
				printf("############################## HOUSTON WE HAVE AN HOLE %d %d\n",i,g_crc32.size());
				printf("Start %08lld\n",g_crc32[i].crc32start);
				printf("Size  %08lld\n",g_crc32[i].crc32size);
				printf("End   %08lld\n",g_crc32[i].crc32start+g_crc32[i].crc32size);
				printf("Next  %08lld\n",g_crc32[i+1].crc32start);
				*/
				///uint64_t holestart=g_crc32[i].crc32start+g_crc32[i].crc32size;
				///printf("Holestart  %08lld\n",holestart);
				
				uint64_t holesize=g_crc32[i+1].crc32start-(g_crc32[i].crc32start+g_crc32[i].crc32size);
				///printf("Hole Size  %08lld\n",holesize);
				/*
				uint32_t zerocrc;
				zerocrc=crc32_16bytes (allzeros,(uint32_t)holesize);
				//printf("zero  %08X\n",zerocrc);
				currentcrc32=crc32_combine(currentcrc32, zerocrc,(uint32_t)holesize);
				
				*/
				uint32_t zerocrc=crc32zeros(holesize);
				currentcrc32=crc32_combine(currentcrc32, zerocrc,holesize);
				lavorati+=holesize;
				zeroedblocks+=holesize;
				howmanyzero++;
				/*
				char mynomefile[100];
				sprintf(mynomefile,"z:\\globals_%014lld_%014lld_%08X_zero",holestart,holestart+holesize,zerocrc);
				FILE* myfile=fopen(mynomefile, "wb");
				fwrite(g_allzeros, 1, holesize, myfile);
				fclose(myfile);
				*/
				
			}
			i++;
			s_crc32block myit=g_crc32[i];
			currentcrc32=crc32_combine (currentcrc32, myit.crc32,myit.crc32size);
			lavorati+=myit.crc32size;
			parti++;
		}
		
		string filedefinitivo=g_crc32[i].filename;
		uint32_t crc32fromfilesystem=0; 
		
		if (flagverify)
		{
			checkedfiles++;
			if (flagverbose)
			printf("Taking CRC32 for %s\r",filedefinitivo.c_str());
			crc32fromfilesystem=crc32_calc_file(filedefinitivo.c_str(),-1,dimtotalefile,lavorati);
/*		
		if (crc32fromfilesystem==0)
			{
				printf("WARNING impossible to read file %s\n",filedefinitivo.c_str());
				status_w++;
				status_0++;
			}
			*/
		}
		
		if (flagverbose)
			printf("Stored %08X calculated %08X fromfile %08X %s\n",crc32stored,currentcrc32,crc32fromfilesystem,filedefinitivo.c_str());

		if (crc32storedhex)
		{
			
			if (currentcrc32==crc32stored)
			{
				if (flagverify)
				{
					if (crc32stored==crc32fromfilesystem)
					{
						if (flagverbose)
						printf("SURE:  STORED %08X = DECOMPRESSED = FROM FILE %s\n",crc32stored,filedefinitivo.c_str());
						status_2++;
					}
					else
					{
						printf("ERROR:  STORED %08X XTRACTED %08X FILE %08X NOT THE SAME! (ck %08d) %s\n",crc32stored,currentcrc32,(unsigned int)crc32fromfilesystem,parti,filedefinitivo.c_str());
						status_e++;
					}
				
				}
				else
				{
					if (flagverbose)
					printf("GOOD: STORED %08X = DECOMPRESSED %s\n",crc32stored,filedefinitivo.c_str());
					status_1++;
				}
	
			}
			else
			{
				printf("ERROR: STORED %08X != DECOMPRESSED %08X (ck %08d) %s\n",crc32stored,currentcrc32,parti,filedefinitivo.c_str());
				status_e++;
			}
		}
		else
		{	// we have and old style ZPAQ without CRC32
		
			uncheckedfiles++;
			///printf("CRC32: %08X %08X (parti %08d) %s\n",currentcrc32,chekkone,parti,filedefinitivo.c_str());
		}
		parti=1;
		currentcrc32=0;
		i++;
	}
	printf("\nVerify time %f s\n",(mtime()-startverify)/1000.0);
	
	printf("Blocks %19s (%12s)\n",migliaia(dalavorare),migliaia2(g_crc32.size()));
	printf("Zeros  %19s (%12s) %f s\n",migliaia(zeroedblocks),migliaia2(howmanyzero),(g_zerotime/1000.0));
	printf("Total  %19s speed %s/sec\n",migliaia(dalavorare+zeroedblocks),migliaia2((dalavorare+zeroedblocks)/((mtime()-startverify+1)/1000.0)));
	
	if (checkedfiles>0)
	printf("Checked : %08d (zpaqfranz)\n",checkedfiles);
	if (uncheckedfiles>0)
	{
	printf("UNcheck : %08d (zpaq 7.15)\n",uncheckedfiles);
		status_0=uncheckedfiles;
	}
	
	
	if (status_e)
	printf("ERRORS  : %08d (ERROR:  something WRONG)\n",status_e);
	
	if (status_0)
	printf("WARNING : %08d (Cannot say anything)\n",status_0);
	
	if (status_1)
	printf("GOOD    : %08d of %08d (stored=decompressed)\n",status_1,total_files);
	
	if (status_2)
	printf("SURE    : %08d of %08d (stored=decompressed=file on disk)\n",status_2,total_files);
		
	if (status_e==0)
	{
		if (status_0)
			printf("Verdict: unknown   (cannot say anything)\n");
		else
		{
			if (flagverify)
			printf("All OK (with verification from filesystem)\n");
			else
			printf("All OK (normal test)\n");
				
		}
	}
	else
	{
		printf("WITH ERRORS\n");
		return 2;
	}
	
	
  return (errors+status_e)>0;
}
/*
template <typename I> std::string n2hexstr(I w, size_t hex_len = sizeof(I)<<1) {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len,'0');
    for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}
*/


struct s_fileandsize
{
	string	filename;
	uint64_t size;
	int64_t attr;
	int64_t date;
	bool isdir;
	string hashhex;
	bool flaghashstored;
	s_fileandsize(): size(0),attr(0),date(0),isdir(false),flaghashstored(false) {hashhex="";}
};

bool comparecrc32(s_fileandsize a, s_fileandsize b)
{
	return a.hashhex>b.hashhex;
///	return a.crc32>b.crc32;
}
bool comparesizehash(s_fileandsize a, s_fileandsize b)
{
	
	return (a.size < b.size) ||
           ((a.size == b.size) && (a.hashhex > b.hashhex)) || 
           ((a.size == b.size) && (a.hashhex == b.hashhex) &&
              (a.filename<b.filename));
			  ///(strcmp(a.filename.c_str(), b.filename.c_str()) <0));
			  
}
bool comparefilenamesize(s_fileandsize a, s_fileandsize b)
{
	char a_size[40];
	char b_size[40];
	sprintf(a_size,"%014lld",(long long)a.size);
	sprintf(b_size,"%014lld",(long long)b.size);
	return a_size+a.filename<b_size+b.filename;
}
bool comparefilenamedate(s_fileandsize a, s_fileandsize b)
{
	char a_size[40];
	char b_size[40];
	sprintf(a_size,"%014lld",(long long)a.date);
	sprintf(b_size,"%014lld",(long long)b.date);
	return a_size+a.filename<b_size+b.filename;
}






pthread_mutex_t mylock = PTHREAD_MUTEX_INITIALIZER;

vector<uint64_t> g_arraybytescanned;
vector<uint64_t> g_arrayfilescanned;

/*
We need something out of an object (Jidac), addfile() and scandir(), 
because pthread does not like very much objects.
Yes, quick and dirty
*/
void myaddfile(uint32_t i_tnumber,DTMap& i_edt,string i_filename, int64_t i_date,int64_t i_size, bool i_flagcalchash) 
{
	///Raze to the ground ads and zfs as soon as possible
	if (isads(i_filename))
		return;
		
	if (!flagforcezfs)
		if (iszfs(i_filename))
			return;
		
	int64_t dummy;
	DT& d=i_edt[i_filename];
	d.date=i_date;
	d.size=i_size;
	d.attr=0;
	d.data=0;
	d.hexhash="";
	
	if (i_flagcalchash)
		if (!isdirectory(i_filename))
		{
			int64_t starthash=mtime();
			d.hexhash=hash_calc_file(flag2algo(),i_filename.c_str(),-1,-1,dummy);
			if (flagverbose)
			{
				printf("%s: |%s| [%d] %6.3f ",mygetalgo().c_str(),d.hexhash.c_str(),i_tnumber,(mtime()-starthash)/1000.0);
				printUTF8(i_filename.c_str());
				printf("\n");
			}
		}
///  thread safe, but... who cares? 
	pthread_mutex_lock(&mylock);

	///printf("i_tnumber %d\n",i_tnumber);
	///printf("size %d\n",g_arraybytescanned.size());
	g_arraybytescanned[i_tnumber]+=i_size;
	g_arrayfilescanned[i_tnumber]++;
	
	if (!flagnoeta)
	{
		if (i_flagcalchash)
		{
			if (!(g_arrayfilescanned[i_tnumber] % 100))
			{
				for (unsigned int i=0; i<g_arraybytescanned.size();i++)
					printf("Checksumming |%02d|%10s %12s\n",i,tohuman(g_arraybytescanned[i]),migliaia(g_arrayfilescanned[i]));
				setupConsole();
				printf("\033[%dA",(int)g_arraybytescanned.size());
				restoreConsole();
			}
		}
		else
		{
			if (!(g_arrayfilescanned[i_tnumber] % 1000))
			{
				for (unsigned int i=0; i<g_arraybytescanned.size();i++)
					printf("|%02d|%10s %12s\n",i,tohuman(g_arraybytescanned[i]),migliaia(g_arrayfilescanned[i]));
				setupConsole();
				printf("\033[%dA",(int)g_arraybytescanned.size());
				restoreConsole();
			}
		}
		fflush(stdout);
	}
	pthread_mutex_unlock(&mylock);
}

void myscandir(uint32_t i_tnumber,DTMap& i_edt,string filename, bool i_recursive,bool i_flagcalchash)
{
	///Raze to the ground ads and zfs as soon as possible
	if (isads(filename))
	{
		if (flagverbose)
			printf("Skip :$DATA ----> %s\n",filename.c_str());
		return;
	}
	if (!flagforcezfs)
		if (iszfs(filename))
		{
			if (flagverbose)
				printf("Skip .zfs ----> %s\n",filename.c_str());
			return;
		}
	
#ifdef unix
// Add regular files and directories
  while (filename.size()>1 && filename[filename.size()-1]=='/')
    filename=filename.substr(0, filename.size()-1);  // remove trailing /
	struct stat sb;
	if (!lstat(filename.c_str(), &sb)) 
	{
		if (S_ISREG(sb.st_mode))
		myaddfile(i_tnumber,i_edt,filename, decimal_time(sb.st_mtime), sb.st_size,i_flagcalchash);

    // Traverse directory
		if (S_ISDIR(sb.st_mode)) 
		{
			myaddfile(i_tnumber,i_edt,filename=="/" ? "/" : filename+"/", decimal_time(sb.st_mtime),0, i_flagcalchash);
			DIR* dirp=opendir(filename.c_str());
			if (dirp) 
			{
				for (dirent* dp=readdir(dirp); dp; dp=readdir(dirp)) 
				{
					if (strcmp(".", dp->d_name) && strcmp("..", dp->d_name)) 
					{
						string s=filename;
						if (s!="/") s+="/";
						s+=dp->d_name;
						if (i_recursive)        
							myscandir(i_tnumber,i_edt,s,true,i_flagcalchash);
						else
						{
							if (!lstat(s.c_str(), &sb)) 
							{
								if (S_ISREG(sb.st_mode))
									myaddfile(i_tnumber,i_edt,s, decimal_time(sb.st_mtime), sb.st_size,i_flagcalchash);
								if (S_ISDIR(sb.st_mode)) 
									myaddfile(i_tnumber,i_edt,s=="/" ? "/" :s+"/", decimal_time(sb.st_mtime),0, i_flagcalchash);
							}
						}          			
					}
				}
				closedir(dirp);
			}
			else
				perror(filename.c_str());
		}
	}
	else
		perror(filename.c_str());

#else  // Windows: expand wildcards in filename
	
  // Expand wildcards
	WIN32_FIND_DATA ffd;
	string t=filename;
	if (t.size()>0 && t[t.size()-1]=='/') 
		t+="*";
  
	HANDLE h=FindFirstFile(utow(t.c_str()).c_str(), &ffd);
	if (h==INVALID_HANDLE_VALUE && GetLastError()!=ERROR_FILE_NOT_FOUND && GetLastError()!=ERROR_PATH_NOT_FOUND)
		printerr("29617",t.c_str());
	
	while (h!=INVALID_HANDLE_VALUE) 
	{

    // For each file, get name, date, size, attributes
		SYSTEMTIME st;
		int64_t edate=0;
		if (FileTimeToSystemTime(&ffd.ftLastWriteTime, &st))
			edate=st.wYear*10000000000LL+st.wMonth*100000000LL+st.wDay*1000000
				+st.wHour*10000+st.wMinute*100+st.wSecond;
		const int64_t esize=ffd.nFileSizeLow+(int64_t(ffd.nFileSizeHigh)<<32);
    
    // Ignore links, the names "." and ".." or any unselected file
		t=wtou(ffd.cFileName);
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT || t=="." || t=="..") 
		edate=0;  // don't add
		
		string fn=path(filename)+t;
	
    // Save directory names with a trailing / and scan their contents
    // Otherwise, save plain files
		if (edate) 
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
				fn+="/";
		
			myaddfile(i_tnumber,i_edt,fn, edate, esize, i_flagcalchash);
		
			if (i_recursive)
				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
				{
					fn+="*";
					myscandir(i_tnumber,i_edt,fn,true,i_flagcalchash);
				}
		}
		if (!FindNextFile(h, &ffd)) 
		{
			if (GetLastError()!=ERROR_NO_MORE_FILES) 
				printerr("29656",fn.c_str());
			break;
		}
	}
  FindClose(h);
#endif
}



///	parameters to run scan threads
struct tparametri
{
	string 		directorytobescanned;
	DTMap		theDT;
	bool		flagcalchash;
	uint64_t	timestart;
	uint64_t	timeend;
	int	tnumber;
};


///	run a myscandir() instead of Jidac::scandir() (too hard to use a member)
void * scansiona(void *t) 
{
	assert(t);
	tparametri* par= ((struct tparametri*)(t));
	DTMap& tempDTMap = par->theDT;
	myscandir(par->tnumber,tempDTMap,par->directorytobescanned,true,par->flagcalchash);
	par->timeend=mtime();
	pthread_exit(NULL);
	return 0;
}

/// return size, date and attr
bool getfileinfo(string i_filename,int64_t& o_size,int64_t& o_date,int64_t& o_attr)
{
	o_size=0;
	o_date=0;
	o_attr=0;
#ifdef unix
	while (i_filename.size()>1 && i_filename[i_filename.size()-1]=='/')
		i_filename=i_filename.substr(0, i_filename.size()-1);  
	struct stat sb;
	if (!lstat(i_filename.c_str(), &sb)) 
	{
		if (S_ISREG(sb.st_mode))
		{
			
			o_date=decimal_time(sb.st_mtime);
			o_size=sb.st_size;
			o_attr='u'+(sb.st_mode<<8);
			return true;
		}
	}
#endif
	
#ifdef _WIN32
	WIN32_FIND_DATA ffd;
	string t=i_filename;
	if (t.size()>0 && t[t.size()-1]=='/') 
		t+="*";
  
	HANDLE h=FindFirstFile(utow(t.c_str()).c_str(), &ffd);
	if (h==INVALID_HANDLE_VALUE && GetLastError()!=ERROR_FILE_NOT_FOUND && GetLastError()!=ERROR_PATH_NOT_FOUND)
		printerr("29617",t.c_str());
	
	if (h!=INVALID_HANDLE_VALUE) 
	{
		SYSTEMTIME st;
		if (FileTimeToSystemTime(&ffd.ftLastWriteTime, &st))
			o_date=st.wYear*10000000000LL+st.wMonth*100000000LL+st.wDay*1000000
				+st.wHour*10000+st.wMinute*100+st.wSecond;
		o_size=ffd.nFileSizeLow+(int64_t(ffd.nFileSizeHigh)<<32);
		o_attr=ffd.dwFileAttributes;
		FindClose(h);
		return true;
    }
	FindClose(h);
#endif
	return false;
}


/// possible problems with unsigned to calculate the differences. We do NOT want to link abs()
int64_t myabs(int64_t i_first,int64_t i_second)
{
	if (i_first>i_second)
		return i_first-i_second;
	else
		return i_second-i_first;
}

int64_t g_robocopy_check_sorgente;
int64_t g_robocopy_check_destinazione;

/// make a "robocopy" from source to destination file.
/// if "someone" call with valid parameters do NOT do a getfileinfo (slow)
string secure_copy_file(
const string i_filename,const string i_outfilename,int64_t i_startcopy,int64_t i_totalsize,int64_t i_totalcount,int64_t& o_donesize,int64_t& o_donecount,
int64_t i_sorgente_size,
int64_t i_sorgente_date,
int64_t i_sorgente_attr,
int64_t i_destinazione_size,
int64_t i_destinazione_date,
int64_t i_destinazione_attr
)
{
	if (flagdebug)
	{
		printf("\n");
		printf("From:   %s\n",i_filename.c_str());
		printf("To:     %s\n",i_outfilename.c_str());
	}
	if (i_filename=="")
		return "30833:SOURCE-EMPTY";
	
	if (i_outfilename=="")
		return "30836:DEST-EMPTY";
	
	if (isdirectory(i_filename))
	{
		makepath(i_outfilename);
		return "OK";
	}
	
	int64_t sorgente_dimensione=0;
	int64_t sorgente_data=0;
	int64_t sorgente_attr=0;
	
	int64_t destinazione_dimensione=0;
	int64_t destinazione_data=0;
	int64_t destinazione_attr=0;
	
	bool	sorgente_esiste=false;
	bool	destinazione_esiste=false;
	/*
	hopefully the source is ALWAYS existing!
	int64_t start_sorgente_esiste=mtime();
	sorgente_esiste		=getfileinfo(i_filename,sorgente_dimensione,sorgente_data,sorgente_attr);
	g_robocopy_check_sorgente+=mtime()-start_sorgente_esiste;
	*/
	sorgente_esiste=true;
	sorgente_dimensione=i_sorgente_size;
	sorgente_data=i_sorgente_date;
	sorgente_attr=i_sorgente_attr;
	
	if (i_destinazione_size>=0)
	{
		/// someone call us with valid size=> take the parameters
		destinazione_esiste=true;
		destinazione_dimensione=i_destinazione_size;
		destinazione_data=i_destinazione_date;
		destinazione_attr=i_destinazione_attr;
	}
	else
	{
		/// houston, we have to make ourself
		int64_t start_destinazione_esiste=mtime();
		destinazione_esiste	=getfileinfo(i_outfilename,destinazione_dimensione,destinazione_data,destinazione_attr);
		g_robocopy_check_destinazione+=mtime()-start_destinazione_esiste;
	}
	
	if (flagdebug)
	{
		printf("Sorgente     esiste  %d\n",(int)sorgente_esiste);
		printf("Sorgente     size    %s\n",migliaia(sorgente_dimensione));
		printf("Sorgente     data    %s\n",migliaia(sorgente_data));
		printf("Destinazione esiste  %d\n",(int)destinazione_esiste);
		printf("Destinazione size    %s\n",migliaia(destinazione_dimensione));
		printf("Destinazione data    %s\n",migliaia(destinazione_data));
		printf("\n");
	}
	if (destinazione_esiste)
	{
		if (flagdebug)
			printf("Esiste1\n");
			
		if (sorgente_esiste)
		{
			if (flagdebug)
			{
				printf("Esiste2\n");
			}
				if (destinazione_dimensione==sorgente_dimensione)
				{
					if (flagdebug)
						printf("Stessa dimensione\n");
					if (flagdebug)
					{
						printf("PPP %s\n",migliaia(myabs(destinazione_data,sorgente_data)));
					}
				/// this 2 is really important: it is the modulo-differences
				/// 1 or even 0 is not good for NTFS or Windows
				
					if (myabs(destinazione_data,sorgente_data)<=2)
					{
						if (flagdebug)
							printf("Stessa data\n");
						if (flagkill)
							if (sorgente_attr!=destinazione_attr)
								close(i_outfilename.c_str(),sorgente_data,sorgente_attr);
					
						o_donesize+=sorgente_dimensione;
						o_donecount++;
						if (o_donecount%1000==0)
							printf("File %10s/%10s (%9s/%9s)\r",migliaia(o_donecount),migliaia2(i_totalcount),tohuman(o_donesize),tohuman2(i_totalsize));
						return "=";
					}
				}
		}
		if (flagdebug)
			printf("Cancellerei\n");
		if (flagkill)
			delete_file(i_outfilename.c_str());
	}
	
	if (!flagkill)
		return "OK";
		
	size_t const blockSize = 65536;
	unsigned char buffer[blockSize];
	FILE* inFile = freadopen(i_filename.c_str());

	if (inFile==NULL) 
	{
#ifdef _WIN32
		int err=GetLastError();
#else
		int err=1;
#endif
		printf("\nERR <%s> kind %d\n",i_filename.c_str(),err); 

		return "KAPUTT";
	}
	
#ifdef _WIN32
	wstring widename=utow(i_outfilename.c_str());
	FILE* outFile=_wfopen(widename.c_str(), L"wb" );
#else
	FILE* outFile=fopen(i_outfilename.c_str(), "wb");
#endif

	if (outFile==NULL) 
		return "30847:CANNOT OPEN outfile "+i_outfilename;
	
	size_t readSize;
	
	while ((readSize = fread(buffer, 1, blockSize, inFile)) > 0) 
	{
		o_donesize+=fwrite(buffer,1,readSize,outFile);
		myavanzamento(o_donesize,i_totalsize,i_startcopy);
	}
	fclose(inFile);
	fclose(outFile);
	
/// note: this is a "touch" for the attr
	close(i_outfilename.c_str(),sorgente_data,sorgente_attr);
	
	o_donecount++;
	
	return "OK";
	
}
/*
void stermina(string i_directory)
{
	assert					(sizeof(wchar_t)==2);
	SHFILEOPSTRUCTW fileop	={0};
    wstring wide			=utow(i_directory.c_str());
	const wchar_t* wcs 		=wide.c_str();
	fileop.wFunc			=FO_DELETE;
	fileop.pFrom			=wcs;
	fileop.fFlags			=FOF_NO_UI;
	SHFileOperation			(&fileop);
}
*/

/// risky command to make a rd folder /s (or rm -r)
int erredbarras(const std::string &i_path,bool i_flagrecursive=true)
{
#ifdef unix
///		bool	flagdebug=true;
		bool 	risultato=false;

		DIR *d=opendir(i_path.c_str());

		if (d) 
		{
			struct dirent *p;
			risultato=true;
			while (risultato && (p=readdir(d))) 
			{
				if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
					continue;
				bool risultato2=false;
				struct stat statbuf;
				
				std::string temp;
				if (isdirectory(i_path))
					temp=i_path+p->d_name;
				else
					temp=i_path+"/"+p->d_name;

				if (!stat(temp.c_str(), &statbuf)) 
				{
					if (S_ISDIR(statbuf.st_mode))
						risultato2=erredbarras(temp);
					else
					{
						if (flagdebug)
							printf("Delete file %s\n",temp.c_str());
						risultato2=delete_file(temp.c_str());
					}
				}
				risultato=risultato2;
			}
			closedir(d);
		}

		if (risultato)
		{
			if (flagdebug)
				printf("Delete dir  %s\n\n",i_path.c_str());
			delete_dir(i_path.c_str());
		}
	   return risultato;	
#endif

#ifdef _WIN32

///	bool	flagdebug=true;
	bool	flagsubdir=false;
	HANDLE	myhandle;
	std::wstring wfilepath;
	WIN32_FIND_DATA findfiledata;

	std::string pattern=i_path+"\\*.*";
  
	std::wstring wpattern	=utow(pattern.c_str());
	std::wstring wi_path	=utow(i_path.c_str());

	myhandle=FindFirstFile(wpattern.c_str(),&findfiledata);
  
	if (myhandle!=INVALID_HANDLE_VALUE)
	{
		do
		{
			std::string t=wtou(findfiledata.cFileName);
			
			if ((t!=".") && (t!=".."))
			{
				wfilepath=wi_path+L"\\"+findfiledata.cFileName;
		        if (findfiledata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (i_flagrecursive)
					{
						const std::string temp(wfilepath.begin(),wfilepath.end());
						if (flagdebug)
							printf("\n\nDelete directory   %s\n",temp.c_str());
						int myresult=erredbarras(temp,i_flagrecursive);
						if (myresult)
							return myresult;
					}
					else
						flagsubdir=true;
				}
				else
				{
					const std::string ttemp(wfilepath.begin(), wfilepath.end() );
					if (flagdebug)
						printf("Try to delete file %s\n",ttemp.c_str());

					if (SetFileAttributes(wfilepath.c_str(),FILE_ATTRIBUTE_NORMAL) == FALSE)
					{
						if (flagdebug)
							printf("31019: ERROR cannot change attr of file %s\n",ttemp.c_str());
						return GetLastError();
					}
					
					if (DeleteFile(wfilepath.c_str())==FALSE)
					{
						if (flagdebug)
							printf("31025: ERROR highlander file %s\n",ttemp.c_str());
						return GetLastError();
					}
				}
			}
		} while(FindNextFile(myhandle,&findfiledata)==TRUE);

		FindClose(myhandle);

		DWORD myerror=GetLastError();
		if (myerror==ERROR_NO_MORE_FILES)
		{
			if (!flagsubdir)
			{
				const std::string dtemp(wi_path.begin(), wi_path.end());
				
				if (flagdebug)
					printf("Delete no subdir   %s\n",dtemp.c_str());
							
				if (SetFileAttributes(wi_path.c_str(),FILE_ATTRIBUTE_NORMAL)==FALSE)
				{
					if (flagdebug)
						printf("30135: ERROR cannot change folder attr %s\n",dtemp.c_str());
					return GetLastError();
				}
								
				if (RemoveDirectory(wi_path.c_str())==FALSE)
				{
					if (flagdebug)
						printf("31047: ERROR highlander dir %s\n",dtemp.c_str());
					return GetLastError();
				}
			}
		}
		else
			return myerror;
	}
#endif
	return 0;
}

/// find and delete 0-length dirs
/// in a slow (but hopefully) safe way

int Jidac::zero()
{
	printf("*** Delete empty folders (zero length) *** ");
	if (!flagkill)
		printf("*** -kill missing: dry run *** ");
	if (!flagforcezfs)
		printf("*** ignoring .zfs and :$DATA ***");
	printf("\n");
	
	franzparallelscandir(false);
	
	vector<string> scannedfiles;
	
	for (unsigned i=0; i<files.size(); ++i)
		for (DTMap::iterator p=files_edt[i].begin(); p!=files_edt[i].end(); ++p) 
			scannedfiles.push_back(p->first);
	
	vector<string> tobedeleted;
	
	int candidati=0;
	for (unsigned int i=0;i<scannedfiles.size();i++)
	{
		if (i<scannedfiles.size())
			if (isdirectory(scannedfiles[i]))
				{
					string corrente=scannedfiles[i];
					bool flagfigli=false;
					for (unsigned int j=i+1;j<scannedfiles.size();j++)
					{
						string prossima=extractfilepath(scannedfiles[j]);
						string nomefile=extractfilename(scannedfiles[j]);
				/// we count everything, even Thumbs.db as "non empty dirs"
				///		if ((nomefile!="Thumbs.db"))
						{
							if (mypos(corrente,prossima)!=0)
							{
			///				printf("---> esco prossima non  corrente |%s| |%s|\n",prossima.c_str(),corrente.c_str());
								break;
							}
							else
							{
								if (!isdirectory(scannedfiles[j]))
								{
				///				printf("Figliuolo!! %s\n",scannedfiles[j].c_str());
									flagfigli=true;
									break;
								}
							}
						}
					}
					if (flagfigli==0)
						tobedeleted.push_back(scannedfiles[i]);
				}
	}
	candidati=tobedeleted.size();
	
	printf("\n");
	printf("Candidate (empty dirs) %12s\n",migliaia(candidati));
	if (candidati==0)
	{
		printf("Nothing to do\n");
		return 0;
	}
	if (flagverbose)
	{
		printbar('-');
		for (unsigned int i=0;i<tobedeleted.size();i++)
		{
			printf("TO BE DELETED <<");
			printUTF8(tobedeleted[i].c_str());
			printf(">>\n");
		}
		printbar('-');
	}
	
	if (!flagkill)
	{
		printf("dry run, exiting (no -kill)\n");
		return 0;
	}	

	int lastrun=tobedeleted.size();
	int newrun=0;
	int64_t startdelete=mtime();
	int runs=0;
///	OK we want to be safe, so we iterate as many times as possible
/// to use delete_dir() instead of "erredibarras".
/// So only "really" empty will be deleted.
/// Not fast at all but more safe.

	while (lastrun>newrun)
	{
		runs++;
		for (unsigned int i=0;i<tobedeleted.size();i++)
			if (delete_dir(tobedeleted[i].c_str()))
				tobedeleted.erase(tobedeleted.begin()+i);
		newrun=tobedeleted.size();
		if (flagdebug)
			printf("lastrun - newrun %d %d\n",lastrun,newrun);
		if (lastrun>newrun)
		{
			lastrun=newrun;
			newrun=0;
		}
	}
	
	printf("Delete time %9.2f sec, runs %d\n",(mtime()-startdelete)/1000.0,runs);
	printf("Survived (not deleted) %12s\n",migliaia(tobedeleted.size()));
	if (tobedeleted.size()>0)
		if (flagverbose)
		{
			for (unsigned int i=0;i<tobedeleted.size();i++)
			{
				printf("Highlander <<");
				printUTF8(tobedeleted[i].c_str());
				printf(">>\n");
			}
		}
	if (tobedeleted.size()==0)
		return 0;
	else
		return 1;
}	


/// robocopy /mir a master folder to one or more

int Jidac::robocopy()
{
	int risultato=0;
	
	printf("*** ROBOCOPY MODE *** ");
	if (!flagkill)
		printf("*** -kill missing: dry run *** ");
	if (!flagforcezfs)
		printf("*** ignoring .zfs and :$DATA ***");
	printf("\n");
	
	franzparallelscandir(false);
	
	int64_t 	startscan=mtime();
	uint64_t 	strangethings;
	
	printf("\nMaster  %s (%s files %s) <<%s>>\n",migliaia(files_size[0]),tohuman(files_size[0]),migliaia2(files_count[0]),files[0].c_str());
	printbar('-');
	
	int64_t total_size				=files_size[0];
	int64_t total_count				=files_count[0];
	int64_t done_size				=0;
	int64_t done_count				=0;
	int robocopied					=0;
	uint64_t robocopiedsize			=0;
	int robodeleted					=0;
	uint64_t robodeletedsize		=0;
	int roboequal					=0;
	uint64_t roboequalsize			=0;
	int64_t timecopy				=0;
	int64_t timedelete				=0;

	g_robocopy_check_sorgente		=0;
	g_robocopy_check_destinazione	=0;

///	we start by 1 (the first slave); 0 is the master
	for (unsigned i=1; i<files.size(); ++i)
	{
		strangethings=0;
		if (flagkill)
		{
			if (isdirectory(files[i]))
			{
				if (!exists(files[i]))
				{
					makepath(files[i]);
				}
			}
		}
		if (!exists(files[i]))
		{
			if (!flagkill)
			{
				printf("Dir %d (slavez) DOES NOT EXISTS %s\n",i,files[i].c_str());
				risultato=1;
			}
		}
		else
		{
		///	first stage: delete everything in slave-i that is NOT i master-0
			int64_t startdelete=mtime();
			for (DTMap::iterator p=files_edt[i].begin(); p!=files_edt[i].end(); ++p) 
			{
				string filenamei=p->first;
				string filename0=filenamei;
				myreplace(filename0,files[i],files[0]);
				DTMap::iterator cerca=files_edt[0].find(filename0);
				if  (cerca==files_edt[0].end())
				{
					if (flagdebug)
						printf("Delete %s\n",p->first.c_str());
					if (!flagkill)
					{
							//fake: dry run
						robodeleted++;
						robodeletedsize+=p->second.size;
					}
					else
					{
						string temp=p->first;
						bool riuscito=true;
						if (isdirectory(temp))
						{
							erredbarras(temp,true); // risky!!
							riuscito=delete_dir(temp.c_str())==0;
						}
						else
						{ 
						/// delete without mercy!
							riuscito=delete_file(temp.c_str());
						}
						if (!riuscito)
							printf("31293: ERROR DELETING  <%s>\n",p->first.c_str());
						else
						{
							robodeleted++;
							robodeletedsize+=p->second.size;
						}
					}
				}
				if (menoenne>0)
					if (strangethings>menoenne)
					{
						printf("30146: **** TOO MANY STRANGE THINGS (-n %d)  %s\n",menoenne,migliaia(strangethings));
						break;
					}
			}	
			/// OK now do the copy from master-0 to slave-i
			timedelete=mtime()-startdelete;
			int64_t globalstartcopy=mtime();
			
			for (DTMap::iterator p=files_edt[0].begin(); p!=files_edt[0].end(); ++p) 
			{
				string filename0=p->first;
				string filenamei=filename0;
				myreplace(filenamei,files[0],files[i]);
				DTMap::iterator cerca=files_edt[i].find(filenamei);
					
				int64_t dest_size=-1;
				int64_t dest_date=-1;
				int64_t dest_attr=-1;
				if  (cerca==files_edt[i].end())
				{
					/// the file does not exists, maintain default -1 (=>secure_copy_file do yourself)
					///printf("NON ! %s\n",filenamei.c_str());
				}
				else
				{
				/// the destination exists, get data from the scanned-data
					dest_size=cerca->second.size;
					dest_date=cerca->second.date;
					dest_attr=cerca->second.attr;
				}

					int64_t startcopy=mtime();
					string copyfileresult=secure_copy_file(
					filename0,filenamei,globalstartcopy,total_size*(files.size()-1),total_count*(files.size()-1),done_size,done_count,
					p->second.size,
					p->second.date,
					p->second.attr,
					dest_size,
					dest_date,
					dest_attr);
					/// the return code can be OK (file copied) or = (file not copied because it is ==)
					if ((copyfileresult!="OK") && (copyfileresult!="="))
						printf("31170: error robocoping data  <%s> to %s\n",copyfileresult.c_str(),filenamei.c_str());
					else
					{
						if (!isdirectory(filename0))
						{
							if (copyfileresult=="OK")
							{
								robocopiedsize+=p->second.size;
								robocopied++;
								timecopy+=mtime()-startcopy;
							}
							else
							{
								roboequalsize+=p->second.size;
								roboequal++;
							}
						}
					}
								
				if (menoenne>0)
					if (strangethings>menoenne)
					{
						printf("30125: **** TOO MANY STRANGE THINGS (-n %d)  %s\n",menoenne,migliaia(strangethings));
						break;
					}
			}
		}
	}	

	timecopy++; //avoid some div by zero
	timedelete++;
	printf("\n");
	if (!flagkill)
		printf("FAKE: dry run!\n");
	printf("=  %12s %20s B\n",migliaia(roboequal),migliaia2(roboequalsize));
	printf("+  %12s %20s B in %9.2fs %15s/sec\n",migliaia(robocopied),migliaia2(robocopiedsize),(timecopy/1000.0),migliaia3(robocopiedsize/(timecopy/1000.0)));
	printf("-  %12s %20s B in %9.2fs %15s/sec\n",migliaia(robodeleted),migliaia2(robodeletedsize),(timedelete/1000.0),migliaia3(robodeletedsize/(timedelete/1000.0)));

	printf("\nRobocopy time %9.2f s\n",((mtime()-startscan)/1000.0));
	printf("Get slaves    %9.2f s\n\n",g_robocopy_check_destinazione/1000.0);
	
	if (not flagverify)
		return risultato;
		
	printf("*** ROBOCOPY: do a (-verify) ");
	if (!flagforcezfs)
		printf(" ignoring .zfs and :$DATA");
	printf("\n");
	return dircompare(false,true);
}

/// do a s (get size) or c (compare). The second flag is for output when called by robocopy
int Jidac::dircompare(bool i_flagonlysize,bool i_flagrobocopy)
{
	int risultato=0;
	
	if (i_flagonlysize)
		printf("Get directory size ");
	else
	{
		if (!i_flagrobocopy)
			printf("Dir compare (%s dirs to be checked) ",migliaia(files.size()));
		if (files.size()<2)
			error("28212: At least two directories required\n");
	}
	
	if (!flagforcezfs)
			printf(" ignoring .zfs and :$DATA\n");

	franzparallelscandir(flagchecksum);
	
	printbar('=');
	for (unsigned i=0; i<files.size(); ++i)
	{
		int64_t spazio=getfreespace(files[i].c_str());
		if (spazio<0)
			spazio=0;
		printf("Free %d %21s (%12s)  <<%s>>\n",i,migliaia(spazio),tohuman(spazio),files[i].c_str());
	}

	uint64_t total_size	=0;
	uint64_t total_files=0;
	int64_t delta_size	=0;
	int64_t delta_files	=0;
	
	printbar('=');
	for (unsigned int i=0;i<files_size.size();i++)
	{
		if (i==0)
			printf("Dir  %d %21s |%12s| |%12s| %9.2f <<%s>>\n",i,migliaia(files_size[i]),"Delta bytes",									 migliaia3(files_count[i]),files_time[i]/1000.0,files[i].c_str());
		else
		{
			printf("Dir  %d %21s |%12s| |%12s| %9.2f <<%s>>\n",i,migliaia(files_size[i]),tohuman(myabs(files_size[0],files_size[i])),migliaia3(files_count[i]),files_time[i]/1000.0,files[i].c_str());
			delta_size	+= myabs(files_size[0],files_size[i]);
			delta_files	+= myabs(files_count[0],files_count[i]);
		}
		total_size	+=files_size[i];
		total_files	+=files_count[i];
	}
	printbar('=');
	printf("Total  |%21s| (%s)\n",migliaia(total_size),tohuman(total_size));
	printf("Delta  |%21s| %21s|files\n",migliaia(delta_size),migliaia2(delta_files));
	
/// only a s (size)? Done	
	if (i_flagonlysize)
		return 0;
	
	uint64_t strangethings;
	printf("\nDir 0 (master) %s (files %s) <<%s>>\n",migliaia(files_size[0]),migliaia2(files_count[0]),files[0].c_str());
	printbar('-');
	
	bool flagerror=false;

/// check from 1 (the first slave)
	for (unsigned i=1; i<files.size(); ++i)
	{
		strangethings=0;
		if (!exists(files[i]))
		{
			printf("Dir %d (slave1) DOES NOT EXISTS %s\n",i,files[i].c_str());
			risultato=1;
		}
		else
		{
			if ((files_size[i]!=files_size[0]) || (files_count[i]!=files_count[0]))
			{
				printf("Dir %d (slave) IS DIFFERENT time %6g <<%s>>\n",i,files_time[i]/1000.0,files[i].c_str());
				printf("size  %24s (files %s)\n",migliaia(files_size[i]),migliaia2(files_count[i]));
			}
				
			for (DTMap::iterator p=files_edt[0].begin(); p!=files_edt[0].end(); ++p) 
			{
				string filename0=p->first;
				string filenamei=filename0;
				myreplace(filenamei,files[0],files[i]);
				DTMap::iterator cerca=files_edt[i].find(filenamei);
				if  (cerca==files_edt[i].end())
				{
					printf("missing (not in %d) ",i);
					printUTF8(filename0.c_str());
					printf("\n");
					flagerror=true;
					strangethings++;
				}
				if (menoenne>0)
					if (strangethings>menoenne)
					{
						printf("30125: **** TOO MANY STRANGE THINGS (-n %d)  %s\n",menoenne,migliaia(strangethings));
						break;
					}
			}
			for (DTMap::iterator p=files_edt[i].begin(); p!=files_edt[i].end(); ++p) 
			{
				string filenamei=p->first;
				string filename0=filenamei;
				myreplace(filename0,files[i],files[0]);
				DTMap::iterator cerca=files_edt[0].find(filename0);
				if  (cerca==files_edt[0].end())
				{
					printf("excess  (not in 0) ");
					printUTF8(filename0.c_str());
					printf("\n");
					flagerror=true;
					strangethings++;
				}
				if (menoenne>0)
					if (strangethings>menoenne)
					{
						printf("30146: **** TOO MANY STRANGE THINGS (-n %d)  %s\n",menoenne,migliaia(strangethings));
						break;
					}
			}	
			for (DTMap::iterator p=files_edt[0].begin(); p!=files_edt[0].end(); ++p) 
			{
				string filename0=p->first;
				string filenamei=filename0;
				myreplace(filenamei,files[0],files[i]);
				DTMap::iterator cerca=files_edt[i].find(filenamei);
				if  (cerca!=files_edt[i].end())
				{
					if (p->second.size!=cerca->second.size)
					{
						printf("diff size  %18s (0) %18s (%d) %s\n",migliaia(p->second.size),migliaia2(cerca->second.size),i,filename0.c_str());
						flagerror=true;
						strangethings++;
					}
					else
					if (flagchecksum)
					{
						if (p->second.hexhash!=cerca->second.hexhash)
						{
							printf("%s different %s (0) vs %s (%d) ",mygetalgo().c_str(),p->second.hexhash.c_str(),cerca->second.hexhash.c_str(),i);
							printUTF8(filename0.c_str());
							printf("\n");
							flagerror=true;
							strangethings++;
						}
					}
				}
				if (menoenne>0)
					if (strangethings>menoenne)
					{
						printf("30146: **** TOO MANY STRANGE THINGS (-n %d)  %s\n",menoenne,migliaia(strangethings));
						break;
					}
			}
			if (!flagerror)
				if ((files_size[i]==files_size[0]) && (files_count[i]==files_count[0]))
				{
					printf("== <<");
					printUTF8(files[i].c_str());	
					printf(">>\n");
				}
			printbar('-');
		}
	}	
	if (!flagerror)
	{
		if (flagchecksum)
			printf("NO diff FOR SURE in slave dirs (checksum check)\n");
		else
			printf("NO diff in slave dirs (fast check, only size)\n");
	}
	else
		printf("DIFFERENT SLAVE DIRS!!\n");
	
	return risultato;
}


/// scans one or more directories, with one or more threads
/// return total time
int64_t Jidac::franzparallelscandir(bool i_flaghash)
{

	if (flagverbose)
		flagnoeta=true;

	files_edt.clear();
	files_size.clear();
	files_count.clear();
	files_time.clear();
	g_arraybytescanned.clear();
	g_arrayfilescanned.clear();

	if (files.size()==0)
	{
		if (flagdebug)
			printf("28403: files.size zero\n");
		return 0;
	}
	
	for (unsigned i=0; i<files.size(); ++i)
		if (!isdirectory(files[i]))
			files[i]+='/';

	int64_t startscan=mtime();
	
	int quantifiles			=0;
	g_bytescanned			=0;
	g_worked				=0;
			
	if (all)	// OK let's make different threads
	{
		vector<tparametri> 	vettoreparametri;
		vector<DTMap>		vettoreDT;
		
		tparametri 	myblock;
		DTMap		mydtmap;
	
		for (unsigned int i=0;i<files.size();i++)
		{
			vettoreDT.push_back(mydtmap);
			myblock.tnumber=i;
			myblock.directorytobescanned=files[i];
			myblock.theDT=vettoreDT[i];
			myblock.flagcalchash=i_flaghash;//flagverify;
			myblock.timestart=mtime();
			vettoreparametri.push_back(myblock);
		}
	
		int rc;
		pthread_t* threads = new pthread_t[files.size()];
		pthread_attr_t attr;
		void *status;

		// ini and set thread joinable
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

		printf("Creating %s scan threads\n",migliaia(files.size()));
			
		if (!flagnoeta)
		{
			setupConsole();
			printf("\e[?25l");	
			fflush(stdout);
			restoreConsole();
		}
		for(unsigned int i = 0; i < files.size(); i++ ) 
		{
			print_datetime();
			printf("Scan dir |%02d| <<%s>>\n",i,files[i].c_str());
		}
		for(unsigned int i = 0; i < files.size(); i++ ) 
		{
			g_arraybytescanned.push_back(0);
			g_arrayfilescanned.push_back(0);
			
			rc = pthread_create(&threads[i], &attr, scansiona, (void*)&vettoreparametri[i]);
			if (rc) 
			{
				printf("Error creating thread\n");
				exit(-1);
			}
		}
		printf("\n");

		// free attribute and wait for the other threads
		pthread_attr_destroy(&attr);
		for(unsigned int i = 0; i < files.size(); i++ ) 
		{
			rc = pthread_join(threads[i], &status);
			if (rc) 
			{
				error("Unable to join\n");
				exit(-1);
			}
			///printf("Thread completed %d status %d\n",i,status);
		}
		if (!flagnoeta)
		{
			setupConsole();
			printf("\033[%dB",(int)g_arraybytescanned.size());
			printf("\e[?25h");
			fflush(stdout);
			restoreConsole();
		}
		printf("\nParallel scan ended in %f s\n",(mtime()-startscan)/1000.0);
	
		for (unsigned i=0; i<files.size(); ++i)
		{
			uint64_t sizeofdir=0;
			uint64_t dircount=0;
			for (DTMap::iterator p=vettoreparametri[i].theDT.begin(); p!=vettoreparametri[i].theDT.end(); ++p) 
			{
				string filename=rename(p->first);
				if (p->second.date && p->first!="" && (!isdirectory(p->first)) && (!isads(filename))) 
				{
					sizeofdir+=p->second.size;
					dircount++;
					quantifiles++;
				}
				if (flagdebug)
				{
					printUTF8(filename.c_str());
					printf("\n");
				}
			}
			files_edt.push_back(vettoreparametri[i].theDT);
			files_size.push_back(sizeofdir);
			files_count.push_back(dircount);
			files_time.push_back(vettoreparametri[i].timeend-vettoreparametri[i].timestart+1);
		}
		delete[] threads;
	
	}
	else
	{	// single thread. Do a sequential scan

		g_arraybytescanned.push_back(0);
		g_arrayfilescanned.push_back(0);
	
		for (unsigned i=0; i<files.size(); ++i)
		{
			DTMap myblock;
			print_datetime();
			printf("Scan dir <<%s>>\n",files[i].c_str());
			uint64_t blockstart=mtime();
			myscandir(0,myblock,files[i].c_str(),true,i_flaghash);
			
			uint64_t sizeofdir=0;
			uint64_t dircount=0;
		
			for (DTMap::iterator p=myblock.begin(); p!=myblock.end(); ++p) 
			{
				string filename=rename(p->first);
				if (p->second.date && p->first!="" && (!isdirectory(p->first)) && (!isads(filename))) 
				{
					sizeofdir+=p->second.size;
					dircount++;
					quantifiles++;
				}
				if (flagdebug)
				{
					printUTF8(filename.c_str());
					printf("\n");
				}
			}
			files_edt.push_back(myblock);
			files_size.push_back(sizeofdir);
			files_count.push_back(dircount);
			files_time.push_back(mtime()-blockstart);
		}
	}
	return mtime()-startscan;
}	


/// wipe free space-check if can write and read OK
int Jidac::fillami() 
{

#ifdef ESX
	printf("GURU: sorry: ESXi does not like this things\n");
	exit(0);
#endif
	if (files.size()!=1)
	{
		printf("FILL: exactly one directory\n");
		return 2;
	}
	if (!isdirectory(files[0]))
	{
		printf("FILL: you need to specify a directory, not a file (last bar)\n");
		return 2;
	}
	moreprint("");
	moreprint("Almost all free space will be filled by pseudorandom 512MB files,");
	moreprint("then checked from the ztempdir-created folder (2GB+ needed).");
	moreprint("");
	moreprint("These activities can reduce the media life,");
	moreprint("especially for solid state drives (SSDs) and if repeated several times.");
	moreprint("");
	if (!flagkill)
	{
		moreprint("*** Temporary files are NOT deleted (no -kill, to enforce zfs's scrub) ***");
		moreprint("");
	}
	
	if (!getcaptcha("ok","Fill (wipe) free space"))
				return 1;
	
	string outputdir=files[0]+"ztempdir/";
	makepath(outputdir);
	if (!direxists(outputdir))
	{
		printf("\n28599: impossible to makepath %s\n",outputdir.c_str());
		return 2;
	}
	unsigned int percent=99;
	
	int64_t spazio=getfreespace(outputdir.c_str());
	printf("Free space %12s (%s) <<%s>>\n",migliaia(spazio),tohuman(spazio),outputdir.c_str());
	if (spazio<600000000)
	{
		printf("28607: less than 600.000.000 bytes free on %s\n",outputdir.c_str());
		return 2;
	}
	
	uint64_t spacetowrite=spazio*percent/100;
	printf("To write   %12s (%s) %d percent\n",migliaia(spacetowrite),tohuman(spacetowrite),percent);

	uint32_t chunksize=(2<<28)/sizeof(uint32_t); //half gigabyte in 32 bits at time
	int chunks=spacetowrite/(chunksize*sizeof(uint32_t));
	chunks--; // just to be sure
	if (chunks<=0)
	{
		printf("Abort: there is something strange on free space (2GB+)\n");
		return 1;
	}
	printf("%d chuks of (%s) will be written\n",chunks,tohuman(chunksize*sizeof(uint32_t)));
		
	uint32_t *buffer32bit = (uint32_t*)malloc(chunksize*sizeof(uint32_t));
	if (buffer32bit==0)
	{
		printf("28628: GURU cannot alloc the buffer\n");
		return 2;
	}
	uint64_t starttutto=mtime();	
	uint64_t hashtime=0;
	uint64_t totaliotime=0;
	uint64_t totalhashtime=0;
	uint64_t totalrandtime=0;
		
	vector<string> chunkfilename;     
	vector<string> chunkhash;     
	
	char mynomefile[outputdir.size()+100];
	for (int i=0;i<chunks;i++)
	{
		/// pseudorandom population (not cryptographic-level, but enough)
		uint64_t startrandom=mtime();
		populateRandom_xorshift128plus(buffer32bit, chunksize,324+i,4444+i);
		uint64_t randtime=mtime()-startrandom;
		
		/// get XXH3, fast and reliable (not cryptographic-level, but enough)
		uint64_t starthash=mtime();
		XXH3_state_t state128;
		(void)XXH3_128bits_reset(&state128);
		(void)XXH3_128bits_update(&state128, buffer32bit, chunksize*4);
		XXH128_hash_t myhash=XXH3_128bits_digest(&state128);
		char risultato[33];
		sprintf(risultato,"%016llX%016llX",(unsigned long long)myhash.high64,(unsigned long long)myhash.low64);
		chunkhash.push_back(risultato);
		hashtime=mtime()-starthash;
		
		sprintf(mynomefile,"%szchunk_%05d_$%s",outputdir.c_str(),i,risultato);
		chunkfilename.push_back(mynomefile);
		double percentuale=(double)i/(double)chunks*100.0;
		if (i==0)
			percentuale=0;
			
		printf("%03d%% ",(int)percentuale);
		
		uint64_t startio=mtime();
		FILE* myfile=fopen(mynomefile, "wb");
		fwrite(buffer32bit, sizeof(uint32_t), chunksize, myfile);
		fclose(myfile);			
		uint64_t iotime=mtime()-startio;
		
		
		uint64_t randspeed=	(chunksize*sizeof(uint32_t)/((randtime+1)/1000.0));
		uint64_t hashspeed=	(chunksize*sizeof(uint32_t)/((hashtime+1)/1000.0));
		uint64_t iospeed=	(chunksize*sizeof(uint32_t)/((iotime+1)/1000.0));
		
		double trascorso=(mtime()-starttutto+1)/1000.0;
		double eta=((double)trascorso*(double)chunks/(double)i)-trascorso;
		if (i==0)
			eta=0;

		if (eta<356000)
		{
			printf("ETA %0d:%02d:%02d",int(eta/3600), int(eta/60)%60, int(eta)%60);
			printf(" todo (%10s) rnd (%10s/s) hash (%10s/s) W (%10s/s)",
			tohuman(sizeof(uint32_t)*uint64_t(chunksize)*(uint64_t)(chunks-i)),
			tohuman2(randspeed),
			tohuman3(hashspeed),
			tohuman4(iospeed));
			if (flagverbose)
				printf("\n");
			else
				printf("\r");
		}
		totaliotime		+=iotime;
		totalrandtime	+=randtime;
		totalhashtime	+=hashtime;
		
	}
	uint64_t timetutto=mtime()-starttutto;
	printf("Total time %f  rnd %f  hash %f  write %f\n",timetutto/1000.0,totalrandtime/1000.0,totalhashtime/1000.0,totaliotime/1000.0);
	free(buffer32bit);
	
	if (chunkfilename.size()!=chunkhash.size())
	{
		printf("Guru 23925: filename size != hash size\n");
		return 2;
	}
	if (chunkfilename.size()!=(unsigned int)chunks)
	{
		printf("Abort: expecting %d chunks but %d founded\n",chunks,(unsigned int)chunkfilename.size());
		return 2;
	}
	
	printf("******* VERIFY\n");
	
	g_bytescanned=0;
	g_filescanned=0;
	g_worked=0;
	edt.clear();
	int64_t lavorati=0;
	bool flagallok=true;
	
	uint64_t starverify=mtime();
	
	for (int unsigned i=0;i<chunkfilename.size();i++)
	{
		string filename=chunkfilename[i];
		printf("Reading chunk %05d ",i);
		
		uint64_t dummybasso64;
		uint64_t dummyalto64;
		uint64_t starthash=mtime();
		string filehash=xxhash_calc_file(filename.c_str(),-1,-1,lavorati,dummybasso64,dummyalto64);
		uint64_t hashspeed=chunksize/((mtime()-starthash+1)/1000.0);
		printf(" (%s/s) ",tohuman(hashspeed));

		bool flagerrore=(filehash!=chunkhash[i]);
			
		if (flagerrore)
		{
			printf("ERROR\n");
			flagallok=false;
		}
		else
		{
			printf("OK %s",chunkhash[i].c_str());
			if (flagverbose)
				printf("\n");
			else
				printf("\r");
		}
	}

	printf("\n");
	uint64_t verifytime=mtime()-starverify;
	printf("Verify time %f (%10s) speed (%10s/s)\n",verifytime/1000.0,tohuman(lavorati),tohuman2(lavorati/(verifytime/1000.0)));
		
	if (flagallok)
	{
		printf("+OK all OK\n");
		if (flagkill)
		{
			for (int unsigned i=0;i<chunkfilename.size();i++)
			{
				delete_file(chunkfilename[i].c_str());
				printf("Deleting tempfile %05d / %05d\r",i,(unsigned int)chunkfilename.size());
			}
			delete_dir(outputdir.c_str());
			printf("\n");
		}
		else
		{
			printf("REMEMBER: temp file in %s\n",outputdir.c_str());
		}
	}	
	else
		printf("ERROR: SOMETHING WRONG\n");
	return 0;
}
/*
clang++ -march=native -Dunix zpaqfranz.cpp -pthread -o zclang2 -static -ffunction-sections -fdata-sections -Wl,--gc-sections,--print-gc-section > & 1.txt
*/

int  Jidac::dir() 
{
	bool barras	=false;
	bool barraos=false;
	bool barraod=false;
	bool barraa	=false;
	
/*esx*/

	if (files.size()==0)
		files.push_back(".");
	else
	if (files[0].front()=='/')
		files.insert(files.begin(),1, ".");


	string cartella=files[0];
	
	if	(!isdirectory(cartella))
		cartella+='/';
		
	if (files.size()>1)
		for (unsigned i=0; i<files.size(); i++) 
		{
				barras	|=(stringcomparei(files[i],"/s"));
				barraos	|=(stringcomparei(files[i],"/os"));
				barraod	|=(stringcomparei(files[i],"/od"));
				barraa	|=(stringcomparei(files[i],"/a"));
		}
	
	printf("==== Scanning dir <<");
	printUTF8(cartella.c_str());
	printf(">> ");
		
	if (barras)
		printf(" /s");
	if (barraos)
		printf(" /os");
	if (barraod)
		printf(" /od");
	if (barraa)
		printf(" /a");
			
	printf("\n");

	bool		flagduplicati	=false;
	int64_t 	total_size		=0;
	int 		quantifiles		=0;
	int 		quantedirectory	=0;
	int64_t		dummylavorati	=0;
	int			quantihash		=0;
	int64_t		hash_calcolati	=0;
	uint64_t	iniziohash		=0;
	uint64_t	finehash			=0;
		
	flagduplicati=(flagcrc32 || flagcrc32c ||flagxxhash || flagxxhash || flagsha256);
			
	g_bytescanned=0;
	g_filescanned=0;
	g_worked=0;
	scandir(true,edt,cartella,barras);
	
	vector <s_fileandsize> fileandsize;
	
	for (DTMap::iterator p=edt.begin(); p!=edt.end(); ++p) 
	{
		bool flagaggiungi=true;

#ifdef _WIN32	
		if (!barraa)
			flagaggiungi=((int)attrToString(p->second.attr).find("H") < 0);
		flagaggiungi &= (!isads(p->first));
#endif
		
		if (searchfrom!="")
			flagaggiungi &= (stristr(p->first.c_str(),searchfrom.c_str())!=0);
	
		if (flagaggiungi)
		{
			s_fileandsize myblock;
			myblock.filename=p->first;
			myblock.size=p->second.size;
			myblock.attr=p->second.attr;
			myblock.date=p->second.date;
			myblock.isdir=isdirectory(p->first);
			myblock.flaghashstored=false;
				
			if (flagduplicati)
			{
				if (!myblock.isdir)
				{
					if (minsize)
					{
						if (myblock.size>minsize)
							fileandsize.push_back(myblock);
					}
					else
					if (maxsize)
					{
						if (myblock.size<minsize)
							fileandsize.push_back(myblock);
					}
					else
						fileandsize.push_back(myblock);
					
				}
			}
			else
				fileandsize.push_back(myblock);
		}
	}
		
	if (flagduplicati)
	{
		if (flagdebug)
		{
			printf("Pre-sort\n");
			for (unsigned int i=0;i<fileandsize.size();i++)
				printf("PRE %08d  %s %s\n",i,migliaia(fileandsize[i].size),fileandsize[i].filename.c_str());
		}
	
		sort(fileandsize.begin(),fileandsize.end(),comparefilenamesize);
	
		if (flagdebug)
		{
			printf("Post-sort\n");
			for (unsigned int i=0;i<fileandsize.size();i++)
				printf("POST %08d  %s %s\n",i,migliaia(fileandsize[i].size),fileandsize[i].filename.c_str());
		}
	
		iniziohash=mtime();
	
		int64_t dalavorare=0;
		for (unsigned int i=0;i<fileandsize.size();i++)
			if (i<fileandsize.size()-1)
			{
				bool entrato=false;
				while (fileandsize[i].size==fileandsize[i+1].size)
				{
					if (!entrato)
					{
						dalavorare+=fileandsize[i].size;
						entrato=true;
					}
					
					dalavorare+=fileandsize[i+1].size;
					i++;
					if (i==fileandsize.size())
						break;
				}
			}
		
		printf("\nStart checksumming %s / %s bytes ...\n",migliaia(fileandsize.size()),migliaia2(dalavorare));
		int larghezzaconsole=terminalwidth()-2;
		if (larghezzaconsole<10)
			larghezzaconsole=80;

		int64_t ultimapercentuale=0;
		int64_t rapporto;
		int64_t percentuale;
		
		for (unsigned int i=0;i<fileandsize.size();i++)
		{
			if (i<fileandsize.size()-1)
			{
				bool entrato=false;
				while (fileandsize[i].size==fileandsize[i+1].size)
				{
					if (!entrato)
					{
						bool saveeta=flagnoeta;
						flagnoeta=true;
						string temp=hash_calc_file(flag2algo(),fileandsize[i].filename.c_str(),0,0,dummylavorati);
						flagnoeta=saveeta;

						fileandsize[i].hashhex=temp;
						fileandsize[i].flaghashstored=true;
						quantihash++;
						hash_calcolati+=fileandsize[i].size;
						entrato=true;
						if (flagdebug)
							printf("%08d HASH %s %s %19s %s\n",i,fileandsize[i].hashhex.c_str(),dateToString(fileandsize[i].date).c_str(),migliaia(fileandsize[i].size),fileandsize[i].filename.c_str());
						else
						{
							rapporto=larghezzaconsole*hash_calcolati/(dalavorare+1);
							percentuale=100*hash_calcolati/(dalavorare+1);
							if (!flagnoeta)
							{
								printf("Done %03d ",(unsigned int)percentuale);
								if (percentuale>ultimapercentuale)
								{
									if (rapporto>10)
									{
										for (unsigned j=0;j<rapporto-10;j++)
											printf(".");
										ultimapercentuale=percentuale;
									}
								}
								printf("\r");
							}
						}
					
					}
					
					bool saveeta=flagnoeta;
					flagnoeta=true;
					string temp=hash_calc_file(flag2algo(),fileandsize[i+1].filename.c_str(),0,0,dummylavorati);
					flagnoeta=saveeta;
					
					fileandsize[i+1].hashhex=temp;
					fileandsize[i+1].flaghashstored=true;
					quantihash++;
					hash_calcolati+=fileandsize[i+1].size;
					if (flagdebug)
						printf("%08d HASH-2 %s %s %19s %s\n",i+1,fileandsize[i+1].hashhex.c_str(),dateToString(fileandsize[i+1].date).c_str(),migliaia(fileandsize[i+1].size),fileandsize[i+1].filename.c_str());
					else
					{
						rapporto=larghezzaconsole*hash_calcolati/(dalavorare+1);
						percentuale=100*hash_calcolati/(dalavorare+1);
						if (!flagnoeta)
						{
							printf("Done %03d ",(unsigned int)percentuale);
							if (percentuale>ultimapercentuale)
								{
									if (rapporto>10)
									{
										for (unsigned j=0;j<rapporto-10;j++)
											printf(".");
										ultimapercentuale=percentuale;
									}
								}
								
							printf("\r");
						}
					}
					
					i++;
					if (i==fileandsize.size())
						break;
					
				}
				
			}
		}
		finehash=mtime();
		printf("\n");
		sort(fileandsize.begin(),fileandsize.end(),comparecrc32);
	
		if (flagdebug)
		{
			printf("Hash taken %08d %s\n",quantihash,migliaia(hash_calcolati));
			printf("Before shrink %s\n",migliaia(fileandsize.size()));
			printf("Time %f\n",(finehash-iniziohash)/1000.0);
	
			for (unsigned int i=0;i<fileandsize.size();i++)
				printf("before shrink %08d  %s %s\n",i,fileandsize[i].hashhex.c_str(),fileandsize[i].filename.c_str());
		}
	
		int limite=-1;
		for (unsigned int i=0;i<fileandsize.size(); i++)
		{
			if (flagdebug)
				printf("%d size %s <<%s>>\n",i,migliaia(fileandsize.size()),fileandsize[i].hashhex.c_str());
		
			if (!fileandsize[i].flaghashstored)
			{
				limite=i;
				break;
			}	
		}
	
		if (flagverbose)
		printf("Limit founded %d\n",limite);
		
		if (limite>-1)
			for (int i=fileandsize.size()-1;i>=limite;i--)
				fileandsize.pop_back();
	
	
		if (flagdebug)
		{
			printf("After shrink %s\n",migliaia(fileandsize.size()));

			for (unsigned int i=0;i<fileandsize.size();i++)
				printf("After shrinking %08d  %s %s\n",i,fileandsize[i].hashhex.c_str(),fileandsize[i].filename.c_str());
		}
	
		sort(fileandsize.begin(),fileandsize.end(),comparesizehash);
	
		if (flagdebug)
		{
			printf("After re-sort %s\n",migliaia(fileandsize.size()));

			for (unsigned int i=0;i<fileandsize.size();i++)
				printf("After re-sort %08d  %s %s\n",i,fileandsize[i].hashhex.c_str(),fileandsize[i].filename.c_str());
		}
	
	}
	else
	{

		if (barraod)
			sort(fileandsize.begin(),fileandsize.end(),comparefilenamedate);
	
		if (barraos)
			sort(fileandsize.begin(),fileandsize.end(),comparefilenamesize);
	}
	unsigned int inizio=0;
	if (menoenne)
		if (menoenne<fileandsize.size())
			inizio=fileandsize.size()-menoenne;
	
	
	int64_t tot_duplicati=0;
	
	for (unsigned int i=inizio;i<fileandsize.size();i++)
		if (fileandsize[i].isdir)
		{
			printf("%s <DIR>               %s\n",dateToString(fileandsize[i].date).c_str(),fileandsize[i].filename.c_str());
			quantedirectory++;
		}
		else
		{
			total_size+=fileandsize[i].size;
			quantifiles++;
			
			if (flagduplicati)
			{
			
				if (i<fileandsize.size()-1)
				{
					bool entrato=false;

					//if (memcmp(fileandsize[i].crc32hex,fileandsize[i+1].crc32hex,8)==0)
					if (fileandsize[i].hashhex==fileandsize[i+1].hashhex)
					{
						if (flagverbose)
							printf("%s ",fileandsize[i].hashhex.c_str());
						
						printf("%s %19s %s\n",dateToString(fileandsize[i].date).c_str(),migliaia(fileandsize[i].size),fileandsize[i].filename.c_str());
						
						while (fileandsize[i].hashhex==fileandsize[i+1].hashhex)
						{
							if (flagverbose)
							printf("%s ",fileandsize[i].hashhex.c_str());
							
							printf("=================== %19s %s\n",migliaia(fileandsize[i+1].size),fileandsize[i+1].filename.c_str());
							tot_duplicati+=fileandsize[i].size;
							entrato=true;
							i++;
							if (i==fileandsize.size())
								break;
						}
					}
					if (entrato)
						printf("\n");
				}
				
			}
			else
				printf("%s %19s %s\n",dateToString(fileandsize[i].date).c_str(),migliaia(fileandsize[i].size),fileandsize[i].filename.c_str());
			
		}
	
	           
	printf("  %8d File     %19s byte\n",quantifiles,migliaia(total_size));
	if (!flagduplicati)
	printf("  %8d directory",quantedirectory);
	
	int64_t spazio=getfreespace(cartella.c_str());
	printf(" %s bytes (%10s) free",migliaia(spazio),tohuman(spazio));
	
	printf("\n");
	if (flagduplicati)
	{
		printf("\n          Duplicate %19s byte\n",migliaia(tot_duplicati));
		printf("Hashed %08d files %s in %f s %s /s\n",quantihash,migliaia(hash_calcolati),(finehash-iniziohash)/1000.0,migliaia2(hash_calcolati/((finehash-iniziohash+1)/1000.0)));
	}
	return 0;
}
