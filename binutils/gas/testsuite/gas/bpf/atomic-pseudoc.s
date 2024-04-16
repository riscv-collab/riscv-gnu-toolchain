        # Test for eBPF atomic pseudo-C instructions.
        .text
	lock *(u64 *)(r1 + 0x1eef) += r2
	lock *(u32 *)(r1 + 0x1eef) += w2
	lock *(u64*)(r1+0x1eef)+=r2
	lock *(u32*)(r1+0x1eef)+=w2
	lock *(u64*)(r1+0x1eef)&=r2
	lock *(u32*)(r1+0x1eef)&=w2
	lock *(u64*)(r1+0x1eef)|=r2
	lock *(u32*)(r1+0x1eef)|=w2
	lock *(u64*)(r1+0x1eef)^=r2
	lock *(u32*)(r1+0x1eef)^=w2
	r2 = atomic_fetch_add((u64*)(r1+0x1eef),r2)
	w2 = atomic_fetch_add((u32*)(r1+0x1eef),w2)
	r2 = atomic_fetch_and((u64*)(r1+0x1eef),r2)
	w2 = atomic_fetch_and((u32*)(r1+0x1eef),w2)
	r2 = atomic_fetch_or((u64*)(r1+0x1eef),r2)
	w2 = atomic_fetch_or((u32*)(r1+0x1eef),w2)
	r2 = atomic_fetch_xor((u64*)(r1+0x1eef),r2)
	w2 = atomic_fetch_xor((u32*)(r1+0x1eef),w2)
	r0 = cmpxchg_64(r1+0x4,r0,r2)
	w0 = cmpxchg32_32(r2+0x4,w0,w3)
	r2 = xchg_64(r1+0x8,r2)
	w3 = xchg32_32(r1+0x8,w3)
