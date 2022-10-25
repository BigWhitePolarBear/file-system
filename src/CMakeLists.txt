set(File_System_SRC
  cmd.c
  device.c
  fs.c
  fs_bitmap.c
  simdisk.c
)

add_executable(simdisk
  ${File_System_SRC})

  
set(Shell_SRC
  shell.c
)

add_executable(shell
  ${Shell_SRC})