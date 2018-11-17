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

#include "ocreio.h"

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

using namespace v8;

Persistent<Function> OcrEio::constructor;

void OcrEio::Init(Handle<Object> exports)
{
  Isolate* isolate = Isolate::GetCurrent(); // fixme, see: https://strongloop.com/strongblog/node-js-v0-12-c-apis-breaking/
                                            // exports->GetIsolate() as stated in https://iojs.org/api/addons.html does not work

  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "OcrEio"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tpl, "ocr", Ocr);

  fprintf(stderr, "Tesseract Open Source OCR Engine v%s with Leptonica\n", tesseract::TessBaseAPI::Version());

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "OcrEio"), tpl->GetFunction());
}

void OcrEio::New(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Context> ctx = Context::New(isolate);

  if (args.IsConstructCall())
  {
    OcrEio* obj = new OcrEio();
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  }
  else
  {
    const int argc = 0;
    Local<Value> argv[argc] = {};
    Local<Function> cons = Local<Function>::New(isolate, constructor);
    args.GetReturnValue().Set(cons->NewInstance(ctx, argc, argv).ToLocalChecked());
  }
}

void OcrEio::Ocr(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = args.GetIsolate();
  EscapableHandleScope scope(isolate);
  Local<Value> result; // default undefined
  
  char *language = NULL;
  int psm = 3; // default value
  char *tessdata = NULL;
  int *rect = NULL;
  
  if (args.Length() < 2 || args.Length() > 3)
  {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected 2 or 3 arguments", String::kInternalizedString)));
    scope.Escape(result);
    return;
  }
  
  if ((!args[0]->ToObject()->GetConstructorName()->Equals(String::NewFromUtf8(isolate, "Buffer", String::kInternalizedString)))
      && (!args[0]->ToObject()->GetConstructorName()->Equals(String::NewFromUtf8(isolate, "Uint8Array", String::kInternalizedString))))
  {
    isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument 1 must be an object of type Buffer", String::kInternalizedString)));
    scope.Escape(result);
    return;
  }

  Handle<Object> buf = args[0]->ToObject();   
  Local<Function> cb;
  
  if(args.Length() == 2)
  {
    if (!args[1]->IsFunction())
    {
      isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument 2 must be a function", String::kInternalizedString)));
      scope.Escape(result);
      return; 
    }
  
    cb = Local<Function>::Cast(args[1]);
  }
  else if(args.Length() == 3)
  {
    if (!args[1]->IsObject())
    {
      isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument 2 must be an config object, e.g. { lang:\"eng\" }", String::kInternalizedString)));
      scope.Escape(result);
      return; 
    }
  
    Handle<Object> config = args[1]->ToObject();
  
    Local<Value> lang_value = config->Get(String::NewFromUtf8(isolate, "lang", String::kInternalizedString));
    if (lang_value->IsString())
    {
      String::Utf8Value str(isolate, lang_value);
      if(str.length() == 3)
      {
        language = *str;
      }
    }
    
    Local<Value> psm_value = config->Get(String::NewFromUtf8(isolate, "psm", String::kInternalizedString));
    if (psm_value->IsNumber())
    {
      psm = psm_value->ToInteger()->Value();
    }
  
    Local<Value> tessdata_value = config->Get(String::NewFromUtf8(isolate, "tessdata", String::kInternalizedString));
    
    if (tessdata_value->IsString())
    {
      String::Utf8Value str(isolate, tessdata_value);
      if(str.length() < 4095)
      {
        tessdata = *str;
      }
    }
  
    Local<Value> box_value = config->Get(String::NewFromUtf8(isolate, "rect", String::kInternalizedString));
    
    if (box_value->IsArray())
    {
      Handle<Object> box = box_value->ToObject();
      int length = box->Get(String::NewFromUtf8(isolate, "length", String::kInternalizedString))->ToObject()->Uint32Value();
      if(length == 4)
      {
        rect = new int[length];
        for(int i = 0; i < length; i++)
        {
          Local<Value> element = box->Get(i);
          if(element->IsNumber()) 
          {
            rect[i] = element->ToInteger()->Value();
          }
        }
      }
    }
  
    if (!args[2]->IsFunction())
    {
      isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument 3 must be a function", String::kInternalizedString)));
      scope.Escape(result);
      return; 
    }
  
    cb = Local<Function>::Cast(args[2]);
  }

  OcrEio* oe = ObjectWrap::Unwrap<OcrEio>(args.Holder());
  oe->Ref();
    
  ocr_baton_t *baton = new ocr_baton_t();
  baton->oe = oe;
  baton->error = 0;
  baton->textresult = NULL;
  baton->rect = rect;
  baton->psm = psm;
    
  if(language)
    baton->language = language;
  else
    baton->language = strdup("eng");

  if(tessdata)
    baton->tessdata = tessdata;
  else
    baton->tessdata = strdup("/usr/local/share/tessdata/");

  baton->cb.Reset(isolate, cb);
  baton->buf.Reset(isolate, buf);
  baton->buf_ptr = (unsigned char*) node::Buffer::Data(buf);
  baton->buf_len = node::Buffer::Length(buf);

  uv_work_t *req = new uv_work_t;
  req->data = baton;

  uv_queue_work(uv_default_loop(), req, EIO_Ocr, EIO_AfterOcr);
  
  args.GetReturnValue().SetUndefined();
}

void OcrEio::EIO_Ocr(uv_work_t *req)
{
  ocr_baton_t *baton = static_cast<ocr_baton_t *>(req->data);
  tesseract::TessBaseAPI api;
  int r = api.Init(baton->tessdata, baton->language, tesseract::OEM_DEFAULT, NULL, 0, NULL, NULL, false);
  if(r == 0)
  {
    PIX* pix = pixReadMem(baton->buf_ptr, baton->buf_len);
    if(pix)
    {
      api.SetImage(pix);
      api.SetPageSegMode((tesseract::PageSegMode)baton->psm);
      if(baton->rect)
      {
        api.SetRectangle(baton->rect[0], baton->rect[1], baton->rect[2], baton->rect[3]);
      }
      baton->textresult = api.GetUTF8Text();
      api.End();
      pixDestroy(&pix);
    }
    else
    {
      baton->error = 2;
      api.End();
    }
  }
  else
  {
    baton->error = r;
  }
}

void OcrEio::EIO_AfterOcr(uv_work_t *req, int status)
{
  ocr_baton_t *baton = static_cast<ocr_baton_t *>(req->data);

  baton->oe->Unref();

  Local<Value> argv[2];
  
  Isolate* isolate = Isolate::GetCurrent(); // fixme, see: https://strongloop.com/strongblog/node-js-v0-12-c-apis-breaking/

  // Add below line to fix error "Cannot create a handle without a HandleScope"
  HandleScope scope(isolate);
  argv[0] = Number::New(isolate, baton->error + status);
  argv[1] = String::NewFromUtf8(isolate, baton->textresult, String::kInternalizedString, strlen(baton->textresult));

  TryCatch try_catch(isolate);

  Handle<Context> context = isolate->GetCurrentContext();
  Handle<Function> callback = Local<Function>::New(isolate, baton->cb);
  callback->Call(context->Global(), 2, argv);

  delete baton;
  delete req;

  if (try_catch.HasCaught())
  {
    node::FatalException(isolate, try_catch);
  }
}
