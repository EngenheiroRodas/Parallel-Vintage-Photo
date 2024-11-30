#include <dirent.h> 
#include <stdio.h> 
#include <sys/stat.h>

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
    return 1;
  }

  DIR *d;
  struct dirent *f;
  d = opendir(argv[1]);
  if (d)
  {
    while ((f = readdir(d)) != NULL)
    {
      printf("%s ", f->d_name);
      struct stat st;
      char filepath[1024];
      snprintf(filepath, sizeof(filepath), "%s/%s", argv[1], f->d_name);
      stat(filepath, &st);
      printf("%ld\n", st.st_size);
    }
    closedir(d);
  }
  else
  {
    perror("opendir");
    return 1;
  }
  return 0;
}