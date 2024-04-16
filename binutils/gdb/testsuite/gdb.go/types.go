package main

import "fmt"

// Self-referential type.
type T *T

// Mutually recursive types.
type T1 *T2
type T2 *T1

// Mutually recursive struct types.
type S1 struct { p_s2 *S2 }
type S2 struct { p_s1 *S1 }

func main () {
  fmt.Println ("Shall we?")
  var t T
  fmt.Println (t)
  var s1 S1
  var s2 S2
  fmt.Println (s1)
  fmt.Println (s2)
}
