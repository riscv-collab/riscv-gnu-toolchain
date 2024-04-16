character(kind=1) :: x
character(kind=4) :: c
character(kind=4,len=5) :: str
x = 'j'
c = 4_'k'
str = 4_'lmnop'
! break-here
print *, c
print *, str
end
