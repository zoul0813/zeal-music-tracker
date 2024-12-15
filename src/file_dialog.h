#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

typedef enum {
  FILE_LOAD,
  FILE_SAVE,
} file_dialog_t;

void file_dialog_show(file_dialog_t type);

#endif