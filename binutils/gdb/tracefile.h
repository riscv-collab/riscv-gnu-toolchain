#ifndef TRACEFILE_H
#define TRACEFILE_H 1

#include "tracepoint.h"
#include "target.h"
#include "process-stratum-target.h"

struct trace_file_writer;

/* Operations to write trace frames to a specific trace format.  */

struct trace_frame_write_ops
{
  /* Write a new trace frame.  The tracepoint number of this trace
     frame is TPNUM.  */
  void (*start) (struct trace_file_writer *self, uint16_t tpnum);

  /* Write an 'R' block.  Buffer BUF contains its contents and SIZE is
     its size.  */
  void (*write_r_block) (struct trace_file_writer *self,
			 gdb_byte *buf, int32_t size);

  /* Write an 'M' block, the header and memory contents respectively.
     The header of 'M' block is composed of the start address and the
     length of memory collection, and the memory contents contain
     the collected memory contents in tracing.
     For extremely large M block, GDB is unable to get its contents
     and write them into trace file in one go, due to the limitation
     of the remote target or the size of internal buffer, we split
     the operation to 'M' block to two operations.  */
  /* Write the head of 'M' block.  ADDR is the start address of
     collected memory and LENGTH is the length of memory contents.  */
  void (*write_m_block_header) (struct trace_file_writer *self,
				uint64_t addr, uint16_t length);
  /* Write the memory contents of 'M' block.  Buffer BUF contains
     its contents and LENGTH is its length.  This method can be called
     multiple times to write large memory contents of a single 'M'
     block.  */
  void (*write_m_block_memory) (struct trace_file_writer *self,
				gdb_byte *buf, uint16_t length);

  /* Write a 'V' block.  NUM is the trace variable number and VAL is
     the value of the trace variable.  */
  void (*write_v_block) (struct trace_file_writer *self, int32_t num,
			 uint64_t val);

  /* The end of the trace frame.  */
  void (*end) (struct trace_file_writer *self);
};

/* Operations to write trace buffers to a specific trace format.  */

struct trace_file_write_ops
{
  /* Destructor.  Releases everything from SELF (but not SELF
     itself).  */
  void (*dtor) (struct trace_file_writer *self);

  /*  Save the data to file or directory NAME of desired format in
      target side.  Return true for success, otherwise return
      false.  */
  int (*target_save) (struct trace_file_writer *self,
		      const char *name);

  /* Write the trace buffers to file or directory NAME.  */
  void (*start) (struct trace_file_writer *self,
		 const char *name);

  /* Write the trace header.  */
  void (*write_header) (struct trace_file_writer *self);

  /* Write the type of block about registers.  SIZE is the size of
     all registers on the target.  */
  void (*write_regblock_type) (struct trace_file_writer *self,
			       int size);

  /* Write trace status TS.  */
  void (*write_status) (struct trace_file_writer *self,
			struct trace_status *ts);

  /* Write the uploaded TSV.  */
  void (*write_uploaded_tsv) (struct trace_file_writer *self,
			      struct uploaded_tsv *tsv);

  /* Write the uploaded tracepoint TP.  */
  void (*write_uploaded_tp) (struct trace_file_writer *self,
			     struct uploaded_tp *tp);

  /* Write target description.  */
  void (*write_tdesc) (struct trace_file_writer *self);

  /* Write to mark the end of the definition part.  */
  void (*write_definition_end) (struct trace_file_writer *self);

  /* Write the data of trace buffer without parsing.  The content is
     in BUF and length is LEN.  */
  void (*write_trace_buffer) (struct trace_file_writer *self,
			      gdb_byte *buf, LONGEST len);

  /* Operations to write trace frames.  The user of this field is
     responsible to parse the data of trace buffer.  Either field
     'write_trace_buffer' or field ' frame_ops' is NULL.  */
  const struct trace_frame_write_ops *frame_ops;

  /* The end of writing trace buffers.  */
  void (*end) (struct trace_file_writer *self);
};

/* Trace file writer for a given format.  */

struct trace_file_writer
{
  const struct trace_file_write_ops *ops;
};

extern struct trace_file_writer *tfile_trace_file_writer_new (void);

/* Base class for tracefile related targets.  */

class tracefile_target : public process_stratum_target
{
public:
  tracefile_target () = default;

  int get_trace_status (trace_status *ts) override;
  bool has_all_memory () override;
  bool has_memory () override;
  bool has_stack () override;
  bool has_registers () override;
  bool has_execution (inferior *inf) override { return false; }
  bool thread_alive (ptid_t ptid) override;
};

extern void tracefile_fetch_registers (struct regcache *regcache, int regno);

#endif /* TRACEFILE_H */
