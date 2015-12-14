#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#define  MAXPROGLEN 1000000
#define  MAXPROGLINELEN 999
#define  STACKSIZE    99999
#define  ENVSTACKSIZE 99999
#define  LOCSIZE         26
#define  MAXADDR   99999999
#define  HALFWIDTH       39
#define  MOUSE_EXT   ".m02"
#define  ARRAYSIZE    10000
#define  MAXFILES       100
#define  BACKSPACE     charpos--
#define  VALUE(digit)  (digit - '0')
#define  UPPERCASE     ch = toupper(ch)
#define  TOLERANCE     1.0e-6

#ifndef  PI
#define  PI  3.14159265358979323846264338327950288419716939937510582097494459230
#endif

#define  SPEED_OF_LIGHT  299792458.0
#define  ELEMENTARY_CHG  1.60217653e-19
#define  GRAV_ACCEL      9.80665
#define  GRAV_CONST      6.6742e-11
#define  PLANCK          6.6260693e-34
#define  H_BAR           1.05457168e-34
#define  PERMEABILITY    (4.0e-7*PI)
#define  PERMITTIVITY    (1.0/(PERMEABILITY*SPEED_OF_LIGHT*SPEED_OF_LIGHT))
#define  MASS_ELECTRON   9.1093826e-31
#define  MASS_PROTON     1.67262171e-27
#define  MASS_NEUTRON    1.67492728e-27
#define  AVAGADRO        6.0221415e23
#define  BOLTZMANN       1.3806505e-23
#define  AU              1.49597870e11
#define  GM_EARTH        3.9860005e14
#define  GM_SUN          1.32712438e20
#define  R_EARTH         6.378140e6
#define  LB_KG           0.45359237
#define  IN_CM           2.54
#define  GAL_L           3.7854118


#define  DEFAULT_ANGLE_FACTOR    1.0
#define  DEFAULT_DISPLAY_MODE    2
#define  DEFAULT_DISPLAY_DIGITS  15
#define  DEFAULT_DISPLAY_WIDTH   0
#define  DEFAULT_WORDSIZE        32
#define  DEFAULT_OCTHEX_DIGITS   ((DEFAULT_WORDSIZE-1)/4+1)

#define  VERSION         20
#define  PROMPT          "\n> "


enum tagtype
{ macro, parameter, loop };
typedef struct
{
  enum tagtype tag;
  long charpos;
  long offset;
} environment;


FILE *progfile;
char prog[MAXPROGLEN];
char prog_line[MAXPROGLINELEN + 2];
double stack[STACKSIZE];
environment envstack[ENVSTACKSIZE];
double data[MAXADDR];
long macdefs[26];
char ch;
long charpos;
long proglen;
long sp;
long esp;
long tsp;
long offset;
long nextfree;
double temp, temp2, temp3;
long itemp, itemp2;
long parbal;
long parnum;
int tracing;
int disaster;
int j;
char filename[101];
char format_str[11];
long ntemp;
int done;
char line[133];
int source;
double array[ARRAYSIZE];
int error_flag;
FILE *fp[MAXFILES];
char filename_str[13];
char filenum_str[4];
char filemode_str[3];
char temp_str[25];
enum tagtype envtag;
double angle_factor = DEFAULT_ANGLE_FACTOR;
long display_mode = DEFAULT_DISPLAY_MODE;
long display_digits = DEFAULT_DISPLAY_DIGITS;
long display_width = DEFAULT_DISPLAY_WIDTH;
long wordsize = DEFAULT_WORDSIZE;
long octhex_digits = DEFAULT_OCTHEX_DIGITS;
long octhex_mask = 0xFFFFFFFF;


void chomp (char *str);
void display (long charpos);
void error (short code);
void Getchar (void);
void push (double datum);
double pop (void);
void skipstring (void);
void skip (char lch, char rch);
void skip2 (char lch, char rch1, char rch2);
void pushenv (enum tagtype tag);
void popenv (void);
void load (void);
void makedeftable (void);
void interpret (void);
void process_amp (char *str);
double Int (double f);
double Frac (double f);
int Round (double x);

int
main (int argc, char *argv[])
{

  if (argc == 1)
    {
      source = 1;
      done = 0;
      printf ("Mouse-2002 Interpreter Version %d\n", VERSION);
      sp = -1;
      esp = -1;
      do
	{
	  printf (PROMPT);
	  fgets (line, 132, stdin);
	  load ();
	  interpret ();
	}
      while (!done);
      exit (0);
    }

  source = 0;

  strcpy (filename, argv[1]);
  if (strchr (filename, (int) '.') == NULL)
    strcat (filename, MOUSE_EXT);

  if ((progfile = fopen (filename, "rb")) == NULL)
    {
      printf ("Error opening file %s\n", filename);
      exit (1);
    }

  load ();
  fclose (progfile);

  if (!disaster)
    {
      makedeftable ();
      interpret ();
    }

  return 0;
}






void
display (long charpos)
{
  long pos;
  char *prog_ptr;

  if (source == 0)
    prog_ptr = prog;
  else
    prog_ptr = prog_line;

  for (j = 0; j < 4; j++)
    {
      if (j > sp)
	printf ("  ..........");
      else
	printf ("%12.4e", stack[sp - j]);
    }
  printf ("      ");

  for (pos = charpos - HALFWIDTH; pos <= charpos + HALFWIDTH; pos++)
    {
      if ((pos >= 0) && (pos < proglen) && (prog_ptr[pos] >= ' '))
	printf ("%c", prog_ptr[pos]);
      else
	printf (" ");
    }

  printf ("\n");
  for (j = 0; j < HALFWIDTH + 54; j++)
    printf (" ");
  printf ("^\n");
}

void
error (short code)
{
  short tsp;

  printf ("\nEnvironment:\n");
  for (tsp = 0; tsp < esp; tsp++)
    display (envstack[tsp].charpos);
  printf ("Instruction pointer:\n");
  display (charpos);

  printf ("Stack:");
  for (tsp = 0; tsp <= sp; tsp++)
    printf (" [%17.10E] ", stack[tsp]);
  printf ("\n");

  printf ("***** Error %d: ", code);
  switch (code)
    {
    case 1:
      printf ("Ran off end of program");
      break;
    case 2:
      printf ("Calculation stack overflowed");
      break;
    case 3:
      printf ("Calculation stack underflowed");
      break;
    case 4:
      printf ("Attempted to divide by zero");
      break;
    case 5:
      printf ("Attempted to find modulus by zero");
      break;
    case 6:
      printf ("Undefined macro");
      break;
    case 7:
      printf ("Illegal character follows \"#\"");
      break;
    case 8:
      printf ("Environment stack overflowed");
      break;
    case 9:
      printf ("Environment stack underflowed");
      break;
    case 10:
      printf ("Data space exhausted");
      break;
    case 11:
      printf ("Illegal character %d", ch);
      break;
    case 12:
      printf ("Invalid argument for &acos");
      break;
    case 13:
      printf ("Invalid argument for &acosh");
      break;
    case 14:
      printf ("Invalid argument for &asin");
      break;
    case 15:
      printf ("Invalid argument for &atanh");
      break;
    case 16:
      printf ("Invalid argument for &ln");
      break;
    case 17:
      printf ("Invalid argument for &log2");
      break;
    case 18:
      printf ("Invalid argument for &log10");
      break;
    case 19:
      printf ("Invalid argument for &recip");
      break;
    case 20:
      printf ("Invalid argument for &sqrt");
      break;
    case 21:
      printf ("Invalid argument for &!");
      break;
    case 22:
      printf ("Invalid word size");
      break;
    case 23:
      printf ("Invalid arguments for &cnr");
      break;
    case 24:
      printf ("Invalid arguments for &pnr");
      break;
    case 25:
      printf ("Array index out of bounds");
      break;
    case 26:
      printf ("Invalid argument for ` or &power");
      break;
    case 27:
      printf ("Invalid arguments for &root");
      break;
    case 28:
      printf ("Error opening file");
      break;
    case 29:
      printf ("Invalid & function name");
      break;
    case 30:
      printf ("Invalid argument for &cubert");
      break;
    case 31:
      printf ("Invalid argument for &4thrt");
      break;
    }
  printf ("\n");
  disaster = 1;
  sp = -1;
}





void
Getchar (void)
{
  if (charpos < proglen - 1)
    {
      charpos++;
      if (source == 0)
	ch = prog[charpos];
      else
	ch = prog_line[charpos];
    }
  else
    error (1);
}

void
push (double datum)
{
  if (sp < STACKSIZE - 1)
    {
      sp++;
      stack[sp] = datum;
    }
  else
    error (2);
}

double
pop (void)
{
  double result;
  if (sp >= 0)
    {
      result = stack[sp];
      sp--;
    }
  else
    error (3);
  return result;
}

void
skipstring (void)
{
  do
    {
      Getchar ();
    }
  while (ch != '"');
}

void
skip (char lch, char rch)
{
  short count;
  count = 1;
  do
    {
      Getchar ();
      if (ch == '"')
	skipstring ();
      else if (ch == lch)
	count++;
      else if (ch == rch)
	count--;
    }
  while (count != 0);
}

void
skip2 (char lch, char rch1, char rch2)
{
  short count;
  count = 1;
  do
    {
      Getchar ();
      if (ch == '"')
	skipstring ();
      else if (ch == lch)
	count++;
      else if (ch == rch1 || ch == rch2)
	count--;
    }
  while (count != 0);
}

void
pushenv (enum tagtype tag)
{
  if (esp < ENVSTACKSIZE - 1)
    {
      esp++;
      envstack[esp].tag = tag;
      envstack[esp].charpos = charpos;
      envstack[esp].offset = offset;
    }
  else
    error (8);
}

void
popenv (void)
{
  if (esp >= 0)
    {
      envtag = envstack[esp].tag;
      charpos = envstack[esp].charpos;
      offset = envstack[esp].offset;
      esp--;
    }
  else
    error (9);
}

void
load (void)
{
  char lastchr;
  char in = 0;
  char in_amp = 0;
  char *p;
  char *prog_ptr;
  long maxlen;


  if (source == 0)
    {
      for (charpos = 0; charpos < MAXPROGLEN; charpos++)
	prog[charpos] = ' ';
      rewind (progfile);
      prog_ptr = prog;
      maxlen = MAXPROGLEN;
    }
  else
    {
      p = line;
      prog_ptr = prog_line;
      maxlen = MAXPROGLINELEN;
    }
  charpos = -1;
  disaster = 0;
  ch = '~';
  while (!disaster)
    {
      lastchr = ch;
      if (source == 0)
	{
	  fread (&ch, 1, 1, progfile);
	  if (feof (progfile))
	    break;
	}
      else
	{
	  ch = *p++;
	  if (ch == '\0' || ch == '\n')
	    break;
	}
      if (ch == '~')
	{
	  if (source == 0)
	    do
	      {
		fread (&ch, 1, 1, progfile);
	      }
	    while (ch != '\n');
	  else
	    break;
	}
      else if (charpos < maxlen - 1)
	{
	  charpos++;
	  prog_ptr[charpos] = ch;
	  if (ch == '\"')
	    in = !in;
	  if (ch == '&' && !in)
	    in_amp = 1;
	  if (ch == 10 || ch == 13 || ch == '\n' || ch == '\t' || ch == '\r')
	    prog_ptr[charpos] = ch = ' ';
	  if (in_amp && ch == ' ')
	    {
	      prog_ptr[charpos] = ch = '&';
	      in_amp = 0;
	    }
	  if (in_amp && ch == ';')
	    {
	      prog_ptr[charpos] = ch = '&';
	      charpos++;
	      prog_ptr[charpos] = ch = ';';
	      in_amp = 0;
	    }
	  if (ch == ' ' && !in && !isdigit (lastchr) && (lastchr != '\''))
	    {
	      charpos--;
	      ch = prog_ptr[charpos];
	    }
	  else if (!in && lastchr == ' ' && !isdigit (ch) && ch != '\"'
		   && prog_ptr[charpos - 2] != '\'')
	    prog_ptr[--charpos] = ch;
	}
      else
	{
	  printf ("Program is too long\n");
	  disaster = 1;
	}
    }
  proglen = charpos + 1;
  if (source == 1)
    {
      prog_ptr[charpos + 1] = '$';
      charpos++;
      proglen = charpos + 1;
    }

}

void
makedeftable (void)
{
  for (ch = 'A'; ch <= 'Z'; ch++)
    macdefs[ch - 'A'] = 0;
  charpos = -1;
  do
    {
      Getchar ();
      if (ch == '$' && charpos < proglen - 1)
	{
	  Getchar ();
	  UPPERCASE;
	  if ((ch >= 'A') && (ch <= 'Z'))
	    macdefs[ch - 'A'] = charpos;
	}
    }
  while (charpos < proglen - 1);
}

void
interpret (void)
{
  char amp_str[11];
  char *p;
  char instr[26];

  charpos = -1;
  if (source == 0)
    {
      sp = -1;
      esp = -1;
    }
  offset = 0;
  nextfree = LOCSIZE;
  do
    {
      Getchar ();
      if (ch == ' ')
	continue;
      if (tracing)
	display (charpos);
      if (isdigit (ch))
	{
	  temp = 0;
	  while (isdigit (ch))
	    {
	      temp = 10 * temp + VALUE (ch);
	      Getchar ();
	    }
	  if (ch == '.')
	    {
	      Getchar ();
	      temp2 = 1.0;
	      while (isdigit (ch))
		{
		  temp2 /= 10.0;
		  temp += temp2 * VALUE (ch);
		  Getchar ();
		}
	    }
	  push (temp);
	  BACKSPACE;
	}

      else if ((ch >= 'A') && (ch <= 'Z'))
	push (ch - 'A');
      else if ((ch >= 'a') && (ch <= 'z'))
	push (ch - 'a' + offset);
      else
	switch (ch)
	  {

	  case '$':
	    break;
	  case '_':
	    push (-pop ());
	    break;

	  case '+':
	    push (pop () + pop ());
	    break;

	  case '-':
	    temp = pop ();
	    push (pop () - temp);
	    break;

	  case '*':
	    push (pop () * pop ());
	    break;

	  case '/':
	    temp = pop ();
	    if (temp != 0)
	      push (pop () / temp);
	    else
	      error (4);
	    break;

	  case '\\':
	    temp = pop ();
	    if (temp != 0)
	      push ((long) pop () % (long) temp);
	    else
	      error (5);
	    break;

	  case '?':
	    Getchar ();
	    if (ch == '\'')
	      {
		fgets (instr, 2, stdin);
		chomp (instr);
		sscanf (instr, "%c", &ch);
		push ((double) ch);
	      }
	    else
	      {
		fgets (instr, 25, stdin);
		chomp (instr);
		sscanf (instr, "%lf", &temp);
		push (temp);
		BACKSPACE;
	      }
	    break;

	  case '!':
	    Getchar ();
	    if (ch == '\'')
	      printf ("%c", Round (pop ()));
	    else
	      {
		sprintf (format_str, "%%%d.", display_width);
		sprintf (temp_str, "%d", display_digits);
		strcat (format_str, temp_str);
		if (display_mode == 0)
		  strcat (format_str, "f");
		else if (display_mode == 1)
		  strcat (format_str, "E");
		else
		  strcat (format_str, "G");
		printf (format_str, pop ());
		BACKSPACE;
	      }
	    break;

	  case '"':
	    do
	      {
		Getchar ();
		if (ch == '!')
		  printf ("\n");
		else if (ch != '"')
		  printf ("%c", ch);
	      }
	    while (ch != '"');
	    break;

	  case ':':
	    temp = pop ();
	    data[Round (temp)] = pop ();
	    break;

	  case '.':
	    push (data[Round (pop ())]);
	    break;

	  case '<':
	    temp = pop ();
	    push ((pop () < temp) ? 1 : 0);
	    break;

	  case '=':
	    push ((pop () == pop ())? 1 : 0);
	    break;

	  case '>':
	    temp = pop ();
	    push ((pop () > temp) ? 1 : 0);
	    break;

	  case '[':
	    if (pop () <= 0)
	      skip2 ('[', '|', ']');
	    break;

	  case ']':
	    break;
	  case '|':
	    skip ('[', ']');
	    break;

	  case '(':
	    pushenv (loop);
	    break;

	  case ')':
	    charpos = envstack[esp].charpos;
	    break;

	  case '^':
	    if (pop () <= 0)
	      {
		popenv ();
		skip ('(', ')');
	      }
	    break;

	  case '#':
	    Getchar ();
	    UPPERCASE;
	    if ((ch >= 'A') && (ch <= 'Z'))
	      {
		if (macdefs[ch - 'A'] > 0)
		  {
		    pushenv (macro);
		    charpos = macdefs[ch - 'A'];
		    if (nextfree + LOCSIZE <= MAXADDR)
		      {
			offset = nextfree;
			nextfree += LOCSIZE;
		      }
		    else
		      error (10);
		  }
		else
		  error (6);
	      }
	    else
	      error (7);
	    break;

	  case '@':
	    do
	      {
		popenv ();
	      }
	    while (envtag != macro);
	    skip ('#', ';');
	    nextfree -= LOCSIZE;
	    break;

	  case '%':
	    pushenv (parameter);
	    parbal = 1;
	    tsp = esp;
	    do
	      {
		tsp--;
		switch (envstack[tsp].tag)
		  {
		  case macro:
		    parbal--;
		    break;
		  case parameter:
		    parbal++;
		    break;
		  case loop:
		    break;
		  }
	      }
	    while (parbal != 0);
	    charpos = envstack[tsp].charpos;
	    offset = envstack[tsp].offset;
	    parnum = pop ();
	    do
	      {
		Getchar ();
		if (ch == '"')
		  skipstring ();
		else if (ch == '#')
		  skip ('#', ';');
		else if (ch == ',')
		  parnum--;
		else if (ch == ';')
		  {
		    parnum = 0;
		    popenv ();
		  }
	      }
	    while (parnum != 0);
	    break;

	  case ',':
	  case ';':
	    popenv ();
	    break;

	  case '\'':
	    Getchar ();
	    push (ch);
	    break;

	  case '{':
	    tracing = 1;
	    break;

	  case '}':
	    tracing = 0;
	    break;

	  case '&':
	    p = amp_str;
	    Getchar ();
	    while (ch != '&' && ch != '$')
	      {
		*p++ = tolower (ch);
		Getchar ();
	      }
	    *p = '\0';
	    process_amp (amp_str);
	    break;

	  default:
	    error (11);
	    break;
	  }
    }
  while (!((ch == '$') || disaster));
}

void
process_amp (char *str)
{
  long i, j;
  double hr, min, sec;
  struct tm *systime;
  time_t t;
  char instr[26];

  if (!strcmp (str, "2x"))
    push (pow (2.0, pop ()));

  else if (!strcmp (str, "4th"))
    {
      temp = pop ();
      push (temp * temp * temp * temp);
    }

  else if (!strcmp (str, "4thrt"))
    {
      temp = pop ();
      if (temp >= 0.0)
	push (sqrt (sqrt (temp)));
      else
	error (31);
    }

  else if (!strcmp (str, "10x"))
    push (pow (10.0, pop ()));

  else if (!strcmp (str, "abs"))
    push (fabs (pop ()));

  else if (!strcmp (str, "acos"))
    {
      temp = pop ();
      if (fabs (temp) <= 1.0)
	push (acos (temp) / angle_factor);
      else
	error (12);
    }

  else if (!strcmp (str, "acosh"))
    {
      temp = pop ();
      if (temp >= 1.0)
	push (log (temp + sqrt (temp * temp - 1.0)));
      else
	error (13);
    }

  else if (!strcmp (str, "and"))
    {
      itemp = Round (pop ());
      itemp2 = Round (pop ());
      push ((double) (itemp & itemp2));
    }

  else if (!strcmp (str, "asin"))
    {
      temp = pop ();
      if (fabs (temp) <= 1.0)
	push (asin (temp) / angle_factor);
      else
	error (14);
    }

  else if (!strcmp (str, "asinh"))
    {
      temp = pop ();
      push (log (temp + sqrt (temp * temp + 1.0)));
    }

  else if (!strcmp (str, "atan"))
    push (atan (pop ()) / angle_factor);

  else if (!strcmp (str, "atan2"))
    {
      temp = pop ();
      push (atan2 (pop (), temp) / angle_factor);
    }

  else if (!strcmp (str, "atanh"))
    {
      temp = pop ();
      if (fabs (temp) < 1.0)
	push (0.5 * log ((1.0 + temp) / (1.0 - temp)));
      else
	error (15);
    }

  else if (!strcmp (str, "au"))
    push (AU);

  else if (!strcmp (str, "beep"))
    printf ("\a");

  else if (!strcmp (str, "c"))
    push (SPEED_OF_LIGHT);

  else if (!strcmp (str, "clrstk"))
    sp = -1;

  else if (!strcmp (str, "cm>in"))
    push (pop () / IN_CM);

  else if (!strcmp (str, "cnr"))
    {
      itemp = Round (pop ());
      itemp2 = Round (pop ());
      if ((itemp >= 0) && (itemp2 >= 0) && (itemp <= itemp2))
	{
	  temp = 1.0;
	  for (i = itemp2, j = (itemp2 - itemp); j >= 1; i--, j--)
	    temp *= (double) i / (double) j;
	  push (temp);
	}
      else
	error (23);
    }

  else if (!strcmp (str, "cont"))
    charpos = envstack[esp].charpos;

  else if (!strcmp (str, "cos"))
    push (cos (pop () * angle_factor));

  else if (!strcmp (str, "cosh"))
    push (cosh (pop ()));

  else if (!strcmp (str, "cube"))
    {
      temp = pop ();
      push (temp * temp * temp);
    }

  else if (!strcmp (str, "cubert"))
    {
      temp = pop ();
      if (temp > 0.0)
	push (pow (temp, 1.0 / 3.0));
      else if (temp == 0.0)
	push (0.0);
      else
	error (30);
    }

  else if (!strcmp (str, "c>f"))
    push (pop () * 9.0 / 5.0 + 32.0);

  else if (!strcmp (str, "deg"))
    angle_factor = PI / 180.0;

  else if (!strcmp (str, "dom"))
    {
      t = time (NULL);
      systime = localtime (&t);
      push ((double) systime->tm_mday);
    }

  else if (!strcmp (str, "dow"))
    {
      t = time (NULL);
      systime = localtime (&t);
      push ((double) (systime->tm_wday + 1));
    }

  else if (!strcmp (str, "doy"))
    {
      t = time (NULL);
      systime = localtime (&t);
      push ((double) (systime->tm_yday + 1));
    }

  else if (!strcmp (str, "drop"))
    pop ();

  else if (!strcmp (str, "dup"))
    {
      temp = pop ();
      push (temp);
      push (temp);
    }

  else if (!strcmp (str, "d>r"))
    push (pop () * PI / 180.0);

  else if (!strcmp (str, "e"))
    push (ELEMENTARY_CHG);

  else if (!strcmp (str, "eex"))
    {
      temp = pop ();
      push (pop () * pow (10.0, temp));
    }

  else if (!strcmp (str, "eps0"))
    push (PERMITTIVITY);

  else if (!strcmp (str, "exit"))
    done = 1;

  else if (!strcmp (str, "exp"))
    push (exp (pop ()));

  else if (!strcmp (str, "fact"))
    {
      ntemp = Round (pop ());
      if (ntemp >= 0)
	{
	  temp = 1.0;
	  for (i = 2; i <= ntemp; i++)
	    temp *= (double) i;
	  push (temp);
	}
      else
	error (21);
    }

  else if (!strcmp (str, "fclose"))
    fclose (fp[Round (pop ())]);

  else if (!strcmp (str, "feof"))
    push (feof (fp[Round (pop ())])? 1 : 0);

  else if (!strcmp (str, "fix"))
    {
      display_mode = 0;
      display_digits = Round (pop ());
    }

  else if (!strcmp (str, "fopen"))
    {
      itemp = Round (pop ());
      itemp2 = Round (pop ());
      sprintf (filenum_str, "%03d", itemp2);
      strcpy (filename_str, "mouse.");
      strcat (filename_str, filenum_str);
      switch ((int) itemp)
	{
	case 0:
	  strcpy (filemode_str, "r");
	  break;
	case 1:
	  strcpy (filemode_str, "w");
	  break;
	case 2:
	  strcpy (filemode_str, "rb");
	  break;
	case 3:
	  strcpy (filemode_str, "wb");
	  break;
	}
      if ((fp[itemp2] = fopen (filename_str, filemode_str)) == NULL)
	{
	  error (28);
	  return;
	}
    }

  else if (!strcmp (str, "frac"))
    push (Frac (pop ()));

  else if (!strcmp (str, "frewind"))
    rewind (fp[Round (pop ())]);

  else if (!strcmp (str, "f>c"))
    push ((pop () - 32.0) * 5.0 / 9.0);

  else if (!strcmp (str, "f?"))
    {
      fscanf (fp[Round (pop ())], "%lf", &temp);
      push (temp);
    }

  else if (!strcmp (str, "f?'"))
    {
      fscanf (fp[Round (pop ())], "%c", &ch);
      push ((double) ch);
    }

  else if (!strcmp (str, "f!"))
    {
      sprintf (format_str, "%%%d.", display_width);
      sprintf (temp_str, "%d", display_digits);
      strcat (format_str, temp_str);
      if (display_mode == 0)
	strcat (format_str, "f");
      else if (display_mode == 1)
	strcat (format_str, "E");
      else
	strcat (format_str, "G");
      itemp = Round (pop ());
      fprintf (fp[itemp], format_str, pop ());
    }

  else if (!strcmp (str, "f!'"))
    {
      itemp = Round (pop ());
      fprintf (fp[itemp], "%c", Round (pop ()));
    }

  else if (!strcmp (str, "f\""))
    {
      itemp = Round (pop ());
      do
	{
	  Getchar ();
	  if (ch == '!')
	    fprintf (fp[itemp], "\n");
	  else if (ch != '"')
	    fprintf (fp[itemp], "%c", ch);
	}
      while (ch != '"');
    }

  else if (!strcmp (str, "g"))
    push (GRAV_CONST);

  else if (!strcmp (str, "g0"))
    push (GRAV_ACCEL);

  else if (!strcmp (str, "gal>l"))
    push (pop () * GAL_L);

  else if (!strcmp (str, "ge"))
    {
      temp = pop ();
      push ((pop () >= temp) ? 1 : 0);
    }

  else if (!strcmp (str, "gen"))
    {
      display_mode = 2;
      display_digits = Round (pop ());
    }

  else if (!strcmp (str, "gmearth"))
    push (GM_EARTH);

  else if (!strcmp (str, "gmsun"))
    push (GM_SUN);

  else if (!strcmp (str, "grad"))
    angle_factor = PI / 200.0;

  else if (!strcmp (str, "h"))
    push (PLANCK);

  else if (!strcmp (str, "halfpi"))
    push (0.5 * PI);

  else if (!strcmp (str, "hbar"))
    push (H_BAR);

  else if (!strcmp (str, "hms>h"))
    {
      temp = pop ();
      hr = Int (temp);
      min = Int (100.0 * Frac (temp));
      sec = 100.0 * Frac (100.0 * temp);
      push (hr + min / 60.0 + sec / 3600.0);
    }

  else if (!strcmp (str, "hour"))
    {
      t = time (NULL);
      systime = localtime (&t);
      push ((double) systime->tm_hour);
    }

  else if (!strcmp (str, "h>hms"))
    {
      temp = pop ();
      hr = Int (temp);
      min = Int (60.0 * Frac (temp));
      sec = 60.0 * Frac (60.0 * temp);
      push (hr + min / 100.0 + sec / 10000.0);
    }

  else if (!strcmp (str, "int"))
    push (Int (pop ()));

  else if (!strcmp (str, "in>cm"))
    push (pop () * IN_CM);

  else if (!strcmp (str, "kb"))
    push (BOLTZMANN);

  else if (!strcmp (str, "kg>lb"))
    push (pop () / LB_KG);

  else if (!strcmp (str, "lb>kg"))
    push (pop () * LB_KG);

  else if (!strcmp (str, "le"))
    {
      temp = pop ();
      push ((pop () <= temp) ? 1 : 0);
    }

  else if (!strcmp (str, "ln"))
    {
      temp = pop ();
      if (temp > 0.0)
	push (log (temp));
      else
	error (16);
    }

  else if (!strcmp (str, "log"))
    {
      temp = pop ();
      if (temp > 0.0)
	push (log (temp));
      else
	error (16);
    }

  else if (!strcmp (str, "log2"))
    {
      temp = pop ();
      if (temp > 0.0)
	push (log (temp) / log (2.0));
      else
	error (17);
    }

  else if (!strcmp (str, "log10"))
    {
      temp = pop ();
      if (temp > 0.0)
	push (log10 (temp));
      else
	error (18);
    }

  else if (!strcmp (str, "l>gal"))
    push (pop () / GAL_L);

  else if (!strcmp (str, "me"))
    push (MASS_ELECTRON);

  else if (!strcmp (str, "min"))
    {
      t = time (NULL);
      systime = localtime (&t);
      push ((double) systime->tm_min);
    }

  else if (!strcmp (str, "mn"))
    push (MASS_NEUTRON);

  else if (!strcmp (str, "month"))
    {
      t = time (NULL);
      systime = localtime (&t);
      push ((double) (systime->tm_mon + 1));
    }

  else if (!strcmp (str, "mp"))
    push (MASS_PROTON);

  else if (!strcmp (str, "mu0"))
    push (PERMEABILITY);

  else if (!strcmp (str, "na"))
    push (AVAGADRO);

  else if (!strcmp (str, "ne"))
    {
      temp = pop ();
      push ((pop () != temp) ? 1 : 0);
    }

  else if (!strcmp (str, "nip"))
    {
      temp = pop ();
      pop ();
      push (temp);
    }

  else if (!strcmp (str, "not"))
    {
      itemp = Round (pop ());
      push ((double) (~itemp));
    }

  else if (!strcmp (str, "or"))
    {
      itemp = Round (pop ());
      itemp2 = Round (pop ());
      push ((double) (itemp | itemp2));
    }

  else if (!strcmp (str, "over"))
    {
      temp = pop ();
      temp2 = pop ();
      push (temp2);
      push (temp);
      push (temp2);
    }

  else if (!strcmp (str, "pi"))
    push (PI);

  else if (!strcmp (str, "pnr"))
    {
      itemp = Round (pop ());
      itemp2 = Round (pop ());
      if ((itemp >= 0) && (itemp2 >= 0) && (itemp <= itemp2))
	{
	  temp = 1.0;
	  for (i = itemp2; i >= (itemp2 - itemp + 1); i--)
	    temp *= (double) i;
	  push (temp);
	}
      else
	error (24);
    }

  else if (!strcmp (str, "pow"))
    {
      temp = pop ();
      temp2 = pop ();
      error_flag = ((temp2 == 0.0) &&
		    (temp <= 0.0)) || ((temp2 < 0) && (temp != Round (temp)));
      if (!error_flag)
	push (pow (temp2, temp));
      else
	error (26);
    }

  else if (!strcmp (str, "p>r"))
    {
      temp = pop ();
      temp2 = pop ();
      push (temp * cos (temp2 * angle_factor));
      push (temp * sin (temp2 * angle_factor));
    }

  else if (!strcmp (str, "quit"))
    done = 1;

  else if (!strcmp (str, "rad"))
    angle_factor = 1.0;

  else if (!strcmp (str, "rand"))
    push ((double) rand () / (double) RAND_MAX);

  else if (!strcmp (str, "rcl"))
    {
      itemp = Round (pop ());
      if ((itemp >= 0) && (itemp < ARRAYSIZE))
	push (array[itemp]);
      else
	error (25);
    }

  else if (!strcmp (str, "rearth"))
    push (R_EARTH);

  else if (!strcmp (str, "recip"))
    {
      temp = pop ();
      if (temp != 0.0)
	push (1.0 / temp);
      else
	error (19);
    }

  else if (!strcmp (str, "rev"))
    angle_factor = PI + PI;

  else if (!strcmp (str, "root"))
    {
      temp = pop ();
      temp2 = pop ();
      error_flag = (temp == 0.0) ||
	((temp2 == 0.0) && (temp <= 0.0)) ||
	((temp2 < 0) && ((1.0 / temp) != Round (1.0 / temp)));
      if (!error_flag)
	push (pow (temp2, 1.0 / temp));
      else
	error (27);
    }

  else if (!strcmp (str, "rot"))
    {
      temp = pop ();
      temp2 = pop ();
      temp3 = pop ();
      push (temp2);
      push (temp);
      push (temp3);
    }

  else if (!strcmp (str, "Round"))
    push ((double) Round (pop ()));

  else if (!strcmp (str, "r>d"))
    push (pop () * 180.0 / PI);

  else if (!strcmp (str, "r>p"))
    {
      temp = pop ();
      temp2 = pop ();
      push (atan2 (temp2, temp) / angle_factor);
      push (sqrt (temp * temp + temp2 * temp2));
    }

  else if (!strcmp (str, "sci"))
    {
      display_mode = 1;
      display_digits = Round (pop ());
    }

  else if (!strcmp (str, "sec"))
    {
      t = time (NULL);
      systime = localtime (&t);
      push ((double) systime->tm_sec);
    }

  else if (!strcmp (str, "seed"))
    srand (Round (pop ()));

  else if (!strcmp (str, "shl"))
    {
      itemp = Round (pop ());
      itemp2 = Round (pop ());
      push ((double) (itemp2 << itemp));
    }

  else if (!strcmp (str, "shr"))
    {
      itemp = Round (pop ());
      itemp2 = Round (pop ());
      push ((double) (itemp2 >> itemp));
    }

  else if (!strcmp (str, "sin"))
    push (sin (pop () * angle_factor));

  else if (!strcmp (str, "sinh"))
    push (sinh (pop ()));

  else if (!strcmp (str, "sqr"))
    {
      temp = pop ();
      push (temp * temp);
    }

  else if (!strcmp (str, "sqrt"))
    {
      temp = pop ();
      if (temp >= 0.0)
	push (sqrt (temp));
      else
	error (20);
    }

  else if (!strcmp (str, "sto"))
    {
      itemp = Round (pop ());
      if ((itemp >= 0) && (itemp < ARRAYSIZE))
	array[itemp] = pop ();
      else
	error (25);
    }

  else if (!strcmp (str, "swap"))
    {
      temp = pop ();
      temp2 = pop ();
      push (temp);
      push (temp2);
    }

  else if (!strcmp (str, "tan"))
    push (tan (pop () * angle_factor));

  else if (!strcmp (str, "tanh"))
    push (tanh (pop ()));

  else if (!strcmp (str, "time"))
    push ((double) time (NULL));

  else if (!strcmp (str, "tuck"))
    {
      temp = pop ();
      temp2 = pop ();
      push (temp);
      push (temp2);
      push (temp);
    }

  else if (!strcmp (str, "twopi"))
    push (PI + PI);

  else if (!strcmp (str, "ver"))
    push (VERSION);

  else if (!strcmp (str, "width"))
    display_width = Round (pop ());

  else if (!strcmp (str, "wsize"))
    {
      if ((wordsize >= 1) && (wordsize <= 32))
	wordsize = Round (pop ());
      else
	error (22);
    }

  else if (!strcmp (str, "xor"))
    {
      itemp = Round (pop ());
      itemp2 = Round (pop ());
      push ((double) (itemp ^ itemp2));
    }

  else if (!strcmp (str, "y2x"))
    {
      temp = pop ();
      push (pop () * pow (2.0, temp));
    }

  else if (!strcmp (str, "year"))
    {
      t = time (NULL);
      systime = localtime (&t);
      push ((double) (systime->tm_year + 1900));
    }

  else if (!strcmp (str, "?hex"))
    {
      fgets (instr, 25, stdin);
      chomp (instr);
      sscanf (instr, "%lx", &itemp);
      push ((double) itemp);
    }

  else if (!strcmp (str, "?oct"))
    {
      fgets (instr, 25, stdin);
      chomp (instr);
      sscanf (instr, "%lo", &itemp);
      push ((double) itemp);
    }

  else if (!strcmp (str, "!dec"))
    {
      sprintf (format_str, "%%%d.", display_width);
      sprintf (temp_str, "%dd", display_width);
      strcat (format_str, temp_str);
      printf (format_str, (long) pop ());
    }

  else if (!strcmp (str, "!hex"))
    {
      octhex_digits = ((wordsize - 1) / 4) + 1;
      if (wordsize == 32)
	octhex_mask = 0xFFFFFFFF;
      else
	octhex_mask = (1L << wordsize) - 1;
      sprintf (format_str, "%%%d.", octhex_digits);
      sprintf (temp_str, "%dX", octhex_digits);
      strcat (format_str, temp_str);
      printf (format_str, (long) pop () & octhex_mask);
    }

  else if (!strcmp (str, "!oct"))
    {
      octhex_digits = ((wordsize - 1) / 3) + 1;
      if (wordsize == 32)
	octhex_mask = 0xFFFFFFFF;
      else
	octhex_mask = (1L << wordsize) - 1;
      sprintf (format_str, "%%%d.", octhex_digits);
      sprintf (temp_str, "%do", octhex_digits);
      strcat (format_str, temp_str);
      printf (format_str, (long) pop () & octhex_mask);
    }

  else if (!strcmp (str, "!stk"))
    {
      sprintf (format_str, "%%%d.", display_width);
      sprintf (temp_str, "%d", display_digits);
      strcat (format_str, temp_str);
      if (display_mode == 0)
	strcat (format_str, "f\n");
      else if (display_mode == 1)
	strcat (format_str, "E\n");
      else
	strcat (format_str, "G\n");
      if (sp < 0)
	printf ("Stack empty");
      else
	for (i = 0; i <= sp; i++)
	  printf (format_str, stack[i]);
    }

  else
    error (29);
}

void
chomp (char *str)
{
  int len;
  len = strlen (str);
  if (str[len - 1] == '\n')
    str[len - 1] = '\0';
}

double
Int (double f)
{
  return ((long) (f));
}

double
Frac (double f)
{
  return (f - (long) (f));
}

int
Round (double x)
{
  int result;

  if (x < 0.0)
    result = (int) (x - 0.5);
  else
    result = (int) (x + 0.5);
  return result;
}
