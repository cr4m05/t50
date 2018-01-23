#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <setjmp.h>

/*
   IPv4: xxx[.yyy[.zzz[.www]]][/ii]
*/
int get_ip_and_cidr(char *str, uint32_t *ip, int *cidr)
{
  char *p, *q, *r;
  char *t;
  unsigned long l;
  jmp_buf jb;

  // Ignore initial spaces...
  while (isspace(*str)) str++;

  t = strdup(str);

  if (setjmp(jb))
  {
    free(t);
    return 0;
  }

  p = t + strlen(t) - 1;
  while (p >= t && isspace(*p)) *p-- = '\0';

  // Empty string is a special case...
  if (!*t) longjmp(jb, 1);

  *cidr = 0;
  *ip = 0;
  errno = 0;
  l = strtoul(t, &q, 10);
  r = (char *)ip;

  // if still has chars on the string...
  while (*q && !errno)
  {
    // is it a invalid octect?
    if (l > 255) longjmp(jb, 1);

    if (*q == '.')
    { 
      *r++ = l;
      *cidr += 8;

      // this is also a special case...
      if (!*(q+1))
        break;
    }
    else
      if (*q == '/') 
      {
        *r++ = l;
        *cidr += 8;

        p = q + 1;
        errno = 0;
        l = strtoul(p, &q, 10);

        // nulchar is expected at this point...
        if (*q || errno)
          longjmp(jb, 1);

        // is it an invalid cidr?
        if (l < 1 || l > 32) 
          longjmp(jb, 1);

        *cidr = l;
        break;
      }
      else
        if (*q)
          longjmp(jb, 1);

    errno = 0;
    p = q + 1;
    l = strtoul(p, &q, 10);
    if (!*q)
    {
      *r++ = l;
      *cidr += 8;
      break;
    }
  }

  free(t);

  // any convertion errors found?
  if (errno)
    return 0;

  // IP must be bigendian!
  *ip = __builtin_bswap32(*ip);

  return 1;
}

/* Test routine. */
void main(void)
{
  // I need to make some more tests here!
  char *str[] = { "",               // invalid.
                  " ",              // invalid.
                  "10/8", 
                  "10.1/16", 
                  "10.1.2/24", 
                  "10.1.2.3/32",
                  "10.1.2.3.4/32",  // invalid. 
                  "teste/31",       // invalid.
                  "1.2.3.4/",       // invalid.
                  "256.0.0.0/32",   // invalid.
                  "10.0.0.1/33",    // invalid.
                  "10.1./16",  
                  "10.1.2./16",
                  "1.2.3.4",
                  "1.2.3",
                  "10.11.12.",
                  "10.11.12.13 ",
                  NULL };
  char **p = str;
  uint32_t ip;
  int cidr;

  while (*p)
  {
    printf("%s -> ", *p);
    if (get_ip_and_cidr(*p++, &ip, &cidr))
      printf("%#x/%d\n", ip, cidr);
    else
      puts("invalid address");
  }
}
