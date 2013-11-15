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
    return scope.Close(args.This());
  }

  struct ocr_baton_t {
    OcrEio *oe;
    unsigned timeout;
    int error;
    STRING language; // this is a tesseract string
    STRING tessdata; // this is a tesseract string
    STRING textresult; // this is a tesseract string
    Persistent<Function> cb;
    Persistent<Object> buf;
    unsigned char* buf_ptr;
    size_t buf_len;
  };

  static Handle<Value> Ocr(const Arguments& args)
  {
    HandleScope scope;

    if (args.Length() < 2 || args.Length() > 3)
    {
      ThrowException(Exception::TypeError(String::New("Expected 2 or 3 arguments")));
      return scope.Close(Undefined());
    }
    
    if (!args[0]->ToObject()->GetConstructorName()->Equals(String::New("Buffer")))
    {
      ThrowException(Exception::TypeError(String::New("Argument 1 must be an object of type Buffer")));
      return scope.Close(Undefined()); 
    }
    
    Handle<Object> buf = args[0]->ToObject();   
    Local<Function> cb;
    
    STRING language("eng"); // default language, this is a tesseract string
    STRING tessdata("/usr/local/share/tessdata/"); // default tessdata path, this is a tesseract string
    unsigned timeout = 500; // default timeout

    if(args.Length() == 2)
    {
      if (!args[1]->IsFunction())
      {
        ThrowException(Exception::TypeError(String::New("Argument 2 must be a function")));
        return scope.Close(Undefined()); 
      }
        
      cb = Local<Function>::Cast(args[1]);
    }
    else if(args.Length() == 3)
    {
      if (!args[1]->IsObject())
      {
        ThrowException(Exception::TypeError(String::New("Argument 2 must be an config object, e.g. { timeout:500, lang:\"eng\" }")));
        return scope.Close(Undefined()); 
      }

      Handle<Object> config = args[1]->ToObject();
      
      Local<Value> timeout_value = config->Get(String::New("timeout"));
      if(timeout_value->IsNumber()) 
      {
        timeout = timeout_value->ToInteger()->Value();
      }
      
      Local<Value> lang_value = config->Get(String::New("lang"));
      if(lang_value->IsString())
      {
        String::AsciiValue str(lang_value);
        if(str.length() == 3)
        {
          language = STRING(*str);
        }
      }
      
      Local<Value> tessdata_value = config->Get(String::New("tessdata"));
      if(tessdata_value->IsString())
      {
        String::AsciiValue str(tessdata_value);
        tessdata = STRING(*str);
      }

      if (!args[2]->IsFunction())
      {
        ThrowException(Exception::TypeError(String::New("Argument 3 must be a function")));
        return scope.Close(Undefined());
      }

      cb = Local<Function>::Cast(args[2]);
    }
    
    OcrEio* oe = ObjectWrap::Unwrap<OcrEio>(args.This());
    oe->Ref();
    
    ocr_baton_t *baton = new ocr_baton_t();
    baton->oe = oe;
    baton->timeout = timeout;
    baton->error = 0;
    baton->textresult = "";
    baton->language = language;
    baton->tessdata = tessdata;
    baton->cb = Persistent<Function>::New(cb);
    baton->buf = Persistent<Object>::New(buf);
    baton->buf_ptr = (unsigned char*) Buffer::Data(baton->buf);
    baton->buf_len = Buffer::Length(baton->buf);

    uv_work_t *req = new uv_work_t;
    req->data = baton;

    uv_queue_work(uv_default_loop(), req, EIO_Ocr, EIO_AfterOcr);

    return scope.Close(Undefined());
  }

  static void EIO_Ocr(uv_work_t *req)
  {
    ocr_baton_t *baton = static_cast<ocr_baton_t *>(req->data);
    tesseract::TessBaseAPI api;
    int r = api.Init(baton->tessdata.string(), baton->language.string(), tesseract::OEM_DEFAULT, NULL, 0, NULL, NULL, false);
    
    if(r == 0)
    {
      PIX* pix = pixReadMem(baton->buf_ptr, baton->buf_len); // leptonica function
      if(pix)
      {
        if(!api.ProcessPage(pix, 0, "", NULL, baton->timeout, &baton->textresult)) // textresult will be empty string if error occured
        {
          baton->error = 3;
        }
        pixDestroy(&pix); // leptonica function
      }
      else
      {
        baton->error = 2;
      }
    }
    else
    {
      baton->error = r;
    }
  }

  static void EIO_AfterOcr(uv_work_t *req, int status)
  {
    ocr_baton_t *baton = static_cast<ocr_baton_t *>(req->data);
    
    baton->oe->Unref();

    Local<Value> argv[2];
    
    argv[0] = Number::New(baton->error + status);
    argv[1] = String::New(baton->textresult.string(), baton->textresult.length());

    TryCatch try_catch;

    baton->cb->Call(Context::GetCurrent()->Global(), 2, argv);

    baton->cb.Dispose();
    baton->buf.Dispose();

    delete baton;
    delete req;

    if (try_catch.HasCaught())
    {
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
