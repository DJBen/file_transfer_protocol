#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 10

int main(int argc, char **argv)
{
  FILE *fr; /* Pointer to source file, which we read */
  FILE *fw; /* Pointer to dest file, which we write  */
  char buf[BUF_SIZE];
  int nread, nwritten;

  if(argc != 3) {
    printf("Usage: file_copy <source_file> <destination_file>\n");
    exit(0);
  }

  /* Open the source file for reading */
  if((fr = fopen(argv[1], "r")) == NULL) {
    perror("fopen");
    exit(0);
  }
  printf("Opened %s for reading...\n", argv[1]);

  /* Open or create the destination file for writing */
  if((fw = fopen(argv[2], "w")) == NULL) {
    perror("fopen");
    exit(0);
  }

  /* We'll read in the file BUF_SIZE bytes at a time, and write it
   * BUF_SIZE bytes at a time.*/
  for(;;) {

    /* Read in a chunk of t`he file */
    nread = fread(buf, 1, BUF_SIZE, fr);

    /* If there is something to write, write it */
    if(nread > 0)
      nwritten = fwrite(buf, 1, nread, fw);

    /* fread returns a short count either at EOF or when an error occurred */
    if(nread < BUF_SIZE) {

      /* Did we reach the EOF? */
      if(feof(fr)) {
	printf("Finished writing.\n");
	break;
      }
      else {
	printf("An error occurred...\n");
	exit(0);
      }
    }
  }

  /* Cleanup */
  fclose(fr);
  fclose(fw);

  return 0;
}
