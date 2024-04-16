package main

var i = 0
var j = 0
var k = 0
var l = 0

func main () {
  i = 0
  j = 0
  k = 0
  l = 0 // set breakpoint 1 here
  i = 1
  j = 2
  k = 3
  l = k

  i = j + k

  j = 0 // set breakpoint 2 here
  k = 0
}
