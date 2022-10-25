set(File_System_SRC
  simdisk.c
  device.c
  fs.c
  fs_bitmap.c
)

add_executable(simdisk
  ${File_System_SRC})