
#include "config.h"

#include <orc/orc.h>
#include <orc-test/orctest.h>
#include <orc/orcparse.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char * read_file (const char *filename);
void output_code (OrcProgram *p, FILE *output);
void output_code_header (OrcProgram *p, FILE *output);
void output_code_test (OrcProgram *p, FILE *output);
void output_code_backup (OrcProgram *p, FILE *output);
void output_code_no_orc (OrcProgram *p, FILE *output);
void output_code_assembly (OrcProgram *p, FILE *output);
void output_code_execute (OrcProgram *p, FILE *output, int is_inline);
void output_program_generation (OrcProgram *p, FILE *output, int is_inline);
void output_init_function (FILE *output);
static char * get_barrier (const char *s);
static const char * my_basename (const char *s);

int verbose = 0;
int error = 0;
int compat;
int n_programs;
OrcProgram **programs;

int use_inline = FALSE;

const char *init_function = NULL;

char *target = "sse";

#define ORC_VERSION(a,b,c,d) ((a)*1000000 + (b)*10000 + (c)*100 + (d))
#define REQUIRE(a,b,c,d) do { \
  if (ORC_VERSION((a),(b),(c),(d)) > compat) { \
    fprintf(stderr, "Feature used that is incompatible with --compat\n"); \
    exit (1); \
  } \
} while (0)

enum {
  MODE_IMPL,
  MODE_HEADER,
  MODE_TEST,
  MODE_ASSEMBLY
};
int mode = MODE_IMPL;

void help (void)
{
  printf("Usage:\n");
  printf("  orcc [OPTION...] INPUT_FILE\n");
  printf("\n");
  printf("Help Options:\n");
  printf("  -h, --help              Show help options\n");
  printf("\n");
  printf("Application Options:\n");
  printf("  -v, --verbose           Output more information\n");
  printf("  -o, --output FILE       Write output to FILE\n");
  printf("  --implementation        Produce C code implementing functions\n");
  printf("  --header                Produce C header for functions\n");
  printf("  --test                  Produce test code for functions\n");
  printf("  --assembly              Produce assembly code for functions\n");
  printf("  --include FILE          Add #include <FILE> to code\n");
  printf("  --target TARGET         Generate assembly for TARGET\n");
  printf("  --compat VERSION        Generate code compatible with Orc version VERSION\n");
  printf("  --inline                Generate inline functions in header\n");
  printf("  --no-inline             Do not generate inline functions in header\n");
  printf("  --init-function FUNCTION  Generate initialization function\n");
  printf("\n");

  exit (0);
}

int
main (int argc, char *argv[])
{
  char *code;
  int n;
  int i;
  char *output_file = NULL;
  char *input_file = NULL;
  char *include_file = NULL;
  char *compat_version = VERSION;
  FILE *output;
  char *log = NULL;

  orc_init ();
  orc_test_init ();

  for(i=1;i<argc;i++) {
    if (strcmp(argv[i], "--header") == 0) {
      mode = MODE_HEADER;
    } else if (strcmp(argv[i], "--implementation") == 0) {
      mode = MODE_IMPL;
    } else if (strcmp(argv[i], "--test") == 0) {
      mode = MODE_TEST;
    } else if (strcmp(argv[i], "--assembly") == 0) {
      mode = MODE_ASSEMBLY;
    } else if (strcmp(argv[i], "--include") == 0) {
      if (i+1 < argc) {
        include_file = argv[i+1];
        i++;
      } else {
        help();
      }
    } else if (strcmp (argv[i], "--output") == 0 ||
        strcmp(argv[i], "-o") == 0) {
      if (i+1 < argc) {
        output_file = argv[i+1];
        i++;
      } else {
        help();
      }
    } else if (strcmp(argv[i], "--target") == 0 ||
        strcmp(argv[i], "-t") == 0) {
      if (i+1 < argc) {
        target = argv[i+1];
        i++;
      } else {
      }
    } else if (strcmp(argv[i], "--inline") == 0) {
      use_inline = TRUE;
    } else if (strcmp(argv[i], "--no-inline") == 0) {
      use_inline = FALSE;
    } else if (strcmp(argv[i], "--init-function") == 0) {
      if (i+1 < argc) {
        init_function = argv[i+1];
        i++;
      } else {
        help();
      }
    } else if (strcmp(argv[i], "--help") == 0 ||
        strcmp(argv[i], "-h") == 0) {
      help ();
    } else if (strcmp(argv[i], "--verbose") == 0 ||
        strcmp(argv[i], "-v") == 0) {
      verbose = 1;
    } else if (strcmp(argv[i], "--version") == 0) {
      printf("Orc Compiler " PACKAGE_VERSION "\n");
      exit (0);
    } else if (strcmp(argv[i], "--compat") == 0) {
      if (i+1 < argc) {
        compat_version = argv[i+1];
        i++;
      } else {
        help();
      }
    } else if (strncmp(argv[i], "-", 1) == 0) {
      printf("Unknown option: %s\n", argv[i]);
      exit (1);
    } else {
      if (input_file == NULL) {
        input_file = argv[i];
      } else {
        printf("More than one input file specified: %s\n", argv[i]);
        exit (1);
      }
    }
  }

  if (input_file == NULL) {
    printf("No input file specified\n");
    exit (1);
  }

  if (orc_target_get_by_name (target) == NULL) {
    printf("Unknown target \"%s\"\n", target);
    exit (1);
  }

  if (compat_version) {
    int major, minor, micro, nano = 0;
    int n;

    n = sscanf (compat_version, "%d.%d.%d.%d", &major, &minor, &micro, &nano);

    if (n < 3) {
      printf("Unknown version \"%s\"\n", compat_version);
      exit (1);
    }

    compat = ORC_VERSION(major,minor,micro,nano);
    if (compat < ORC_VERSION(0,4,5,0)) {
      printf("Compatibility version \"%s\" not supported.  Minimum 0.4.5\n",
          compat_version);
      exit (1);
    }
  }

  if (output_file == NULL) {
    switch (mode) {
      case MODE_IMPL:
        output_file = "out.c";
        break;
      case MODE_HEADER:
        output_file = "out.h";
        break;
      case MODE_TEST:
        output_file = "out_test.c";
        break;
      case MODE_ASSEMBLY:
        output_file = "out.s";
        break;
    }
  }

  code = read_file (input_file);
  if (!code) {
    printf("Could not read input file: %s\n", input_file);
    exit(1);
  }

  n = orc_parse_full (code, &programs, &log);
  n_programs = n;
  printf("%s", log);

  if (init_function == NULL) {
    init_function = orc_parse_get_init_function (programs[0]);
  }

  output = fopen (output_file, "w");
  if (!output) {
    printf("Could not write output file: %s\n", output_file);
    exit(1);
  }

  fprintf(output, "\n");
  fprintf(output, "/* autogenerated from %s */\n", my_basename(input_file));
  fprintf(output, "\n");

  if (mode == MODE_IMPL) {
    fprintf(output, "#ifdef HAVE_CONFIG_H\n");
    fprintf(output, "#include \"config.h\"\n");
    fprintf(output, "#endif\n");
    fprintf(output, "#ifndef DISABLE_ORC\n");
    fprintf(output, "#include <orc/orc.h>\n");
    fprintf(output, "#endif\n");
    if (include_file) {
      fprintf(output, "#include <%s>\n", include_file);
    }
    fprintf(output, "\n");
    fprintf(output, "%s", orc_target_c_get_typedefs ());
    fprintf(output, "\n");
    for(i=0;i<n;i++){
      output_code_header (programs[i], output);
    }
    if (init_function) {
      fprintf(output, "\n");
      fprintf(output, "void %s (void);\n", init_function);
    }
    fprintf(output, "\n");
    fprintf(output, "%s", orc_target_get_asm_preamble ("c"));
    fprintf(output, "\n");
    for(i=0;i<n;i++){
      output_code (programs[i], output);
    }
    fprintf(output, "\n");
    if (init_function) {
      output_init_function (output);
      fprintf(output, "\n");
    }
  } else if (mode == MODE_HEADER) {
    char *barrier = get_barrier (output_file);

    fprintf(output, "#ifndef _%s_\n", barrier);
    fprintf(output, "#define _%s_\n", barrier);
    free (barrier);
    fprintf(output, "\n");
    if (include_file) {
      fprintf(output, "#include <%s>\n", include_file);
    }
    fprintf(output, "\n");
    fprintf(output, "#ifdef __cplusplus\n");
    fprintf(output, "extern \"C\" {\n");
    fprintf(output, "#endif\n");
    fprintf(output, "\n");
    if (init_function) {
      fprintf(output, "void %s (void);\n", init_function);
      fprintf(output, "\n");
    }
    fprintf(output, "\n");
    if (!use_inline) {
      fprintf(output, "\n");
      fprintf(output, "%s", orc_target_c_get_typedefs ());
      for(i=0;i<n;i++){
        output_code_header (programs[i], output);
      }
    } else {
      fprintf(output, "\n");
      fprintf(output, "#include <orc/orc.h>\n");
      fprintf(output, "\n");
      for(i=0;i<n;i++){
        output_code_execute (programs[i], output, TRUE);
      }
    }
    fprintf(output, "\n");
    fprintf(output, "#ifdef __cplusplus\n");
    fprintf(output, "}\n");
    fprintf(output, "#endif\n");
    fprintf(output, "\n");
    fprintf(output, "#endif\n");
    fprintf(output, "\n");
  } else if (mode == MODE_TEST) {
    fprintf(output, "#include <orc/orc.h>\n");
    fprintf(output, "#include <orc-test/orctest.h>\n");
    fprintf(output, "#include <stdio.h>\n");
    fprintf(output, "#include <string.h>\n");
    fprintf(output, "#include <stdlib.h>\n");
    fprintf(output, "#include <math.h>\n");
    if (include_file) {
      fprintf(output, "#include <%s>\n", include_file);
    }
    fprintf(output, "\n");
    fprintf(output, "%s", orc_target_c_get_typedefs ());
    fprintf(output, "%s", orc_target_get_asm_preamble ("c"));
    fprintf(output, "\n");
    for(i=0;i<n;i++){
      fprintf(output, "/* %s */\n", programs[i]->name);
      output_code_backup (programs[i], output);
    }
    fprintf(output, "\n");
    fprintf(output, "static int quiet = 0;\n");
    fprintf(output, "static int benchmark = 0;\n");
    fprintf(output, "\n");
    fprintf(output, "static void help (const char *argv0)\n");
    fprintf(output, "{\n");
    fprintf(output, "  printf(\"Usage:\\n\");\n");
    fprintf(output, "  printf(\"  %%s [OPTION]\\n\", argv0);\n");
    fprintf(output, "  printf(\"Help Options:\\n\");\n");
    fprintf(output, "  printf(\"  -h, --help          Show help options\\n\");\n");
    fprintf(output, "  printf(\"Application Options:\\n\");\n");
    fprintf(output, "  printf(\"  -b, --benchmark     Run benchmark and show results\\n\");\n");
    fprintf(output, "  printf(\"  -q, --quiet         Don't output anything except on failures\\n\");\n");
    fprintf(output, "\n");
    fprintf(output, "  exit(0);\n");
    fprintf(output, "}\n");
    fprintf(output, "\n");
    fprintf(output, "int\n");
    fprintf(output, "main (int argc, char *argv[])\n");
    fprintf(output, "{\n");
    fprintf(output, "  int error = FALSE;\n");
    fprintf(output, "  int i;\n");
    fprintf(output, "\n");
    fprintf(output, "  orc_test_init ();\n");
    fprintf(output, "\n");
    fprintf(output, "  for(i=1;i<argc;i++) {\n");
    fprintf(output, "    if (strcmp(argv[i], \"--help\") == 0 ||\n");
    fprintf(output, "      strcmp(argv[i], \"-h\") == 0) {\n");
    fprintf(output, "      help(argv[0]);\n");
    fprintf(output, "    } else if (strcmp(argv[i], \"--quiet\") == 0 ||\n");
    fprintf(output, "      strcmp(argv[i], \"-q\") == 0) {\n");
    fprintf(output, "      quiet = 1;\n");
    fprintf(output, "      benchmark = 0;\n");
    fprintf(output, "    } else if (strcmp(argv[i], \"--benchmark\") == 0 ||\n");
    fprintf(output, "      strcmp(argv[i], \"-b\") == 0) {\n");
    fprintf(output, "      benchmark = 1;\n");
    fprintf(output, "      quiet = 0;\n");
    fprintf(output, "    }\n");
    fprintf(output, "  }\n");
    fprintf(output, "\n");
    for(i=0;i<n;i++){
      output_code_test (programs[i], output);
    }
    fprintf(output, "\n");
    fprintf(output, "  if (error) {\n");
    fprintf(output, "    return 1;\n");
    fprintf(output, "  };\n");
    fprintf(output, "  return 0;\n");
    fprintf(output, "}\n");
  } else if (mode == MODE_ASSEMBLY) {
    fprintf(output, "%s", orc_target_get_asm_preamble (target));
    for(i=0;i<n;i++){
      output_code_assembly (programs[i], output);
    }
  }

  fclose (output);

  if (error) exit(1);

  return 0;
}


static char *
get_barrier (const char *s)
{
  char *barrier;
  int n;
  int i;

  n = strlen(s);
  barrier = malloc (n + 1);
  for(i=0;i<n;i++) {
    if (isalnum (s[i])) {
      barrier[i] = toupper(s[i]);
    } else {
      barrier[i] = '_';
    }
  }
  barrier[n] = 0;

  return barrier;
}

static char *
read_file (const char *filename)
{
  FILE *file = NULL;
  char *contents = NULL;
  long size;
  int ret;

  file = fopen (filename, "r");
  if (file == NULL) return NULL;

  ret = fseek (file, 0, SEEK_END);
  if (ret < 0) goto bail;

  size = ftell (file);
  if (size < 0) goto bail;

  ret = fseek (file, 0, SEEK_SET);
  if (ret < 0) goto bail;

  contents = malloc (size + 1);
  if (contents == NULL) goto bail;

  ret = fread (contents, size, 1, file);
  if (ret < 0) goto bail;

  contents[size] = 0;

  return contents;
bail:
  /* something failed */
  if (file) fclose (file);
  if (contents) free (contents);

  return NULL;
}

const char *varnames[] = {
  "d1", "d2", "d3", "d4",
  "s1", "s2", "s3", "s4",
  "s5", "s6", "s7", "s8",
  "a1", "a2", "a3", "d4",
  "c1", "c2", "c3", "c4",
  "c5", "c6", "c7", "c8",
  "p1", "p2", "p3", "p4",
  "p5", "p6", "p7", "p8",
  "t1", "t2", "t3", "t4",
  "t5", "t6", "t7", "t8",
  "t9", "t10", "t11", "t12",
  "t13", "t14", "t15", "t16"
};

const char *enumnames[] = {
  "ORC_VAR_D1", "ORC_VAR_D2", "ORC_VAR_D3", "ORC_VAR_D4",
  "ORC_VAR_S1", "ORC_VAR_S2", "ORC_VAR_S3", "ORC_VAR_S4",
  "ORC_VAR_S5", "ORC_VAR_S6", "ORC_VAR_S7", "ORC_VAR_S8",
  "ORC_VAR_A1", "ORC_VAR_A2", "ORC_VAR_A3", "ORC_VAR_A4",
  "ORC_VAR_C1", "ORC_VAR_C2", "ORC_VAR_C3", "ORC_VAR_C4",
  "ORC_VAR_C5", "ORC_VAR_C6", "ORC_VAR_C7", "ORC_VAR_C8",
  "ORC_VAR_P1", "ORC_VAR_P2", "ORC_VAR_P3", "ORC_VAR_P4",
  "ORC_VAR_P5", "ORC_VAR_P6", "ORC_VAR_P7", "ORC_VAR_P8",
  "ORC_VAR_T1", "ORC_VAR_T2", "ORC_VAR_T3", "ORC_VAR_T4",
  "ORC_VAR_T5", "ORC_VAR_T6", "ORC_VAR_T7", "ORC_VAR_T8",
  "ORC_VAR_T9", "ORC_VAR_T10", "ORC_VAR_T11", "ORC_VAR_T12",
  "ORC_VAR_T13", "ORC_VAR_T14", "ORC_VAR_T15", "ORC_VAR_T16"
};

void
output_prototype (OrcProgram *p, FILE *output)
{
  OrcVariable *var;
  int i;
  int need_comma;

  fprintf(output, "%s (", p->name);
  need_comma = FALSE;
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_D1 + i];
    if (var->size) {
      if (need_comma) fprintf(output, ", ");
      if (var->type_name) {
        fprintf(output, "%s * %s", var->type_name,
            varnames[ORC_VAR_D1 + i]);
      } else {
        fprintf(output, "orc_uint%d * %s", var->size*8,
            varnames[ORC_VAR_D1 + i]);
      }
      if (p->is_2d) {
        fprintf(output, ", int %s_stride", varnames[ORC_VAR_D1 + i]);
      }
      need_comma = TRUE;
    }
  }
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_A1 + i];
    if (var->size) {
      if (need_comma) fprintf(output, ", ");
      if (var->type_name) {
        fprintf(output, "%s * %s", var->type_name,
            varnames[ORC_VAR_A1 + i]);
      } else {
        fprintf(output, "orc_uint%d * %s", var->size*8,
            varnames[ORC_VAR_A1 + i]);
      }
      need_comma = TRUE;
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_S1 + i];
    if (var->size) {
      if (need_comma) fprintf(output, ", ");
      if (var->type_name) {
        fprintf(output, "const %s * %s", var->type_name,
            varnames[ORC_VAR_S1 + i]);
      } else {
        fprintf(output, "const orc_uint%d * %s", var->size*8,
            varnames[ORC_VAR_S1 + i]);
      }
      if (p->is_2d) {
        fprintf(output, ", int %s_stride", varnames[ORC_VAR_S1 + i]);
      }
      need_comma = TRUE;
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_P1 + i];
    if (var->size) {
      if (need_comma) fprintf(output, ", ");
      if (var->is_float_param) {
        REQUIRE(0,4,5,1);
      }
      fprintf(output, "%s %s",
          var->is_float_param ? "float" : "int",
          varnames[ORC_VAR_P1 + i]);
      need_comma = TRUE;
    }
  }
  if (p->constant_n == 0) {
    if (need_comma) fprintf(output, ", ");
    fprintf(output, "int n");
    need_comma = TRUE;
  }
  if (p->is_2d && p->constant_m == 0) {
    if (need_comma) fprintf(output, ", ");
    fprintf(output, "int m");
  }
  fprintf(output, ")");
}

void
output_code_header (OrcProgram *p, FILE *output)
{
  fprintf(output, "void ");
  output_prototype (p, output);
  fprintf(output, ";\n");
}

void
output_code_backup (OrcProgram *p, FILE *output)
{

  fprintf(output, "static void\n");
  fprintf(output, "_backup_%s (OrcExecutor *ex)\n", p->name);
  fprintf(output, "{\n");
  {
    OrcCompileResult result;

    result = orc_program_compile_full (p, orc_target_get_by_name("c"),
        ORC_TARGET_C_BARE);
    if (ORC_COMPILE_RESULT_IS_SUCCESSFUL(result)) {
      fprintf(output, "%s\n", orc_program_get_asm_code (p));
    } else {
      printf("Failed to compile %s\n", p->name);
      error = TRUE;
    }
  }
  fprintf(output, "}\n");
  fprintf(output, "\n");

}

void
output_code_no_orc (OrcProgram *p, FILE *output)
{

  fprintf(output, "void\n");
  output_prototype (p, output);
  fprintf(output, "{\n");
  {
    OrcCompileResult result;

    result = orc_program_compile_full (p, orc_target_get_by_name("c"),
        ORC_TARGET_C_BARE | ORC_TARGET_C_NOEXEC);
    if (ORC_COMPILE_RESULT_IS_SUCCESSFUL(result)) {
      fprintf(output, "%s\n", orc_program_get_asm_code (p));
    } else {
      printf("Failed to compile %s\n", p->name);
      error = TRUE;
    }
  }
  fprintf(output, "}\n");
  fprintf(output, "\n");

}

void
output_code (OrcProgram *p, FILE *output)
{
  fprintf(output, "\n");
  fprintf(output, "/* %s */\n", p->name);
  fprintf(output, "#ifdef DISABLE_ORC\n");
  output_code_no_orc (p, output);
  fprintf(output, "#else\n");
  output_code_backup (p, output);
  output_code_execute (p, output, FALSE);
  fprintf(output, "#endif\n");
  fprintf(output, "\n");
}

void
output_code_execute (OrcProgram *p, FILE *output, int is_inline)
{
  OrcVariable *var;
  int i;

  if (init_function) {
    if (is_inline) {
      fprintf(output, "extern OrcProgram *_orc_program_%s;\n", p->name);
    } else {
      if (use_inline) {
        fprintf(output, "OrcProgram *_orc_program_%s;\n", p->name);
      } else {
        fprintf(output, "static OrcProgram *_orc_program_%s;\n", p->name);
      }
    }
  }
  if (is_inline) {
    fprintf(output, "static inline void\n");
  } else {
    fprintf(output, "void\n");
  }
  output_prototype (p, output);
  fprintf(output, "\n");
  fprintf(output, "{\n");
  fprintf(output, "  OrcExecutor _ex, *ex = &_ex;\n");
  if (init_function) {
    fprintf(output, "  OrcProgram *p = _orc_program_%s;\n", p->name);
  } else {
    fprintf(output, "  static int p_inited = 0;\n");
    fprintf(output, "  static OrcProgram *p = 0;\n");
  }
  fprintf(output, "  void (*func) (OrcExecutor *);\n");
  fprintf(output, "\n");
  if (init_function == NULL) {
    fprintf(output, "  if (!p_inited) {\n");
    fprintf(output, "    orc_once_mutex_lock ();\n");
    fprintf(output, "    if (!p_inited) {\n");
    fprintf(output, "      OrcCompileResult result;\n");
    fprintf(output, "\n");
    output_program_generation (p, output, is_inline);
    fprintf(output, "\n");
    fprintf(output, "      result = orc_program_compile (p);\n");
    fprintf(output, "    }\n");
    fprintf(output, "    p_inited = TRUE;\n");
    fprintf(output, "    orc_once_mutex_unlock ();\n");
    fprintf(output, "  }\n");
  }
  fprintf(output, "  ex->program = p;\n");
  fprintf(output, "\n");
  if (p->constant_n) {
    fprintf(output, "  ex->n = %d;\n", p->constant_n);
  } else {
    fprintf(output, "  ex->n = n;\n");
  }
  if (p->is_2d) {
    if (p->constant_m) {
      fprintf(output, "  ORC_EXECUTOR_M(ex) = %d;\n", p->constant_m);
    } else {
      fprintf(output, "  ORC_EXECUTOR_M(ex) = m;\n");
    }
  }
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_D1 + i];
    if (var->size) {
      fprintf(output, "  ex->arrays[%s] = %s;\n",
          enumnames[ORC_VAR_D1 + i], varnames[ORC_VAR_D1 + i]);
      if (p->is_2d) {
        fprintf(output, "  ex->params[%s] = %s_stride;\n",
            enumnames[ORC_VAR_D1 + i], varnames[ORC_VAR_D1 + i]);
      }
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_S1 + i];
    if (var->size) {
      fprintf(output, "  ex->arrays[%s] = (void *)%s;\n",
          enumnames[ORC_VAR_S1 + i], varnames[ORC_VAR_S1 + i]);
      if (p->is_2d) {
        fprintf(output, "  ex->params[%s] = %s_stride;\n",
            enumnames[ORC_VAR_S1 + i], varnames[ORC_VAR_S1 + i]);
      }
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_P1 + i];
    if (var->size) {
      if (var->is_float_param) {
        REQUIRE(0,4,5,1);
        fprintf(output, "  {\n");
        fprintf(output, "    orc_union32 tmp;\n");
        fprintf(output, "    tmp.f = %s;\n", varnames[ORC_VAR_P1 + i]);
        fprintf(output, "    ex->params[%s] = tmp.i;\n",
            enumnames[ORC_VAR_P1 + i]);
        fprintf(output, "  }\n");
      } else {
        fprintf(output, "  ex->params[%s] = %s;\n",
            enumnames[ORC_VAR_P1 + i], varnames[ORC_VAR_P1 + i]);
      }
    }
  }
  fprintf(output, "\n");
  fprintf(output, "  func = p->code_exec;\n");
  fprintf(output, "  func (ex);\n");
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_A1 + i];
    if (var->size) {
      fprintf(output, "  *%s = orc_executor_get_accumulator (ex, %s);\n",
          varnames[ORC_VAR_A1 + i], enumnames[ORC_VAR_A1 + i]);
    }
  }
  fprintf(output, "}\n");

}

void
output_program_generation (OrcProgram *p, FILE *output, int is_inline)
{
  OrcVariable *var;
  int i;

  fprintf(output, "      p = orc_program_new ();\n");
  if (p->constant_n != 0) {
    fprintf(output, "      orc_program_set_constant_n (p, %d);\n",
        p->constant_n);
  }
  if (p->is_2d) {
    fprintf(output, "      orc_program_set_2d (p);\n");
    if (p->constant_m != 0) {
      fprintf(output, "      orc_program_set_constant_m (p, %d);\n",
          p->constant_m);
    }
  }
  fprintf(output, "      orc_program_set_name (p, \"%s\");\n", p->name);
  if (!is_inline) {
    fprintf(output, "      orc_program_set_backup_function (p, _backup_%s);\n",
        p->name);
  }
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_D1 + i];
    if (var->size) {
      fprintf(output, "      orc_program_add_destination (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_D1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_S1 + i];
    if (var->size) {
      fprintf(output, "      orc_program_add_source (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_S1 + i]);
    }
  }
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_A1 + i];
    if (var->size) {
      fprintf(output, "      orc_program_add_accumulator (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_A1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_C1 + i];
    if (var->size) {
      fprintf(output, "      orc_program_add_constant (p, %d, 0x%08x, \"%s\");\n",
          var->size, var->value, varnames[ORC_VAR_C1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_P1 + i];
    if (var->size) {
      if (var->is_float_param) {
        REQUIRE(0,4,5,1);
      }
      fprintf(output, "      orc_program_add_parameter%s (p, %d, \"%s\");\n",
          var->is_float_param ? "_float" : "",
          var->size, varnames[ORC_VAR_P1 + i]);
    }
  }
  for(i=0;i<16;i++){
    var = &p->vars[ORC_VAR_T1 + i];
    if (var->size) {
      fprintf(output, "      orc_program_add_temporary (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_T1 + i]);
    }
  }
  fprintf(output, "\n");

  for(i=0;i<p->n_insns;i++){
    OrcInstruction *insn = p->insns + i;

    if (compat < ORC_VERSION(0,4,6,1)) {
      if (insn->flags) {
        REQUIRE(0,4,6,1);
      }

      if (p->vars[insn->src_args[1]].size != 0) {
        fprintf(output, "      orc_program_append (p, \"%s\", %s, %s, %s);\n",
            insn->opcode->name, enumnames[insn->dest_args[0]],
            enumnames[insn->src_args[0]], enumnames[insn->src_args[1]]);
      } else {
        fprintf(output, "      orc_program_append_ds (p, \"%s\", %s, %s);\n",
            insn->opcode->name, enumnames[insn->dest_args[0]],
            enumnames[insn->src_args[0]]);
      }
    } else {
      int args[4] = { 0, 0, 0, 0 };
      int n_args = 0;

      if (insn->opcode->dest_size[0] != 0) {
        args[n_args++] = insn->dest_args[0];
      }
      if (insn->opcode->dest_size[1] != 0) {
        args[n_args++] = insn->dest_args[1];
      }
      if (insn->opcode->src_size[0] != 0) {
        args[n_args++] = insn->src_args[0];
      }
      if (insn->opcode->src_size[1] != 0) {
        args[n_args++] = insn->src_args[1];
      }
      if (insn->opcode->src_size[2] != 0) {
        args[n_args++] = insn->src_args[2];
      }

      fprintf(output, "      orc_program_append_2 (p, \"%s\", %d, %s, %s, %s, %s);\n",
          insn->opcode->name, insn->flags, enumnames[args[0]],
          enumnames[args[1]], enumnames[args[2]],
          enumnames[args[3]]);
    }
  }
}

void
output_init_function (FILE *output)
{
  int i;

  fprintf(output, "void\n");
  fprintf(output, "%s (void)\n", init_function);
  fprintf(output, "{\n");
  fprintf(output, "#ifndef DISABLE_ORC\n");
  for(i=0;i<n_programs;i++){
    fprintf(output, "  {\n");
    fprintf(output, "    /* %s */\n", programs[i]->name);
    fprintf(output, "    OrcProgram *p;\n");
    fprintf(output, "    OrcCompileResult result;\n");
    fprintf(output, "    \n");
    output_program_generation (programs[i], output, FALSE);
    fprintf(output, "\n");
    fprintf(output, "      result = orc_program_compile (p);\n");
    fprintf(output, "\n");
    fprintf(output, "    _orc_program_%s = p;\n", programs[i]->name);
    fprintf(output, "  }\n");
  }
  fprintf(output, "#endif\n");
  fprintf(output, "}\n");
  fprintf(output, "\n");
}

void
output_code_test (OrcProgram *p, FILE *output)
{
  OrcVariable *var;
  int i;

  fprintf(output, "  /* %s */\n", p->name);
  fprintf(output, "  {\n");
  fprintf(output, "    OrcProgram *p = NULL;\n");
  fprintf(output, "    int ret;\n");
  fprintf(output, "\n");
  fprintf(output, "    if (!quiet)");
  fprintf(output, "      printf (\"%s:\\n\");\n", p->name);
  fprintf(output, "    p = orc_program_new ();\n");
  if (p->constant_n != 0) {
    fprintf(output, "      orc_program_set_constant_n (p, %d);\n",
        p->constant_n);
  }
  if (p->is_2d) {
    fprintf(output, "      orc_program_set_2d (p);\n");
    if (p->constant_m != 0) {
      fprintf(output, "      orc_program_set_constant_m (p, %d);\n",
          p->constant_m);
    }
  }
  fprintf(output, "    orc_program_set_name (p, \"%s\");\n", p->name);
  fprintf(output, "    orc_program_set_backup_function (p, _backup_%s);\n",
      p->name);
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_D1 + i];
    if (var->size) {
      fprintf(output, "    orc_program_add_destination (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_D1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_S1 + i];
    if (var->size) {
      fprintf(output, "    orc_program_add_source (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_S1 + i]);
    }
  }
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_A1 + i];
    if (var->size) {
      fprintf(output, "    orc_program_add_accumulator (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_A1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_C1 + i];
    if (var->size) {
      if (var->value != 0x80000000) {
        fprintf(output, "      orc_program_add_constant (p, %d, %u, \"%s\");\n",
            var->size, var->value, varnames[ORC_VAR_C1 + i]);
      } else {
        fprintf(output, "      orc_program_add_constant (p, %d, 0x%08x, \"%s\");\n",
            var->size, var->value, varnames[ORC_VAR_C1 + i]);
      }
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_P1 + i];
    if (var->size) {
      if (var->is_float_param) {
        REQUIRE(0,4,5,1);
      }
      fprintf(output, "    orc_program_add_parameter%s (p, %d, \"%s\");\n",
          var->is_float_param ? "_float" : "",
          var->size, varnames[ORC_VAR_P1 + i]);
    }
  }
  for(i=0;i<16;i++){
    var = &p->vars[ORC_VAR_T1 + i];
    if (var->size) {
      fprintf(output, "    orc_program_add_temporary (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_T1 + i]);
    }
  }
  fprintf(output, "\n");

  for(i=0;i<p->n_insns;i++){
    OrcInstruction *insn = p->insns + i;
    if (compat < ORC_VERSION(0,4,6,1)) {
      if (insn->flags) {
        REQUIRE(0,4,6,1);
      }

      if (p->vars[insn->src_args[1]].size != 0) {
        fprintf(output, "      orc_program_append (p, \"%s\", %s, %s, %s);\n",
            insn->opcode->name, enumnames[insn->dest_args[0]],
            enumnames[insn->src_args[0]], enumnames[insn->src_args[1]]);
      } else {
        fprintf(output, "      orc_program_append_ds (p, \"%s\", %s, %s);\n",
            insn->opcode->name, enumnames[insn->dest_args[0]],
            enumnames[insn->src_args[0]]);
      }
    } else {
      if (p->vars[insn->src_args[1]].size != 0) {
        fprintf(output, "      orc_program_append_2 (p, \"%s\", %d, %s, %s, %s, -1);\n",
            insn->opcode->name, insn->flags, enumnames[insn->dest_args[0]],
            enumnames[insn->src_args[0]], enumnames[insn->src_args[1]]);
      } else {
        fprintf(output, "      orc_program_append_2 (p, \"%s\", %d, %s, %s, -1, -1);\n",
            insn->opcode->name, insn->flags, enumnames[insn->dest_args[0]],
            enumnames[insn->src_args[0]]);
      }
    }
  }

  fprintf(output, "\n");
  fprintf(output, "    ret = orc_test_compare_output_backup (p);\n");
  fprintf(output, "    if (!ret) {\n");
  fprintf(output, "      error = TRUE;\n");
  fprintf(output, "    } else if (!quiet) {\n");
  fprintf(output, "      printf (\"    backup function  :   PASSED\\n\");\n");
  fprintf(output, "    }\n");
  fprintf(output, "\n");
  fprintf(output, "    if (benchmark) {\n");
  fprintf(output, "      printf (\"    cycles (backup)  :   \");\n");
  fprintf(output, "      orc_test_performance (p, ORC_TEST_FLAGS_BACKUP);\n");
  fprintf(output, "    }\n");
  fprintf(output, "\n");
  fprintf(output, "    ret = orc_test_compare_output (p);\n");
  fprintf(output, "    if (ret == ORC_TEST_INDETERMINATE && !quiet) {\n");
  fprintf(output, "      printf (\"    compiled function:   COMPILE FAILED\\n\");\n");
  fprintf(output, "    } else if (!ret) {\n");
  fprintf(output, "      error = TRUE;\n");
  fprintf(output, "    } else if (!quiet) {\n");
  fprintf(output, "      printf (\"    compiled function:   PASSED\\n\");\n");
  fprintf(output, "    }\n");
  fprintf(output, "\n");
  fprintf(output, "    if (benchmark) {\n");
  fprintf(output, "      printf (\"    cycles (compiled):   \");\n");
  fprintf(output, "      orc_test_performance (p, 0);\n");
  fprintf(output, "    }\n");
  fprintf(output, "\n");
  fprintf(output, "    orc_program_free (p);\n");
  fprintf(output, "  }\n");
  fprintf(output, "\n");

}

void
output_code_assembly (OrcProgram *p, FILE *output)
{

  fprintf(output, "/* %s */\n", p->name);
  //output_prototype (p, output);
  {
    OrcCompileResult result;
    OrcTarget *t = orc_target_get_by_name(target);

    result = orc_program_compile_full (p, orc_target_get_by_name(target),
        orc_target_get_default_flags (t));
    if (ORC_COMPILE_RESULT_IS_SUCCESSFUL(result)) {
      fprintf(output, "%s\n", orc_program_get_asm_code (p));
    } else {
      printf("Failed to compile %s\n", p->name);
      error = TRUE;
    }
  }
  fprintf(output, "\n");

}

static const char *
my_basename (const char *s)
{
  const char *ret = s;
  const char *t;

  t = s;
  while (t[0] != 0) {
    if (t[0] == '/') ret = t+1;
    t++;
  }

  return ret;
}

