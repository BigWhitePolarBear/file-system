set(File_System_SRC
  simdisk.c
  device.c
  fs.c
  fs_bitmap.c
)

add_executable(simdisk
  ${File_System_SRC})

  
set(Shell_SRC
  shell.c
)

add_executable(shell
  ${Shell_SRC})