{
  "targets": [{
    "target_name": "tesseract_native",
    "sources": [ "node-tesseract-native.cc" ],
    "link_settings": {
      "libraries": [ "-llept", "-ltesseract" ]
    }
  }]
}

