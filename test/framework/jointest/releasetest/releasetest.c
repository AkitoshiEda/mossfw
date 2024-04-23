#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "component.h"
#include "mossfw/mossfw_component.h"
#include "mossfw/mossfw_data.h"
#include "mossfw/mossfw_memoryallocator.h"
#include "releasetest.h"
#include "releasetest_sink.h"

struct pipeline_s {
  mossfw_output_t *src[2];
  struct sink_s sink;
};

static struct pipeline_s pipeline;
static void pipeline_create(struct pipeline_s *pipeline, int type, int input_blocknum);
static void pipeline_delete(struct pipeline_s *pipeline);
static void src_create(mossfw_output_t *src[], int size, int type);
static void src_delete(mossfw_output_t *src[], int size);
static void src_input_data(struct pipeline_s *pipeline, mossfw_allocator_t *alloc, int blocksize,
                           mossfw_allocator_t *sync_alloc, int sync_blocksize);
static const int num_inblock = 256;
static const int blocknum = 10;
static volatile int passed = 0;

void releasetest(void)
{
  const uint32_t type = MOSSFW_DATA_TYPE_SHORT +
    MOSSFW_DATA_TYPEGRP_V1 +
    MOSSFW_DATA_TYPENAME_NONE +
    MOSSFW_DATA_TYPEARRAY_ARRAY;
  int blocksize = sizeof(short);
  int sync_blocksize = sizeof(char);
  int blocknum = 256;
  int sync_blocknum = 1;
  pipeline_create(&pipeline, type, blocknum);
  mossfw_allocator_t *alloc = mossfw_fixedmem_create(blocksize, blocknum * 2);
  assert(alloc);
  mossfw_allocator_t *sync_alloc = mossfw_fixedmem_create(sync_blocksize, sync_blocknum);
  assert(sync_alloc);
  src_input_data(&pipeline, alloc, blocksize, sync_alloc, sync_blocksize);
  int edel = mossfw_fixedmem_delete(alloc);
  assert(!edel);
  int esyncdel = mossfw_fixedmem_delete(sync_alloc);
  assert(!esyncdel);
  pipeline_delete(&pipeline);
  printf("framework jointest releasetest passed\n");
}

void pipeline_create(struct pipeline_s *pipeline, int type, int input_blocknum)
{
  src_create(pipeline->src, sizeof pipeline->src / sizeof pipeline->src[0], type);
  pipeline->sink = sink_create(type, input_blocknum);
  uint32_t mask = 0u;
  for (int i = 0; i < sizeof pipeline->src / sizeof pipeline->src[0]; i++) {
    int ebind = mossfw_bind_inout(pipeline->src[i], pipeline->sink.input[i], ~mask);
    assert(!ebind);
  }
}

void pipeline_delete(struct pipeline_s *pipeline)
{
  for (int i = 0; i < sizeof pipeline->src / sizeof pipeline->src[0]; i++) {
    int eunbind = mossfw_unbind_inout(pipeline->src[i], pipeline->sink.input[i]);
    assert(!eunbind);
  }
  src_delete(pipeline->src, sizeof pipeline->src / sizeof pipeline->src[0]);
  pipeline->src[0] = NULL;
  pipeline->src[1] = NULL;
  sink_delete(pipeline->sink);
  pipeline->sink.input[0] = NULL;
  pipeline->sink.input[1] = NULL;
  pipeline->sink.callback_op = NULL;
}

void src_create(mossfw_output_t *src[], int size, int type)
{
  for (int i = 0; i < size; i++) {
    src[i] = mossfw_output_create(type);
    assert(src[i] != NULL);
  }
}

void src_delete(mossfw_output_t *src[], int size)
{
  for (int i = 0; i < size; i++) {
    mossfw_output_delete(src[i]);
  }
}

void src_input_data(struct pipeline_s *pipeline, mossfw_allocator_t *alloc, int blocksize,
                    mossfw_allocator_t *sync_alloc, int sync_blocksize)
{
  short num = 0;
  for (int j = 0; j < blocknum; j++) {
    for (int i = 0; i < num_inblock; i++) {
      mossfw_data_t *data = mossfw_data_alloc(alloc, blocksize, false);
      assert(data);
      data->data.v1s[0].v[0] = num;
      num = num + 1;
      data->data_bytes = blocksize;
      int edeliver = mossfw_deliver_dataarray(pipeline->src[1], data);
      assert(!edeliver);
      mossfw_data_free(data);
    }
    mossfw_data_t *sync_data = mossfw_data_alloc(sync_alloc, sync_blocksize, false);
    assert(sync_data);
    sync_data->data.v1c[0].v[0] = 0;
    sync_data->data_bytes = blocksize * sizeof(mossfw_data_v1c_t);
    int esyncdeliver = mossfw_deliver_dataarray(pipeline->src[0], sync_data);
    assert(!esyncdeliver);
    mossfw_data_free(sync_data);
  }

  // sync end of data
  mossfw_data_t *sync_data_last = mossfw_data_alloc(sync_alloc, sync_blocksize, false);
  assert(sync_data_last != NULL);
  mossfw_data_free(sync_data_last);
  assert(passed);
}

void releasetest_setval(int val)
{
  if (passed == 0) {
    if (val == num_inblock * blocknum - 1) {
      passed = 1;
    }
  } else {
    if (val != num_inblock * blocknum - 1) {
      passed = 0;
    }
  }
}
