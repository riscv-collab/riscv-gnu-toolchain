.*: Assembler messages:
.*:4: Error: `movbe' is not supported on `x86_64.nomovbe'
.*:5: Error: `movbe' is not supported on `x86_64.nomovbe'
.*:8: Error: `invept' is not supported on `x86_64.noept'
.*:9: Error: `invept' is not supported on `x86_64.noept'
.*:12: Error: `kmovq' is not supported on `x86_64.noavx512bw'
.*:13: Error: `kmovq' is not supported on `x86_64.noavx512bw'
.*:16: Error: `kmovb' is not supported on `x86_64.noavx512dq'
.*:17: Error: `kmovb' is not supported on `x86_64.noavx512dq'
.*:20: Error: `kmovw' is not supported on `x86_64.noavx512f'
.*:21: Error: `kmovw' is not supported on `x86_64.noavx512f'
.*:24: Error: `andn' is not supported on `x86_64.nobmi'
.*:25: Error: `andn' is not supported on `x86_64.nobmi'
.*:28: Error: `bzhi' is not supported on `x86_64.nobmi2'
.*:29: Error: `bzhi' is not supported on `x86_64.nobmi2'
.*:33: Error: .*`andn'.*
.*:34: Error: .*`bzhi'.*
.*:35: Error: .*`kmovw'.*
.*:36: Error: .*`kmovq'.*
.*:37: Error: .*`kmovb'.*
.*:38: Error: .*`ldtilecfg'.*
.*:39: Error: .*`cmpexadd'.*
GAS LISTING .*
#...
[ 	]*1[ 	]+\# Check illegal 64bit APX EVEX promoted instructions
[ 	]*2[ 	]+\.text
#pass
