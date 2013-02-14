node-tesseract-native
=====================

C++ module for node providing OCR with tesseract and leptonica

Prerequisites
-------------
 * Have linux
 * Have node and node-waf installed
 * Have leptonica (~1.68) libs and headers installed
 * Have tesseract (~3.01) libs and headers installed

Build
-----

    node-waf configure && node-waf build

Supported Picture Formats
-------------------------

The module can handle every picture format leptonica can handle (see there), but as this module is likely to be used in an online service, pictures should be as small as possible. A 1.3MP Picture converted to B/W using adaptive threshold filtering, saved as PNG will be 50KB on average. This is were you want to go.

Test your setup
---------------

You can test your setup using the provided *test.js* script on the command-line

    node test.js some-picture-with-text-in-it.png

Example server
--------------

The code below shows a fully functional server where you can POST pictures to. The response will contain the recognized plain text or be empty if nothing was recognized or something went wrong.

    var tesseract = require('./tesseract-native');
    var http = require('http');
    
    var server = http.createServer(function(request, response)
    {
        if(request.method === 'POST')
        {
            var totalSize = 0;
            var bufferList = new Array();
            var myOcr = new tesseract.OcrEio();
            
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
                myOcr.ocr(buffer, function(result) {
                    response.writeHead(200, {'Content-Type': 'text/plain'});
                    response.end(result);
                });
            });
            
        } else {
            request.connection.destroy();
        }
    }).listen(process.argv[2]);
    
Parameters
----------
    
The OCR function also accepts a config object as second and the callback as third parameter like this:

    myOcr.ocr(buffer, { lang:"deu", timeout:300 }, function(result) {
        // do something
    });
    
The first supported parameter is *timeout*, which is the limit in milliseconds tesseract should try to detect text in the picture, the second is *lang* which can be any three-character code for a language you have installed in tesseract.
