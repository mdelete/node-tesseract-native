var tesseract = require('./tesseract_native');
var myocr = new tesseract.OcrEio();
var fs = require('fs');

fs.readFile( __dirname + '/' + process.argv[2], function (err, data) {
  if (err) {
    throw err; 
  }
  console.log(require('util').inspect(data));
  myocr.ocr(data, function(result){
    console.log(result);
  });
});
