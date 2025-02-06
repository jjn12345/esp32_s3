# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "E:/esp_idf/v5.2.3/esp-idf/components/bootloader/subproject"
  "E:/git_store/esp32_szp/wifi_scan_sample/build/bootloader"
  "E:/git_store/esp32_szp/wifi_scan_sample/build/bootloader-prefix"
  "E:/git_store/esp32_szp/wifi_scan_sample/build/bootloader-prefix/tmp"
  "E:/git_store/esp32_szp/wifi_scan_sample/build/bootloader-prefix/src/bootloader-stamp"
  "E:/git_store/esp32_szp/wifi_scan_sample/build/bootloader-prefix/src"
  "E:/git_store/esp32_szp/wifi_scan_sample/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/git_store/esp32_szp/wifi_scan_sample/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/git_store/esp32_szp/wifi_scan_sample/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
