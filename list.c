#include "calf.h"

int is_visible(const struct dirent *entry)
{
	return entry->d_name[0] != '.';
}

int list(const char *root, struct tm *date, struct entry ***entries)
{
	char dirname[16];
	char *dirpath;
	strftime(dirname, 16, "%F", date);
	asprintf(&dirpath, "%s/%s", root, dirname);
	struct dirent **items;
	int count = scandir(dirpath, &items, is_visible, alphasort);
	if (count > 0)
		*entries = calloc(count + 1, sizeof(**entries));
	else
		*entries = NULL;
	for (int i = 0; i < count; i++) {
		struct entry *entry;
		entry = malloc(sizeof(*entry));
		entry->ino = items[i]->d_ino;
		entry->date = date;
		asprintf(&entry->path, "%s/%s", dirpath, items[i]->d_name);
		entry->name = entry->path + strlen(dirpath) + 1;
		stat(entry->path, &entry->st);
		(*entries)[i] = entry;
		free(items[i]);
	}
	if (count >= 0)
		free(items);
	free(dirpath);
	return count;
}

void free_entries(struct entry **entries)
{
	if (!entries)
		return;
	for (struct entry **i = entries; *i; i++) {
		free((*i)->path);
		free(*i);
	}
	free(entries);
}
