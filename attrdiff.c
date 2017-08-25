/* attrdiff.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <err.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>

#define PROGNAME	"attrdiff"

#define USERMAX		32
#define GROUPMAX	32

int	cmp(char *, char *);
void	pwname(uid_t, char *);
void	grpname(gid_t,char *);
void	usage(void);

int
main(int argc, char *argv[])
{
	int	ch;
	char	*lpath, *rpath;
	DIR	*dh;

	struct stat	lstat, rstat;
	struct dirent	*ent;

	lpath = rpath = NULL;

	while ((ch = getopt(argc,argv,"h")) != -1) {
		switch(ch) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 2) {
		usage();
		exit(EXIT_FAILURE);
	}

	lpath = argv[0];
	rpath = argv[1];

	if (stat(lpath,&lstat) != 0)
		err(EXIT_FAILURE,"failed to stat left %s",lpath);
	if (stat(rpath,&rstat) != 0)
		err(EXIT_FAILURE,"failed to stat right %s",rpath);

	if (S_ISREG(lstat.st_mode)) {
		if (!S_ISREG(rstat.st_mode))
			errx(EXIT_FAILURE,"left and right must be the same type");
		return cmp(lpath,rpath);
	} else if (S_ISDIR(lstat.st_mode)) {
		if (!S_ISDIR(rstat.st_mode))
			errx(EXIT_FAILURE,"left and right must be the same type");
		if ((dh = opendir(lpath)) == NULL)
			err(EXIT_FAILURE,"cannot open left directory");
		while ((ent = readdir(dh)) != NULL) {
			if (strcmp(ent->d_name,".") == 0)
				continue;
			if (strcmp(ent->d_name,"..") == 0)
				continue;
			printf("----> %s\n",ent->d_name);
		}
	} else {
		errx(EXIT_FAILURE,"must be a file or directory");
	}

	return EXIT_SUCCESS;
}

int
cmp(char *lpath, char *rpath)
{
	struct stat	lst, rst;

	char	luser[USERMAX + 1];
	char	lgroup[GROUPMAX + 1];
	char	ruser[USERMAX + 1];
	char	rgroup[GROUPMAX + 1];

	if (lstat(lpath,&lst) != 0) {
		warn("failed to stat left %s",lpath);
		return errno;
	}
	if (lstat(rpath,&rst) != 0) {
		warn("failed to stat right %s",rpath);
		return errno;
	}

	pwname(lst.st_uid,luser);
	pwname(rst.st_uid,ruser);

	if (lst.st_uid != rst.st_uid)
		fprintf(stdout,"%s user %s %s\n",lpath,luser,ruser);

	grpname(lst.st_gid,lgroup);
	grpname(rst.st_gid,rgroup);

	if (lst.st_gid != rst.st_gid)
		fprintf(stdout,"%s group %s %s\n",lpath,lgroup,rgroup);

	return EXIT_SUCCESS;
}

void
usage(void)
{
	fprintf(stderr,"usage: %s <left> <right>\n",PROGNAME);
}

void
pwname(uid_t uid, char *name)
{
	struct passwd	*pwd;

	if ((pwd = getpwuid(uid)) != NULL)
		snprintf(name,USERMAX,"%s",pwd->pw_name);
	else
		snprintf(name,USERMAX,"%u",uid);

}

void
grpname(gid_t gid, char *name)
{
	struct group	*grp;

	if ((grp = getgrgid(gid)) != NULL)
		snprintf(name,GROUPMAX,"%s",grp->gr_name);
	else
		snprintf(name,GROUPMAX,"%u",gid);
}

