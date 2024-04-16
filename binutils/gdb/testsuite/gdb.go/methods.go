package main

import "fmt"

type T struct { i int }

func (t T) Foo () {
  fmt.Println (t.i)
}

func (t *T) Bar () {
  fmt.Println (t.i)
}

func main () {
  fmt.Println ("Shall we?")
  var t T
  t.Foo ()
  var pt = new (T)
  pt.Bar ()
}
