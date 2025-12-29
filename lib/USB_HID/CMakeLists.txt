add_library(USB_HID INTERFACE)

target_sources(USB_HID INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
  ${CMAKE_CURRENT_LIST_DIR}/hid_app.c
)

target_include_directories(USB_HID INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(USB_HID INTERFACE
  pico_stdlib
  tinyusb_device
  tinyusb_board
)
