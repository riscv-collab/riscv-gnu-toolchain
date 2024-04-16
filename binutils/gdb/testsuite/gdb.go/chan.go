package main

import "fmt"

func generate() chan int {
    ch := make(chan int)
    go func() {
        for i := 0; ; i++ {
            ch <- i // set breakpoint 1 here
        }
    }()
    return ch
}

func main() {
    integers := generate()
    for i := 0; i < 100; i++ { // Print the first hundred integers.
        fmt.Println(<-integers) // set breakpoint 2 here
    }
}
