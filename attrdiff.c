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

#define USERMAX		32
#define GROUPMAX	32
#define FTYPEMAX	10

typedef enum sttype {
	TYPE_REG,
	TYPE_DIR,
	TYPE_LINK,
	TYPE_UNKNOWN = 99
} sttype;

int	dir(char *, char *, char *);
int	reverse(char *, char *, char *);
int	cmp(char *, char *);
sttype	ftype(struct stat *);
int	strftype(sttype, char *);
void	pwname(uid_t, char *);
void	grpname(gid_t,char *);
int	perm_int(struct stat *);
char	*perm_str(struct stat *);	/* you must free this */
void	usage(void);

char	lprefix[PATH_MAX], rprefix[PATH_MAX];

int	Rflag = 0;
int	Iflag = 0;

char	*ignoreglob;

int
main(int argc, char *argv[])
{
	int	ch;

	struct stat	lstat, rstat;

	while ((ch = getopt(argc,argv,"hRI:")) != -1) {
		switch(ch) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			case 'I':
				if (Iflag)
					errx(EINVAL,"only one -I is allowed");
				Iflag = 1;
				ignoreglob = strdup(optarg);
				break;
			case 'R':
				Rflag = 1;
				break;
			case '?':
				usage();
				exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 2) {
		usage();
		exit(EXIT_FAILURE);
	}

	snprintf(lprefix,sizeof(lprefix),"%s",argv[0]);
	snprintf(rprefix,sizeof(rprefix),"%s",argv[1]);

	if (stat(lprefix,&lstat) != 0)
		err(EXIT_FAILURE,"failed to stat left %s",lprefix);
	if (stat(rprefix,&rstat) != 0)
		err(EXIT_FAILURE,"failed to stat right %s",rprefix);

	if (S_ISREG(lstat.st_mode)) {
		if (!S_ISREG(rstat.st_mode))
			errx(EXIT_FAILURE,"left and right must be the same type");
		return cmp(lprefix,rprefix);
	} else if (S_ISDIR(lstat.st_mode)) {
		if (!S_ISDIR(rstat.st_mode))
			errx(EXIT_FAILURE,"left and right must be the same type");
		if (dir(lprefix,rprefix,NULL))
			errx(EXIT_FAILURE,"failed to process directories");
		if (Rflag) {
			if (reverse(lprefix,rprefix,NULL))
				errx(EXIT_FAILURE,"failed to process reverse mode");
		}
	} else {
		free(ignoreglob);
		errx(EXIT_FAILURE,"must be a file or directory");
	}

	return EXIT_SUCCESS;
}

int
dir(char *lpath, char *rpath, char *subdir)
{
	char		*l, *r, *lw, *rw;
	DIR		*dh;
	struct dirent	*ent;
	struct stat	st;

	l = calloc(PATH_MAX,sizeof(char));
	r = calloc(PATH_MAX,sizeof(char));
	lw = calloc(PATH_MAX,sizeof(char));
	rw = calloc(PATH_MAX,sizeof(char));

	if (subdir == NULL) {
		snprintf(l,PATH_MAX,"%s",lpath);
		snprintf(r,PATH_MAX,"%s",rpath);
	} else {
		snprintf(l,PATH_MAX,"%s/%s",lpath,subdir);
		snprintf(r,PATH_MAX,"%s/%s",rpath,subdir);
	}

	if ((dh = opendir(l)) == NULL) {
		warn("failed to open left directory %s",l);
		free(l);
		free(r);
		free(lw);
		free(rw);
		return EXIT_FAILURE;
	}
	while ((ent = readdir(dh)) != NULL) {
		if (strcmp(ent->d_name,".") == 0)
			continue;
		if (strcmp(ent->d_name,"..") == 0)
			continue;
		snprintf(lw,PATH_MAX,"%s/%s",l,ent->d_name);
		if ((Iflag) && (strstr(lw,ignoreglob) != NULL))
			continue;
		snprintf(rw,PATH_MAX,"%s/%s",r,ent->d_name);
		if (lstat(lw,&st) != 0) {
			switch(errno) {
				case EACCES:
					printf("%s\n\terror access (%s)\n",lw,strerror(errno));
					break;
				default:
					printf("%s\n\terror unknown (%s)\n",lw,strerror(errno));
					break;
			}
			continue;
		}
		cmp(lw,rw);
		if (S_ISDIR(st.st_mode)) {
			dir(l,r,ent->d_name);
			continue;
		}
	}

	closedir(dh);
	free(l);
	free(r);
	free(lw);
	free(rw);
	return EXIT_SUCCESS;
}

int
reverse(char *lpath, char *rpath, char *subdir)
{
	char		*l, *r, *lw, *rw;
	DIR		*dh;
	struct dirent	*ent;
	struct stat	st;

	static int	banner = 0;
#define RBANNER()	if (banner == 0) { \
				printf("--\n"); \
				banner = 1; \
			}

	l = calloc(PATH_MAX,sizeof(char));
	r = calloc(PATH_MAX,sizeof(char));
	lw = calloc(PATH_MAX,sizeof(char));
	rw = calloc(PATH_MAX,sizeof(char));

	if (subdir == NULL) {
		snprintf(l,PATH_MAX,"%s",lpath);
		snprintf(r,PATH_MAX,"%s",rpath);
	} else {
		snprintf(l,PATH_MAX,"%s/%s",lpath,subdir);
		snprintf(r,PATH_MAX,"%s/%s",rpath,subdir);
	}

	if ((dh = opendir(r)) == NULL) {
		warn("failed to open right directory %s",l);
		free(l);
		free(r);
		free(lw);
		free(rw);
		return EXIT_FAILURE;
	}
	while ((ent = readdir(dh)) != NULL) {
		if (strcmp(ent->d_name,".") == 0)
			continue;
		if (strcmp(ent->d_name,"..") == 0)
			continue;
		if ((Iflag) && (strstr(ent->d_name,ignoreglob)))
			continue;
		snprintf(lw,PATH_MAX,"%s/%s",l,ent->d_name);
		snprintf(rw,PATH_MAX,"%s/%s",r,ent->d_name);
		if (lstat(rw,&st) != 0) {
			switch(errno) {
				case EACCES:
					RBANNER();
					printf("%s\n\terror access (%s)\n",lw,strerror(errno));
					break;
				default:
					RBANNER();
					printf("%s\n\terror unknown (%s)\n",lw,strerror(errno));
					break;
			}
			continue;
		}
		if (access(lw,F_OK) != 0) {
			switch (errno) {
				case EACCES:
					RBANNER();
					printf("%s\n\terror access (%s)\n",lw,strerror(errno));
					break;
				case ENOENT:
					RBANNER();
					printf("%s\n\tmissing\n",lw);
					break;
				default:
					RBANNER();
					printf("%s\n\terror unknown (%s)\n",lw,strerror(errno));
					break;
			}
		}
		if (S_ISDIR(st.st_mode)) {
			reverse(l,r,ent->d_name);
			continue;
		}
	}

	closedir(dh);
	free(l);
	free(r);
	free(lw);
	free(rw);
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

	int	lperm, rperm;

	sttype	ltype, rtype;

	int banner = 0;
#define BANNER()	if (banner == 0) { \
				size_t llen = strlen(lprefix); \
				if (strstr(lprefix,"/") == lprefix) \
					llen++; \
				printf("%s\n",(lpath + llen)); \
				banner = 1; \
			}

	if (lstat(lpath,&lst) != 0) {
		switch(errno) {
			case ENOENT:
				BANNER();
				printf("\tmissing\n");
				break;
			case EACCES:
				BANNER();
				printf("\terror access (%s)\n",strerror(errno));
				break;
			case ENOTDIR:
				BANNER();
				printf("\terror directory\n");
				break;
			default:
				BANNER();
				printf("\terror stat (%s)\n",strerror(errno));
				break;
		}
		return errno;
	}
	if (lstat(rpath,&rst) != 0) {
		switch(errno) {
			case ENOENT:
				BANNER();
				printf("\tmissing\n");
				break;
			case EACCES:
				BANNER();
				printf("\terror access (%s)\n",strerror(errno));
				break;
			case ENOTDIR:
				BANNER();
				printf("\terror directory\n");
				break;
			default:
				BANNER();
				printf("\terror stat (%s)\n",strerror(errno));
				break;
		}
		return errno;
	}

	if ((ltype = ftype(&lst)) == TYPE_UNKNOWN) {
		BANNER();
		printf("\tfiletype unknown left\n");
	}
	if ((rtype = ftype(&rst)) == TYPE_UNKNOWN) {
		BANNER();
		printf("\tfiletype unknown right\n");
	}

	if (ltype != rtype) {
		char *lstr = calloc(FTYPEMAX,sizeof(char));
		char *rstr = calloc(FTYPEMAX,sizeof(char));
		strftype(ltype,lstr);
		strftype(rtype,rstr);
		BANNER();
		printf("\tfiletype %s %s\n",lstr,rstr);
		free(lstr);
		free(rstr);
	}

	pwname(lst.st_uid,luser);
	pwname(rst.st_uid,ruser);

	if (lst.st_uid != rst.st_uid) {
		BANNER();
		printf("\tuser %s %s\n",luser,ruser);
	}

	grpname(lst.st_gid,lgroup);
	grpname(rst.st_gid,rgroup);

	if (lst.st_gid != rst.st_gid) {
		BANNER();
		printf("\tgroup %s %s\n",lgroup,rgroup);
	}

	lperm = perm_int(&lst);
	rperm = perm_int(&rst);

	if (lperm != rperm) {
		BANNER();
		printf("\tpermissions %d %d\n",lperm,rperm);
	}

	return EXIT_SUCCESS;
}

sttype
ftype(struct stat *st)
{
	if (S_ISREG(st->st_mode))
		return TYPE_REG;
	else if (S_ISDIR(st->st_mode))
		return TYPE_DIR;
	else if (S_ISLNK(st->st_mode))
		return TYPE_LINK;
	else
		return TYPE_UNKNOWN;
}

int
strftype(sttype t, char *r)
{
	switch(t) {
		case TYPE_REG:
			snprintf(r,FTYPEMAX,"%s","file");
			break;
		case TYPE_DIR:
			snprintf(r,FTYPEMAX,"%s","directory");
			break;
		case TYPE_LINK:
			snprintf(r,FTYPEMAX,"%s","symlink");
			break;
		default:
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void
usage(void)
{
	fprintf(stderr,"usage: attrdiff [-R] <left> <right>\n");
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

int
perm_int(struct stat *st)
{
	int r = 0;

	if (st->st_mode & S_IRUSR)
		r += 400;
	if (st->st_mode & S_IWUSR)
		r += 200;
	if (st->st_mode & S_IXUSR)
		r += 100;
	if (st->st_mode & S_IRGRP)
		r += 40;
	if (st->st_mode & S_IWGRP)
		r += 20;
	if (st->st_mode & S_IXGRP)
		r += 10;
	if (st->st_mode & S_IROTH)
		r += 4;
	if (st->st_mode & S_IWOTH)
		r += 2;
	if (st->st_mode & S_IXOTH)
		r += 1;

	return r;
}

char *
perm_str(struct stat *st)
{
	char	*buf = calloc(10,sizeof(char));
	strncat(buf,(st->st_mode & S_IRUSR) ? "r" : "-",sizeof(char));
	strncat(buf,(st->st_mode & S_IWUSR) ? "w" : "-",sizeof(char));
	strncat(buf,(st->st_mode & S_IXUSR) ? "x" : "-",sizeof(char));
	strncat(buf,(st->st_mode & S_IRGRP) ? "r" : "-",sizeof(char));
	strncat(buf,(st->st_mode & S_IWGRP) ? "w" : "-",sizeof(char));
	strncat(buf,(st->st_mode & S_IXGRP) ? "x" : "-",sizeof(char));
	strncat(buf,(st->st_mode & S_IROTH) ? "r" : "-",sizeof(char));
	strncat(buf,(st->st_mode & S_IWOTH) ? "w" : "-",sizeof(char));
	strncat(buf,(st->st_mode & S_IXOTH) ? "x" : "-",sizeof(char));
	return buf;
}

