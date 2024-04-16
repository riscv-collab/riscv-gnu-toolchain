struct bfin_dmasg {
  bu32 next_desc_addr;
  bu32 start_addr;
  bu16 cfg;
  bu16 x_count;
  bs16 x_modify;
  bu16 y_count;
  bs16 y_modify;
} __attribute__((packed));

struct bfin_dma {
  bu32 next_desc_ptr;
  bu32 start_addr;

  bu16 BFIN_MMR_16 (config);
  bu32 _pad0;
  bu16 BFIN_MMR_16 (x_count);
  bs16 BFIN_MMR_16 (x_modify);
  bu16 BFIN_MMR_16 (y_count);
  bs16 BFIN_MMR_16 (y_modify);
  bu32 curr_desc_ptr, curr_addr;
  bu16 BFIN_MMR_16 (irq_status);
  bu16 BFIN_MMR_16 (peripheral_map);
  bu16 BFIN_MMR_16 (curr_x_count);
  bu32 _pad1;
  bu16 BFIN_MMR_16 (curr_y_count);
  bu32 _pad2;
};
