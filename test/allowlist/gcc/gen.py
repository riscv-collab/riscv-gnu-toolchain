from tqdm import tqdm
import re

def processlog(filename: str) -> set:
    with open(filename, "r", encoding='utf-8') as f:
        data = f.readlines()
    set1 = set()
    for line in tqdm(data):
        result = re.match('^.*((FAIL|UNRESOLVED): .*.c) .*$',line)     #使用正则表达式筛选每一行的数据,自行查找正则表达式
        if result:
            t = (result.group(1))                        #group(1)将正则表达式的(/d.*/d)提取出来
            set1.add(t)
    return set1

def writeset(filename: str, set1: set) -> None:
    if len(set1)!=0:
        with open(filename, "a", encoding='utf-8') as f1:   
            for t in tqdm(set1):
                f1.write(t+'\n')

newlib64=processlog("10_build (ubuntu-24.04, newlib, rv64gc-lp64d, gcc).txt")
newlib32=processlog("25_build (ubuntu-24.04, newlib, rv32gc-ilp32d, gcc).txt")
linux64=processlog("24_build (ubuntu-24.04, linux, rv64gc-lp64d, gcc).txt")
linux32=processlog("15_build (ubuntu-24.04, linux, rv32gc-ilp32d, gcc).txt")

# common should be 4 intersection
commonerror=set.intersection(newlib64, newlib32, linux64, linux32)
writeset("res/common.log",commonerror)
# rv32
rv32=set.intersection(newlib32, linux32)
rv32=rv32-commonerror
writeset("res/rv32.log",rv32)
# rv64
rv64=set.intersection(newlib64, linux64)
rv64=rv64-commonerror
writeset("res/rv64.log",rv64)
# glibc
glibc=set.intersection(linux32, linux64)
glibc=glibc-commonerror
writeset("res/glibc.log",glibc)
# newlib
newlib=set.intersection(newlib64, newlib32)
newlib=newlib-commonerror
writeset("res/newlib.log",newlib)

# everything out of it
writeset("res/glibc.rv32.log",linux32-rv32-glibc-commonerror)
writeset("res/glibc.rv64.log",linux64-rv64-glibc-commonerror)
writeset("res/newlib.rv32.log",newlib32-rv32-newlib-commonerror)
writeset("res/newlib.rv64.log",newlib64-rv64-newlib-commonerror)
