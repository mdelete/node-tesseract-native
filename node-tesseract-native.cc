/* This code is PUBLIC DOMAIN, and is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND.
 */

#include <v8.h>
#include <node.h>
#include <node_buffer.h>

using namespace node;
using namespace v8;

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
#include <tesseract/strngs.h>

class OcrEio : ObjectWrap
{
public:

  static Persistent<FunctionTemplate> s_ct;
  static void Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("OcrEio"));

    NODE_SET_PROTOTYPE_METHOD(s_ct, "ocr", Ocr);

    target->Set(String::NewSymbol("OcrEio"), s_ct->GetFunction());

    fprintf(stderr, "Tesseract Open Source OCR Engine v%s with Leptonica\n", tesseract::TessBaseAPI::Version());
  }

  OcrEio()
  {
  }

  ~OcrEio()
  {
  }

  static Handle<Value> New(const Arguments& args)
  {
    HandleScope scope;
    OcrEio* oe = new OcrEio();
    oe->Wrap(args.This());
    return args.This();
  }

  struct ocr_baton_t {
    OcrEio *oe;
    unsigned timeout;
    STRING language; // this is a tesseract string
    STRING textresult; // this is a tesseract string
    Persistent<Function> cb;
    Persistent<Object> buf;
  };

  static Handle<Value> Ocr(const Arguments& args)
  {
    HandleScope scope;

    if (args.Length() < 2 || args.Length() > 3)
      return ThrowException(Exception::TypeError(String::New("Expected 2 or 3 arguments"))); 
    
    if (!args[0]->ToObject()->GetConstructorName()->Equals(String::New("Buffer")))
      return ThrowException(Exception::TypeError(String::New("Argument 1 must be an object of type Buffer")));
    
    Handle<Object> buf = args[0]->ToObject();   
    Local<Function> cb;
    
    STRING language("eng"); // default language, this is a tesseract string
    unsigned timeout = 500; // default timeout

    if(args.Length() == 2)
    {
      if (!args[1]->IsFunction())
        return ThrowException(Exception::TypeError(String::New("Argument 2 must be a function")));
        
      cb = Local<Function>::Cast(args[1]);
    }
    else if(args.Length() == 3)
    {
      if (!args[1]->IsObject())
        return ThrowException(Exception::TypeError(String::New("Argument 2 must be an config object, e.g. { timeout:500, lang:\"eng\" }")));

      Handle<Object> config = args[1]->ToObject();
      
      Local<Value> timeout_value = config->Get(String::New("timeout"));
      if(timeout_value->IsNumber())
        timeout = timeout_value->ToInteger()->Value();
      
      Local<Value> lang_value = config->Get(String::New("lang"));
      if(lang_value->IsString())
      {
        String::AsciiValue str(lang_value);
        if(str.length() == 3)
          language = STRING(*str);
      }

      if (!args[2]->IsFunction())
        return ThrowException(Exception::TypeError(String::New("Argument 3 must be a function")));

      cb = Local<Function>::Cast(args[2]);
    }
    
    OcrEio* oe = ObjectWrap::Unwrap<OcrEio>(args.This());
    oe->Ref();
    
    ocr_baton_t *baton = new ocr_baton_t();
    baton->oe = oe;
    baton->timeout = timeout;
    baton->textresult = "";
    baton->language = language;
    baton->cb = Persistent<Function>::New(cb);
    baton->buf = Persistent<Object>::New(buf);

    uv_work_t *req = new uv_work_t;
    req->data = baton;

    uv_queue_work(uv_default_loop(), req, EIO_Ocr, EIO_AfterOcr);

    return Undefined();
  }

  static void EIO_Ocr(uv_work_t *req)
  {
    ocr_baton_t *baton = static_cast<ocr_baton_t *>(req->data);
    tesseract::TessBaseAPI api;
    api.Init("/usr/local/share/tessdata/", baton->language.string(), tesseract::OEM_DEFAULT, NULL, 0, NULL, NULL, false);
    PIX* pix = pixReadMem((unsigned char*)Buffer::Data(baton->buf), Buffer::Length(baton->buf)); // leptonica function
    if (pix && !api.ProcessPage(pix, 0, "", NULL, baton->timeout, &baton->textresult)) // textresult will be empty if error occured
    {
      fprintf(stderr, "*** error during tesseract processing ***\n");
    }
    pixDestroy(&pix); // leptonica function
  }

  static void EIO_AfterOcr(uv_work_t *req)
  {
    HandleScope scope;
    ocr_baton_t *baton = static_cast<ocr_baton_t *>(req->data);
    
    baton->oe->Unref();

    Local<Value> argv[1];
    
    argv[0] = String::New(baton->textresult.string(), baton->textresult.length());

    TryCatch try_catch;

    baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

    baton->cb.Dispose();
    baton->buf.Dispose();

    delete baton;
    delete req;

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }
  }

};

Persistent<FunctionTemplate> OcrEio::s_ct;

extern "C" {
  static void init (Handle<Object> target)
  {
    OcrEio::Init(target);
  }

  NODE_MODULE(tesseract_native, init);
}
