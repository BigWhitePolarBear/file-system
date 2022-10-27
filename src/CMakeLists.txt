set(File_System_SRC
  bitmap.c
  cmd.c
  common.c
  device.c
  format.c
  fun.c
  simdisk.c
)

add_executable(simdisk
  ${File_System_SRC})

  
set(Shell_SRC
  shell.c
)

add_executable(shell
  ${Shell_SRC})