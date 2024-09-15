# mp1.s - Your solution goes here

        .section .data                    # Data section (optional for global data)
        .extern skyline_beacon             # Declare the external global variable

        .global skyline_star_cnd
        .type   skyline_star_cnd, @object

        .text
        .global start_beacon
        .type   start_beacon, @function

        .global add_star
        .type   add_star, @function

        .global remove_star
        .type   remove_star, @function

        .global draw_star
        .type   draw_star, @function

        .global add_window
        .type   add_window, @function

        .global remove_window
        .type   remove_window, @function

        .global draw_window
        .type   draw_window, @function

        .global draw_beacon
        .type   draw_beacon, @function


start_beacon:

        la t0, skyline_beacon             # Load address of skyline_beacon into t0 (t0 is 64-bit)

        # Store the function arguments into the struct fields
        sd a0, 0(t0)                      # Store img (a0, 64-bit) at offset 0 (8 bytes)

        sh a1, 8(t0)                      # Store x (a1, 16-bit) at offset 8 (after img pointer)

        sh a2, 10(t0)                     # Store y (a2, 16-bit) at offset 10

        sb a3, 12(t0)                     # Store dia (a3, 8-bit) at offset 12

        sh a4, 14(t0)                     # Store period (a4, 16-bit) at offset 14

        sh a5, 16(t0)                     # Store ontime (a5, 16-bit) at offset 16

        ret                               # Return to caller

        .end








add_star:

        # Load the address of the global count of stars into t0
        la t0, skyline_star_cnt
        
        # Load the current count of stars into t1
        lw t1, 0(t0)
        
        # Check if there is room for another star
        li t2, SKYLINE_STARS_MAX   # Load the max number of stars into t2
        bge t1, t2, end_function   # If current count >= max, branch to end

        # Calculate the address of the new star in the array
        la t3, skyline_stars       # Load the base address of the stars array
        slli t4, t1, 3             
        add t3, t3, t4             # Add the offset to the base address
        
        # Store the x, y, and color values at the calculated address
        sh a0, 0(t3)               # Store x at the current index
        sh a1, 2(t3)               # Store y 2 bytes after x
        sh a2, 6(t3)               # Store color 4 bytes after y

        # Increment the star count and store it back
        addi t1, t1, 1             # Increment the current count of stars
        sw t1, 0(t0)               # Store the new count back to the global variable

end_function:
        ret                        # Return from the function

        .end










remove_star:

        # Load base address of the stars array and star count
        la t0, skyline_stars       # Load the address of the skyline_stars array
        la t1, skyline_star_cnt    # Load the address of the skyline_star_cnt
        lw t2, 0(t1)               # Load the current star count into t2

        # Initialize loop variables
        li t3, 0                   # t3 will keep the current index

loop_start:
        # Check if index is less than star count
        bge t3, t2, end_function  # If index >= star count, exit loop

        # Calculate address of current star in array
        slli t4, t3, 3             # t3 * 8 (assuming each star takes 8 bytes, adjust as needed)
        add t4, t0, t4             # t0 + offset -> address of current star

        # Load current star's x and y coordinates
        lh t5, 0(t4)               # Load x of current star
        lh t6, 2(t4)               # Load y of current star

        # Check if current star is the one to remove
        bne t5, a0, continue_loop # If x is not equal, continue loop
        bne t6, a1, continue_loop # If y is not equal, continue loop

        # If match is found, shift subsequent stars
shift_array:
        beq t3, t2, update_count   # If this is the last star, skip shifting
        addi t5, t4, 8             # Address of the next star

        # Start shifting stars forward by one position
shift_loop:
        lw t6, 0(t5)               # Load word from next star position
        sw t6, -8(t5)              # Store word into current star position
        addi t5, t5, 8             # Move to next star
        addi t3, t3, 1             # Increment loop index
        blt t3, t2, shift_loop     # Continue shifting if not at end of array

update_count:
        # Decrement star count
        addi t2, t2, -1            # t2 - 1
        sw t2, 0(t1)               # Store updated count
        
        # Return from function
        ret

continue_loop:
        addi t3, t3, 1             # Increment index
        j loop_start               # Jump back to start of loop

end_function:
        ret

        .end










draw_star:

        # Load the base address of the framebuffer
        mv t0, a0                  # t0 = framebuffer pointer (fbuf)

        # Load the x and y coordinates of the star from the star structure
        lh t1, 0(a1)               # t1 = x
        lh t2, 2(a1)               # t2 = y
        lb t3, 4(a1)               # t3 = dia
        lh t4, 6(a1)               # t4 = color

        # Calculate the memory offset within the framebuffer
        li t5, SKYLINE_WIDTH       # t5 = SKYLINE_WIDTH
        mul t6, t2, t5             # t6 = y * SKYLINE_WIDTH
        add t6, t6, t1             # t6 = y * SKYLINE_WIDTH + x





        # Calculate the memory offset within the framebuffer
        li t4, 640                 # t4 = SKYLINE_WIDTH
        mul t5, t2, t4             # t5 = y * SKYLINE_WIDTH
        add t5, t5, t1             # t5 = y * SKYLINE_WIDTH + x
        slli t5, t5, 1             # t5 = (y * SKYLINE_WIDTH + x) * 2

        # Draw the star at the calculated position in the framebuffer
        add t6, t0, t5             # t6 = framebuffer address + offset
        sh t3, 0(t6)               # Store the star's color at the calculated framebuffer address

        ret                         # Return from the function

        .end










add_window:

        # Step 1: Allocate memory for the new window node using `brk`
        li a7, 214                # Syscall number for brk in RISC-V 64
        mv a0, zero               # Set a0 to 0 to get the current end of the data segment
        ecall                     # System call to get current program break
        
        # After ecall, a0 has the current break address; store it in t0
        mv t0, a0                 # Store the current end of the data segment in t0
        
        # Increase the break by 16 bytes for new window structure allocation
        addi a0, t0, 16           # Increment by 16 bytes for new window structure
        li a7, 214                # Syscall number for brk again
        ecall                     # Adjust program break to allocate new memory
        
        # Check for allocation failure (if a0 returns -1, allocation failed)
        bgez a0, alloc_success    # If a0 >= 0, continue
        j alloc_fail              # If a0 < 0, jump to allocation failure handling

alloc_success:
        # Now, t0 contains the starting address of the newly allocated window structure
        # Step 2: Populate the new window structure
        sw zero, 0(t0)            # Set next pointer to NULL (0)
        sh a0, 8(t0)              # Store x-coordinate at offset 8
        sh a1, 10(t0)             # Store y-coordinate at offset 10
        sb a2, 12(t0)             # Store width at offset 12
        sb a3, 13(t0)             # Store height at offset 13
        sh a4, 14(t0)             # Store color at offset 14
        
        # Step 3: Add to the window list
        la t1, skyline_win_list   # Load the address of skyline_win_list
        lw t2, 0(t1)              # Load the current head of the window list into t2
        
        # Check if the list is empty
        beqz t2, insert_new_head  # If t2 (skyline_win_list) is NULL, insert as new head

        # Traverse the list to find the last node
traverse_list:
        lw t3, 0(t2)              # Load the next pointer of the current node
        beqz t3, end_of_list      # If the next pointer is NULL, we found the end
        mv t2, t3                 # Move to the next node
        j traverse_list           # Continue traversing

end_of_list:
        # Link the new window to the end of the list
        sw t0, 0(t2)              # Set the next pointer of the last node to the new node
        j add_window_end          # Jump to end of function

insert_new_head:
        # Insert the new window as the head of the list
        sw t0, 0(t1)              # Set skyline_win_list to the new window node

add_window_end:
        ret                       # Return from function

alloc_fail:
        # Allocation failed handling
        li a0, -1                 # Return -1 to indicate failure
        ret

        .end











remove_window:

        # Step 1: Load the window list head pointer
        la t0, skyline_win_list    # Load the address of skyline_win_list
        lw t1, 0(t0)               # Load the current head of the window list into t1
        
        # Check if the list is empty
        beqz t1, remove_window_end # If t1 is NULL (skyline_win_list), list is empty, return

        # Step 2: Initialize pointers for traversal
        mv t2, zero                # t2 will be the previous window pointer (initially NULL)
        mv t3, t1                  # t3 will be the current window pointer (starting from head)

# Traverse the linked list to find the matching window
find_window:
        # Load the x and y coordinates of the current window
        lh t4, 8(t3)               # Load x of the current window
        lh t5, 10(t3)              # Load y of the current window
        
        # Check if this is the window to remove
        bne t4, a0, next_window    # If x is not equal, go to the next window
        bne t5, a1, next_window    # If y is not equal, go to the next window

        # Found the window to remove
        # Step 3: Remove the window from the list
        beqz t2, remove_head       # If t2 is NULL, we are removing the head

        # Remove the window from the middle or end
        lw t6, 0(t3)               # Load the next pointer of the current window
        sw t6, 0(t2)               # Set the next pointer of the previous window to t6 (skip the current window)
        j remove_window_end        # Jump to end

remove_head:
        # Removing the head of the list
        lw t6, 0(t3)               # Load the next pointer of the current window
        sw t6, 0(t0)               # Update skyline_win_list to point to the new head (next window)

remove_window_end:
        ret                        # Return from function

next_window:
        # Move to the next window in the list
        mv t2, t3                  # Update the previous window pointer to current
        lw t3, 0(t3)               # Move to the next window
        bnez t3, find_window       # If t3 is not NULL, continue searching

        # End of function
        ret                        # Return if window is not found

        .end










draw_window:











draw_beacon: