# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif/frameworks/esp-idf-v5.0.2/components/bootloader/subproject"
  "C:/Users/vikto/Desktop/Work/Codec2/PROJECT_microphone_monitoring/build/bootloader"
  "C:/Users/vikto/Desktop/Work/Codec2/PROJECT_microphone_monitoring/build/bootloader-prefix"
  "C:/Users/vikto/Desktop/Work/Codec2/PROJECT_microphone_monitoring/build/bootloader-prefix/tmp"
  "C:/Users/vikto/Desktop/Work/Codec2/PROJECT_microphone_monitoring/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/vikto/Desktop/Work/Codec2/PROJECT_microphone_monitoring/build/bootloader-prefix/src"
  "C:/Users/vikto/Desktop/Work/Codec2/PROJECT_microphone_monitoring/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/vikto/Desktop/Work/Codec2/PROJECT_microphone_monitoring/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/vikto/Desktop/Work/Codec2/PROJECT_microphone_monitoring/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
