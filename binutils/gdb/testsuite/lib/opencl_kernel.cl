/* OpenCL kernel for testing purposes.  */
__kernel void testkernel (__global int *data)
{
  data[get_global_id(0)] = 0x1;
}
