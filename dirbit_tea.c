
/**
 * DirBitTea 0.0.1
 * File creation watcher for Linux system.
 *
 * @author Ammar Faizi <ammarfaizi2@gmail.com>
 * @license MIT
 * @version 0.0.1
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

#define SCAN_DELAY 500000 // in ms
#define HASH_TABLE_GROWTH_SIZE 5000

typedef struct dirbit_tea_hash_table_ {
	time_t ctim;
	time_t mtim;
	size_t filenamelen;
	char filename[1];
} dirbit_tea_hash_table;

time_t last_scan;
dirbit_tea_hash_table **hash_tables;
size_t hash_table_allocated = HASH_TABLE_GROWTH_SIZE;
size_t hash_table_count = 0;
bool has_been_scanned = false;

bool findOrFail(char *filename, size_t filenamelen, struct stat *stfile)
{
	for (size_t i = 0; i < hash_table_count; ++i) {
		if (!strncmp(hash_tables[i]->filename, filename, hash_tables[i]->filenamelen)) {
			if (
				((stfile->st_ctim.tv_sec) > (hash_tables[i]->ctim)) ||
				((stfile->st_mtim.tv_sec) > (hash_tables[i]->mtim))
			) {
				hash_tables[i]->ctim = stfile->st_ctim.tv_sec;
				hash_tables[i]->mtim = stfile->st_mtim.tv_sec;
				return true;
			}

			return false;
		}
	}

	hash_tables[hash_table_count] = (dirbit_tea_hash_table *)malloc(sizeof(dirbit_tea_hash_table) + filenamelen + 2);
	hash_tables[hash_table_count]->ctim = stfile->st_ctim.tv_sec;
	hash_tables[hash_table_count]->mtim = stfile->st_mtim.tv_sec;
	hash_tables[hash_table_count]->filenamelen = filenamelen;
	memcpy(hash_tables[hash_table_count]->filename, filename, filenamelen);
	hash_table_count++;

	if (
		(stfile->st_ctim.tv_sec > last_scan) ||
		(stfile->st_mtim.tv_sec > last_scan)
	) {
		return true;
	}

	return false;
}

void recursive_dirbit_tea_scan(char *dir)
{
	int n;
	struct dirent **namelist;
	struct stat stfile;

	n = scandir(dir, &namelist, NULL, alphasort);

	if (n == -1) {
		printf("Cannot scan directory: %s\n", dir);
		return;
	}

	while (n--) {
		if ((!strcmp(namelist[n]->d_name, ".")) || (!strcmp(namelist[n]->d_name, ".."))) {
			continue;
		}
		size_t dirlen = strlen(dir);
    	size_t filenamelen = strlen(namelist[n]->d_name);
    	char tmp_set[dirlen + filenamelen + 3];
    	memcpy(tmp_set, dir, dirlen);
    	tmp_set[dirlen] = '/';
    	memcpy(tmp_set+dirlen+1, namelist[n]->d_name, filenamelen);
    	tmp_set[dirlen + filenamelen + 1] = '\0';

    	free(namelist[n]);
	    namelist[n] = NULL;

	    if (lstat(tmp_set, &stfile) == -1) {
	    	printf("Cannot stat file: %s\n", tmp_set);
	    	continue;
	    }

	    if ((stfile.st_mode & S_IFMT) == S_IFDIR) {
	    	recursive_dirbit_tea_scan(tmp_set);
	    } else {

	    	if ((hash_table_count + 5) >= hash_table_allocated) {
	    		hash_table_allocated += HASH_TABLE_GROWTH_SIZE;
	    		hash_tables = (dirbit_tea_hash_table **)realloc(hash_tables, sizeof(dirbit_tea_hash_table *) * hash_table_allocated);
	    	}

	    	if (findOrFail(tmp_set, strlen(tmp_set), &stfile)) {
	    		printf("Found changed/new file: %s\n", tmp_set);
	    	}
	    }
	}
	free(namelist);
	namelist = NULL;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Invalid parameter!\n");
		printf("Usage: %s [directory]\n", argv[0]);
		exit(1);
	}

	hash_tables = (dirbit_tea_hash_table **)malloc(sizeof(dirbit_tea_hash_table *) * hash_table_allocated);

	printf("Scanning files state...\n");

	last_scan = time(NULL);
	while (1) {
		recursive_dirbit_tea_scan(argv[1]);
		usleep(SCAN_DELAY);

		if (!has_been_scanned) {
			has_been_scanned = true;
			printf("Scanning finished!\n");
			printf("Watching files...\n");
		}
	}

	return 0;
}
