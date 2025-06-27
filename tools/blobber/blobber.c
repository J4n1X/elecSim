#include <errno.h>
#include <stdio.h>
#include <string.h>
#define HOPE_IMPLEMENTATION
#include "hope.h"

hope_t create_hope(const char *prog_name) {
  hope_t hope = hope_init(prog_name, "Turns binary files into a C array.");
  hope_set_t main_set = hope_init_set("Main");
  hope_add_param(&main_set, hope_init_param("-i", "Input file to read",
                                            HOPE_TYPE_STRING, 1));
  hope_add_param(&main_set, hope_init_param("-o", "Output file to write",
                                            HOPE_TYPE_STRING, 1));

  hope_set_t help_set = hope_init_set("Help");
  // If this is optional, then this set is chosen if no arguments are passed
  hope_add_param(&help_set, hope_init_param("-h", "Show this help message",
                                            HOPE_TYPE_SWITCH, HOPE_ARGC_OPT));

  hope_add_set(&hope, main_set);
  hope_add_set(&hope, help_set);
  return hope;
}

int main(int argc, char *argv[]) {
  hope_t hope = create_hope(argv[0]);

  int parse_code = hope_parse_argv(&hope, argv);
  if (parse_code != HOPE_SUCCESS_CODE) {
    hope_err_any(parse_code, "Invalid command line arguments.");
    return 1;
  }
  if (strcmp(hope.used_set_name, "Help") == 0) {
    hope_print_help(&hope, stdout);
    return 0;
  }

  const char *input_file_path = hope_get_single_string(&hope, "-i");
  const char *output_file_path = hope_get_single_string(&hope, "-o");

  // Open the files before we manipulate the file name
  FILE *input_file = fopen(input_file_path, "rb");
  FILE *output_file = fopen(output_file_path, "wb");
  if (!input_file) {
    fprintf(stderr, "Error: Could not open input file '%s': %s\n",
            input_file_path, strerror(errno));
    return 1;
  }
  if (!output_file) {
    fprintf(stderr, "Error: Could not open output file '%s': %s\n",
            output_file_path, strerror(errno));
    fclose(input_file);
    return 1;
  }

  // Get the file name of the input file, removing the path
  const char *file_name = strrchr(input_file_path, '/');
  if (file_name) {
    file_name++;
  } else {
    // No path separator found, use whole string
    file_name = input_file_path;
  }

  // Furthermore, also replace all periods with underscores
  for (char *p = strrchr(file_name, '.'); p != NULL;
       p = strrchr(file_name, '.')) {
    *p = '_';
  }

  fprintf(output_file, "#ifndef %s_BLOB_\n#define %s_BLOB_\n", file_name, file_name);
  fprintf(output_file, "const static unsigned char %s_data[] = {\n  ", file_name);
  
  char buffer[1024];
  size_t bytes_read = 0, bytes_processed = 0;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_file)) > 0) {
    for (size_t i = 0; i < bytes_read; i++) {
      if (bytes_processed > 0 && bytes_processed % 12 == 0) {
        fprintf(output_file, "\n  ");
      }
      fprintf(output_file, "0x%02x", (unsigned char)buffer[i]);
      if (i < bytes_read - 1 || !feof(input_file)) {
        fprintf(output_file, ", ");
      }
      bytes_processed++;
    }
  }
  fprintf(output_file, "\n};\nconst static unsigned long %s_len = %lu;\n#endif", file_name,
          bytes_processed);
  fclose(input_file);
  fclose(output_file);

  hope_free(&hope);
  return 0;
}