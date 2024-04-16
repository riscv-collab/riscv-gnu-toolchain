package main

import "fmt"

var st = "We shall"

func main () {
  fmt.Println ("Before assignment")
  st := "Hello, world!" // this intentionally shadows the global "st"
  fmt.Println (st) // set breakpoint 1 here
}
