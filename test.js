var tesseract = require('./build/Release/tesseract_native');
var myocr = new tesseract.OcrEio();
var fs = require('fs');

fs.readFile(process.argv[2], function (err, data) {
  if (err) {
    throw err; 
  }
  myocr.ocr(data, function(err, result) {
    if(err)
      throw err;
    console.log(result);
  });
});
