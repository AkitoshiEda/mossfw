#include <assert.h>
#include <stdio.h>
#include "mossfw/mossfw_component.h"
#include "mossfw/mossfw_memoryallocator.h"
#include "mossfw/mossfw_data.h"
#include "releasetest_sink.h"
#include "releasetest.h"

struct sink_s sink_create(int type, int blocknum)
{
  struct sink_s ret;
  ret.input[0] = mossfw_input_create(type, 1);
  assert(ret.input[0] != NULL);
  ret.input[1] = mossfw_input_create(type, blocknum * 2);
  assert(ret.input[1] != NULL);
  ret.callback_op      = mossfw_callback_op_create(sink_operator, 0, true);
  assert(ret.callback_op != NULL);
  int esetwait0 = mossfw_set_waitcondition(ret.input[0], sizeof(mossfw_data_v1c_t), ret.callback_op);
  assert(!esetwait0);
  int esetwait1 = mossfw_set_waitcondition(ret.input[1], sizeof(mossfw_data_v1s_t), ret.callback_op);
  assert(!esetwait1);
  return ret;
}

void sink_delete(struct sink_s sink)
{
  mossfw_callback_op_delete(sink.callback_op);
  for (int i = 0; i < sizeof sink.input / sizeof sink.input[0]; i++) {
    mossfw_input_delete(sink.input[i]);
  }
}

int sink_operator(struct mossfw_callback_op_s *in, unsigned long arg)
{
  int used = 0;
  short val = 0;
  static short num = 0;
  mossfw_data_t *sync_data = mossfw_get_delivereddata_array(in->input, sizeof(mossfw_data_v1c_t), &used);
  while (sync_data) {
    mossfw_data_t *data = mossfw_release_delivereddata_array(in->input->cb_next);
    while (data) {
      val = data->data.v1s[0].v[0];
      assert(val == num);
      num = num + 1;
      mossfw_data_free(data);
      data = mossfw_release_delivereddata_array(in->input->cb_next);
    }
    mossfw_data_free(sync_data);
    sync_data = mossfw_get_delivereddata_array(in->input, sizeof(mossfw_data_v1c_t), &used);
  }
  releasetest_setval(val);
  return 0;
}
