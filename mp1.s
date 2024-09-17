# mp1.s - Your solution goes here

        .section .data                    # Data section (optional for global data)
        .extern skyline_beacon             # Declare the external global variable


        .extern skyline_star_cnt


        .global skyline_star_cnd
        .type   skyline_star_cnd, @object

        .extern skyline_win_list

        .extern skyline_stars
        
        
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



        .global link
        .type link, @function


        .equ SKYLINE_WIDTH, 640
        .equ SKYLINE_HEIGHT, 480
        .equ SKYLINE_STARS_MAX, 1000


# load boundries into temp regs
link:
        li s1, SKYLINE_WIDTH
        li s2, SKYLINE_HEIGHT
        li s3, SKYLINE_STARS_MAX

        ret




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











remove_star:

        # Load base address of the stars array and star count
        la t0, skyline_stars       # Load the address of the skyline_stars array
        la t1, skyline_star_cnt    # Load the address of the skyline_star_cnt
        lw t2, 0(t1)               # Load the current star count into t2

        # Initialize loop variables
        li t3, 0                   # t3 will keep the current index

loop_start:
        # Check if index is less than star count
        bge t3, t2, E  # If index >= star count, exit loop

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

E:
        ret













draw_star:


        mv t0, ra                # Save the current return address in t0
        jal ra, link             # Jump to `link` and save the return address in `ra`
        mv ra, t0                # Restore the original return address from t0

        # Load the base address of the framebuffer
        mv t0, a0                  # t0 = framebuffer pointer (fbuf)

        # Load the x and y coordinates of the star from the star structure
        lh t1, 0(a1)               # t1 = x
        lh t2, 2(a1)               # t2 = y
        lb t3, 4(a1)               # t3 = dia
        lh t4, 6(a1)               # t4 = color

        # Calculate the memory offset within the framebuffer
        mul t6, t2, s1             # t6 = y * SKYLINE_WIDTH
        add t6, t6, t1             # t6 = y * SKYLINE_WIDTH + x
        slli t6, t6, 1             # t6 = (y * SKYLINE_WIDTH + x) * 2

        # Draw the star at the calculated position in the framebuffer
        add t6, t0, t6             # t6 = framebuffer address + offset
        sh t4, 0(t6)               # Store the star's color at the calculated framebuffer address

        ret                        # Return from the function
















add_window:
        # Prologue: Adjust the stack and save registers
        addi sp, sp, -48            # Allocate 48 bytes on the stack (multiple of 16 for alignment)
        sd ra, 40(sp)               # Save return address (ra is 64 bits)
        sd s0, 32(sp)               # Save s0 (used for x)
        sd a1, 24(sp)               # Save a1 (y)
        sd a2, 16(sp)               # Save a2 (w)
        sd a3, 8(sp)                # Save a3 (h)
        sd a4, 0(sp)                # Save a4 (color)

        # Save a0 (x coordinate) in s0
        mv s0, a0                   # s0 = a0 (x)

        # Allocate memory for a new node
        li a0, 16                   # a0 = size of skyline_window (16 bytes)
        call malloc                 # Allocate memory
        beqz a0, end_add_window     # If malloc failed, exit function

        mv t0, a0                   # t0 = pointer to new node

        # Restore arguments from the stack
        ld a1, 24(sp)               # Restore a1 (y)
        ld a2, 16(sp)               # Restore a2 (w)
        ld a3, 8(sp)                # Restore a3 (h)
        ld a4, 0(sp)                # Restore a4 (color)

        # Initialize next pointer to NULL
        sd zero, 0(t0)              # *(t0 + 0) = NULL

        # Store the window data into the new node
        sh s0, 8(t0)                # x coordinate at offset 8
        sh a1, 10(t0)               # y coordinate at offset 10
        sb a2, 12(t0)               # width (w) at offset 12
        sb a3, 13(t0)               # height (h) at offset 13
        sh a4, 14(t0)               # color at offset 14

        # Load the head of the window list
        la t1, skyline_win_list     # t1 = address of skyline_win_list
        ld t2, 0(t1)                # t2 = *skyline_win_list
        beqz t2, insert_first_node  # If list is empty, insert at head

find_last_node:
        ld t3, 0(t2)                # t3 = t2->next
        beqz t3, insert_after_last_node
        mv t2, t3                   # Move to next node
        j find_last_node            # Repeat loop

insert_after_last_node:
        sd t0, 0(t2)                # t2->next = t0 (new node)
        j end_add_window            # Jump to function epilogue

insert_first_node:
        sd t0, 0(t1)                # *skyline_win_list = t0 (new node)

end_add_window:
        # Epilogue: Restore registers and stack pointer
        ld a4, 0(sp)                # Restore a4 (color)
        ld a3, 8(sp)                # Restore a3 (h)
        ld a2, 16(sp)               # Restore a2 (w)
        ld a1, 24(sp)               # Restore a1 (y)
        ld s0, 32(sp)               # Restore s0
        ld ra, 40(sp)               # Restore return address
        addi sp, sp, 48             # Deallocate stack space
        ret                         # Return from function


























remove_window:

        addi sp, sp, -16            # Allocate 16 bytes on the stack
        sd ra, 8(sp)                # Save return address
        sd s0, 0(sp)                # Save s0 if you use it (not used in this code)

        # Step 1: Load the window list head pointer
        la t0, skyline_win_list    # Load the address of skyline_win_list
        ld t1, 0(t0)               # Load the current head of the window list into t1
        
        # Check if the list is empty
        beqz t1, remove_window_end # If t1 is NULL (skyline_win_list), list is empty, return

        # Step 2: Initialize pointers for traversal
        mv t2, zero                # t2 will be the previous window pointer (initially NULL)
        mv t3, t1                  # t3 will be the current window pointer (starting from head)

# Traverse the linked list to find the matching window
find_window:

        beqz t3, remove_window_end

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
        ld t6, 0(t3)               # Load the next pointer of the current window
        sd t6, 0(t2)               # Set the next pointer of the previous window to t6 (skip the current window)

        mv a0, t3
        call free

        j remove_window_end        # Jump to end

remove_head:
        # Removing the head of the list
        ld t6, 0(t3)               # Load the next pointer of the current window
        sd t6, 0(t0)               # Update skyline_win_list to point to the new head (next window)

        mv a0, t3
        call free


remove_window_end:

        ld s0, 0(sp)                # Restore s0
        ld ra, 8(sp)                # Restore return address
        addi sp, sp, 16             # Deallocate stack space

        ret                        # Return from function

next_window:
        # Move to the next window in the list
        mv t2, t3                  # Update the previous window pointer to current
        ld t3, 0(t3)               # Move to the next window
        j find_window              # If t3 is not NULL, continue searching

        ld s0, 0(sp)                # Restore s0
        ld ra, 8(sp)                # Restore return address
        addi sp, sp, 16             # Deallocate stack space

        # End of function
        ret                        # Return if window is not found
















draw_window:

        mv t0, ra                # Save the current return address in t0
        jal ra, link             # Jump to `link` and save the return address in `ra`
        mv ra, t0                # Restore the original return address from t0

        # initilize row counter
        li t0, 0

draw_window_row:

        lh t2, 10(a1)             # t2 = y (Load y from window structure)
        add t2, t2, t0            # t2 = y + i
        bge t2, s2, skip_row      # If y + i >= SKYLINE_HEIGHT, skip drawing

        # Calculate the base offset for the start of the row in the framebuffer
        mul t2, t2, s1            # t2 = (y + i) * SKYLINE_WIDTH
        lh t3, 8(a1)              # t3 = x (Load x from window structure)
        add t2, t2, t3            # t2 = (y + i) * SKYLINE_WIDTH + x
        slli t2, t2, 1            # t2 = ((y + i) * SKYLINE_WIDTH + x) * 2 (byte offset)
        
        add t2, a0, t2            # t2 = framebuffer + offset (start of the row in framebuffer)

        # Draw the row of pixels for the width (w)
        li t1, 0                  # t1 for column counter j (initialize j = 0)

draw_window_column:

        lh t3, 8(a1)              # t3 = x (Load x from window structure)
        add t3, t3, t1            # t3 = x + j
        bge t3, s1, skip_column   # If x + j >= SKYLINE_WIDTH, skip the pixel

        # Draw the pixel
        lh t4, 14(a1)             # t4 = color (Load color from window structure)
        sh t4, 0(t2)              # Store color at the current framebuffer position
        addi t2, t2, 2            # Move to the next pixel (2 bytes per pixel)
        addi t1, t1, 1            # Increment column counter j
        lbu t4, 12(a1)            # t4 = w (Load width from window structure)
        blt t1, t4, draw_window_column # Repeat until j < width (w)


skip_column:
        # Move to the next row
        addi t0, t0, 1            # Increment row counter i
        lbu t3, 13(a1)            # t3 = h (Load height from window structure)
        blt t0, t3, draw_window_row  # Repeat until i < height (h)


skip_row:
        ret                       # Return from function












draw_beacon:









.end
