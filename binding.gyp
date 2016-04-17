{
  "targets": [{
    "target_name": "tesseract_native",
    "sources": [ "addon.cc", "ocreio.cc" ],
    "include_dirs": [
      "/usr/local/include",
    ],
    "libraries": [
      "-L/usr/local/lib"
    ],
    "link_settings": {
      "libraries": [ "-llept", "-ltesseract" ]
    },
    "configurations": {
      "Release": {
        "cflags": [ "-Wno-ignored-qualifiers" ],
        "xcode_settings": { "OTHER_CFLAGS": [ "-Wno-ignored-qualifiers" ] }
      }
    }
  }]
}

