node-tesseract-native
=====================

C++ module for node providing OCR with tesseract and leptonica

Prerequisites
-------------
 * have linux
 * have node and node-waf installed
 * have leptonica libs and headers installed
 * have tesseract libs and headers installed


Example server
--------------
The code below shows a fully functional server where you can POST pictures to. The response will contain the recognized plain text or be empty if nothing was recognized or something went wrong.

    var ocreio = require('./node-tesseract-native');
    var http = require('http');
    
    var server = http.createServer(function(request, response)
    {
        if(request.method === 'POST')
        {
            var totalSize = 0;
            var bufferList = new Array();
            var myOcr = new ocreio.OcrEio();
            
            request.on('data', function(data) {
                bufferList.push(data);
                totalSize += data.length;
                if (totalSize > 1e6) {
                    console.log('Request body too large');
                    request.connection.destroy();
                }
            });
            
            request.on('end', function() {
                var buffer = Buffer.concat(bufferList, totalSize);
                myOcr.ocr(buffer, 500, function(result) {
                    response.writeHead(200, {'Content-Type': 'text/plain'});
                    response.end(result);
                });
            });
            
        } else {
            request.connection.destroy();
        }
    }).listen(process.argv[2]);