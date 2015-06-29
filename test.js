var assert = require('assert');
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
    assert.equal(result.trim(), 'hello, world');
    console.log(result.trim());
  });
  myocr.ocr(data, {rect:[0,0,400,400]}, function(err, result) {
    if(err)
      throw err;
    assert.equal(result.trim(), 'hell');
  });
});
