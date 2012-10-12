/*

 B G E T

 Buffer allocator

 Designed and implemented in April of 1972 by John Walker, based on the
 Case Algol OPRO$ algorithm implemented in 1966.

 Reimplemented in 1975 by John Walker for the Interdata 70.
 Reimplemented in 1977 by John Walker for the Marinchip 9900.
 Reimplemented in 1982 by Duff Kurland for the Intel 8080.

 Portable C version implemented in September of 1990 by an older, wiser
 instance of the original implementor.

 Souped up and/or weighed down  slightly  shortly  thereafter  by  Greg
 Lutz.

 AMIX  edition, including the new compaction call-back option, prepared
 by John Walker in July of 1992.

 Bug in built-in test program fixed, ANSI compiler warnings eradicated,
 buffer pool validator  implemented,  and  guaranteed  repeatable  test
 added by John Walker in October of 1995.

 This program is in the public domain.

 d'b: not so funny shit removed. doc removed. tests related code removed.
 the code customized to use with embedded applications.

 BGET CONFIGURATION
 ==================
 */

#define TestProg    20000       /* Generate built-in test program
           if defined.  The value specifies
           how many buffer allocation attempts
           the test program should make. */

#define SizeQuant   4         /* Buffer allocation size quantum:
           all buffers allocated are a
           multiple of this size.  This
           MUST be a power of two. */

#define BufStats    1         /* Define this symbol to enable the
           bstats() function which calculates
           the total free space in the buffer
           pool, the largest available
           buffer, and the total space
           currently allocated. */

#define FreeWipe    1         /* Wipe free buffers to a guaranteed
           pattern of garbage to trip up
           miscreants who attempt to use
           pointers into released buffers. */

/* Use a best fit algorithm when
#define BestFit     1
           searching for space for an
           allocation request.  This uses
           memory more efficiently, but
           allocation will be much slower. */

#include <stdio.h>

#ifdef lint
#define NDEBUG            /* Exits in asserts confuse lint */
/* LINTLIBRARY *//* Don't complain about def, no ref */
extern char *sprintf(); /* Sun includes don't define sprintf */
#endif

#include <assert.h>
#include <memory.h>

/*  Declare the interface, including the requested buffer size type,
 bufsize.  */

#include "bget.h"

#define MemSize     int         /* Type for size arguments to memxxx()
           functions such as memcmp(). */

/* Queue links */

struct qlinks
{
  struct bfhead *flink; /* Forward link */
  struct bfhead *blink; /* Backward link */
};

/* Header in allocated and free buffers */

struct bhead
{
  bufsize prevfree; /* Relative link back to previous
   free buffer in memory or 0 if
   previous buffer is allocated.  */
  bufsize bsize; /* Buffer size: positive if free,
   negative if allocated. */
};
#define BH(p) ((struct bhead *) (p))

/*  Header in directly allocated buffers (by acqfcn) */

struct bdhead
{
  bufsize tsize; /* Total size, including overhead */
  struct bhead bh; /* Common header */
};
#define BDH(p)  ((struct bdhead *) (p))

/* Header in free buffers */

struct bfhead
{
  struct bhead bh; /* Common allocated/free header */
  struct qlinks ql; /* Links on free list */
};
#define BFH(p)  ((struct bfhead *) (p))

static struct bfhead freelist =
{ /* List of free buffers */
{ 0, 0 },
{ &freelist, &freelist } };

#ifdef BufStats
static bufsize totalloc = 0; /* Total space currently allocated */
static long numget = 0, numrel = 0; /* Number of bget() and brel() calls */
#endif /* BufStats */

/*  Minimum allocation quantum: */

#define QLSize  (sizeof(struct qlinks))
#define SizeQ ((SizeQuant > QLSize) ? SizeQuant : QLSize)

#define V   (void)          /* To denote unwanted returned values */

/* End sentinel: value placed in bsize field of dummy block delimiting
 end of pool block.  The most negative number which will  fit  in  a
 bufsize, defined in a way that the compiler will accept. */

#define ESent ((bufsize) (-(((1L << (sizeof(bufsize) * 8 - 2)) - 1) * 2) - 2))

/*  BGET  --  Allocate a buffer.  */

void *bget(requested_size)
  bufsize requested_size;
{
  bufsize size = requested_size;
  struct bfhead *b;
#ifdef BestFit
  struct bfhead *best;
#endif
  void *buf;

  assert(size > 0);

  if(size < SizeQ)
  { /* Need at least room for the */
    size = SizeQ; /*    queue links.  */
  }
#ifdef SizeQuant
#if SizeQuant > 1
  size = (size + (SizeQuant - 1)) & (~(SizeQuant - 1));
#endif
#endif

  size += sizeof(struct bhead); /* Add overhead in allocated buffer
   to size required. */

  b = freelist.ql.flink;
#ifdef BestFit
  best = &freelist;
#endif

  /* Scan the free list searching for the first buffer big enough
   to hold the requested size buffer. */

#ifdef BestFit
  while(b != &freelist)
  {
    if(b->bh.bsize >= size)
    {
      if((best == &freelist) || (b->bh.bsize < best->bh.bsize))
      {
        best = b;
      }
    }
    b = b->ql.flink; /* Link to next buffer */
  }
  b = best;
#endif /* BestFit */

  while(b != &freelist)
  {
    if((bufsize)b->bh.bsize >= size)
    {

      /* Buffer  is big enough to satisfy  the request.  Allocate it
       to the caller.  We must decide whether the buffer is  large
       enough  to  split  into  the part given to the caller and a
       free buffer that remains on the free list, or  whether  the
       entire  buffer  should  be  removed  from the free list and
       given to the caller in its entirety.   We  only  split  the
       buffer if enough room remains for a header plus the minimum
       quantum of allocation. */

      if((b->bh.bsize - size) > (SizeQ + (sizeof(struct bhead))))
      {
        struct bhead *ba, *bn;

        ba = BH(((char *) b) + (b->bh.bsize - size));
        bn = BH(((char *) ba) + size);
        assert(bn->prevfree == b->bh.bsize);
        /* Subtract size from length of free block. */
        b->bh.bsize -= size;
        /* Link allocated buffer to the previous free buffer. */
        ba->prevfree = b->bh.bsize;
        /* Plug negative size into user buffer. */
        ba->bsize = -(bufsize)size;
        /* Mark buffer after this one not preceded by free block. */
        bn->prevfree = 0;

#ifdef BufStats
        totalloc += size;
        numget++; /* Increment number of bget() calls */
#endif
        buf = (void *)((((char *)ba) + sizeof(struct bhead)));
        return buf;
      }
      else
      {
        struct bhead *ba;

        ba = BH(((char *) b) + b->bh.bsize);
        assert(ba->prevfree == b->bh.bsize);

        /* The buffer isn't big enough to split.  Give  the  whole
         shebang to the caller and remove it from the free list. */

        assert(b->ql.blink->ql.flink == b);
        assert(b->ql.flink->ql.blink == b);
        b->ql.blink->ql.flink = b->ql.flink;
        b->ql.flink->ql.blink = b->ql.blink;

#ifdef BufStats
        totalloc += b->bh.bsize;
        numget++; /* Increment number of bget() calls */
#endif
        /* Negate size to mark buffer allocated. */
        b->bh.bsize = -(b->bh.bsize);

        /* Zero the back pointer in the next buffer in memory
         to indicate that this buffer is allocated. */
        ba->prevfree = 0;

        /* Give user buffer starting at queue links. */
        buf = (void *)&(b->ql);
        return buf;
      }
    }
    b = b->ql.flink; /* Link to next buffer */
  }

  return NULL;
}

/*  BGETZ  --  Allocate a buffer and clear its contents to zero.  We clear
 the  entire  contents  of  the buffer to zero, not just the
 region requested by the caller. */

void *bgetz(size)
  bufsize size;
{
  char *buf = (char *) bget(size);

  if(buf != NULL)
  {
    struct bhead *b;
    bufsize rsize;

    b = BH(buf - sizeof(struct bhead));
    rsize = -(b->bsize);
    if(rsize == 0)
    {
      struct bdhead *bd;

      bd = BDH(buf - sizeof(struct bdhead));
      rsize = bd->tsize - sizeof(struct bdhead);
    }
    else
    {
      rsize -= sizeof(struct bhead);
    }assert(rsize >= size);
    V memset(buf, 0, (MemSize) rsize);
  }
  return ((void *) buf);
}

/*  BGETR  --  Reallocate a buffer.  This is a minimal implementation,
 simply in terms of brel()  and  bget().   It  could  be
 enhanced to allow the buffer to grow into adjacent free
 blocks and to avoid moving data unnecessarily.  */

void *bgetr(buf, size)
  void *buf;bufsize size;
{
  void *nbuf;
  bufsize osize; /* Old size of buffer */
  struct bhead *b;

  if((nbuf = bget(size)) == NULL)
  { /* Acquire new buffer */
    return NULL;
  }
  if(buf == NULL)
  {
    return nbuf;
  }
  b = BH(((char *) buf) - sizeof(struct bhead));
  osize = -b->bsize;
  osize -= sizeof(struct bhead);
  assert(osize > 0);
  V memcpy((char *) nbuf, (char *) buf, /* Copy the data */
           (MemSize) ((size < osize) ? size : osize));
  brel(buf);
  return nbuf;
}

/*  BREL  --  Release a buffer.  */

void brel(buf)
  void *buf;
{
  struct bfhead *b, *bn;

  b = BFH(((char *) buf) - sizeof(struct bhead));
#ifdef BufStats
  numrel++; /* Increment number of brel() calls */
#endif
  assert(buf != NULL);

  /* Buffer size must be negative, indicating that the buffer is
   allocated. */

  if(b->bh.bsize >= 0)
  {
    bn = NULL;
  }assert(b->bh.bsize < 0);

  /*  Back pointer in next buffer must be zero, indicating the
   same thing: */

  assert(BH((char *) b - b->bh.bsize)->prevfree == 0);

#ifdef BufStats
  totalloc += b->bh.bsize;
  assert(totalloc >= 0);
#endif

  /* If the back link is nonzero, the previous buffer is free.  */

  if(b->bh.prevfree != 0)
  {

    /* The previous buffer is free.  Consolidate this buffer  with  it
     by  adding  the  length  of  this  buffer  to the previous free
     buffer.  Note that we subtract the size  in  the  buffer  being
     released,  since  it's  negative to indicate that the buffer is
     allocated. */

    register bufsize size = b->bh.bsize;

    /* Make the previous buffer the one we're working on. */
    assert(BH((char *) b - b->bh.prevfree)->bsize == b->bh.prevfree);
    b = BFH(((char *) b) - b->bh.prevfree);
    b->bh.bsize -= size;
  }
  else
  {

    /* The previous buffer isn't allocated.  Insert this buffer
     on the free list as an isolated free block. */

    assert(freelist.ql.blink->ql.flink == &freelist);
    assert(freelist.ql.flink->ql.blink == &freelist);
    b->ql.flink = &freelist;
    b->ql.blink = freelist.ql.blink;
    freelist.ql.blink = b;
    b->ql.blink->ql.flink = b;
    b->bh.bsize = -b->bh.bsize;
  }

  /* Now we look at the next buffer in memory, located by advancing from
   the  start  of  this  buffer  by its size, to see if that buffer is
   free.  If it is, we combine  this  buffer  with  the  next  one  in
   memory, dechaining the second buffer from the free list. */

  bn = BFH(((char *) b) + b->bh.bsize);
  if(bn->bh.bsize > 0)
  {

    /* The buffer is free.  Remove it from the free list and add
     its size to that of our buffer. */

    assert(BH((char *) bn + bn->bh.bsize)->prevfree == bn->bh.bsize);
    assert(bn->ql.blink->ql.flink == bn);
    assert(bn->ql.flink->ql.blink == bn);
    bn->ql.blink->ql.flink = bn->ql.flink;
    bn->ql.flink->ql.blink = bn->ql.blink;
    b->bh.bsize += bn->bh.bsize;

    /* Finally,  advance  to   the  buffer  that   follows  the  newly
     consolidated free block.  We must set its  backpointer  to  the
     head  of  the  consolidated free block.  We know the next block
     must be an allocated block because the process of recombination
     guarantees  that  two  free  blocks will never be contiguous in
     memory.  */

    bn = BFH(((char *) b) + b->bh.bsize);
  }
#ifdef FreeWipe
  V memset(((char *) b) + sizeof(struct bfhead), 0x55,
           (MemSize) (b->bh.bsize - sizeof(struct bfhead)));
#endif
  assert(bn->bh.bsize < 0);

  /* The next buffer is allocated.  Set the backpointer in it  to  point
   to this buffer; the previous free buffer in memory. */

  bn->bh.prevfree = b->bh.bsize;
}

/*  BPOOL  --  Add a region of memory to the buffer pool.  */

void bpool(buf, len)
  void *buf;bufsize len;
{
  struct bfhead *b = BFH(buf);
  struct bhead *bn;

#ifdef SizeQuant
  len &= ~(SizeQuant - 1);
#endif

  /* Since the block is initially occupied by a single free  buffer,
   it  had  better  not  be  (much) larger than the largest buffer
   whose size we can store in bhead.bsize. */

  assert(len - sizeof(struct bhead) <= -((bufsize) ESent + 1));

  /* Clear  the  backpointer at  the start of the block to indicate that
   there  is  no  free  block  prior  to  this   one.    That   blocks
   recombination when the first block in memory is released. */

  b->bh.prevfree = 0;

  /* Chain the new block to the free list. */

  assert(freelist.ql.blink->ql.flink == &freelist);
  assert(freelist.ql.flink->ql.blink == &freelist);
  b->ql.flink = &freelist;
  b->ql.blink = freelist.ql.blink;
  freelist.ql.blink = b;
  b->ql.blink->ql.flink = b;

  /* Create a dummy allocated buffer at the end of the pool.  This dummy
   buffer is seen when a buffer at the end of the pool is released and
   blocks  recombination  of  the last buffer with the dummy buffer at
   the end.  The length in the dummy buffer  is  set  to  the  largest
   negative  number  to  denote  the  end  of  the pool for diagnostic
   routines (this specific value is  not  counted  on  by  the  actual
   allocation and release functions). */

  len -= sizeof(struct bhead);
  b->bh.bsize = (bufsize) len;
#ifdef FreeWipe
  V memset(((char *) b) + sizeof(struct bfhead), 0x55, (MemSize) (len - sizeof(struct bfhead)));
#endif
  bn = BH(((char *) b) + len);
  bn->prevfree = (bufsize) len;
  /* Definition of ESent assumes two's complement! */
  assert((~0) == -1);
  bn->bsize = ESent;
}

#ifdef BufStats

/*  BSTATS  --  Return buffer allocation free space statistics.  */

void bstats(curalloc, totfree, maxfree, nget, nrel)
  bufsize *curalloc, *totfree, *maxfree; long *nget, *nrel;
{
  struct bfhead *b = freelist.ql.flink;

  *nget = numget;
  *nrel = numrel;
  *curalloc = totalloc;
  *totfree = 0;
  *maxfree = -1;
  while(b != &freelist)
  {
    assert(b->bh.bsize > 0);
    *totfree += b->bh.bsize;
    if(b->bh.bsize > *maxfree)
    {
      *maxfree = b->bh.bsize;
    }
    b = b->ql.flink; /* Link to next buffer */
  }
}

#endif
