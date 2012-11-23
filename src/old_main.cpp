/* main.cpp */
/* Vladimir Kolmogorov (vnk@cs.cornell.edu), 2001-2003. */

#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "match.h"

/************************************************************/
/************************************************************/
/************************************************************/

/* global interface variables */
char *g_config;				/* data from the configuration file */
char *g_current;			/* pointer to the current location within g_conf */
char *g_next_line;			/* pointer to the next line */
int g_line_num;				/* number of current line */
#ifdef WIN32
struct _timeb g_tstruct;	/* time */
#else
struct timeb g_tstruct;		/* time */
#endif

/* global interface functions */
void LoadConfigFile(char *name);
char * NextLine();
char * GetWord();
char * GetWord(bool flag);
int GetNumber();
int GetNonnegativeNumber();
int cmp(const char *s1, const char *s2);
void Error(const char *msg);
void StartTimer();
void StopTimer();

/************************************************************/
/************************************************************/
/************************************************************/

void LoadConfigFile(char *name)
{
	int fp;
	int size;

#ifdef WIN32
	if ( (fp = _open(name, _O_RDONLY)) == -1 ) { fprintf(stderr, "Can't open %s\n", name); exit(1); }
	if ( (size = _lseek(fp, 0, SEEK_END)) == -1 ) { fprintf(stderr, "Can't read %s\n", name); exit(1); }
	if ( _lseek(fp, 0, SEEK_SET) == -1 ) { fprintf(stderr, "Can't read %s\n", name); exit(1); }

	if ( (g_config = new char[size+1]) == NULL ) { fprintf(stderr, "Not enough memory to load config file\n"); exit(1); }
	if ( (size = _read(fp, g_config, size)) == -1 ) { fprintf(stderr, "Can't read %s\n", name); exit(1); }
	_close(fp);
#else
	if ( (fp = open(name, O_RDONLY)) == -1 ) { fprintf(stderr, "Can't open %s\n", name); exit(1); }
	if ( (size = lseek(fp, 0, SEEK_END)) == -1 ) { fprintf(stderr, "Can't read %s\n", name); exit(1); }
	if ( lseek(fp, 0, SEEK_SET) == -1 ) { fprintf(stderr, "Can't read %s\n", name); exit(1); }

	if ( (g_config = new char[size+1]) == NULL ) { fprintf(stderr, "Not enough memory to load config file\n"); exit(1); }
	if ( (size = read(fp, g_config, size)) == -1 ) { fprintf(stderr, "Can't read %s\n", name); exit(1); }
	close(fp);
#endif

	g_config[size] = 0;

	g_current = NULL;
	g_next_line = g_config;
	g_line_num = 0;
}

char * NextLine()
{
	g_current = g_next_line;
	char c;

	if (g_next_line)
	{
		for ( ; ; g_next_line++)
		{
			c = *g_next_line;
			if (!c) { g_next_line = NULL; break; }
			if (c==(char)10) { *g_next_line++ = 0; break; }
		}
	}

	g_line_num ++;
	return g_current;
}

char * GetWord()
{
	char *ptr;
	char c;

	if (!g_current) return NULL;

	for (; ; g_current++)
	{
		c = *g_current;
		if (!c || c==(char)10 || c=='#') return NULL;
		if (c!=' ' && c!='\t' && c!=(char)13) break;
	}

	ptr = g_current;

	for (; ; g_current++)
	{
		c = *g_current;
		if (!c || c=='#') { *g_current = 0; g_current = NULL; return ptr; }
		if (c==' ' || c=='\t' || c==(char)13) { *g_current++ = 0; return ptr; }
	}
}

char * GetWord(bool flag)
{
	char *ptr = GetWord();

	if (flag && !ptr) Error("too few parameters!\n");
	if (!flag && ptr) Error("too many parameters!\n");
	return ptr;
}

int GetNumber()
{
	char *ptr;
	int n;

	ptr = GetWord(true);
	if (!cmp(ptr, "INFINITY")) return INFINITY;
	if (sscanf(ptr, "%d", &n) != 1) Error("wrong parameter\n");
	return n;
}

void GetFraction(int *nominator, int *denominator)
{
	char *ptr;

	ptr = GetWord(true);
	if (!cmp(ptr, "INFINITY")) { *nominator = INFINITY; *denominator = 1; return; }
	if (!cmp(ptr, "AUTO")) { *nominator = -1; *denominator = 1; return; }
	if (sscanf(ptr, "%d/%d", nominator, denominator) != 2)
	{
		if (sscanf(ptr, "%d", nominator) != 1) Error("wrong parameter\n");
		*denominator = 1;
	}
	if (*nominator < 0 || *denominator < 1) Error("wrong parameter\n");
}

int GetNonnegativeNumber()
{
	int n = GetNumber();
	if (n < 0) Error("wrong parameter\n");
	return n;
}

void GetNonnegativeFraction(int *nominator, int *denominator)
{
	char *ptr;

	ptr = GetWord(true);
	if (!cmp(ptr, "INFINITY")) { *nominator = INFINITY; *denominator = 1; return; }
	if (sscanf(ptr, "%d/%d", nominator, denominator) != 2)
	{
		if (sscanf(ptr, "%d", nominator) != 1) Error("wrong parameter\n");
		*denominator = 1;
	}
	if (*nominator < 0 || *denominator < 1) Error("wrong parameter\n");
}

int cmp(const char *s1, const char *s2)
{
	char c1, c2;

	do
	{
		c1 = *s1 ++;
		c2 = *s2 ++;
		if (c1>='a' && c1<='z') c1 += 'A' - 'a';
		if (c2>='a' && c2<='z') c2 += 'A' - 'a';
		if (c1 != c2) return 1;
	} while (c1);

	return 0;
}

void Error(const char *msg)
{
	fprintf(stderr, "Error in line %d: %s\n", g_line_num, msg);
	exit(1);
}

#ifdef WIN32
void StartTimer()
{
	_ftime(&g_tstruct);
}

void StopTimer()
{
	struct _timeb tstruct_new;
	time_t t;

	_ftime(&tstruct_new);
	t = 1000*(tstruct_new.time - g_tstruct.time) + (tstruct_new.millitm - g_tstruct.millitm);
	printf("%.2f secs\n\n", (float)t/1000);
}
#else
void StartTimer()
{
	ftime(&g_tstruct);
}

void StopTimer()
{
	struct timeb tstruct_new;
	time_t t;

	ftime(&tstruct_new);
	t = 1000*(tstruct_new.time - g_tstruct.time) + (tstruct_new.millitm - g_tstruct.millitm);
	printf("%.2f secs\n\n", (float)t/1000);
}
#endif

/************************************************************/
/************************************************************/
/************************************************************/

int gcd(int a, int b)
{
	if (a < b) return gcd(b, a);
	if (b == 0) return a;
	int r = a % b;
	if (r == 0) return b;
	return gcd(b, r);
}

/************************************************************/
/************************************************************/
/************************************************************/

int main(int argc, char *argv[])
{
	/* default parameters */
	Match *m = NULL;
	Coord disp_base = Coord(0, 0), disp_max = Coord(0, 0);
	struct Match::Parameters params = {
		true, Match::Parameters::L2, 1, /* subpixel, data_cost, denominator */
		8, -1, -1,       /* I_threshold2, lambda1, lambda2 */
		-1,                 /* K */
		INFINITY, false,    /* iter_max, randomize_every_iteration */
	};
	int lambda = -1, denom = 1;

	char *ptr, *ptr1, *ptr2;
	bool flag;

	if (argc != 2) { printf("Usage: %s <config_file>\n", argv[0]); exit(1); }
	LoadConfigFile(argv[1]);

	/* main loop */
	while (NextLine())
	{
		ptr = GetWord();
		if (!ptr) continue;

		if (!cmp(ptr, "LOAD"))
		{
			ptr1 = GetWord(true);
			ptr2 = GetWord(true);
			GetWord(false);
			if (m) delete m;
			m = new Match(ptr1, ptr2, false);
			m -> SetDispRange(disp_base, disp_max);
			continue;
		}

		if (!cmp(ptr, "LOAD_COLOR"))
		{
			ptr1 = GetWord(true);
			ptr2 = GetWord(true);
			GetWord(false);
			if (m) delete m;
			m = new Match(ptr1, ptr2, true);
			m -> SetDispRange(disp_base, disp_max);
			continue;
		}

		if (!cmp(ptr, "LOAD_SEGM"))
		{
			ptr1 = GetWord(true);
			ptr2 = GetWord(true);
			GetWord(false);
			if (!m) Error("load images first!");
			m -> LoadSegm(ptr1, ptr2, false);
			continue;
		}

		if (!cmp(ptr, "LOAD_SEGM_COLOR"))
		{
			ptr1 = GetWord(true);
			ptr2 = GetWord(true);
			GetWord(false);
			if (!m) Error("load images first!");
			m -> LoadSegm(ptr1, ptr2, true);
			continue;
		}

		if (!cmp(ptr, "SAVE_X"))
		{
			ptr = GetWord(true);
			if ((ptr1=GetWord())!=0)
			{
				if (!(cmp(ptr1, "LARGER_DISPARITIES_BRIGHTER"))) flag = true;
				else Error("unknown parameter!");
				GetWord(false);
			}
			else flag = false;
			if (!m) Error("load images before calling the function!");
			m -> SaveXLeft(ptr, flag);
			continue;
		}

		if (!cmp(ptr, "SAVE_Y"))
		{
			ptr = GetWord(true);
			if ((ptr1=GetWord())!=0)
			{
				if (!(cmp(ptr1, "LARGER_DISPARITIES_BRIGHTER"))) flag = true;
				else Error("unknown parameter!");
				GetWord(false);
			}
			else flag = false;
			if (!m) Error("load images before calling the function!");
			m -> SaveYLeft(ptr, flag);
			continue;
		}

		if (!cmp(ptr, "SAVE_X_SCALED"))
		{
			ptr = GetWord(true);
			if ((ptr1=GetWord())!=0)
			{
				if (!(cmp(ptr1, "LARGER_DISPARITIES_BRIGHTER"))) flag = true;
				else Error("unknown parameter!");
				GetWord(false);
			}
			else flag = false;
			if (!m) Error("load images before calling the function!");
			m -> SaveScaledXLeft(ptr, flag);
			continue;
		}

		if (!cmp(ptr, "SAVE_Y_SCALED"))
		{
			ptr = GetWord(true);
			if ((ptr1=GetWord())!=0)
			{
				if (!(cmp(ptr1, "LARGER_DISPARITIES_BRIGHTER"))) flag = true;
				else Error("unknown parameter!");
				GetWord(false);
			}
			else flag = false;
			if (!m) Error("load images before calling the function!");
			m -> SaveScaledYLeft(ptr, flag);
			continue;
		}

		if (!cmp(ptr, "LOAD_X"))
		{
			ptr = GetWord(true);
			if ((ptr1=GetWord())!=0)
			{
				if (!(cmp(ptr1, "LARGER_DISPARITIES_BRIGHTER"))) flag = true;
				else Error("unknown parameter!");
				GetWord(false);
			}
			else flag = false;
			if (!m) Error("load images before calling the function!");
			m -> LoadXLeft(ptr, flag);
			continue;
		}

		if (!cmp(ptr, "LOAD_Y"))
		{
			ptr = GetWord(true);
			if ((ptr1=GetWord())!=0)
			{
				if (!(cmp(ptr1, "LARGER_DISPARITIES_BRIGHTER"))) flag = true;
				else Error("unknown parameter!");
				GetWord(false);
			}
			else flag = false;
			if (!m) Error("load images before calling the function!");
			m -> LoadYLeft(ptr, flag);
			continue;
		}

		if (!cmp(ptr, "UNSET"))
		{
			ptr = GetWord(true);
			if (!cmp(ptr, "randomize_every_iteration"))
			{
				GetWord(false);
				params.randomize_every_iteration = false;
				continue;
			}
			if (!cmp(ptr, "sub_pixel"))
			{
				GetWord(false);
				params.sub_pixel = false;
				continue;
			}
			Error("unknown parameter!");
		}

		if (!cmp(ptr, "SET"))
		{
			ptr = GetWord(true);
			if (!cmp(ptr, "disp_range"))
			{
				disp_base.x = GetNumber();
				disp_base.y = GetNumber();
				disp_max.x = GetNumber();
				disp_max.y = GetNumber();
				GetWord(false);
				if (m) m -> SetDispRange(disp_base, disp_max);
				printf("Disparity range: x_min = %d, y_min = %d, x_max = %d, y_max = %d\n\n", disp_base.x, disp_base.y, disp_max.x, disp_max.y);
				continue;
			}
			if (!cmp(ptr, "iter_max"))
			{
				params.iter_max = GetNonnegativeNumber();
				GetWord(false);
				continue;
			}
			if (!cmp(ptr, "randomize_every_iteration"))
			{
				GetWord(false);
				params.randomize_every_iteration = true;
				continue;
			}
			if (!cmp(ptr, "sub_pixel"))
			{
				GetWord(false);
				params.sub_pixel = true;
				continue;
			}
			if (!cmp(ptr, "data_cost"))
			{
				ptr = GetWord(true);
				if      (!cmp(ptr, "L1")) params.data_cost = Match::Parameters::L1;
				else if (!cmp(ptr, "L2")) params.data_cost = Match::Parameters::L2;
				else Error("unknown parameter!");
				GetWord(false);
				continue;
			}
			if (!cmp(ptr, "I_threshold2"))
			{
				params.I_threshold2 = GetNonnegativeNumber();
				GetWord(false);
				continue;
			}
			if (!cmp(ptr, "lambda"))
			{
				GetFraction(&lambda, &denom);
				lambda *= params.denominator;
				params.lambda1 *= denom;
				params.lambda2 *= denom;
				params.K *= denom;
				params.denominator *= denom;
				GetWord(false);
				continue;
			}
			if (!cmp(ptr, "lambda1"))
			{
				GetFraction(&params.lambda1, &denom);
				lambda *= denom;
				params.lambda1 *= params.denominator;
				params.lambda2 *= denom;
				params.K *= denom;
				params.denominator *= denom;
				GetWord(false);
				continue;
			}
			if (!cmp(ptr, "lambda2"))
			{
				GetFraction(&params.lambda2, &denom);
				lambda *= denom;
				params.lambda1 *= denom;
				params.lambda2 *= params.denominator;
				params.K *= denom;
				params.denominator *= denom;
				GetWord(false);
				continue;
			}
			if (!cmp(ptr, "K"))
			{
				GetFraction(&params.K, &denom);
				lambda *= denom;
				params.lambda1 *= denom;
				params.lambda2 *= denom;
				params.K *= params.denominator;
				params.denominator *= denom;
				GetWord(false);
				continue;
			}
			Error("unknown parameter!");
		}

		if (!cmp(ptr, "CLEAR"))
		{
			GetWord(false);
			if (!m) Error("load images before calling the function!");
			m -> CLEAR();
			continue;
		}

		if (!cmp(ptr, "SWAP_IMAGES"))
		{
			GetWord(false);
			if (!m) Error("load images before calling the function!");
			m -> SWAP_IMAGES();
			continue;
		}

		if (!cmp(ptr, "MAKE_UNIQUE"))
		{
			GetWord(false);
			if (!m) Error("load images before calling the function!");
			m -> MAKE_UNIQUE();
			continue;
		}

		if (!cmp(ptr, "FILL_OCCLUSIONS"))
		{
			GetWord(false);
			if (!m) Error("load images before calling the function!");
			m -> FILL_OCCLUSIONS();
			continue;
		}

		if (!cmp(ptr, "KZ2"))
		{
			GetWord(false);
			if (!m) Error("load images before calling the function!");
			StartTimer();
			if (lambda<0 && (params.K<0 || params.lambda1 || params.lambda2<0))
			{
				m -> SetParameters(&params);
				float v = m->GetK()/5; denom = 1;
				while (v < 3) { v *= 2; denom *= 2; }
				lambda = int(v + (float)0.5);
				lambda *= params.denominator;
				params.lambda1 *= denom;
				params.lambda2 *= denom;
				params.K *= denom;
				params.denominator *= denom;
			}
			if (params.K<0) params.K = 5*lambda;
			if (params.lambda1<0) params.lambda1 = 3*lambda;
			if (params.lambda2<0) params.lambda2 = lambda;
			denom = gcd(params.K, gcd(params.lambda1, gcd(params.lambda2, params.denominator)));
			if (denom)
			{
				params.K /= denom;
				params.lambda1 /= denom;
				params.lambda2 /= denom;
				params.denominator /= denom;
			}
			m -> SetParameters(&params);
			m -> KZ2();
			StopTimer();
			continue;
		}

		Error("unknown parameter!");
	}

	printf("done\n");

	return 0;
}

