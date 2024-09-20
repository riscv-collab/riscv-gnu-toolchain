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


# Function: link
# Purpose: Initializes skyline properties such as width, height, and maximum stars.
# Registers:
#   s1 - SKYLINE_WIDTH
#   s2 - SKYLINE_HEIGHT
#   s3 - SKYLINE_STARS_MAX
link:
        li s1, SKYLINE_WIDTH            # Load SKYLINE_WIDTH (640) into s1 (framebuffer width)
        li s2, SKYLINE_HEIGHT           # Load SKYLINE_HEIGHT (480) into s2
        li s3, SKYLINE_STARS_MAX        # Load SKYLINE_STARS_MAX (100) into s3

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








# Function: add_star
# Purpose: Adds a new star with specified x, y coordinates and color to the skyline_stars array.
# Parameters:
#   a0 - x-coordinate of the new star (uint16_t x)
#   a1 - y-coordinate of the new star (uint16_t y)
#   a2 - color/intensity of the new star (uint16_t color)

add_star:

        # Load the address of skyline_star_cnt into register t0
        la t0, skyline_star_cnt                 # t0 = &skyline_star_cnt
        
        # Load the current count of stars from skyline_star_cnt into register t1
        lw t1, 0(t0)                            # t1 = skyline_star_cnt
        
        # Load the maximum number of stars allowed into register t2
        li t2, SKYLINE_STARS_MAX                # t2 = SKYLINE_STARS_MAX
        
        # Compare current star count (t1) with maximum allowed (t2)
        # If current count >= max, branch to end_function to prevent overflow
        bge t1, t2, end_function                
        
        # Load the base address of the skyline_stars array into register t3
        la t3, skyline_stars                    # t3 = &skyline_stars[0]
        
        # Calculate the byte offset for the new star's position
        # Assuming each star occupies 8 bytes, multiply the current count by 8
        slli t4, t1, 3                           # t4 = t1 * 8 (shift left by 3 bits)
        
        # Add the offset to the base address to get the address of the new star
        add t3, t3, t4                           # t3 = &skyline_stars[t1]
        
        # Store the x-coordinate of the new star at offset 0 of the new star's address
        sh a0, 0(t3)                             # skyline_stars[t1].x = a0
        
        # Store the y-coordinate of the new star at offset 2 of the new star's address
        sh a1, 2(t3)                             # skyline_stars[t1].y = a1
        
        # Store the color/intensity of the new star at offset 6 of the new star's address
        sh a2, 6(t3)                             # skyline_stars[t1].color = a2
        
        # Increment the current star count by 1
        addi t1, t1, 1                           # t1 = t1 + 1
        
        # Store the updated star count back to skyline_star_cnt
        sw t1, 0(t0)                             # skyline_star_cnt = t1

end_function:

        # Return from the add_star function
        ret









# Function: remove_star
# Purpose: Removes a star with specified x and y coordinates from the skyline_stars array.
# Parameters:
#   a0 - x-coordinate of the star to remove (uint16_t x)
#   a1 - y-coordinate of the star to remove (uint16_t y)

remove_star:

        # Load the base address of the skyline_stars array into register t0
        la t0, skyline_stars       # t0 = &skyline_stars[0]
        
        # Load the address of skyline_star_cnt (star count) into register t1
        la t1, skyline_star_cnt    # t1 = &skyline_star_cnt
        
        # Load the current star count from skyline_star_cnt into register t2
        lw t2, 0(t1)               # t2 = skyline_star_cnt
        
        # Initialize loop counter t3 to 0 (starting index)
        li t3, 0                   # t3 = 0
    
loop_start:
    
        # Compare loop counter t3 with star count t2
        # If t3 >= t2, all stars have been processed; branch to remove_end
        bge t3, t2, remove_end     # if (t3 >= t2) goto remove_end
        
        # Calculate the byte offset for the current star
        # Each star occupies 8 bytes (assuming structure size)
        slli t4, t3, 3             # t4 = t3 * 8 (shift left by 3 bits)
        
        # Calculate the address of the current star
        add t4, t0, t4             # t4 = &skyline_stars[t3]
        
        # Load the x-coordinate of the current star
        lh t5, 0(t4)               # t5 = skyline_stars[t3].x
        
        # Load the y-coordinate of the current star
        lh t6, 2(t4)               # t6 = skyline_stars[t3].y
        
        # Compare the loaded x-coordinate with the target x (a0)
        # If not equal, skip to continue_loop
        bne t5, a0, continue_loop  # if (t5 != a0) goto continue_loop
        
        # Compare the loaded y-coordinate with the target y (a1)
        # If not equal, skip to continue_loop
        bne t6, a1, continue_loop  # if (t6 != a1) goto continue_loop
        
        # At this point, skyline_stars[t3].x == a0 and skyline_stars[t3].y == a1
        # Proceed to remove the star by shifting subsequent stars left
shift_array:
        
        # Check if the current index t3 is the last star (t3 == t2)
        # If true, no shifting is needed; branch to update_count
        beq t3, t2, update_count    # if (t3 == t2) goto update_count
        
        # Calculate the address of the next star (t3 + 1)
        addi t5, t4, 8              # t5 = &skyline_stars[t3 + 1]
    
shift_loop:
        
        # Load the entire next star's data (assuming 8 bytes per star)
        lw t6, 0(t5)                # t6 = skyline_stars[t3 + 1].data (lower 4 bytes)
        sw t6, -8(t5)               # skyline_stars[t3].data = t6
        
        # Load the upper 4 bytes of the next star's data
        lw t6, 4(t5)                # t6 = skyline_stars[t3 + 1].data (upper 4 bytes)
        sw t6, -4(t5)               # skyline_stars[t3].data_upper = t6
        
        # Move to the next star's address
        addi t5, t5, 8              # t5 = &skyline_stars[t3 + 2]
        
        # Increment the loop counter t3
        addi t3, t3, 1              # t3 = t3 + 1
        
        # Continue shifting if there are more stars to shift
        blt t3, t2, shift_loop      # if (t3 < t2) goto shift_loop    
    
update_count:
    
        # Decrement the star count as one star has been removed
        addi t2, t2, -1             # t2 = t2 - 1
            
        # Store the updated star count back to skyline_star_cnt
        sw t2, 0(t1)                # skyline_star_cnt = t2
        
        # Return from the remove_star function
        ret                         # return
    
continue_loop:
    
        # Increment the loop counter t3 to process the next star
        addi t3, t3, 1              # t3 = t3 + 1
    
        # Jump back to the start of the loop to process the next star
        j loop_start                # goto loop_start
    
remove_end:
        
        # If no matching star is found, simply return
        ret                         # return









# Function: draw_star
# Purpose: Draws a single star onto the framebuffer based on the star's properties.
# Parameters:
#   a0 - Pointer to the framebuffer (uint16_t *fbuf)
#   a1 - Pointer to the skyline_star structure (const struct skyline_star *star)

draw_star:

        # Save the current return address (ra) into temporary register t0
        mv t0, ra                

        # Jump and link to the 'link' function; ra is set to return address after jump
        jal ra, link             

        # Restore the original return address from t0 back to ra
        mv ra, t0                

        # Move the framebuffer pointer (a0) into t0 for later use
        mv t0, a0                  

        # Load star's x-coordinate from memory address (a1 + 0)
        lh t1, 0(a1)               

        # Load star's y-coordinate from memory address (a1 + 2)
        lh t2, 2(a1)               

        # Load star's diameter from memory address (a1 + 4)
        lb t3, 4(a1)               

        # Load star's brightness/intensity from memory address (a1 + 6)
        lh t4, 6(a1)               

        # Calculate framebuffer index: y * SKYLINE_WIDTH
        mul t6, t2, s1             

        # Add x-coordinate to framebuffer index: y * width + x
        add t6, t6, t1            

        # Calculate byte offset: (y * width + x) * 2 (assuming 16-bit pixels)
        slli t6, t6, 1          

        # Calculate framebuffer memory address: fbuf + byte offset
        add t6, t0, t6         

        # Store the star's brightness/intensity value into framebuffer at calculated address
        sh t4, 0(t6)         

        # Return from the draw_star function
        ret










# Function: add_window
# Purpose: Adds a new window with specified properties to the skyline_win_list.
# Parameters:
#   a0 - Property 1 (e.g., window ID or specific attribute)
#   a1 - Property 2 (e.g., x-coordinate)
#   a2 - Property 3 (e.g., y-coordinate)
#   a3 - Property 4 (e.g., color)
#   a4 - Property 5 (e.g., additional attribute)

add_window:

        addi sp, sp, -48            # Allocate 48 bytes on the stack for saving registers

        sd ra, 40(sp)               # Save return address (ra) at offset 40(sp)
       
        sd s0, 32(sp)               # Save register s0 at offset 32(sp)
       
        sd a1, 24(sp)               # Save register a1 at offset 24(sp)
       
        sd a2, 16(sp)               # Save register a2 at offset 16(sp)
       
        sd a3, 8(sp)                # Save register a3 at offset 8(sp)
       
        sd a4, 0(sp)                # Save register a4 at offset 0(sp)

        mv s0, a0                   # Move the value of a0 into s0 for later use

        li a0, 16                   # Load immediate value 16 into a0 (size of skyline_window)
        
        call malloc                 # Call malloc to allocate 16 bytes of memory

        beqz a0, end_add_window     # If malloc returned NULL (a0 == 0), branch to end_add_window

        mv t0, a0                   # Move the allocated memory address from a0 to t0

        ld a1, 24(sp)               # Load the saved value of a1 from offset 24(sp) into a1
       
        ld a2, 16(sp)               # Load the saved value of a2 from offset 16(sp) into a2
       
        ld a3, 8(sp)                # Load the saved value of a3 from offset 8(sp) into a3
       
        ld a4, 0(sp)                # Load the saved value of a4 from offset 0(sp) into a4

        sd zero, 0(t0)              # Store a zero pointer at offset 0(t0) (next pointer)
        
        sh s0, 8(t0)                # Store the value of s0 (from a0) at offset 8(t0) (x-coordinate)
        
        sh a1, 10(t0)               # Store the value of a1 at offset 10(t0) (y-coordinate)
        
        sb a2, 12(t0)               # Store the value of a2 at offset 12(t0) (color)
        
        sb a3, 13(t0)               # Store the value of a3 at offset 13(t0) (additional attribute)
        
        sh a4, 14(t0)               # Store the value of a4 at offset 14(t0) (additional data)

        la t1, skyline_win_list     # Load the address of skyline_win_list into t1
        
        ld t2, 0(t1)                # Load the current head pointer of skyline_win_list into t2

        beqz t2, insert_first_node  # If skyline_win_list is empty (t2 == 0), branch to insert_first_node

find_last_node:
        
        ld t3, 0(t2)                # Load the 'next' pointer of the current node into t3
        
        beqz t3, insert_after_last_node  # If 'next' is NULL, we've found the last node; branch to insert_after_last_node
        
        mv t2, t3                   # Move to the next node in the list
        
        j find_last_node            # Repeat the loop to find the last node

insert_after_last_node:
        
        sd t0, 0(t2)                # Set the 'next' pointer of the last node to the new window (t0)
        
        j end_add_window            # Jump to end_add_window to conclude the function

insert_first_node:
        
        sd t0, 0(t1)                # Set the head of skyline_win_list to the new window (t0)

end_add_window:

        ld a4, 0(sp)                # Restore register a4 from offset 0(sp)
        
        ld a3, 8(sp)                # Restore register a3 from offset 8(sp)
        
        ld a2, 16(sp)               # Restore register a2 from offset 16(sp)
        
        ld a1, 24(sp)               # Restore register a1 from offset 24(sp)
        
        ld s0, 32(sp)               # Restore register s0 from offset 32(sp)
        
        ld ra, 40(sp)               # Restore return address (ra) from offset 40(sp)
        
        addi sp, sp, 48             # Deallocate the 48 bytes from the stack

        ret                         # Return from the add_window function













# Function: remove_window
# Purpose: Removes a window with specified x and y coordinates from the skyline_win_list.
# Parameters:
#   a0 - x-coordinate of the window to remove (uint16_t x)
#   a1 - y-coordinate of the window to remove (uint16_t y)

remove_window:

        addi sp, sp, -16            # Allocate 16 bytes on the stack for saving registers
        
        sd ra, 8(sp)                # Save return address (ra) at offset 8(sp)
        
        sd s0, 0(sp)                # Save register s0 at offset 0(sp)

        la t0, skyline_win_list     # Load the address of skyline_win_list into t0
        
        ld t1, 0(t0)                # Load the head pointer of skyline_win_list into t1

        beqz t1, remove_window_end  # If the list is empty (t1 == 0), branch to end

        mv t2, zero                 # Initialize previous node pointer (t2) to 0 (NULL)
        
        mv t3, t1                   # Initialize current node pointer (t3) to head of the list

find_window:

        beqz t3, remove_window_end  # If current node is NULL, window not found; branch to end

        lh t4, 8(t3)                # Load x-coordinate of current window from offset 8(t3) into t4
        
        lh t5, 10(t3)               # Load y-coordinate of current window from offset 10(t3) into t5

        bne t4, a0, next_window     # If current x != target x, branch to next_window
        
        bne t5, a1, next_window     # If current y != target y, branch to next_window

        beqz t2, remove_head         # If previous node is NULL, target is head; branch to remove_head

        ld t6, 0(t3)                # Load 'next' pointer of current node into t6
        
        sd t6, 0(t2)                # Set 'next' pointer of previous node to 'next' of current node

        mv a0, t3                   # Move address of current node into a0 for free()
        
        call free                   # Call free() to deallocate memory of the removed window

        j remove_window_end         # Jump to end of function

remove_head:

        ld t6, 0(t3)                # Load 'next' pointer of current head node into t6
       
        sd t6, 0(t0)                # Set head of skyline_win_list to 'next' node

        mv a0, t3                   # Move address of removed head node into a0 for free()
       
        call free                   # Call free() to deallocate memory of the removed window

        j remove_window_end         # Jump to end of function

next_window:

        mv t2, t3                   # Move current node pointer to previous node pointer (t2 = t3)
        
        ld t3, 0(t3)                # Load 'next' pointer of current node into t3

        j find_window               # Jump back to find_window to continue traversal

remove_window_end:

        ld s0, 0(sp)                # Restore register s0 from offset 0(sp)
       
        ld ra, 8(sp)                # Restore return address (ra) from offset 8(sp)
       
        addi sp, sp, 16             # Deallocate the 16 bytes from the stack

        ret                         # Return from the remove_window function













# Function: draw_window
# Purpose: Draws a window onto the framebuffer based on the window's properties.
# Parameters:
#   a0 - Base address of the framebuffer (pointer)
#   a1 - Pointer to the skyline_window structure containing window properties
# Registers:
#   s1 - SKYLINE_WIDTH (framebuffer width)
#   s2 - SKYLINE_HEIGHT (framebuffer height)

draw_window:

        mv t0, ra                  # Save the return address (ra) in temporary register t0
        
        jal ra, link               # Call the 'link' function
        
        mv ra, t0                  # Restore the original return address from t0

        li t0, 0                   # Initialize row counter 'i' to 0

draw_window_row:

        lh t2, 10(a1)              # Load y-coordinate (y) from skyline_window structure into t2
        
        add t2, t2, t0             # Calculate current row's y-coordinate: y + i
        
        bge t2, s2, skip_row       # If (y + i) >= SKYLINE_HEIGHT, skip drawing this row

        mul t2, t2, s1             # Calculate row offset: (y + i) * SKYLINE_WIDTH

        lh t3, 8(a1)               # Load x-coordinate (x) from skyline_window structure into t3
        
        add t2, t2, t3             # Calculate pixel's x-coordinate: (y + i) * SKYLINE_WIDTH + x

        slli t2, t2, 1             # Convert pixel index to byte offset: ((y + i) * SKYLINE_WIDTH + x) * 2

        add t2, a0, t2             # Calculate the framebuffer address for the current pixel: framebuffer + offset

        li t1, 0                   # Initialize column counter 'j' to 0

draw_window_column:

        lh t3, 8(a1)               # Load x-coordinate (x) from skyline_window structure into t3
        
        add t3, t3, t1             # Calculate current column's x-coordinate: x + j
        
        bge t3, s1, skip_column    # If (x + j) >= SKYLINE_WIDTH, skip drawing this pixel

        lh t4, 14(a1)              # Load color from skyline_window structure into t4

        sh t4, 0(t2)               # Store color at the current framebuffer position

        addi t2, t2, 2             # Move to the next pixel (2 bytes per pixel)

        addi t1, t1, 1             # Increment column counter 'j'

        lbu t4, 12(a1)             # Load width (w) from skyline_window structure into t4
        
        blt t1, t4, draw_window_column # If j < w, continue drawing columns

skip_column:

        addi t0, t0, 1             # Increment row counter 'i'

        lbu t3, 13(a1)             # Load height (h) from skyline_window structure into t3
       
        blt t0, t3, draw_window_row # If i < h, continue drawing rows

skip_row:

        ret                         # Return from the draw_window function













# Function: draw_beacon
# Purpose: Draws a beacon onto the framebuffer based on the beacon's properties.
# Parameters:
#   a0 - Base address of the framebuffer (pointer)
#   a1 - Current frame or tick count (used for blinking)
#   a2 - Pointer to the beacon structure containing beacon properties

draw_beacon:

        mv t0, ra                       # Save the current return address in t0
        
        jal ra, link                    # Jump to `link` and save the return address in `ra`
        
        mv ra, t0                       # Restore the original return address from t0

        lh t1, 14(a2)                   # Load the first period value of the beacon into t1 (t1 = period1)
        
        lh t2, 16(a2)                   # Load the second period value of the beacon into t2 (t2 = period2)
        
        remu t0, a1, t1                  # Compute the remainder of a1 divided by t1 and store it in t0 (t0 = a1 % period1)
        
        bge t0, t2, draw_beacon_end      # If the remainder t0 >= period2, skip drawing the beacon and jump to draw_beacon_end

        lh t6, 8(a2)                    # Load the x-coordinate of the beacon into t6 (t6 = x)
        
        lh t5, 10(a2)                   # Load the y-coordinate of the beacon into t5 (t5 = y)
        
        lb t4, 12(a2)                   # Load the diameter of the beacon into t4 (t4 = dia)

        add t3, t6, t4                  # Calculate the maximum x boundary of the beacon (t3 = x + diameter)
        
        add t2, t5, t4                  # Calculate the maximum y boundary of the beacon (t2 = y + diameter)

        addi t1, t5, 0                  # Initialize the y counter (t1) to the beacon's y-coordinate (t1 = y)
        
        addi t0, t6, 0                  # Initialize the x counter (t0) to the beacon's x-coordinate (t0 = x)

        ld t4, 0(a2)                    # Load the pointer to beacon color data into t4 (t4 = pointer to beacon data)

fill_pixel:

        beq t0, t3, skip                # If the x counter t0 equals the maximum x boundary t3, skip to the next row

        mul t5, t1, s1                  # Multiply the current y position t1 by SKYLINE_WIDTH s1 to get the row offset (t5 = y * 640)
        
        add t5, t5, t0                  # Add the current x position t0 to the row offset to get the pixel index (t5 = x + y * 640)
        
        slli t5, t5, 1                  # Multiply the pixel index by 2 to get the byte offset in the framebuffer (t5 = (x + y * 640) * 2)
        
        add t5, t5, a0                  # Add the framebuffer base address a0 to the byte offset to get the exact memory address (t5 = framebuffer + offset)

        lh t6, 0(t4)                    # Load the color value from the beacon data into t6 (t6 = color)
        
        sh t6, 0(t5)                    # Store the color value into the framebuffer at the calculated address (store color)
        
        addi t4, t4, 2                   # Increment the beacon data pointer by 2 bytes to move to the next color value
        
        addi t0, t0, 1                   # Increment the x counter t0 by 1 to move to the next pixel in the row
        
        j fill_pixel                     # Jump back to the beginning of the fill_pixel loop to process the next pixel

skip:

        addi t1, t1, 1                   # Increment the y counter t1 by 1 to move to the next row
        
        beq t1, t2, draw_beacon_end      # If the y counter t1 equals the maximum y boundary t2, finish drawing and jump to draw_beacon_end

        lh t0, 8(a2)                     # Reload the x-coordinate of the beacon into t0 (t0 = x)
        
        j fill_pixel                     # Jump back to the fill_pixel loop to draw the next row of pixels

draw_beacon_end:

        ret                              # Return from the draw_beacon function



.end
