set(File_System_SRC
  base_fun.c
  bitmap.c
  cmd.c
  common.c
  device.c
  free.c
  interface_fun.c
  internal_fun.c
  format.c
  simdisk.c
)

add_executable(simdisk
  ${File_System_SRC})

  
set(Shell_SRC
  shell.c
)

add_executable(shell
  ${Shell_SRC})