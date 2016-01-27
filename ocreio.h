/*
 * Copyright (c) 2015 Marc Delling
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef OCREIO_H
#define OCREIO_H

#include <uv.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

class OcrEio : public node::ObjectWrap {

 public:
  static void Init(v8::Handle<v8::Object> exports);

 private:
  struct ocr_baton_t {
    ~ocr_baton_t() {
      cb.Reset();
      buf.Reset();
      if (language) delete [] language;
      if (tessdata) delete [] tessdata;
      if (textresult) delete [] textresult;
      if (rect) delete [] rect;
    }
    OcrEio *oe;
    int error;
    char* language;
    int psm;
    char* tessdata;
    char* textresult;
    int* rect;
    v8::Persistent<v8::Function> cb;
    v8::Persistent<v8::Object> buf;
    unsigned char* buf_ptr;
    size_t buf_len;
  };

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Ocr(const v8::FunctionCallbackInfo<v8::Value>& args);
  static v8::Persistent<v8::Function> constructor;
  
  static void EIO_Ocr(uv_work_t *req);
  static void EIO_AfterOcr(uv_work_t *req, int status);

};

#endif
