	th.vsetvl a0, a1, a2
	th.vsetvli a0, a1, 0
	th.vsetvli a0, a1, 0x7ff
	th.vsetvli a0, a1, e16,m2,d4

	th.vlb.v v4, (a0)
	th.vlb.v v4, 0(a0)
	th.vlb.v v4, (a0), v0.t
	th.vlh.v v4, (a0)
	th.vlh.v v4, 0(a0)
	th.vlh.v v4, (a0), v0.t
	th.vlw.v v4, (a0)
	th.vlw.v v4, 0(a0)
	th.vlw.v v4, (a0), v0.t
	th.vlbu.v v4, (a0)
	th.vlbu.v v4, 0(a0)
	th.vlbu.v v4, (a0), v0.t
	th.vlhu.v v4, (a0)
	th.vlhu.v v4, 0(a0)
	th.vlhu.v v4, (a0), v0.t
	th.vlwu.v v4, (a0)
	th.vlwu.v v4, 0(a0)
	th.vlwu.v v4, (a0), v0.t
	th.vle.v v4, (a0)
	th.vle.v v4, 0(a0)
	th.vle.v v4, (a0), v0.t
	th.vsb.v v4, (a0)
	th.vsb.v v4, 0(a0)
	th.vsb.v v4, (a0), v0.t
	th.vsh.v v4, (a0)
	th.vsh.v v4, 0(a0)
	th.vsh.v v4, (a0), v0.t
	th.vsw.v v4, (a0)
	th.vsw.v v4, 0(a0)
	th.vsw.v v4, (a0), v0.t
	th.vse.v v4, (a0)
	th.vse.v v4, 0(a0)
	th.vse.v v4, (a0), v0.t

	th.vlsb.v v4, (a0), a1
	th.vlsb.v v4, 0(a0), a1
	th.vlsb.v v4, (a0), a1, v0.t
	th.vlsh.v v4, (a0), a1
	th.vlsh.v v4, 0(a0), a1
	th.vlsh.v v4, (a0), a1, v0.t
	th.vlsw.v v4, (a0), a1
	th.vlsw.v v4, 0(a0), a1
	th.vlsw.v v4, (a0), a1, v0.t
	th.vlsbu.v v4, (a0), a1
	th.vlsbu.v v4, 0(a0), a1
	th.vlsbu.v v4, (a0), a1, v0.t
	th.vlshu.v v4, (a0), a1
	th.vlshu.v v4, 0(a0), a1
	th.vlshu.v v4, (a0), a1, v0.t
	th.vlswu.v v4, (a0), a1
	th.vlswu.v v4, 0(a0), a1
	th.vlswu.v v4, (a0), a1, v0.t
	th.vlse.v v4, (a0), a1
	th.vlse.v v4, 0(a0), a1
	th.vlse.v v4, (a0), a1, v0.t
	th.vssb.v v4, (a0), a1
	th.vssb.v v4, 0(a0), a1
	th.vssb.v v4, (a0), a1, v0.t
	th.vssh.v v4, (a0), a1
	th.vssh.v v4, 0(a0), a1
	th.vssh.v v4, (a0), a1, v0.t
	th.vssw.v v4, (a0), a1
	th.vssw.v v4, 0(a0), a1
	th.vssw.v v4, (a0), a1, v0.t
	th.vsse.v v4, (a0), a1
	th.vsse.v v4, 0(a0), a1
	th.vsse.v v4, (a0), a1, v0.t

	th.vlxb.v v4, (a0), v12
	th.vlxb.v v4, 0(a0), v12
	th.vlxb.v v4, (a0), v12, v0.t
	th.vlxh.v v4, (a0), v12
	th.vlxh.v v4, 0(a0), v12
	th.vlxh.v v4, (a0), v12, v0.t
	th.vlxw.v v4, (a0), v12
	th.vlxw.v v4, 0(a0), v12
	th.vlxw.v v4, (a0), v12, v0.t
	th.vlxbu.v v4, (a0), v12
	th.vlxbu.v v4, 0(a0), v12
	th.vlxbu.v v4, (a0), v12, v0.t
	th.vlxhu.v v4, (a0), v12
	th.vlxhu.v v4, 0(a0), v12
	th.vlxhu.v v4, (a0), v12, v0.t
	th.vlxwu.v v4, (a0), v12
	th.vlxwu.v v4, 0(a0), v12
	th.vlxwu.v v4, (a0), v12, v0.t
	th.vlxe.v v4, (a0), v12
	th.vlxe.v v4, 0(a0), v12
	th.vlxe.v v4, (a0), v12, v0.t
	th.vsxb.v v4, (a0), v12
	th.vsxb.v v4, 0(a0), v12
	th.vsxb.v v4, (a0), v12, v0.t
	th.vsxh.v v4, (a0), v12
	th.vsxh.v v4, 0(a0), v12
	th.vsxh.v v4, (a0), v12, v0.t
	th.vsxw.v v4, (a0), v12
	th.vsxw.v v4, 0(a0), v12
	th.vsxw.v v4, (a0), v12, v0.t
	th.vsxe.v v4, (a0), v12
	th.vsxe.v v4, 0(a0), v12
	th.vsxe.v v4, (a0), v12, v0.t
	th.vsuxb.v v4, (a0), v12
	th.vsuxb.v v4, 0(a0), v12
	th.vsuxb.v v4, (a0), v12, v0.t
	th.vsuxh.v v4, (a0), v12
	th.vsuxh.v v4, 0(a0), v12
	th.vsuxh.v v4, (a0), v12, v0.t
	th.vsuxw.v v4, (a0), v12
	th.vsuxw.v v4, 0(a0), v12
	th.vsuxw.v v4, (a0), v12, v0.t
	th.vsuxe.v v4, (a0), v12
	th.vsuxe.v v4, 0(a0), v12
	th.vsuxe.v v4, (a0), v12, v0.t

	th.vlbff.v v4, (a0)
	th.vlbff.v v4, 0(a0)
	th.vlbff.v v4, (a0), v0.t
	th.vlhff.v v4, (a0)
	th.vlhff.v v4, 0(a0)
	th.vlhff.v v4, (a0), v0.t
	th.vlwff.v v4, (a0)
	th.vlwff.v v4, 0(a0)
	th.vlwff.v v4, (a0), v0.t
	th.vlbuff.v v4, (a0)
	th.vlbuff.v v4, 0(a0)
	th.vlbuff.v v4, (a0), v0.t
	th.vlhuff.v v4, (a0)
	th.vlhuff.v v4, 0(a0)
	th.vlhuff.v v4, (a0), v0.t
	th.vlwuff.v v4, (a0)
	th.vlwuff.v v4, 0(a0)
	th.vlwuff.v v4, (a0), v0.t
	th.vleff.v v4, (a0)
	th.vleff.v v4, 0(a0)
	th.vleff.v v4, (a0), v0.t

	th.vlseg2b.v v4, (a0)
	th.vlseg2b.v v4, 0(a0)
	th.vlseg2b.v v4, (a0), v0.t
	th.vlseg2h.v v4, (a0)
	th.vlseg2h.v v4, 0(a0)
	th.vlseg2h.v v4, (a0), v0.t
	th.vlseg2w.v v4, (a0)
	th.vlseg2w.v v4, 0(a0)
	th.vlseg2w.v v4, (a0), v0.t
	th.vlseg2bu.v v4, (a0)
	th.vlseg2bu.v v4, 0(a0)
	th.vlseg2bu.v v4, (a0), v0.t
	th.vlseg2hu.v v4, (a0)
	th.vlseg2hu.v v4, 0(a0)
	th.vlseg2hu.v v4, (a0), v0.t
	th.vlseg2wu.v v4, (a0)
	th.vlseg2wu.v v4, 0(a0)
	th.vlseg2wu.v v4, (a0), v0.t
	th.vlseg2e.v v4, (a0)
	th.vlseg2e.v v4, 0(a0)
	th.vlseg2e.v v4, (a0), v0.t
	th.vsseg2b.v v4, (a0)
	th.vsseg2b.v v4, 0(a0)
	th.vsseg2b.v v4, (a0), v0.t
	th.vsseg2h.v v4, (a0)
	th.vsseg2h.v v4, 0(a0)
	th.vsseg2h.v v4, (a0), v0.t
	th.vsseg2w.v v4, (a0)
	th.vsseg2w.v v4, 0(a0)
	th.vsseg2w.v v4, (a0), v0.t
	th.vsseg2e.v v4, (a0)
	th.vsseg2e.v v4, 0(a0)
	th.vsseg2e.v v4, (a0), v0.t

	th.vlseg3b.v v4, (a0)
	th.vlseg3b.v v4, 0(a0)
	th.vlseg3b.v v4, (a0), v0.t
	th.vlseg3h.v v4, (a0)
	th.vlseg3h.v v4, 0(a0)
	th.vlseg3h.v v4, (a0), v0.t
	th.vlseg3w.v v4, (a0)
	th.vlseg3w.v v4, 0(a0)
	th.vlseg3w.v v4, (a0), v0.t
	th.vlseg3bu.v v4, (a0)
	th.vlseg3bu.v v4, 0(a0)
	th.vlseg3bu.v v4, (a0), v0.t
	th.vlseg3hu.v v4, (a0)
	th.vlseg3hu.v v4, 0(a0)
	th.vlseg3hu.v v4, (a0), v0.t
	th.vlseg3wu.v v4, (a0)
	th.vlseg3wu.v v4, 0(a0)
	th.vlseg3wu.v v4, (a0), v0.t
	th.vlseg3e.v v4, (a0)
	th.vlseg3e.v v4, 0(a0)
	th.vlseg3e.v v4, (a0), v0.t
	th.vsseg3b.v v4, (a0)
	th.vsseg3b.v v4, 0(a0)
	th.vsseg3b.v v4, (a0), v0.t
	th.vsseg3h.v v4, (a0)
	th.vsseg3h.v v4, 0(a0)
	th.vsseg3h.v v4, (a0), v0.t
	th.vsseg3w.v v4, (a0)
	th.vsseg3w.v v4, 0(a0)
	th.vsseg3w.v v4, (a0), v0.t
	th.vsseg3e.v v4, (a0)
	th.vsseg3e.v v4, 0(a0)
	th.vsseg3e.v v4, (a0), v0.t

	th.vlseg4b.v v4, (a0)
	th.vlseg4b.v v4, 0(a0)
	th.vlseg4b.v v4, (a0), v0.t
	th.vlseg4h.v v4, (a0)
	th.vlseg4h.v v4, 0(a0)
	th.vlseg4h.v v4, (a0), v0.t
	th.vlseg4w.v v4, (a0)
	th.vlseg4w.v v4, 0(a0)
	th.vlseg4w.v v4, (a0), v0.t
	th.vlseg4bu.v v4, (a0)
	th.vlseg4bu.v v4, 0(a0)
	th.vlseg4bu.v v4, (a0), v0.t
	th.vlseg4hu.v v4, (a0)
	th.vlseg4hu.v v4, 0(a0)
	th.vlseg4hu.v v4, (a0), v0.t
	th.vlseg4wu.v v4, (a0)
	th.vlseg4wu.v v4, 0(a0)
	th.vlseg4wu.v v4, (a0), v0.t
	th.vlseg4e.v v4, (a0)
	th.vlseg4e.v v4, 0(a0)
	th.vlseg4e.v v4, (a0), v0.t
	th.vsseg4b.v v4, (a0)
	th.vsseg4b.v v4, 0(a0)
	th.vsseg4b.v v4, (a0), v0.t
	th.vsseg4h.v v4, (a0)
	th.vsseg4h.v v4, 0(a0)
	th.vsseg4h.v v4, (a0), v0.t
	th.vsseg4w.v v4, (a0)
	th.vsseg4w.v v4, 0(a0)
	th.vsseg4w.v v4, (a0), v0.t
	th.vsseg4e.v v4, (a0)
	th.vsseg4e.v v4, 0(a0)
	th.vsseg4e.v v4, (a0), v0.t

	th.vlseg5b.v v4, (a0)
	th.vlseg5b.v v4, 0(a0)
	th.vlseg5b.v v4, (a0), v0.t
	th.vlseg5h.v v4, (a0)
	th.vlseg5h.v v4, 0(a0)
	th.vlseg5h.v v4, (a0), v0.t
	th.vlseg5w.v v4, (a0)
	th.vlseg5w.v v4, 0(a0)
	th.vlseg5w.v v4, (a0), v0.t
	th.vlseg5bu.v v4, (a0)
	th.vlseg5bu.v v4, 0(a0)
	th.vlseg5bu.v v4, (a0), v0.t
	th.vlseg5hu.v v4, (a0)
	th.vlseg5hu.v v4, 0(a0)
	th.vlseg5hu.v v4, (a0), v0.t
	th.vlseg5wu.v v4, (a0)
	th.vlseg5wu.v v4, 0(a0)
	th.vlseg5wu.v v4, (a0), v0.t
	th.vlseg5e.v v4, (a0)
	th.vlseg5e.v v4, 0(a0)
	th.vlseg5e.v v4, (a0), v0.t
	th.vsseg5b.v v4, (a0)
	th.vsseg5b.v v4, 0(a0)
	th.vsseg5b.v v4, (a0), v0.t
	th.vsseg5h.v v4, (a0)
	th.vsseg5h.v v4, 0(a0)
	th.vsseg5h.v v4, (a0), v0.t
	th.vsseg5w.v v4, (a0)
	th.vsseg5w.v v4, 0(a0)
	th.vsseg5w.v v4, (a0), v0.t
	th.vsseg5e.v v4, (a0)
	th.vsseg5e.v v4, 0(a0)
	th.vsseg5e.v v4, (a0), v0.t

	th.vlseg6b.v v4, (a0)
	th.vlseg6b.v v4, 0(a0)
	th.vlseg6b.v v4, (a0), v0.t
	th.vlseg6h.v v4, (a0)
	th.vlseg6h.v v4, 0(a0)
	th.vlseg6h.v v4, (a0), v0.t
	th.vlseg6w.v v4, (a0)
	th.vlseg6w.v v4, 0(a0)
	th.vlseg6w.v v4, (a0), v0.t
	th.vlseg6bu.v v4, (a0)
	th.vlseg6bu.v v4, 0(a0)
	th.vlseg6bu.v v4, (a0), v0.t
	th.vlseg6hu.v v4, (a0)
	th.vlseg6hu.v v4, 0(a0)
	th.vlseg6hu.v v4, (a0), v0.t
	th.vlseg6wu.v v4, (a0)
	th.vlseg6wu.v v4, 0(a0)
	th.vlseg6wu.v v4, (a0), v0.t
	th.vlseg6e.v v4, (a0)
	th.vlseg6e.v v4, 0(a0)
	th.vlseg6e.v v4, (a0), v0.t
	th.vsseg6b.v v4, (a0)
	th.vsseg6b.v v4, 0(a0)
	th.vsseg6b.v v4, (a0), v0.t
	th.vsseg6h.v v4, (a0)
	th.vsseg6h.v v4, 0(a0)
	th.vsseg6h.v v4, (a0), v0.t
	th.vsseg6w.v v4, (a0)
	th.vsseg6w.v v4, 0(a0)
	th.vsseg6w.v v4, (a0), v0.t
	th.vsseg6e.v v4, (a0)
	th.vsseg6e.v v4, 0(a0)
	th.vsseg6e.v v4, (a0), v0.t

	th.vlseg7b.v v4, (a0)
	th.vlseg7b.v v4, 0(a0)
	th.vlseg7b.v v4, (a0), v0.t
	th.vlseg7h.v v4, (a0)
	th.vlseg7h.v v4, 0(a0)
	th.vlseg7h.v v4, (a0), v0.t
	th.vlseg7w.v v4, (a0)
	th.vlseg7w.v v4, 0(a0)
	th.vlseg7w.v v4, (a0), v0.t
	th.vlseg7bu.v v4, (a0)
	th.vlseg7bu.v v4, 0(a0)
	th.vlseg7bu.v v4, (a0), v0.t
	th.vlseg7hu.v v4, (a0)
	th.vlseg7hu.v v4, 0(a0)
	th.vlseg7hu.v v4, (a0), v0.t
	th.vlseg7wu.v v4, (a0)
	th.vlseg7wu.v v4, 0(a0)
	th.vlseg7wu.v v4, (a0), v0.t
	th.vlseg7e.v v4, (a0)
	th.vlseg7e.v v4, 0(a0)
	th.vlseg7e.v v4, (a0), v0.t
	th.vsseg7b.v v4, (a0)
	th.vsseg7b.v v4, 0(a0)
	th.vsseg7b.v v4, (a0), v0.t
	th.vsseg7h.v v4, (a0)
	th.vsseg7h.v v4, 0(a0)
	th.vsseg7h.v v4, (a0), v0.t
	th.vsseg7w.v v4, (a0)
	th.vsseg7w.v v4, 0(a0)
	th.vsseg7w.v v4, (a0), v0.t
	th.vsseg7e.v v4, (a0)
	th.vsseg7e.v v4, 0(a0)
	th.vsseg7e.v v4, (a0), v0.t

	th.vlseg8b.v v4, (a0)
	th.vlseg8b.v v4, 0(a0)
	th.vlseg8b.v v4, (a0), v0.t
	th.vlseg8h.v v4, (a0)
	th.vlseg8h.v v4, 0(a0)
	th.vlseg8h.v v4, (a0), v0.t
	th.vlseg8w.v v4, (a0)
	th.vlseg8w.v v4, 0(a0)
	th.vlseg8w.v v4, (a0), v0.t
	th.vlseg8bu.v v4, (a0)
	th.vlseg8bu.v v4, 0(a0)
	th.vlseg8bu.v v4, (a0), v0.t
	th.vlseg8hu.v v4, (a0)
	th.vlseg8hu.v v4, 0(a0)
	th.vlseg8hu.v v4, (a0), v0.t
	th.vlseg8wu.v v4, (a0)
	th.vlseg8wu.v v4, 0(a0)
	th.vlseg8wu.v v4, (a0), v0.t
	th.vlseg8e.v v4, (a0)
	th.vlseg8e.v v4, 0(a0)
	th.vlseg8e.v v4, (a0), v0.t
	th.vsseg8b.v v4, (a0)
	th.vsseg8b.v v4, 0(a0)
	th.vsseg8b.v v4, (a0), v0.t
	th.vsseg8h.v v4, (a0)
	th.vsseg8h.v v4, 0(a0)
	th.vsseg8h.v v4, (a0), v0.t
	th.vsseg8w.v v4, (a0)
	th.vsseg8w.v v4, 0(a0)
	th.vsseg8w.v v4, (a0), v0.t
	th.vsseg8e.v v4, (a0)
	th.vsseg8e.v v4, 0(a0)
	th.vsseg8e.v v4, (a0), v0.t

	th.vlsseg2b.v v4, (a0), a1
	th.vlsseg2b.v v4, 0(a0), a1
	th.vlsseg2b.v v4, (a0), a1, v0.t
	th.vlsseg2h.v v4, (a0), a1
	th.vlsseg2h.v v4, 0(a0), a1
	th.vlsseg2h.v v4, (a0), a1, v0.t
	th.vlsseg2w.v v4, (a0), a1
	th.vlsseg2w.v v4, 0(a0), a1
	th.vlsseg2w.v v4, (a0), a1, v0.t
	th.vlsseg2bu.v v4, (a0), a1
	th.vlsseg2bu.v v4, 0(a0), a1
	th.vlsseg2bu.v v4, (a0), a1, v0.t
	th.vlsseg2hu.v v4, (a0), a1
	th.vlsseg2hu.v v4, 0(a0), a1
	th.vlsseg2hu.v v4, (a0), a1, v0.t
	th.vlsseg2wu.v v4, (a0), a1
	th.vlsseg2wu.v v4, 0(a0), a1
	th.vlsseg2wu.v v4, (a0), a1, v0.t
	th.vlsseg2e.v v4, (a0), a1
	th.vlsseg2e.v v4, 0(a0), a1
	th.vlsseg2e.v v4, (a0), a1, v0.t
	th.vssseg2b.v v4, (a0), a1
	th.vssseg2b.v v4, 0(a0), a1
	th.vssseg2b.v v4, (a0), a1, v0.t
	th.vssseg2h.v v4, (a0), a1
	th.vssseg2h.v v4, 0(a0), a1
	th.vssseg2h.v v4, (a0), a1, v0.t
	th.vssseg2w.v v4, (a0), a1
	th.vssseg2w.v v4, 0(a0), a1
	th.vssseg2w.v v4, (a0), a1, v0.t
	th.vssseg2e.v v4, (a0), a1
	th.vssseg2e.v v4, 0(a0), a1
	th.vssseg2e.v v4, (a0), a1, v0.t

	th.vlsseg3b.v v4, (a0), a1
	th.vlsseg3b.v v4, 0(a0), a1
	th.vlsseg3b.v v4, (a0), a1, v0.t
	th.vlsseg3h.v v4, (a0), a1
	th.vlsseg3h.v v4, 0(a0), a1
	th.vlsseg3h.v v4, (a0), a1, v0.t
	th.vlsseg3w.v v4, (a0), a1
	th.vlsseg3w.v v4, 0(a0), a1
	th.vlsseg3w.v v4, (a0), a1, v0.t
	th.vlsseg3bu.v v4, (a0), a1
	th.vlsseg3bu.v v4, 0(a0), a1
	th.vlsseg3bu.v v4, (a0), a1, v0.t
	th.vlsseg3hu.v v4, (a0), a1
	th.vlsseg3hu.v v4, 0(a0), a1
	th.vlsseg3hu.v v4, (a0), a1, v0.t
	th.vlsseg3wu.v v4, (a0), a1
	th.vlsseg3wu.v v4, 0(a0), a1
	th.vlsseg3wu.v v4, (a0), a1, v0.t
	th.vlsseg3e.v v4, (a0), a1
	th.vlsseg3e.v v4, 0(a0), a1
	th.vlsseg3e.v v4, (a0), a1, v0.t
	th.vssseg3b.v v4, (a0), a1
	th.vssseg3b.v v4, 0(a0), a1
	th.vssseg3b.v v4, (a0), a1, v0.t
	th.vssseg3h.v v4, (a0), a1
	th.vssseg3h.v v4, 0(a0), a1
	th.vssseg3h.v v4, (a0), a1, v0.t
	th.vssseg3w.v v4, (a0), a1
	th.vssseg3w.v v4, 0(a0), a1
	th.vssseg3w.v v4, (a0), a1, v0.t
	th.vssseg3e.v v4, (a0), a1
	th.vssseg3e.v v4, 0(a0), a1
	th.vssseg3e.v v4, (a0), a1, v0.t

	th.vlsseg4b.v v4, (a0), a1
	th.vlsseg4b.v v4, 0(a0), a1
	th.vlsseg4b.v v4, (a0), a1, v0.t
	th.vlsseg4h.v v4, (a0), a1
	th.vlsseg4h.v v4, 0(a0), a1
	th.vlsseg4h.v v4, (a0), a1, v0.t
	th.vlsseg4w.v v4, (a0), a1
	th.vlsseg4w.v v4, 0(a0), a1
	th.vlsseg4w.v v4, (a0), a1, v0.t
	th.vlsseg4bu.v v4, (a0), a1
	th.vlsseg4bu.v v4, 0(a0), a1
	th.vlsseg4bu.v v4, (a0), a1, v0.t
	th.vlsseg4hu.v v4, (a0), a1
	th.vlsseg4hu.v v4, 0(a0), a1
	th.vlsseg4hu.v v4, (a0), a1, v0.t
	th.vlsseg4wu.v v4, (a0), a1
	th.vlsseg4wu.v v4, 0(a0), a1
	th.vlsseg4wu.v v4, (a0), a1, v0.t
	th.vlsseg4e.v v4, (a0), a1
	th.vlsseg4e.v v4, 0(a0), a1
	th.vlsseg4e.v v4, (a0), a1, v0.t
	th.vssseg4b.v v4, (a0), a1
	th.vssseg4b.v v4, 0(a0), a1
	th.vssseg4b.v v4, (a0), a1, v0.t
	th.vssseg4h.v v4, (a0), a1
	th.vssseg4h.v v4, 0(a0), a1
	th.vssseg4h.v v4, (a0), a1, v0.t
	th.vssseg4w.v v4, (a0), a1
	th.vssseg4w.v v4, 0(a0), a1
	th.vssseg4w.v v4, (a0), a1, v0.t
	th.vssseg4e.v v4, (a0), a1
	th.vssseg4e.v v4, 0(a0), a1
	th.vssseg4e.v v4, (a0), a1, v0.t

	th.vlsseg5b.v v4, (a0), a1
	th.vlsseg5b.v v4, 0(a0), a1
	th.vlsseg5b.v v4, (a0), a1, v0.t
	th.vlsseg5h.v v4, (a0), a1
	th.vlsseg5h.v v4, 0(a0), a1
	th.vlsseg5h.v v4, (a0), a1, v0.t
	th.vlsseg5w.v v4, (a0), a1
	th.vlsseg5w.v v4, 0(a0), a1
	th.vlsseg5w.v v4, (a0), a1, v0.t
	th.vlsseg5bu.v v4, (a0), a1
	th.vlsseg5bu.v v4, 0(a0), a1
	th.vlsseg5bu.v v4, (a0), a1, v0.t
	th.vlsseg5hu.v v4, (a0), a1
	th.vlsseg5hu.v v4, 0(a0), a1
	th.vlsseg5hu.v v4, (a0), a1, v0.t
	th.vlsseg5wu.v v4, (a0), a1
	th.vlsseg5wu.v v4, 0(a0), a1
	th.vlsseg5wu.v v4, (a0), a1, v0.t
	th.vlsseg5e.v v4, (a0), a1
	th.vlsseg5e.v v4, 0(a0), a1
	th.vlsseg5e.v v4, (a0), a1, v0.t
	th.vssseg5b.v v4, (a0), a1
	th.vssseg5b.v v4, 0(a0), a1
	th.vssseg5b.v v4, (a0), a1, v0.t
	th.vssseg5h.v v4, (a0), a1
	th.vssseg5h.v v4, 0(a0), a1
	th.vssseg5h.v v4, (a0), a1, v0.t
	th.vssseg5w.v v4, (a0), a1
	th.vssseg5w.v v4, 0(a0), a1
	th.vssseg5w.v v4, (a0), a1, v0.t
	th.vssseg5e.v v4, (a0), a1
	th.vssseg5e.v v4, 0(a0), a1
	th.vssseg5e.v v4, (a0), a1, v0.t

	th.vlsseg6b.v v4, (a0), a1
	th.vlsseg6b.v v4, 0(a0), a1
	th.vlsseg6b.v v4, (a0), a1, v0.t
	th.vlsseg6h.v v4, (a0), a1
	th.vlsseg6h.v v4, 0(a0), a1
	th.vlsseg6h.v v4, (a0), a1, v0.t
	th.vlsseg6w.v v4, (a0), a1
	th.vlsseg6w.v v4, 0(a0), a1
	th.vlsseg6w.v v4, (a0), a1, v0.t
	th.vlsseg6bu.v v4, (a0), a1
	th.vlsseg6bu.v v4, 0(a0), a1
	th.vlsseg6bu.v v4, (a0), a1, v0.t
	th.vlsseg6hu.v v4, (a0), a1
	th.vlsseg6hu.v v4, 0(a0), a1
	th.vlsseg6hu.v v4, (a0), a1, v0.t
	th.vlsseg6wu.v v4, (a0), a1
	th.vlsseg6wu.v v4, 0(a0), a1
	th.vlsseg6wu.v v4, (a0), a1, v0.t
	th.vlsseg6e.v v4, (a0), a1
	th.vlsseg6e.v v4, 0(a0), a1
	th.vlsseg6e.v v4, (a0), a1, v0.t
	th.vssseg6b.v v4, (a0), a1
	th.vssseg6b.v v4, 0(a0), a1
	th.vssseg6b.v v4, (a0), a1, v0.t
	th.vssseg6h.v v4, (a0), a1
	th.vssseg6h.v v4, 0(a0), a1
	th.vssseg6h.v v4, (a0), a1, v0.t
	th.vssseg6w.v v4, (a0), a1
	th.vssseg6w.v v4, 0(a0), a1
	th.vssseg6w.v v4, (a0), a1, v0.t
	th.vssseg6e.v v4, (a0), a1
	th.vssseg6e.v v4, 0(a0), a1
	th.vssseg6e.v v4, (a0), a1, v0.t

	th.vlsseg7b.v v4, (a0), a1
	th.vlsseg7b.v v4, 0(a0), a1
	th.vlsseg7b.v v4, (a0), a1, v0.t
	th.vlsseg7h.v v4, (a0), a1
	th.vlsseg7h.v v4, 0(a0), a1
	th.vlsseg7h.v v4, (a0), a1, v0.t
	th.vlsseg7w.v v4, (a0), a1
	th.vlsseg7w.v v4, 0(a0), a1
	th.vlsseg7w.v v4, (a0), a1, v0.t
	th.vlsseg7bu.v v4, (a0), a1
	th.vlsseg7bu.v v4, 0(a0), a1
	th.vlsseg7bu.v v4, (a0), a1, v0.t
	th.vlsseg7hu.v v4, (a0), a1
	th.vlsseg7hu.v v4, 0(a0), a1
	th.vlsseg7hu.v v4, (a0), a1, v0.t
	th.vlsseg7wu.v v4, (a0), a1
	th.vlsseg7wu.v v4, 0(a0), a1
	th.vlsseg7wu.v v4, (a0), a1, v0.t
	th.vlsseg7e.v v4, (a0), a1
	th.vlsseg7e.v v4, 0(a0), a1
	th.vlsseg7e.v v4, (a0), a1, v0.t
	th.vssseg7b.v v4, (a0), a1
	th.vssseg7b.v v4, 0(a0), a1
	th.vssseg7b.v v4, (a0), a1, v0.t
	th.vssseg7h.v v4, (a0), a1
	th.vssseg7h.v v4, 0(a0), a1
	th.vssseg7h.v v4, (a0), a1, v0.t
	th.vssseg7w.v v4, (a0), a1
	th.vssseg7w.v v4, 0(a0), a1
	th.vssseg7w.v v4, (a0), a1, v0.t
	th.vssseg7e.v v4, (a0), a1
	th.vssseg7e.v v4, 0(a0), a1
	th.vssseg7e.v v4, (a0), a1, v0.t

	th.vlsseg8b.v v4, (a0), a1
	th.vlsseg8b.v v4, 0(a0), a1
	th.vlsseg8b.v v4, (a0), a1, v0.t
	th.vlsseg8h.v v4, (a0), a1
	th.vlsseg8h.v v4, 0(a0), a1
	th.vlsseg8h.v v4, (a0), a1, v0.t
	th.vlsseg8w.v v4, (a0), a1
	th.vlsseg8w.v v4, 0(a0), a1
	th.vlsseg8w.v v4, (a0), a1, v0.t
	th.vlsseg8bu.v v4, (a0), a1
	th.vlsseg8bu.v v4, 0(a0), a1
	th.vlsseg8bu.v v4, (a0), a1, v0.t
	th.vlsseg8hu.v v4, (a0), a1
	th.vlsseg8hu.v v4, 0(a0), a1
	th.vlsseg8hu.v v4, (a0), a1, v0.t
	th.vlsseg8wu.v v4, (a0), a1
	th.vlsseg8wu.v v4, 0(a0), a1
	th.vlsseg8wu.v v4, (a0), a1, v0.t
	th.vlsseg8e.v v4, (a0), a1
	th.vlsseg8e.v v4, 0(a0), a1
	th.vlsseg8e.v v4, (a0), a1, v0.t
	th.vssseg8b.v v4, (a0), a1
	th.vssseg8b.v v4, 0(a0), a1
	th.vssseg8b.v v4, (a0), a1, v0.t
	th.vssseg8h.v v4, (a0), a1
	th.vssseg8h.v v4, 0(a0), a1
	th.vssseg8h.v v4, (a0), a1, v0.t
	th.vssseg8w.v v4, (a0), a1
	th.vssseg8w.v v4, 0(a0), a1
	th.vssseg8w.v v4, (a0), a1, v0.t
	th.vssseg8e.v v4, (a0), a1
	th.vssseg8e.v v4, 0(a0), a1
	th.vssseg8e.v v4, (a0), a1, v0.t

	th.vlxseg2b.v v4, (a0), v12
	th.vlxseg2b.v v4, 0(a0), v12
	th.vlxseg2b.v v4, (a0), v12, v0.t
	th.vlxseg2h.v v4, (a0), v12
	th.vlxseg2h.v v4, 0(a0), v12
	th.vlxseg2h.v v4, (a0), v12, v0.t
	th.vlxseg2w.v v4, (a0), v12
	th.vlxseg2w.v v4, 0(a0), v12
	th.vlxseg2w.v v4, (a0), v12, v0.t
	th.vlxseg2bu.v v4, (a0), v12
	th.vlxseg2bu.v v4, 0(a0), v12
	th.vlxseg2bu.v v4, (a0), v12, v0.t
	th.vlxseg2hu.v v4, (a0), v12
	th.vlxseg2hu.v v4, 0(a0), v12
	th.vlxseg2hu.v v4, (a0), v12, v0.t
	th.vlxseg2wu.v v4, (a0), v12
	th.vlxseg2wu.v v4, 0(a0), v12
	th.vlxseg2wu.v v4, (a0), v12, v0.t
	th.vlxseg2e.v v4, (a0), v12
	th.vlxseg2e.v v4, 0(a0), v12
	th.vlxseg2e.v v4, (a0), v12, v0.t
	th.vsxseg2b.v v4, (a0), v12
	th.vsxseg2b.v v4, 0(a0), v12
	th.vsxseg2b.v v4, (a0), v12, v0.t
	th.vsxseg2h.v v4, (a0), v12
	th.vsxseg2h.v v4, 0(a0), v12
	th.vsxseg2h.v v4, (a0), v12, v0.t
	th.vsxseg2w.v v4, (a0), v12
	th.vsxseg2w.v v4, 0(a0), v12
	th.vsxseg2w.v v4, (a0), v12, v0.t
	th.vsxseg2e.v v4, (a0), v12
	th.vsxseg2e.v v4, 0(a0), v12
	th.vsxseg2e.v v4, (a0), v12, v0.t

	th.vlxseg3b.v v4, (a0), v12
	th.vlxseg3b.v v4, 0(a0), v12
	th.vlxseg3b.v v4, (a0), v12, v0.t
	th.vlxseg3h.v v4, (a0), v12
	th.vlxseg3h.v v4, 0(a0), v12
	th.vlxseg3h.v v4, (a0), v12, v0.t
	th.vlxseg3w.v v4, (a0), v12
	th.vlxseg3w.v v4, 0(a0), v12
	th.vlxseg3w.v v4, (a0), v12, v0.t
	th.vlxseg3bu.v v4, (a0), v12
	th.vlxseg3bu.v v4, 0(a0), v12
	th.vlxseg3bu.v v4, (a0), v12, v0.t
	th.vlxseg3hu.v v4, (a0), v12
	th.vlxseg3hu.v v4, 0(a0), v12
	th.vlxseg3hu.v v4, (a0), v12, v0.t
	th.vlxseg3wu.v v4, (a0), v12
	th.vlxseg3wu.v v4, 0(a0), v12
	th.vlxseg3wu.v v4, (a0), v12, v0.t
	th.vlxseg3e.v v4, (a0), v12
	th.vlxseg3e.v v4, 0(a0), v12
	th.vlxseg3e.v v4, (a0), v12, v0.t
	th.vsxseg3b.v v4, (a0), v12
	th.vsxseg3b.v v4, 0(a0), v12
	th.vsxseg3b.v v4, (a0), v12, v0.t
	th.vsxseg3h.v v4, (a0), v12
	th.vsxseg3h.v v4, 0(a0), v12
	th.vsxseg3h.v v4, (a0), v12, v0.t
	th.vsxseg3w.v v4, (a0), v12
	th.vsxseg3w.v v4, 0(a0), v12
	th.vsxseg3w.v v4, (a0), v12, v0.t
	th.vsxseg3e.v v4, (a0), v12
	th.vsxseg3e.v v4, 0(a0), v12
	th.vsxseg3e.v v4, (a0), v12, v0.t

	th.vlxseg4b.v v4, (a0), v12
	th.vlxseg4b.v v4, 0(a0), v12
	th.vlxseg4b.v v4, (a0), v12, v0.t
	th.vlxseg4h.v v4, (a0), v12
	th.vlxseg4h.v v4, 0(a0), v12
	th.vlxseg4h.v v4, (a0), v12, v0.t
	th.vlxseg4w.v v4, (a0), v12
	th.vlxseg4w.v v4, 0(a0), v12
	th.vlxseg4w.v v4, (a0), v12, v0.t
	th.vlxseg4bu.v v4, (a0), v12
	th.vlxseg4bu.v v4, 0(a0), v12
	th.vlxseg4bu.v v4, (a0), v12, v0.t
	th.vlxseg4hu.v v4, (a0), v12
	th.vlxseg4hu.v v4, 0(a0), v12
	th.vlxseg4hu.v v4, (a0), v12, v0.t
	th.vlxseg4wu.v v4, (a0), v12
	th.vlxseg4wu.v v4, 0(a0), v12
	th.vlxseg4wu.v v4, (a0), v12, v0.t
	th.vlxseg4e.v v4, (a0), v12
	th.vlxseg4e.v v4, 0(a0), v12
	th.vlxseg4e.v v4, (a0), v12, v0.t
	th.vsxseg4b.v v4, (a0), v12
	th.vsxseg4b.v v4, 0(a0), v12
	th.vsxseg4b.v v4, (a0), v12, v0.t
	th.vsxseg4h.v v4, (a0), v12
	th.vsxseg4h.v v4, 0(a0), v12
	th.vsxseg4h.v v4, (a0), v12, v0.t
	th.vsxseg4w.v v4, (a0), v12
	th.vsxseg4w.v v4, 0(a0), v12
	th.vsxseg4w.v v4, (a0), v12, v0.t
	th.vsxseg4e.v v4, (a0), v12
	th.vsxseg4e.v v4, 0(a0), v12
	th.vsxseg4e.v v4, (a0), v12, v0.t

	th.vlxseg5b.v v4, (a0), v12
	th.vlxseg5b.v v4, 0(a0), v12
	th.vlxseg5b.v v4, (a0), v12, v0.t
	th.vlxseg5h.v v4, (a0), v12
	th.vlxseg5h.v v4, 0(a0), v12
	th.vlxseg5h.v v4, (a0), v12, v0.t
	th.vlxseg5w.v v4, (a0), v12
	th.vlxseg5w.v v4, 0(a0), v12
	th.vlxseg5w.v v4, (a0), v12, v0.t
	th.vlxseg5bu.v v4, (a0), v12
	th.vlxseg5bu.v v4, 0(a0), v12
	th.vlxseg5bu.v v4, (a0), v12, v0.t
	th.vlxseg5hu.v v4, (a0), v12
	th.vlxseg5hu.v v4, 0(a0), v12
	th.vlxseg5hu.v v4, (a0), v12, v0.t
	th.vlxseg5wu.v v4, (a0), v12
	th.vlxseg5wu.v v4, 0(a0), v12
	th.vlxseg5wu.v v4, (a0), v12, v0.t
	th.vlxseg5e.v v4, (a0), v12
	th.vlxseg5e.v v4, 0(a0), v12
	th.vlxseg5e.v v4, (a0), v12, v0.t
	th.vsxseg5b.v v4, (a0), v12
	th.vsxseg5b.v v4, 0(a0), v12
	th.vsxseg5b.v v4, (a0), v12, v0.t
	th.vsxseg5h.v v4, (a0), v12
	th.vsxseg5h.v v4, 0(a0), v12
	th.vsxseg5h.v v4, (a0), v12, v0.t
	th.vsxseg5w.v v4, (a0), v12
	th.vsxseg5w.v v4, 0(a0), v12
	th.vsxseg5w.v v4, (a0), v12, v0.t
	th.vsxseg5e.v v4, (a0), v12
	th.vsxseg5e.v v4, 0(a0), v12
	th.vsxseg5e.v v4, (a0), v12, v0.t

	th.vlxseg6b.v v4, (a0), v12
	th.vlxseg6b.v v4, 0(a0), v12
	th.vlxseg6b.v v4, (a0), v12, v0.t
	th.vlxseg6h.v v4, (a0), v12
	th.vlxseg6h.v v4, 0(a0), v12
	th.vlxseg6h.v v4, (a0), v12, v0.t
	th.vlxseg6w.v v4, (a0), v12
	th.vlxseg6w.v v4, 0(a0), v12
	th.vlxseg6w.v v4, (a0), v12, v0.t
	th.vlxseg6bu.v v4, (a0), v12
	th.vlxseg6bu.v v4, 0(a0), v12
	th.vlxseg6bu.v v4, (a0), v12, v0.t
	th.vlxseg6hu.v v4, (a0), v12
	th.vlxseg6hu.v v4, 0(a0), v12
	th.vlxseg6hu.v v4, (a0), v12, v0.t
	th.vlxseg6wu.v v4, (a0), v12
	th.vlxseg6wu.v v4, 0(a0), v12
	th.vlxseg6wu.v v4, (a0), v12, v0.t
	th.vlxseg6e.v v4, (a0), v12
	th.vlxseg6e.v v4, 0(a0), v12
	th.vlxseg6e.v v4, (a0), v12, v0.t
	th.vsxseg6b.v v4, (a0), v12
	th.vsxseg6b.v v4, 0(a0), v12
	th.vsxseg6b.v v4, (a0), v12, v0.t
	th.vsxseg6h.v v4, (a0), v12
	th.vsxseg6h.v v4, 0(a0), v12
	th.vsxseg6h.v v4, (a0), v12, v0.t
	th.vsxseg6w.v v4, (a0), v12
	th.vsxseg6w.v v4, 0(a0), v12
	th.vsxseg6w.v v4, (a0), v12, v0.t
	th.vsxseg6e.v v4, (a0), v12
	th.vsxseg6e.v v4, 0(a0), v12
	th.vsxseg6e.v v4, (a0), v12, v0.t

	th.vlxseg7b.v v4, (a0), v12
	th.vlxseg7b.v v4, 0(a0), v12
	th.vlxseg7b.v v4, (a0), v12, v0.t
	th.vlxseg7h.v v4, (a0), v12
	th.vlxseg7h.v v4, 0(a0), v12
	th.vlxseg7h.v v4, (a0), v12, v0.t
	th.vlxseg7w.v v4, (a0), v12
	th.vlxseg7w.v v4, 0(a0), v12
	th.vlxseg7w.v v4, (a0), v12, v0.t
	th.vlxseg7bu.v v4, (a0), v12
	th.vlxseg7bu.v v4, 0(a0), v12
	th.vlxseg7bu.v v4, (a0), v12, v0.t
	th.vlxseg7hu.v v4, (a0), v12
	th.vlxseg7hu.v v4, 0(a0), v12
	th.vlxseg7hu.v v4, (a0), v12, v0.t
	th.vlxseg7wu.v v4, (a0), v12
	th.vlxseg7wu.v v4, 0(a0), v12
	th.vlxseg7wu.v v4, (a0), v12, v0.t
	th.vlxseg7e.v v4, (a0), v12
	th.vlxseg7e.v v4, 0(a0), v12
	th.vlxseg7e.v v4, (a0), v12, v0.t
	th.vsxseg7b.v v4, (a0), v12
	th.vsxseg7b.v v4, 0(a0), v12
	th.vsxseg7b.v v4, (a0), v12, v0.t
	th.vsxseg7h.v v4, (a0), v12
	th.vsxseg7h.v v4, 0(a0), v12
	th.vsxseg7h.v v4, (a0), v12, v0.t
	th.vsxseg7w.v v4, (a0), v12
	th.vsxseg7w.v v4, 0(a0), v12
	th.vsxseg7w.v v4, (a0), v12, v0.t
	th.vsxseg7e.v v4, (a0), v12
	th.vsxseg7e.v v4, 0(a0), v12
	th.vsxseg7e.v v4, (a0), v12, v0.t

	th.vlxseg8b.v v4, (a0), v12
	th.vlxseg8b.v v4, 0(a0), v12
	th.vlxseg8b.v v4, (a0), v12, v0.t
	th.vlxseg8h.v v4, (a0), v12
	th.vlxseg8h.v v4, 0(a0), v12
	th.vlxseg8h.v v4, (a0), v12, v0.t
	th.vlxseg8w.v v4, (a0), v12
	th.vlxseg8w.v v4, 0(a0), v12
	th.vlxseg8w.v v4, (a0), v12, v0.t
	th.vlxseg8bu.v v4, (a0), v12
	th.vlxseg8bu.v v4, 0(a0), v12
	th.vlxseg8bu.v v4, (a0), v12, v0.t
	th.vlxseg8hu.v v4, (a0), v12
	th.vlxseg8hu.v v4, 0(a0), v12
	th.vlxseg8hu.v v4, (a0), v12, v0.t
	th.vlxseg8wu.v v4, (a0), v12
	th.vlxseg8wu.v v4, 0(a0), v12
	th.vlxseg8wu.v v4, (a0), v12, v0.t
	th.vlxseg8e.v v4, (a0), v12
	th.vlxseg8e.v v4, 0(a0), v12
	th.vlxseg8e.v v4, (a0), v12, v0.t
	th.vsxseg8b.v v4, (a0), v12
	th.vsxseg8b.v v4, 0(a0), v12
	th.vsxseg8b.v v4, (a0), v12, v0.t
	th.vsxseg8h.v v4, (a0), v12
	th.vsxseg8h.v v4, 0(a0), v12
	th.vsxseg8h.v v4, (a0), v12, v0.t
	th.vsxseg8w.v v4, (a0), v12
	th.vsxseg8w.v v4, 0(a0), v12
	th.vsxseg8w.v v4, (a0), v12, v0.t
	th.vsxseg8e.v v4, (a0), v12
	th.vsxseg8e.v v4, 0(a0), v12
	th.vsxseg8e.v v4, (a0), v12, v0.t

	th.vlseg2bff.v v4, (a0)
	th.vlseg2bff.v v4, 0(a0)
	th.vlseg2bff.v v4, (a0), v0.t
	th.vlseg2hff.v v4, (a0)
	th.vlseg2hff.v v4, 0(a0)
	th.vlseg2hff.v v4, (a0), v0.t
	th.vlseg2wff.v v4, (a0)
	th.vlseg2wff.v v4, 0(a0)
	th.vlseg2wff.v v4, (a0), v0.t
	th.vlseg2buff.v v4, (a0)
	th.vlseg2buff.v v4, 0(a0)
	th.vlseg2buff.v v4, (a0), v0.t
	th.vlseg2huff.v v4, (a0)
	th.vlseg2huff.v v4, 0(a0)
	th.vlseg2huff.v v4, (a0), v0.t
	th.vlseg2wuff.v v4, (a0)
	th.vlseg2wuff.v v4, 0(a0)
	th.vlseg2wuff.v v4, (a0), v0.t
	th.vlseg2eff.v v4, (a0)
	th.vlseg2eff.v v4, 0(a0)
	th.vlseg2eff.v v4, (a0), v0.t

	th.vlseg3bff.v v4, (a0)
	th.vlseg3bff.v v4, 0(a0)
	th.vlseg3bff.v v4, (a0), v0.t
	th.vlseg3hff.v v4, (a0)
	th.vlseg3hff.v v4, 0(a0)
	th.vlseg3hff.v v4, (a0), v0.t
	th.vlseg3wff.v v4, (a0)
	th.vlseg3wff.v v4, 0(a0)
	th.vlseg3wff.v v4, (a0), v0.t
	th.vlseg3buff.v v4, (a0)
	th.vlseg3buff.v v4, 0(a0)
	th.vlseg3buff.v v4, (a0), v0.t
	th.vlseg3huff.v v4, (a0)
	th.vlseg3huff.v v4, 0(a0)
	th.vlseg3huff.v v4, (a0), v0.t
	th.vlseg3wuff.v v4, (a0)
	th.vlseg3wuff.v v4, 0(a0)
	th.vlseg3wuff.v v4, (a0), v0.t
	th.vlseg3eff.v v4, (a0)
	th.vlseg3eff.v v4, 0(a0)
	th.vlseg3eff.v v4, (a0), v0.t

	th.vlseg4bff.v v4, (a0)
	th.vlseg4bff.v v4, 0(a0)
	th.vlseg4bff.v v4, (a0), v0.t
	th.vlseg4hff.v v4, (a0)
	th.vlseg4hff.v v4, 0(a0)
	th.vlseg4hff.v v4, (a0), v0.t
	th.vlseg4wff.v v4, (a0)
	th.vlseg4wff.v v4, 0(a0)
	th.vlseg4wff.v v4, (a0), v0.t
	th.vlseg4buff.v v4, (a0)
	th.vlseg4buff.v v4, 0(a0)
	th.vlseg4buff.v v4, (a0), v0.t
	th.vlseg4huff.v v4, (a0)
	th.vlseg4huff.v v4, 0(a0)
	th.vlseg4huff.v v4, (a0), v0.t
	th.vlseg4wuff.v v4, (a0)
	th.vlseg4wuff.v v4, 0(a0)
	th.vlseg4wuff.v v4, (a0), v0.t
	th.vlseg4eff.v v4, (a0)
	th.vlseg4eff.v v4, 0(a0)
	th.vlseg4eff.v v4, (a0), v0.t

	th.vlseg5bff.v v4, (a0)
	th.vlseg5bff.v v4, 0(a0)
	th.vlseg5bff.v v4, (a0), v0.t
	th.vlseg5hff.v v4, (a0)
	th.vlseg5hff.v v4, 0(a0)
	th.vlseg5hff.v v4, (a0), v0.t
	th.vlseg5wff.v v4, (a0)
	th.vlseg5wff.v v4, 0(a0)
	th.vlseg5wff.v v4, (a0), v0.t
	th.vlseg5buff.v v4, (a0)
	th.vlseg5buff.v v4, 0(a0)
	th.vlseg5buff.v v4, (a0), v0.t
	th.vlseg5huff.v v4, (a0)
	th.vlseg5huff.v v4, 0(a0)
	th.vlseg5huff.v v4, (a0), v0.t
	th.vlseg5wuff.v v4, (a0)
	th.vlseg5wuff.v v4, 0(a0)
	th.vlseg5wuff.v v4, (a0), v0.t
	th.vlseg5eff.v v4, (a0)
	th.vlseg5eff.v v4, 0(a0)
	th.vlseg5eff.v v4, (a0), v0.t

	th.vlseg6bff.v v4, (a0)
	th.vlseg6bff.v v4, 0(a0)
	th.vlseg6bff.v v4, (a0), v0.t
	th.vlseg6hff.v v4, (a0)
	th.vlseg6hff.v v4, 0(a0)
	th.vlseg6hff.v v4, (a0), v0.t
	th.vlseg6wff.v v4, (a0)
	th.vlseg6wff.v v4, 0(a0)
	th.vlseg6wff.v v4, (a0), v0.t
	th.vlseg6buff.v v4, (a0)
	th.vlseg6buff.v v4, 0(a0)
	th.vlseg6buff.v v4, (a0), v0.t
	th.vlseg6huff.v v4, (a0)
	th.vlseg6huff.v v4, 0(a0)
	th.vlseg6huff.v v4, (a0), v0.t
	th.vlseg6wuff.v v4, (a0)
	th.vlseg6wuff.v v4, 0(a0)
	th.vlseg6wuff.v v4, (a0), v0.t
	th.vlseg6eff.v v4, (a0)
	th.vlseg6eff.v v4, 0(a0)
	th.vlseg6eff.v v4, (a0), v0.t

	th.vlseg7bff.v v4, (a0)
	th.vlseg7bff.v v4, 0(a0)
	th.vlseg7bff.v v4, (a0), v0.t
	th.vlseg7hff.v v4, (a0)
	th.vlseg7hff.v v4, 0(a0)
	th.vlseg7hff.v v4, (a0), v0.t
	th.vlseg7wff.v v4, (a0)
	th.vlseg7wff.v v4, 0(a0)
	th.vlseg7wff.v v4, (a0), v0.t
	th.vlseg7buff.v v4, (a0)
	th.vlseg7buff.v v4, 0(a0)
	th.vlseg7buff.v v4, (a0), v0.t
	th.vlseg7huff.v v4, (a0)
	th.vlseg7huff.v v4, 0(a0)
	th.vlseg7huff.v v4, (a0), v0.t
	th.vlseg7wuff.v v4, (a0)
	th.vlseg7wuff.v v4, 0(a0)
	th.vlseg7wuff.v v4, (a0), v0.t
	th.vlseg7eff.v v4, (a0)
	th.vlseg7eff.v v4, 0(a0)
	th.vlseg7eff.v v4, (a0), v0.t

	th.vlseg8bff.v v4, (a0)
	th.vlseg8bff.v v4, 0(a0)
	th.vlseg8bff.v v4, (a0), v0.t
	th.vlseg8hff.v v4, (a0)
	th.vlseg8hff.v v4, 0(a0)
	th.vlseg8hff.v v4, (a0), v0.t
	th.vlseg8wff.v v4, (a0)
	th.vlseg8wff.v v4, 0(a0)
	th.vlseg8wff.v v4, (a0), v0.t
	th.vlseg8buff.v v4, (a0)
	th.vlseg8buff.v v4, 0(a0)
	th.vlseg8buff.v v4, (a0), v0.t
	th.vlseg8huff.v v4, (a0)
	th.vlseg8huff.v v4, 0(a0)
	th.vlseg8huff.v v4, (a0), v0.t
	th.vlseg8wuff.v v4, (a0)
	th.vlseg8wuff.v v4, 0(a0)
	th.vlseg8wuff.v v4, (a0), v0.t
	th.vlseg8eff.v v4, (a0)
	th.vlseg8eff.v v4, 0(a0)
	th.vlseg8eff.v v4, (a0), v0.t

        # Aliases
	th.vneg.v v4, v8
	th.vneg.v v4, v8, v0.t

	th.vadd.vv v4, v8, v12
	th.vadd.vx v4, v8, a1
	th.vadd.vi v4, v8, 15
	th.vadd.vi v4, v8, -16
	th.vadd.vv v4, v8, v12, v0.t
	th.vadd.vx v4, v8, a1, v0.t
	th.vadd.vi v4, v8, 15, v0.t
	th.vadd.vi v4, v8, -16, v0.t
	th.vsub.vv v4, v8, v12
	th.vsub.vx v4, v8, a1
	th.vrsub.vx v4, v8, a1
	th.vrsub.vi v4, v8, 15
	th.vrsub.vi v4, v8, -16
	th.vsub.vv v4, v8, v12, v0.t
	th.vsub.vx v4, v8, a1, v0.t
	th.vrsub.vx v4, v8, a1, v0.t
	th.vrsub.vi v4, v8, 15, v0.t
	th.vrsub.vi v4, v8, -16, v0.t

	# Aliases
	th.vwcvt.x.x.v v4, v8
	th.vwcvtu.x.x.v v4, v8
	th.vwcvt.x.x.v v4, v8, v0.t
	th.vwcvtu.x.x.v v4, v8, v0.t

	th.vwaddu.vv v4, v8, v12
	th.vwaddu.vx v4, v8, a1
	th.vwaddu.vv v4, v8, v12, v0.t
	th.vwaddu.vx v4, v8, a1, v0.t
	th.vwsubu.vv v4, v8, v12
	th.vwsubu.vx v4, v8, a1
	th.vwsubu.vv v4, v8, v12, v0.t
	th.vwsubu.vx v4, v8, a1, v0.t
	th.vwadd.vv v4, v8, v12
	th.vwadd.vx v4, v8, a1
	th.vwadd.vv v4, v8, v12, v0.t
	th.vwadd.vx v4, v8, a1, v0.t
	th.vwsub.vv v4, v8, v12
	th.vwsub.vx v4, v8, a1
	th.vwsub.vv v4, v8, v12, v0.t
	th.vwsub.vx v4, v8, a1, v0.t
	th.vwaddu.wv v4, v8, v12
	th.vwaddu.wx v4, v8, a1
	th.vwaddu.wv v4, v8, v12, v0.t
	th.vwaddu.wx v4, v8, a1, v0.t
	th.vwsubu.wv v4, v8, v12
	th.vwsubu.wx v4, v8, a1
	th.vwsubu.wv v4, v8, v12, v0.t
	th.vwsubu.wx v4, v8, a1, v0.t
	th.vwadd.wv v4, v8, v12
	th.vwadd.wx v4, v8, a1
	th.vwadd.wv v4, v8, v12, v0.t
	th.vwadd.wx v4, v8, a1, v0.t
	th.vwsub.wv v4, v8, v12
	th.vwsub.wx v4, v8, a1
	th.vwsub.wv v4, v8, v12, v0.t
	th.vwsub.wx v4, v8, a1, v0.t

	th.vadc.vvm v4, v8, v12, v0
	th.vadc.vxm v4, v8, a1, v0
	th.vadc.vim v4, v8, 15, v0
	th.vadc.vim v4, v8, -16, v0
	th.vmadc.vvm v4, v8, v12, v0
	th.vmadc.vxm v4, v8, a1, v0
	th.vmadc.vim v4, v8, 15, v0
	th.vmadc.vim v4, v8, -16, v0
	th.vsbc.vvm v4, v8, v12, v0
	th.vsbc.vxm v4, v8, a1, v0
	th.vmsbc.vvm v4, v8, v12, v0
	th.vmsbc.vxm v4, v8, a1, v0

	# Aliases
	th.vnot.v v4, v8
	th.vnot.v v4, v8, v0.t

	th.vand.vv v4, v8, v12
	th.vand.vx v4, v8, a1
	th.vand.vi v4, v8, 15
	th.vand.vi v4, v8, -16
	th.vand.vv v4, v8, v12, v0.t
	th.vand.vx v4, v8, a1, v0.t
	th.vand.vi v4, v8, 15, v0.t
	th.vand.vi v4, v8, -16, v0.t
	th.vor.vv v4, v8, v12
	th.vor.vx v4, v8, a1
	th.vor.vi v4, v8, 15
	th.vor.vi v4, v8, -16
	th.vor.vv v4, v8, v12, v0.t
	th.vor.vx v4, v8, a1, v0.t
	th.vor.vi v4, v8, 15, v0.t
	th.vor.vi v4, v8, -16, v0.t
	th.vxor.vv v4, v8, v12
	th.vxor.vx v4, v8, a1
	th.vxor.vi v4, v8, 15
	th.vxor.vi v4, v8, -16
	th.vxor.vv v4, v8, v12, v0.t
	th.vxor.vx v4, v8, a1, v0.t
	th.vxor.vi v4, v8, 15, v0.t
	th.vxor.vi v4, v8, -16, v0.t

	th.vsll.vv v4, v8, v12
	th.vsll.vx v4, v8, a1
	th.vsll.vi v4, v8, 1
	th.vsll.vi v4, v8, 31
	th.vsll.vv v4, v8, v12, v0.t
	th.vsll.vx v4, v8, a1, v0.t
	th.vsll.vi v4, v8, 1, v0.t
	th.vsll.vi v4, v8, 31, v0.t
	th.vsrl.vv v4, v8, v12
	th.vsrl.vx v4, v8, a1
	th.vsrl.vi v4, v8, 1
	th.vsrl.vi v4, v8, 31
	th.vsrl.vv v4, v8, v12, v0.t
	th.vsrl.vx v4, v8, a1, v0.t
	th.vsrl.vi v4, v8, 1, v0.t
	th.vsrl.vi v4, v8, 31, v0.t
	th.vsra.vv v4, v8, v12
	th.vsra.vx v4, v8, a1
	th.vsra.vi v4, v8, 1
	th.vsra.vi v4, v8, 31
	th.vsra.vv v4, v8, v12, v0.t
	th.vsra.vx v4, v8, a1, v0.t
	th.vsra.vi v4, v8, 1, v0.t
	th.vsra.vi v4, v8, 31, v0.t

	# Aliases
	th.vncvt.x.x.v v4, v8
	th.vncvt.x.x.v v4, v8, v0.t

	th.vnsrl.vv v4, v8, v12
	th.vnsrl.vx v4, v8, a1
	th.vnsrl.vi v4, v8, 1
	th.vnsrl.vi v4, v8, 31
	th.vnsrl.vv v4, v8, v12, v0.t
	th.vnsrl.vx v4, v8, a1, v0.t
	th.vnsrl.vi v4, v8, 1, v0.t
	th.vnsrl.vi v4, v8, 31, v0.t
	th.vnsra.vv v4, v8, v12
	th.vnsra.vx v4, v8, a1
	th.vnsra.vi v4, v8, 1
	th.vnsra.vi v4, v8, 31
	th.vnsra.vv v4, v8, v12, v0.t
	th.vnsra.vx v4, v8, a1, v0.t
	th.vnsra.vi v4, v8, 1, v0.t
	th.vnsra.vi v4, v8, 31, v0.t

	# Aliases
	th.vmsgt.vv v4, v8, v12
	th.vmsgtu.vv v4, v8, v12
	th.vmsge.vv v4, v8, v12
	th.vmsgeu.vv v4, v8, v12
	th.vmsgt.vv v4, v8, v12, v0.t
	th.vmsgtu.vv v4, v8, v12, v0.t
	th.vmsge.vv v4, v8, v12, v0.t
	th.vmsgeu.vv v4, v8, v12, v0.t
	th.vmslt.vi v4, v8, 16
	th.vmslt.vi v4, v8, -15
	th.vmsltu.vi v4, v8, 16
	th.vmsltu.vi v4, v8, -15
	th.vmsge.vi v4, v8, 16
	th.vmsge.vi v4, v8, -15
	th.vmsgeu.vi v4, v8, 16
	th.vmsgeu.vi v4, v8, -15
	th.vmslt.vi v4, v8, 16, v0.t
	th.vmslt.vi v4, v8, -15, v0.t
	th.vmsltu.vi v4, v8, 16, v0.t
	th.vmsltu.vi v4, v8, -15, v0.t
	th.vmsge.vi v4, v8, 16, v0.t
	th.vmsge.vi v4, v8, -15, v0.t
	th.vmsgeu.vi v4, v8, 16, v0.t
	th.vmsgeu.vi v4, v8, -15, v0.t

	# Macros
	th.vmsge.vx v4, v8, a1
	th.vmsgeu.vx v4, v8, a1
	th.vmsge.vx v8, v12, a2, v0.t
	th.vmsgeu.vx v8, v12, a2, v0.t
	th.vmsge.vx v4, v8, a1, v0.t, v12
	th.vmsgeu.vx v4, v8, a1, v0.t, v12

	th.vmseq.vv v4, v8, v12
	th.vmseq.vx v4, v8, a1
	th.vmseq.vi v4, v8, 15
	th.vmseq.vi v4, v8, -16
	th.vmseq.vv v4, v8, v12, v0.t
	th.vmseq.vx v4, v8, a1, v0.t
	th.vmseq.vi v4, v8, 15, v0.t
	th.vmseq.vi v4, v8, -16, v0.t
	th.vmsne.vv v4, v8, v12
	th.vmsne.vx v4, v8, a1
	th.vmsne.vi v4, v8, 15
	th.vmsne.vi v4, v8, -16
	th.vmsne.vv v4, v8, v12, v0.t
	th.vmsne.vx v4, v8, a1, v0.t
	th.vmsne.vi v4, v8, 15, v0.t
	th.vmsne.vi v4, v8, -16, v0.t
	th.vmsltu.vv v4, v8, v12
	th.vmsltu.vx v4, v8, a1
	th.vmsltu.vv v4, v8, v12, v0.t
	th.vmsltu.vx v4, v8, a1, v0.t
	th.vmslt.vv v4, v8, v12
	th.vmslt.vx v4, v8, a1
	th.vmslt.vv v4, v8, v12, v0.t
	th.vmslt.vx v4, v8, a1, v0.t
	th.vmsleu.vv v4, v8, v12
	th.vmsleu.vx v4, v8, a1
	th.vmsleu.vi v4, v8, 15
	th.vmsleu.vi v4, v8, -16
	th.vmsleu.vv v4, v8, v12, v0.t
	th.vmsleu.vx v4, v8, a1, v0.t
	th.vmsleu.vi v4, v8, 15, v0.t
	th.vmsleu.vi v4, v8, -16, v0.t
	th.vmsle.vv v4, v8, v12
	th.vmsle.vx v4, v8, a1
	th.vmsle.vi v4, v8, 15
	th.vmsle.vi v4, v8, -16
	th.vmsle.vv v4, v8, v12, v0.t
	th.vmsle.vx v4, v8, a1, v0.t
	th.vmsle.vi v4, v8, 15, v0.t
	th.vmsle.vi v4, v8, -16, v0.t
	th.vmsgtu.vx v4, v8, a1
	th.vmsgtu.vi v4, v8, 15
	th.vmsgtu.vi v4, v8, -16
	th.vmsgtu.vx v4, v8, a1, v0.t
	th.vmsgtu.vi v4, v8, 15, v0.t
	th.vmsgtu.vi v4, v8, -16, v0.t
	th.vmsgt.vx v4, v8, a1
	th.vmsgt.vi v4, v8, 15
	th.vmsgt.vi v4, v8, -16
	th.vmsgt.vx v4, v8, a1, v0.t
	th.vmsgt.vi v4, v8, 15, v0.t
	th.vmsgt.vi v4, v8, -16, v0.t

	th.vminu.vv v4, v8, v12
	th.vminu.vx v4, v8, a1
	th.vminu.vv v4, v8, v12, v0.t
	th.vminu.vx v4, v8, a1, v0.t
	th.vmin.vv v4, v8, v12
	th.vmin.vx v4, v8, a1
	th.vmin.vv v4, v8, v12, v0.t
	th.vmin.vx v4, v8, a1, v0.t
	th.vmaxu.vv v4, v8, v12
	th.vmaxu.vx v4, v8, a1
	th.vmaxu.vv v4, v8, v12, v0.t
	th.vmaxu.vx v4, v8, a1, v0.t
	th.vmax.vv v4, v8, v12
	th.vmax.vx v4, v8, a1
	th.vmax.vv v4, v8, v12, v0.t
	th.vmax.vx v4, v8, a1, v0.t

	th.vmul.vv v4, v8, v12
	th.vmul.vx v4, v8, a1
	th.vmul.vv v4, v8, v12, v0.t
	th.vmul.vx v4, v8, a1, v0.t
	th.vmulh.vv v4, v8, v12
	th.vmulh.vx v4, v8, a1
	th.vmulh.vv v4, v8, v12, v0.t
	th.vmulh.vx v4, v8, a1, v0.t
	th.vmulhu.vv v4, v8, v12
	th.vmulhu.vx v4, v8, a1
	th.vmulhu.vv v4, v8, v12, v0.t
	th.vmulhu.vx v4, v8, a1, v0.t
	th.vmulhsu.vv v4, v8, v12
	th.vmulhsu.vx v4, v8, a1
	th.vmulhsu.vv v4, v8, v12, v0.t
	th.vmulhsu.vx v4, v8, a1, v0.t

	th.vwmul.vv v4, v8, v12
	th.vwmul.vx v4, v8, a1
	th.vwmul.vv v4, v8, v12, v0.t
	th.vwmul.vx v4, v8, a1, v0.t
	th.vwmulu.vv v4, v8, v12
	th.vwmulu.vx v4, v8, a1
	th.vwmulu.vv v4, v8, v12, v0.t
	th.vwmulu.vx v4, v8, a1, v0.t
	th.vwmulsu.vv v4, v8, v12
	th.vwmulsu.vx v4, v8, a1
	th.vwmulsu.vv v4, v8, v12, v0.t
	th.vwmulsu.vx v4, v8, a1, v0.t

	th.vmacc.vv v4, v12, v8
	th.vmacc.vx v4, a1, v8
	th.vmacc.vv v4, v12, v8, v0.t
	th.vmacc.vx v4, a1, v8, v0.t
	th.vnmsac.vv v4, v12, v8
	th.vnmsac.vx v4, a1, v8
	th.vnmsac.vv v4, v12, v8, v0.t
	th.vnmsac.vx v4, a1, v8, v0.t
	th.vmadd.vv v4, v12, v8
	th.vmadd.vx v4, a1, v8
	th.vmadd.vv v4, v12, v8, v0.t
	th.vmadd.vx v4, a1, v8, v0.t
	th.vnmsub.vv v4, v12, v8
	th.vnmsub.vx v4, a1, v8
	th.vnmsub.vv v4, v12, v8, v0.t
	th.vnmsub.vx v4, a1, v8, v0.t

	th.vwmaccu.vv v4, v12, v8
	th.vwmaccu.vx v4, a1, v8
	th.vwmaccu.vv v4, v12, v8, v0.t
	th.vwmaccu.vx v4, a1, v8, v0.t
	th.vwmacc.vv v4, v12, v8
	th.vwmacc.vx v4, a1, v8
	th.vwmacc.vv v4, v12, v8, v0.t
	th.vwmacc.vx v4, a1, v8, v0.t
	th.vwmaccsu.vv v4, v12, v8
	th.vwmaccsu.vx v4, a1, v8
	th.vwmaccsu.vv v4, v12, v8, v0.t
	th.vwmaccsu.vx v4, a1, v8, v0.t
	th.vwmaccus.vx v4, a1, v8
	th.vwmaccus.vx v4, a1, v8, v0.t

	th.vdivu.vv v4, v8, v12
	th.vdivu.vx v4, v8, a1
	th.vdivu.vv v4, v8, v12, v0.t
	th.vdivu.vx v4, v8, a1, v0.t
	th.vdiv.vv v4, v8, v12
	th.vdiv.vx v4, v8, a1
	th.vdiv.vv v4, v8, v12, v0.t
	th.vdiv.vx v4, v8, a1, v0.t
	th.vremu.vv v4, v8, v12
	th.vremu.vx v4, v8, a1
	th.vremu.vv v4, v8, v12, v0.t
	th.vremu.vx v4, v8, a1, v0.t
	th.vrem.vv v4, v8, v12
	th.vrem.vx v4, v8, a1
	th.vrem.vv v4, v8, v12, v0.t
	th.vrem.vx v4, v8, a1, v0.t

	th.vmerge.vvm v4, v8, v12, v0
	th.vmerge.vxm v4, v8, a1, v0
	th.vmerge.vim v4, v8, 15, v0
	th.vmerge.vim v4, v8, -16, v0

	th.vmv.v.v v8, v12
	th.vmv.v.x v8, a1
	th.vmv.v.i v8, 15
	th.vmv.v.i v8, -16

	th.vsaddu.vv v4, v8, v12
	th.vsaddu.vx v4, v8, a1
	th.vsaddu.vi v4, v8, 15
	th.vsaddu.vi v4, v8, -16
	th.vsaddu.vv v4, v8, v12, v0.t
	th.vsaddu.vx v4, v8, a1, v0.t
	th.vsaddu.vi v4, v8, 15, v0.t
	th.vsaddu.vi v4, v8, -16, v0.t
	th.vsadd.vv v4, v8, v12
	th.vsadd.vx v4, v8, a1
	th.vsadd.vi v4, v8, 15
	th.vsadd.vi v4, v8, -16
	th.vsadd.vv v4, v8, v12, v0.t
	th.vsadd.vx v4, v8, a1, v0.t
	th.vsadd.vi v4, v8, 15, v0.t
	th.vsadd.vi v4, v8, -16, v0.t
	th.vssubu.vv v4, v8, v12
	th.vssubu.vx v4, v8, a1
	th.vssubu.vv v4, v8, v12, v0.t
	th.vssubu.vx v4, v8, a1, v0.t
	th.vssub.vv v4, v8, v12
	th.vssub.vx v4, v8, a1
	th.vssub.vv v4, v8, v12, v0.t
	th.vssub.vx v4, v8, a1, v0.t

	th.vaadd.vv v4, v8, v12
	th.vaadd.vx v4, v8, a1
	th.vaadd.vi v4, v8, 15
	th.vaadd.vi v4, v8, -16
	th.vaadd.vv v4, v8, v12, v0.t
	th.vaadd.vx v4, v8, a1, v0.t
	th.vaadd.vi v4, v8, 15, v0.t
	th.vaadd.vi v4, v8, -16, v0.t
	th.vasub.vv v4, v8, v12
	th.vasub.vx v4, v8, a1
	th.vasub.vv v4, v8, v12, v0.t
	th.vasub.vx v4, v8, a1, v0.t

	th.vsmul.vv v4, v8, v12
	th.vsmul.vx v4, v8, a1
	th.vsmul.vv v4, v8, v12, v0.t
	th.vsmul.vx v4, v8, a1, v0.t

	th.vwsmaccu.vv v4, v12, v8
	th.vwsmaccu.vx v4, a1, v8
	th.vwsmacc.vv v4, v12, v8
	th.vwsmacc.vx v4, a1, v8
	th.vwsmaccsu.vv v4, v12, v8
	th.vwsmaccsu.vx v4, a1, v8
	th.vwsmaccus.vx v4, a1, v8
	th.vwsmaccu.vv v4, v12, v8, v0.t
	th.vwsmaccu.vx v4, a1, v8, v0.t
	th.vwsmacc.vv v4, v12, v8, v0.t
	th.vwsmacc.vx v4, a1, v8, v0.t
	th.vwsmaccsu.vv v4, v12, v8, v0.t
	th.vwsmaccsu.vx v4, a1, v8, v0.t
	th.vwsmaccus.vx v4, a1, v8, v0.t

	th.vssrl.vv v4, v8, v12
	th.vssrl.vx v4, v8, a1
	th.vssrl.vi v4, v8, 1
	th.vssrl.vi v4, v8, 31
	th.vssrl.vv v4, v8, v12, v0.t
	th.vssrl.vx v4, v8, a1, v0.t
	th.vssrl.vi v4, v8, 1, v0.t
	th.vssrl.vi v4, v8, 31, v0.t
	th.vssra.vv v4, v8, v12
	th.vssra.vx v4, v8, a1
	th.vssra.vi v4, v8, 1
	th.vssra.vi v4, v8, 31
	th.vssra.vv v4, v8, v12, v0.t
	th.vssra.vx v4, v8, a1, v0.t
	th.vssra.vi v4, v8, 1, v0.t
	th.vssra.vi v4, v8, 31, v0.t

	th.vnclipu.vv v4, v8, v12
	th.vnclipu.vx v4, v8, a1
	th.vnclipu.vi v4, v8, 1
	th.vnclipu.vi v4, v8, 31
	th.vnclipu.vv v4, v8, v12, v0.t
	th.vnclipu.vx v4, v8, a1, v0.t
	th.vnclipu.vi v4, v8, 1, v0.t
	th.vnclipu.vi v4, v8, 31, v0.t
	th.vnclip.vv v4, v8, v12
	th.vnclip.vx v4, v8, a1
	th.vnclip.vi v4, v8, 1
	th.vnclip.vi v4, v8, 31
	th.vnclip.vv v4, v8, v12, v0.t
	th.vnclip.vx v4, v8, a1, v0.t
	th.vnclip.vi v4, v8, 1, v0.t
	th.vnclip.vi v4, v8, 31, v0.t

	th.vfadd.vv v4, v8, v12
	th.vfadd.vf v4, v8, fa2
	th.vfadd.vv v4, v8, v12, v0.t
	th.vfadd.vf v4, v8, fa2, v0.t
	th.vfsub.vv v4, v8, v12
	th.vfsub.vf v4, v8, fa2
	th.vfsub.vv v4, v8, v12, v0.t
	th.vfsub.vf v4, v8, fa2, v0.t
	th.vfrsub.vf v4, v8, fa2
	th.vfrsub.vf v4, v8, fa2, v0.t

	th.vfwadd.vv v4, v8, v12
	th.vfwadd.vf v4, v8, fa2
	th.vfwadd.vv v4, v8, v12, v0.t
	th.vfwadd.vf v4, v8, fa2, v0.t
	th.vfwsub.vv v4, v8, v12
	th.vfwsub.vf v4, v8, fa2
	th.vfwsub.vv v4, v8, v12, v0.t
	th.vfwsub.vf v4, v8, fa2, v0.t
	th.vfwadd.wv v4, v8, v12
	th.vfwadd.wf v4, v8, fa2
	th.vfwadd.wv v4, v8, v12, v0.t
	th.vfwadd.wf v4, v8, fa2, v0.t
	th.vfwsub.wv v4, v8, v12
	th.vfwsub.wf v4, v8, fa2
	th.vfwsub.wv v4, v8, v12, v0.t
	th.vfwsub.wf v4, v8, fa2, v0.t

	th.vfmul.vv v4, v8, v12
	th.vfmul.vf v4, v8, fa2
	th.vfmul.vv v4, v8, v12, v0.t
	th.vfmul.vf v4, v8, fa2, v0.t
	th.vfdiv.vv v4, v8, v12
	th.vfdiv.vf v4, v8, fa2
	th.vfdiv.vv v4, v8, v12, v0.t
	th.vfdiv.vf v4, v8, fa2, v0.t
	th.vfrdiv.vf v4, v8, fa2
	th.vfrdiv.vf v4, v8, fa2, v0.t

	th.vfwmul.vv v4, v8, v12
	th.vfwmul.vf v4, v8, fa2
	th.vfwmul.vv v4, v8, v12, v0.t
	th.vfwmul.vf v4, v8, fa2, v0.t

	th.vfmadd.vv v4, v12, v8
	th.vfmadd.vf v4, fa2, v8
	th.vfnmadd.vv v4, v12, v8
	th.vfnmadd.vf v4, fa2, v8
	th.vfmsub.vv v4, v12, v8
	th.vfmsub.vf v4, fa2, v8
	th.vfnmsub.vv v4, v12, v8
	th.vfnmsub.vf v4, fa2, v8
	th.vfmadd.vv v4, v12, v8, v0.t
	th.vfmadd.vf v4, fa2, v8, v0.t
	th.vfnmadd.vv v4, v12, v8, v0.t
	th.vfnmadd.vf v4, fa2, v8, v0.t
	th.vfmsub.vv v4, v12, v8, v0.t
	th.vfmsub.vf v4, fa2, v8, v0.t
	th.vfnmsub.vv v4, v12, v8, v0.t
	th.vfnmsub.vf v4, fa2, v8, v0.t
	th.vfmacc.vv v4, v12, v8
	th.vfmacc.vf v4, fa2, v8
	th.vfnmacc.vv v4, v12, v8
	th.vfnmacc.vf v4, fa2, v8
	th.vfmsac.vv v4, v12, v8
	th.vfmsac.vf v4, fa2, v8
	th.vfnmsac.vv v4, v12, v8
	th.vfnmsac.vf v4, fa2, v8
	th.vfmacc.vv v4, v12, v8, v0.t
	th.vfmacc.vf v4, fa2, v8, v0.t
	th.vfnmacc.vv v4, v12, v8, v0.t
	th.vfnmacc.vf v4, fa2, v8, v0.t
	th.vfmsac.vv v4, v12, v8, v0.t
	th.vfmsac.vf v4, fa2, v8, v0.t
	th.vfnmsac.vv v4, v12, v8, v0.t
	th.vfnmsac.vf v4, fa2, v8, v0.t

	th.vfwmacc.vv v4, v12, v8
	th.vfwmacc.vf v4, fa2, v8
	th.vfwnmacc.vv v4, v12, v8
	th.vfwnmacc.vf v4, fa2, v8
	th.vfwmsac.vv v4, v12, v8
	th.vfwmsac.vf v4, fa2, v8
	th.vfwnmsac.vv v4, v12, v8
	th.vfwnmsac.vf v4, fa2, v8
	th.vfwmacc.vv v4, v12, v8, v0.t
	th.vfwmacc.vf v4, fa2, v8, v0.t
	th.vfwnmacc.vv v4, v12, v8, v0.t
	th.vfwnmacc.vf v4, fa2, v8, v0.t
	th.vfwmsac.vv v4, v12, v8, v0.t
	th.vfwmsac.vf v4, fa2, v8, v0.t
	th.vfwnmsac.vv v4, v12, v8, v0.t
	th.vfwnmsac.vf v4, fa2, v8, v0.t

	th.vfsqrt.v v4, v8
	th.vfsqrt.v v4, v8, v0.t

	th.vfmin.vv v4, v8, v12
	th.vfmin.vf v4, v8, fa2
	th.vfmax.vv v4, v8, v12
	th.vfmax.vf v4, v8, fa2
	th.vfmin.vv v4, v8, v12, v0.t
	th.vfmin.vf v4, v8, fa2, v0.t
	th.vfmax.vv v4, v8, v12, v0.t
	th.vfmax.vf v4, v8, fa2, v0.t

        # Aliases
	th.vfneg.v v4, v8
	th.vfneg.v v4, v8, v0.t
	th.vfabs.v v4, v8
	th.vfabs.v v4, v8, v0.t

	th.vfsgnj.vv v4, v8, v12
	th.vfsgnj.vf v4, v8, fa2
	th.vfsgnjn.vv v4, v8, v12
	th.vfsgnjn.vf v4, v8, fa2
	th.vfsgnjx.vv v4, v8, v12
	th.vfsgnjx.vf v4, v8, fa2
	th.vfsgnj.vv v4, v8, v12, v0.t
	th.vfsgnj.vf v4, v8, fa2, v0.t
	th.vfsgnjn.vv v4, v8, v12, v0.t
	th.vfsgnjn.vf v4, v8, fa2, v0.t
	th.vfsgnjx.vv v4, v8, v12, v0.t
	th.vfsgnjx.vf v4, v8, fa2, v0.t

	# Aliases
	th.vmfgt.vv v4, v8, v12
	th.vmfge.vv v4, v8, v12
	th.vmfgt.vv v4, v8, v12, v0.t
	th.vmfge.vv v4, v8, v12, v0.t

	th.vmfeq.vv v4, v8, v12
	th.vmfeq.vf v4, v8, fa2
	th.vmfne.vv v4, v8, v12
	th.vmfne.vf v4, v8, fa2
	th.vmflt.vv v4, v8, v12
	th.vmflt.vf v4, v8, fa2
	th.vmfle.vv v4, v8, v12
	th.vmfle.vf v4, v8, fa2
	th.vmfgt.vf v4, v8, fa2
	th.vmfge.vf v4, v8, fa2
	th.vmfeq.vv v4, v8, v12, v0.t
	th.vmfeq.vf v4, v8, fa2, v0.t
	th.vmfne.vv v4, v8, v12, v0.t
	th.vmfne.vf v4, v8, fa2, v0.t
	th.vmflt.vv v4, v8, v12, v0.t
	th.vmflt.vf v4, v8, fa2, v0.t
	th.vmfle.vv v4, v8, v12, v0.t
	th.vmfle.vf v4, v8, fa2, v0.t
	th.vmfgt.vf v4, v8, fa2, v0.t
	th.vmfge.vf v4, v8, fa2, v0.t

	th.vmford.vv v4, v8, v12
	th.vmford.vf v4, v8, fa2
	th.vmford.vv v4, v8, v12, v0.t
	th.vmford.vf v4, v8, fa2, v0.t

	th.vfclass.v v4, v8
	th.vfclass.v v4, v8, v0.t

	th.vfmerge.vfm v4, v8, fa2, v0
	th.vfmv.v.f v4, fa1

	th.vfcvt.xu.f.v v4, v8
	th.vfcvt.x.f.v v4, v8
	th.vfcvt.f.xu.v v4, v8
	th.vfcvt.f.x.v v4, v8
	th.vfcvt.xu.f.v v4, v8, v0.t
	th.vfcvt.x.f.v v4, v8, v0.t
	th.vfcvt.f.xu.v v4, v8, v0.t
	th.vfcvt.f.x.v v4, v8, v0.t

	th.vfwcvt.xu.f.v v4, v8
	th.vfwcvt.x.f.v v4, v8
	th.vfwcvt.f.xu.v v4, v8
	th.vfwcvt.f.x.v v4, v8
	th.vfwcvt.f.f.v v4, v8
	th.vfwcvt.xu.f.v v4, v8, v0.t
	th.vfwcvt.x.f.v v4, v8, v0.t
	th.vfwcvt.f.xu.v v4, v8, v0.t
	th.vfwcvt.f.x.v v4, v8, v0.t
	th.vfwcvt.f.f.v v4, v8, v0.t

	th.vfncvt.xu.f.v v4, v8
	th.vfncvt.x.f.v v4, v8
	th.vfncvt.f.xu.v v4, v8
	th.vfncvt.f.x.v v4, v8
	th.vfncvt.f.f.v v4, v8
	th.vfncvt.xu.f.v v4, v8, v0.t
	th.vfncvt.x.f.v v4, v8, v0.t
	th.vfncvt.f.xu.v v4, v8, v0.t
	th.vfncvt.f.x.v v4, v8, v0.t
	th.vfncvt.f.f.v v4, v8, v0.t

	th.vredsum.vs v4, v8, v12
	th.vredmaxu.vs v4, v8, v8
	th.vredmax.vs v4, v8, v8
	th.vredminu.vs v4, v8, v8
	th.vredmin.vs v4, v8, v8
	th.vredand.vs v4, v8, v12
	th.vredor.vs v4, v8, v12
	th.vredxor.vs v4, v8, v12
	th.vredsum.vs v4, v8, v12, v0.t
	th.vredmaxu.vs v4, v8, v8, v0.t
	th.vredmax.vs v4, v8, v8, v0.t
	th.vredminu.vs v4, v8, v8, v0.t
	th.vredmin.vs v4, v8, v8, v0.t
	th.vredand.vs v4, v8, v12, v0.t
	th.vredor.vs v4, v8, v12, v0.t
	th.vredxor.vs v4, v8, v12, v0.t

	th.vwredsumu.vs v4, v8, v12
	th.vwredsum.vs v4, v8, v12
	th.vwredsumu.vs v4, v8, v12, v0.t
	th.vwredsum.vs v4, v8, v12, v0.t

	th.vfredosum.vs v4, v8, v12
	th.vfredsum.vs v4, v8, v12
	th.vfredmax.vs v4, v8, v12
	th.vfredmin.vs v4, v8, v12
	th.vfredosum.vs v4, v8, v12, v0.t
	th.vfredsum.vs v4, v8, v12, v0.t
	th.vfredmax.vs v4, v8, v12, v0.t
	th.vfredmin.vs v4, v8, v12, v0.t

	th.vfwredosum.vs v4, v8, v12
	th.vfwredsum.vs v4, v8, v12
	th.vfwredosum.vs v4, v8, v12, v0.t
	th.vfwredsum.vs v4, v8, v12, v0.t

	# Aliases
	th.vmmv.m v4, v8
	th.vmcpy.m v4, v8
	th.vmclr.m v4
	th.vmset.m v4
	th.vmnot.m v4, v8

	th.vmand.mm v4, v8, v12
	th.vmnand.mm v4, v8, v12
	th.vmandnot.mm v4, v8, v12
	th.vmxor.mm v4, v8, v12
	th.vmor.mm v4, v8, v12
	th.vmnor.mm v4, v8, v12
	th.vmornot.mm v4, v8, v12
	th.vmxnor.mm v4, v8, v12

	th.vmpopc.m a0, v12
	th.vmfirst.m a0, v12
	th.vmsbf.m v4, v8
	th.vmsif.m v4, v8
	th.vmsof.m v4, v8
	th.viota.m v4, v8
	th.vid.v v4
	th.vmpopc.m a0, v12, v0.t
	th.vmfirst.m a0, v12, v0.t
	th.vmsbf.m v4, v8, v0.t
	th.vmsif.m v4, v8, v0.t
	th.vmsof.m v4, v8, v0.t
	th.viota.m v4, v8, v0.t
	th.vid.v v4, v0.t

	# Alias
	th.vmv.x.s a0, v12

	th.vext.x.v a0, v12, a2
	th.vmv.s.x v4, a0

	th.vfmv.f.s fa0, v8
	th.vfmv.s.f v4, fa1

	th.vslideup.vx v4, v8, a1
	th.vslideup.vi v4, v8, 0
	th.vslideup.vi v4, v8, 31
	th.vslidedown.vx v4, v8, a1
	th.vslidedown.vi v4, v8, 0
	th.vslidedown.vi v4, v8, 31
	th.vslideup.vx v4, v8, a1, v0.t
	th.vslideup.vi v4, v8, 0, v0.t
	th.vslideup.vi v4, v8, 31, v0.t
	th.vslidedown.vx v4, v8, a1, v0.t
	th.vslidedown.vi v4, v8, 0, v0.t
	th.vslidedown.vi v4, v8, 31, v0.t

	th.vslide1up.vx v4, v8, a1
	th.vslide1down.vx v4, v8, a1
	th.vslide1up.vx v4, v8, a1, v0.t
	th.vslide1down.vx v4, v8, a1, v0.t

	th.vrgather.vv v4, v8, v12
	th.vrgather.vx v4, v8, a1
	th.vrgather.vi v4, v8, 0
	th.vrgather.vi v4, v8, 31
	th.vrgather.vv v4, v8, v12, v0.t
	th.vrgather.vx v4, v8, a1, v0.t
	th.vrgather.vi v4, v8, 0, v0.t
	th.vrgather.vi v4, v8, 31, v0.t

	th.vcompress.vm v4, v8, v12
