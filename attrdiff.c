#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <err.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>

#define	VERSION	"0.0"

struct attrdiff {
	char	*version;
	int	 verbose;
	int	 recursion_max;
	int	 recursion_level;
	int	 do_reverse;
	char	*left_prefix;
	char	*right_prefix;
};

struct attrdiff	*ctx;

void	usage(void);
void	version(void);
void	verbose(char *, ...);

int	walk(char *);
int	compare(char *, struct stat *, struct stat *);

#define EQ(x,y)	(strcmp(x,y) == 0)

void
version(void)
{
	printf("attrdiff %s\n",VERSION);
}

void
usage(void)
{
	fprintf(stderr,"usage: attrdiff [-Vhvr] [-l max-recursion] <left> <right>\n");
}

void
verbose(char *fmt, ...)
{
	va_list	va;

	if (! ctx->verbose)
		return;

	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
}

int
main(int argc, char *argv[])
{
	int	ch;

	if ((ctx = calloc(1,sizeof(struct attrdiff))) == NULL)
		err(EXIT_FAILURE,"no memory");

	ctx->recursion_level = 1;

	while ((ch = getopt(argc,argv,"hVvrl:")) != -1) {
		switch(ch) {
		case 'V':
			version();
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'v':
			ctx->verbose++;
			break;
		case 'r':
			ctx->do_reverse = 1;
			break;
		case 'l':
			ctx->recursion_max = strtol(optarg,NULL,0);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 2) {
		warnx("missing arguments");
		usage();
		exit(EXIT_FAILURE);
	}

	ctx->left_prefix	= strdup(argv[0]);
	ctx->right_prefix	= strdup(argv[1]);

	verbose("====> %s\n",ctx->left_prefix);
	walk(NULL);

	free(ctx->left_prefix);
	free(ctx->right_prefix);

	return (EXIT_SUCCESS);
}

int
walk(char *relative)
{
	DIR		*dp;
	struct dirent	*ent;
	struct stat	lst, rst;
	char		*lwd, *rwd;
	char		*left_abs, *right_abs;
	char		*work;

	if (relative) {
		asprintf(&lwd,"%s/%s",ctx->left_prefix,relative);
		asprintf(&rwd,"%s/%s",ctx->right_prefix,relative);
	} else {
		lwd = strdup(ctx->left_prefix);
		rwd = strdup(ctx->right_prefix);
	}

	if ((dp = opendir(lwd)) == NULL) {
		warn("failed to open '%s'",lwd);
		free(lwd);
		free(rwd);
		return (EXIT_FAILURE);
	}

	while ((ent = readdir(dp)) != NULL) {
		if (EQ(ent->d_name,".") || EQ(ent->d_name,".."))
			continue;

		asprintf(&left_abs,"%s/%s",lwd,ent->d_name);
		asprintf(&right_abs,"%s/%s",rwd,ent->d_name);

		if (relative)
			asprintf(&work,"%s/%s",relative,ent->d_name);
		else
			work = strdup(ent->d_name);

		verbose("----> %s\n",work);

		if (lstat(left_abs,&lst) != 0) {
			warn("failed to stat '%s'",left_abs);
			free(left_abs);
			free(right_abs);
			continue;
		}

		if (lstat(right_abs,&rst) == 0) {
			compare(work,&lst,&rst);
		} else {
			fprintf(stderr,"* %s: missing\n",right_abs);
		}

		if (S_ISDIR(lst.st_mode)
		    && ((ctx->recursion_level < ctx->recursion_max)
		    || (ctx->recursion_max == 0)))
		{
			verbose("====> %s\n",work);
			walk(work);
			free(work);
		}

		free(left_abs);
		free(right_abs);
	}

	closedir(dp);
	free(lwd);
	free(rwd);

	return (EXIT_SUCCESS);
}

int
compare(char *work, struct stat *lst, struct stat *rst)
{

	if (lst->st_uid != rst->st_uid) {
		printf("- %s uid:%d\n",work,rst->st_uid);
		printf("+ %s uid:%d\n",work,lst->st_uid);
	}

	return (EXIT_SUCCESS);
}
