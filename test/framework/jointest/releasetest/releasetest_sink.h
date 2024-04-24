#ifndef RELEASETEST_SINK
#define RELEASETEST_SINK

#include "mossfw/mossfw_component.h"

struct sink_s {
  mossfw_input_t *input[2];
  mossfw_callback_op_t *callback_op;
};

struct sink_s sink_create(int type, int blocknum);
void sink_delete(struct sink_s sink);
int sink_operator(struct mossfw_callback_op_s *in, unsigned long arg);

#endif
