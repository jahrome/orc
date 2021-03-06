
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <orc/orcprogram.h>
#include <orc/orcdebug.h>


OrcCode *
orc_code_new (void)
{
  OrcCode *code;
  code = malloc(sizeof(OrcCode));
  memset (code, 0, sizeof(OrcCode));
  return code;
}

void
orc_code_free (OrcCode *code)
{
  if (code->insns) {
    free (code->insns);
  }

  free (code);
}



