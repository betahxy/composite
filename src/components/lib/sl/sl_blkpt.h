#ifndef SL_BLKPT_H
#define SL_BLKPT_H

#include <sl.h>

typedef u32_t sched_blkpt_id_t;
#define SCHED_BLKPT_NULL 0
typedef word_t sched_blkpt_epoch_t;

sched_blkpt_id_t sl_blkpt_alloc(void);
int sl_blkpt_free(sched_blkpt_id_t id);
int sl_blkpt_trigger(sched_blkpt_id_t blkpt, sched_blkpt_epoch_t epoch, int single);
int sl_blkpt_block(sched_blkpt_id_t blkpt, sched_blkpt_epoch_t epoch, thdid_t dependency);

#endif	/* SL_BLKPT_H */
