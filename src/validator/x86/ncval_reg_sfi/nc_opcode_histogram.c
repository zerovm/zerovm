/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * nc_opcode_histogram.c - Collects histogram information while validating.
 */

#include <stdio.h>

#include "src/validator/x86/ncval_reg_sfi/nc_opcode_histogram.h"

#include "include/portability_io.h"
#include "src/platform/nacl_log.h"
#include "src/validator/x86/decoder/nc_inst_iter.h"
#include "src/validator/x86/decoder/nc_inst_state.h"
#include "src/validator/x86/decoder/nc_inst_state_internal.h"
#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"
#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"

Bool NACL_FLAGS_opcode_histogram = FALSE;

void NaClOpcodeHistogramInitialize(NaClValidatorState* state) {
  int i;
  if (!state->print_opcode_histogram) return;
  for (i = 0; i < 256; ++i) {
    state->opcode_histogram.opcode_histogram[i] = 0;
  }
}

void NaClOpcodeHistogramRecord(NaClValidatorState* state) {
  NaClInstState* inst_state = state->cur_inst_state;
  const NaClInst* inst = state->cur_inst;
  if ((inst->name != InstInvalid) &&
      (inst_state->num_opcode_bytes > 0)) {
    state->opcode_histogram.opcode_histogram[
        inst_state->bytes.byte[inst_state->num_prefix_bytes]]++;
  }
}

#define LINE_SIZE 1024

void NaClOpcodeHistogramPrintStats(NaClValidatorState* state) {
  int i;
  char line[LINE_SIZE];
  int line_size = LINE_SIZE;
  int printed_in_this_row = 0;
  NaClValidatorMessage(LOG_INFO, state, "Opcode Histogram:\n");
  for (i = 0; i < 256; ++i) {
    if (0 != state->opcode_histogram.opcode_histogram[i]) {
      if (line_size < LINE_SIZE) {
        line_size -= SNPRINTF(line, line_size, "%"NACL_PRId32"\t0x%02x\t",
                              state->opcode_histogram.opcode_histogram[i], i);
      }
      ++printed_in_this_row;
      if (printed_in_this_row > 3) {
        printed_in_this_row = 0;
        NaClValidatorMessage(LOG_INFO, state, "%s\n", line);
        line_size = LINE_SIZE;
      }
    }
  }
  if (0 != printed_in_this_row) {
    NaClValidatorMessage(LOG_INFO, state, "%s\n", line);
  }
}
