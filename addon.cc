#include <node.h>
#include "ocreio.h"

using namespace v8;

void InitAll(Handle<Object> exports) {
  OcrEio::Init(exports);
}

NODE_MODULE(ocreio, InitAll)