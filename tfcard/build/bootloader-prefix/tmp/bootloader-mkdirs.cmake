# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "E:/esp_idf/v5.2.3/esp-idf/components/bootloader/subproject"
  "E:/ESP32_jlc/03-esp32_mypro/tfcard/build/bootloader"
  "E:/ESP32_jlc/03-esp32_mypro/tfcard/build/bootloader-prefix"
  "E:/ESP32_jlc/03-esp32_mypro/tfcard/build/bootloader-prefix/tmp"
  "E:/ESP32_jlc/03-esp32_mypro/tfcard/build/bootloader-prefix/src/bootloader-stamp"
  "E:/ESP32_jlc/03-esp32_mypro/tfcard/build/bootloader-prefix/src"
  "E:/ESP32_jlc/03-esp32_mypro/tfcard/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/ESP32_jlc/03-esp32_mypro/tfcard/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/ESP32_jlc/03-esp32_mypro/tfcard/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
