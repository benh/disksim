#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define O_RDONLY _O_RDONLY

char *strdup(char *s)
{
	return _strdup(s);
}
 
int strcasecmp(char *s1, char *s2)
{
	return _stricmp(s1, s2);
}

int read(int fd, void *buf, int bufsiz)
{
	return _read(fd, buf, bufsiz);
}

int write(int fd, void *buf, int bufsiz)
{
	return _write(fd, buf, bufsiz);
}

int dup(int fd)
{
	return _dup(fd);
}

int dup2(int fd1, int fd2)
{
	return _dup2(fd1, fd2);
}

int open(char *filename, int oflag)
{
	return _open(filename, oflag);
}

int close(int fd)
{
	return _close(fd);
}


void bzero(void *ptr, size_t size)
{
	memset(ptr,0,size);
}

#define max(x, y) (((x) > (y)) ? (x) : (y))

/* We accept both forward and backward slash separators */
/* strrchr returns NULL on no match, and NULL should be < any other pointer (???) */ 
#define last_sep(path) max(path, 1+max(strrchr(path, '\\'), strrchr(path, '/')))

	

char *dirname(char *path)
{
	*(last_sep(path)) = '\0';
	return path;
}

char *basename(char *path)
{	
	return last_sep(path);
}

int snprintf(char *str, int n, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	return vsnprintf_s(str, n, _TRUNCATE, format, args);
}
