#as: -march=rv32if_xtheadvector_xtheadzvamo
#objdump: -dr

.*:[ 	]+file format .*


Disassembly of section .text:

0+000 <.text>:
[ 	]+[0-9a-f]+:[ 	]+0685e22f[ 	]+th.vamoaddw.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+0285e22f[ 	]+th.vamoaddw.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+0685f22f[ 	]+th.vamoaddd.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+0285f22f[ 	]+th.vamoaddd.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+0485e22f[ 	]+th.vamoaddw.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+0085e22f[ 	]+th.vamoaddw.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+0485f22f[ 	]+th.vamoaddd.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+0085f22f[ 	]+th.vamoaddd.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+0e85e22f[ 	]+th.vamoswapw.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+0a85e22f[ 	]+th.vamoswapw.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+0e85f22f[ 	]+th.vamoswapd.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+0a85f22f[ 	]+th.vamoswapd.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+0c85e22f[ 	]+th.vamoswapw.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+0885e22f[ 	]+th.vamoswapw.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+0c85f22f[ 	]+th.vamoswapd.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+0885f22f[ 	]+th.vamoswapd.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+2685e22f[ 	]+th.vamoxorw.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+2285e22f[ 	]+th.vamoxorw.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+2685f22f[ 	]+th.vamoxord.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+2285f22f[ 	]+th.vamoxord.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+2485e22f[ 	]+th.vamoxorw.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+2085e22f[ 	]+th.vamoxorw.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+2485f22f[ 	]+th.vamoxord.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+2085f22f[ 	]+th.vamoxord.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+6685e22f[ 	]+th.vamoandw.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+6285e22f[ 	]+th.vamoandw.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+6685f22f[ 	]+th.vamoandd.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+6285f22f[ 	]+th.vamoandd.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+6485e22f[ 	]+th.vamoandw.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+6085e22f[ 	]+th.vamoandw.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+6485f22f[ 	]+th.vamoandd.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+6085f22f[ 	]+th.vamoandd.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+4685e22f[ 	]+th.vamoorw.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+4285e22f[ 	]+th.vamoorw.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+4685f22f[ 	]+th.vamoord.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+4285f22f[ 	]+th.vamoord.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+4485e22f[ 	]+th.vamoorw.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+4085e22f[ 	]+th.vamoorw.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+4485f22f[ 	]+th.vamoord.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+4085f22f[ 	]+th.vamoord.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+8685e22f[ 	]+th.vamominw.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+8285e22f[ 	]+th.vamominw.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+8685f22f[ 	]+th.vamomind.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+8285f22f[ 	]+th.vamomind.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+8485e22f[ 	]+th.vamominw.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+8085e22f[ 	]+th.vamominw.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+8485f22f[ 	]+th.vamomind.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+8085f22f[ 	]+th.vamomind.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+a685e22f[ 	]+th.vamomaxw.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+a285e22f[ 	]+th.vamomaxw.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+a685f22f[ 	]+th.vamomaxd.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+a285f22f[ 	]+th.vamomaxd.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+a485e22f[ 	]+th.vamomaxw.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+a085e22f[ 	]+th.vamomaxw.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+a485f22f[ 	]+th.vamomaxd.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+a085f22f[ 	]+th.vamomaxd.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+c685e22f[ 	]+th.vamominuw.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+c285e22f[ 	]+th.vamominuw.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+c685f22f[ 	]+th.vamominud.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+c285f22f[ 	]+th.vamominud.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+c485e22f[ 	]+th.vamominuw.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+c085e22f[ 	]+th.vamominuw.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+c485f22f[ 	]+th.vamominud.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+c085f22f[ 	]+th.vamominud.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+e685e22f[ 	]+th.vamomaxuw.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+e285e22f[ 	]+th.vamomaxuw.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+e685f22f[ 	]+th.vamomaxud.v[ 	]+v4,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+e285f22f[ 	]+th.vamomaxud.v[ 	]+zero,v8,\(a1\),v4
[ 	]+[0-9a-f]+:[ 	]+e485e22f[ 	]+th.vamomaxuw.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+e085e22f[ 	]+th.vamomaxuw.v[ 	]+zero,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+e485f22f[ 	]+th.vamomaxud.v[ 	]+v4,v8,\(a1\),v4,v0.t
[ 	]+[0-9a-f]+:[ 	]+e085f22f[ 	]+th.vamomaxud.v[ 	]+zero,v8,\(a1\),v4,v0.t
