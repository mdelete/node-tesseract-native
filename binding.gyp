{
  "targets": [{
    "target_name": "tesseract_native",
    "sources": [ "node-tesseract-native.cc" ],
    "include_dirs": [
      "/usr/local/include",
    ],
    "libraries": [
      "-L/usr/local/lib"
    ],
    "link_settings": {
      "libraries": [ "-llept", "-ltesseract" ]
    }
  }]
}

